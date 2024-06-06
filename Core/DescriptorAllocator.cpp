#include "pch.h"
#include "DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
    : m_HeapType(type)
    , m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
}

/// <summary>
/// ������ ������ ���ӵ� ������ ����� �Ҵ��մϴ�.
/// ������ ��(������)�� ��û�� �Ҵ��� ������ �� ���� ������ ��û�� ������ ���� �Ҵ��Ϸ��� �õ��մϴ�.
/// ��û�� ������ �� �ִ� ������ ���� ������ ��û�� ������ �� �ִ� �� ������ ��(������)�� �����˴ϴ�.
/// </summary>
/// <param name="numDescriptors">������ ������ ��</param>
/// <returns></returns>
DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex); // �Ҵ��ϱ� ���� �����尡 �Ҵ��ڿ��� ���������� �����ϵ��� ��ݴϴ�.

    DescriptorAllocation allocation; // �Ҵ� ����� ��� ���� �Դϴ�.

    for (auto iter = m_AvailableHeaps.begin(); iter != m_AvailableHeaps.end(); ++iter)
    {
        auto allocatorPage = m_HeapPool[*iter];

        // �Ҵ��� �̷�����ϴ�.
        // numDescriptors ���� ������ �� ������ ��ȿ�� DescriptorAllocation ��
        // ��ȯ�˴ϴ�.
        allocation = allocatorPage->Allocate(numDescriptors);

        // �Ҵ����� �������� ����ִ� ���(��� ������ ������ ���� 0�� ������ ��)
        // ���� �������� �ε���(iter)�� ��� ������ �� ����(_availableHeaps)����
        // ���ŵ˴ϴ�.
        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = m_AvailableHeaps.erase(iter);
        }

        // �Ҵ��� ���������� ��ȿ�� ������ �ڵ��� �Ҵ�� ���
        // (������ �ڵ��� null�� �ƴ� ��) ������ �ߴܵ˴ϴ�.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // ������ �Ҵ��� ��ȿ���� �˻��մϴ�.
    // �� �ݺ����� �� ���������� �����ڰ� ��ȿ���� ���� ���, ���
    // ��û�� ������ ��(numDescriptors)��ŭ ū ���ο� ������ �������� ����ϴ�.
    if (allocation.IsNull())
    {
        m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, numDescriptors);
        auto newPage            = CreateAllocatorPage();

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

/// <summary>
/// ��� ������ �� �������� ReleaseStaleDescriptors �޼��带 ȣ���մϴ�.
/// ������ �����ڸ� ������ �� �������� ��� ������ �ڵ��� ������
/// ��� ������ �� ���(_availableHeaps)�� �߰��˴ϴ�.
/// </summary>
/// <param name="frameNumber"></param>
void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex); // ���ؽ��� ��ݴϴ�.

    // ��� ������ �� �������� ReleaseStaleDescriptors �޼��带 ȣ���մϴ�.
    for (size_t i = 0; i < m_HeapPool.size(); ++i)
    {
        auto page = m_HeapPool[i];

        page->ReleaseStaleDescriptors(frameNumber);

        // ��� ������ �� �ε����� �߰��մϴ�.
        if (page->NumFreeHandles() > 0)
        {
            m_AvailableHeaps.insert(i);
        }
    }
}

/// <summary>
/// �������� �� �������� ����� �� ���˴ϴ�.
/// </summary>
/// <returns></returns>
std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<DescriptorAllocatorPage>(
        m_HeapType, m_NumDescriptorsPerHeap); // �� DescriptorAllocatorPage ����

    m_HeapPool.emplace_back(newPage);               // �� DescriptorAllocatorPage Ǯ�� �߰�
    m_AvailableHeaps.insert(m_HeapPool.size() - 1); // DescriptorAllocatorPage�� �ε��� �߰�

    return newPage;
}

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return 0;
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    return DescriptorAllocation();
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
}

bool DescriptorAllocation::IsNull() const
{
    return false;
}

#include "pch.h"
#include "DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
    : _device(device)
    , m_HeapType(type)
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
        _device, m_HeapType, m_NumDescriptorsPerHeap); // �� DescriptorAllocatorPage ����

    m_HeapPool.emplace_back(newPage);               // �� DescriptorAllocatorPage Ǯ�� �߰�
    m_AvailableHeaps.insert(m_HeapPool.size() - 1); // DescriptorAllocatorPage�� �ε��� �߰�

    return newPage;
}

DescriptorAllocatorPage::DescriptorAllocatorPage(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    : m_HeapType(type)
    , m_NumDescriptorsInHeap(numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type                       = m_HeapType;
    heapDesc.NumDescriptors             = m_NumDescriptorsInHeap;

    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap)));

    m_BaseDescriptor                = m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
    m_NumFreeHandles                = m_NumDescriptorsInHeap;

    // free lists �ʱ�ȭ
    AddNewBlock(0, m_NumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
    return m_HeapType;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    // std::map::lower_bound �޼���� free list���� ��û�� numDescriptors����
    // ���� ����(���ų� �� ū) ù ��° �׸��� ã�� �� ���˴ϴ�.
    // numDescriptors���� ���� ���� ��Ұ� �������� ������
    // past-the-end ���ͷ�����(������ ������ ���� ���Ҹ� ����Ŵ)�� ��ȯ�˴ϴ�.
    return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return m_NumFreeHandles;
}

/// <summary>
/// free list���� ��ũ���͸� �Ҵ��ϴ� �� ���˴ϴ�.
/// </summary>
/// <param name="numDescriptors">
/// free list���� ��ũ���� ����� �Ҵ�Ǹ� ���� free ����� �����ϰ�
/// ������ ��ũ���͸� free ������� ��ȯ�մϴ�.
/// </param>
/// <returns></returns>
DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    /*
     * ȣ���ڰ� �ϳ��� ��ũ���͸� ��û�ϰ� free ��Ͽ� 100���� ��ũ���ͷ�
     * ������ free ����� �ִ� ��� ������ 100���� ��ũ���ͷ� ������ free �����
     * ���ŵǰ� �ش� ��Ͽ��� 1���� ��ũ���Ͱ� �Ҵ�� �� 99���� ��ũ���ͷ�
     * ������ free ����� free ��Ͽ� �ٽ� �߰��˴ϴ�.
     */

    std::lock_guard<std::mutex> lock(m_AllocationMutex); // ��Ƽ �������� ���̽� ������� �����ϱ� ���� ���

    // ���� ��û�� ��ũ���� �������� ���� ���� ��ũ���Ͱ� ���� ������
    // NULL ��ũ���͸� ��ȯ�ϰ� �ٸ� ���� �õ��մϴ�.
    if (numDescriptors > m_NumFreeHandles)
    {
        return DescriptorAllocation();
    }

    // ��û�� ������ ��ŭ ����� ū ù ��° ����� �����ɴϴ�.
    auto smallestBlockIt = m_FreeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == m_FreeListBySize.end())
    {
        // ��û�� ������ �� �ִ� free ����� �����ϴ�.
        return DescriptorAllocation();
    }

    // ��û�� �����ϴ� ���� ���� ����� ũ���Դϴ�.
    auto blockSize = smallestBlockIt->first;

    // FreeListByOffset ���� ������ �׸� ���� �������Դϴ�.
    auto offsetIt = smallestBlockIt->second;

    // ��ũ���� ���� �������Դϴ�.
    auto offset = offsetIt->first;

    /*
     * �߰ߵ� free ����� free ��Ͽ��� �����ϰ� free ����� �����Ͽ�
     * ������ �� ����� free ��Ͽ� �ٽ� �߰��մϴ�.
     */

    // free ��Ͽ��� ���� free ����� �����մϴ�.
    m_FreeListBySize.erase(smallestBlockIt);
    m_FreeListByOffset.erase(offsetIt);

    //  �� ����� �����Ͽ� �����Ǵ� �� free ����� ����մϴ�.
    auto newOffset = offset + numDescriptors;
    auto newSize   = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // �Ҵ��� ��û�� ũ��� ��Ȯ�� ��ġ���� �ʴ� ���,
        // ���� �κ��� free ������� ��ȯ�մϴ�.
        AddNewBlock(newOffset, newSize);
    }

    // free handles�� ���� ��ŵ�ϴ�..
    m_NumFreeHandles -= numDescriptors;

    return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize),
        numDescriptors,
        m_DescriptorHandleIncrementSize,
        shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber)
{
    // ������ �� ������ �������� �������� ����մϴ�.
    auto offset = ComputeOffset(descriptorHandle.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    // �������� �Ϸ�� ������ ����� free ��Ͽ� �ٷ� �߰����� �ʽ��ϴ�.
    m_StaleDescriptors.emplace(offset, descriptorHandle.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    while (!m_StaleDescriptors.empty() && m_StaleDescriptors.front().FrameNumber <= frameNumber)
    {
        auto& staleDescriptor = m_StaleDescriptors.front();

        // ���� �ִ� ��ũ������ �������Դϴ�.
        auto offset = staleDescriptor.Offset;
        // �Ҵ�� ��ũ���� ���Դϴ�.
        auto numDescriptors = staleDescriptor.Size;

        FreeBlock(offset, numDescriptors);

        m_StaleDescriptors.pop();
    }
}

uint32_t DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>(handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
}

/// <summary>
/// free list�� ����� �߰��մϴ�.
/// ����� FreeListByOffset �ʰ� FreeListBySize �ʿ� ��� �߰��˴ϴ�.
/// </summary>
/// <param name="offset"></param>
/// <param name="numDescriptors"></param>
void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    // std::map::emplace �޼���� ù ��° ��Ұ� ���Ե� ��ҿ� ����
    // ���ͷ������� std::pair �� ��ȯ�մϴ�.
    auto offsetIt = m_FreeListByOffset.emplace(offset, numDescriptors);
    auto sizeIt   = m_FreeListBySize.emplace(numDescriptors, offsetIt.first);

    // FreeBlockInfo�� FreeListBySizeIt ��� ������ FreeListBySize ��Ƽ����
    // ���ͷ����͸� ����Ű���� �մϴ�.
    offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // �������� ������ �����º��� ū ù ��° ��Ҹ� ã���ϴ�.
    // �� ����� �����Ǵ� ��� �ڿ� ��Ÿ���� �ϴ� ����Դϴ�.
    auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);

    // �����Ǵ� ��� �տ� ��Ÿ���� ����� ã���ϴ�.
    auto prevBlockIt = nextBlockIt;
    // ��Ͽ��� ù ��° ����� �ƴ� ���.
    if (prevBlockIt != m_FreeListByOffset.begin())
    {
        // ��Ͽ��� ���� ������� �̵��մϴ�.
        --prevBlockIt;
    }
    else
    {
        // ����� ������ �����Ͽ� �����Ǵ� ��� �տ� ����� ������ ǥ���մϴ�.
        prevBlockIt = m_FreeListByOffset.end();
    }

    // ���� �ڵ��� ���� ���� �ٽ� �߰��մϴ�.
    // �� �۾��� ����� �����ϱ� ���� �����ؾ� �ϴµ�, 
    // ����� �����ϸ� numDescriptors ������ �����Ǳ� �����Դϴ�.
    m_NumFreeHandles += numDescriptors;

    if (prevBlockIt != m_FreeListByOffset.end() && offset == prevBlockIt->first + prevBlockIt->second.Size)
    {
        // ���� ����� ������ ��� �ٷ� �ڿ� �ִ� ���
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

         // ���� ��ϰ� ������ ũ�⸸ŭ ��� ũ�⸦ �ø��ϴ�.
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.Size;

        // free list���� ���� ����� �����մϴ�
        m_FreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(prevBlockIt);
    }

    if (nextBlockIt != m_FreeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        // ���� ����� ������ ��� �ٷ� �տ� �ֽ��ϴ�.
        //
        // Offset               NextBlock.Offset
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // ���� ��ϰ� �����ϴ� ũ�⸸ŭ ��� ũ�⸦ �ø��ϴ�.
        numDescriptors += nextBlockIt->second.Size;

        // ���� ��Ͽ��� ���� ����� �����մϴ�.
        m_FreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(nextBlockIt);
    }

    // ������ ����� ���� ��Ͽ� �߰��մϴ�.
    AddNewBlock(offset, numDescriptors);
}

DescriptorAllocation::DescriptorAllocation()
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
    uint32_t numHandles,
    uint32_t descriptorSize,
    std::shared_ptr<DescriptorAllocatorPage> page)
{
}

bool DescriptorAllocation::IsNull() const
{
    return false;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
{
    return D3D12_CPU_DESCRIPTOR_HANDLE();
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
    return 0;
}

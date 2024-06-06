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
/// 설명자 힙에서 연속된 설명자 블록을 할당합니다.
/// 설명자 힙(페이지)이 요청된 할당을 충족할 수 있을 때까지 요청된 설명자 수를 할당하려고 시도합니다.
/// 요청을 이행할 수 있는 설명자 힙이 없으면 요청을 이행할 수 있는 새 설명자 힙(페이지)이 생성됩니다.
/// </summary>
/// <param name="numDescriptors">생성할 설명자 수</param>
/// <returns></returns>
DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex); // 할당하기 전에 스레드가 할당자에게 독점적으로 접근하도록 잠금니다.

    DescriptorAllocation allocation; // 할당 결과를 담는 변수 입니다.

    for (auto iter = m_AvailableHeaps.begin(); iter != m_AvailableHeaps.end(); ++iter)
    {
        auto allocatorPage = m_HeapPool[*iter];

        // 할당이 이루어집니다.
        // numDescriptors 값을 충족할 수 있으면 유효한 DescriptorAllocation 가
        // 반환됩니다.
        allocation = allocatorPage->Allocate(numDescriptors);

        // 할당으로 페이지가 비어있는 경우(사용 가능한 설명자 수가 0에 도달할 때)
        // 현재 페이지의 인덱스(iter)가 사용 가능한 힙 집합(_availableHeaps)에서
        // 제거됩니다.
        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = m_AvailableHeaps.erase(iter);
        }

        // 할당자 페이지에서 유효한 설명자 핸들이 할당된 경우
        // (설명자 핸들이 null이 아닐 때) 루프가 중단됩니다.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // 설명자 할당의 유효성을 검사합니다.
    // 위 반복문을 다 돌았음에도 설명자가 유효하지 않은 경우, 적어도
    // 요청된 설명자 수(numDescriptors)만큼 큰 새로운 설명자 페이지를 만듭니다.
    if (allocation.IsNull())
    {
        m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, numDescriptors);
        auto newPage            = CreateAllocatorPage();

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

/// <summary>
/// 모든 설명자 힙 페이지의 ReleaseStaleDescriptors 메서드를 호출합니다.
/// 오래된 설명자를 해제한 후 페이지에 사용 가능한 핸들이 있으면
/// 사용 가능한 힙 목록(_availableHeaps)에 추가됩니다.
/// </summary>
/// <param name="frameNumber"></param>
void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex); // 뮤텍스를 잠금니다.

    // 모든 설명자 힙 페이지의 ReleaseStaleDescriptors 메서드를 호출합니다.
    for (size_t i = 0; i < m_HeapPool.size(); ++i)
    {
        auto page = m_HeapPool[i];

        page->ReleaseStaleDescriptors(frameNumber);

        // 사용 가능한 힙 인덱스를 추가합니다.
        if (page->NumFreeHandles() > 0)
        {
            m_AvailableHeaps.insert(i);
        }
    }
}

/// <summary>
/// 설명자의 새 페이지를 만드는 데 사용됩니다.
/// </summary>
/// <returns></returns>
std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
    auto newPage = std::make_shared<DescriptorAllocatorPage>(
        m_HeapType, m_NumDescriptorsPerHeap); // 새 DescriptorAllocatorPage 생성

    m_HeapPool.emplace_back(newPage);               // 새 DescriptorAllocatorPage 풀에 추가
    m_AvailableHeaps.insert(m_HeapPool.size() - 1); // DescriptorAllocatorPage의 인덱스 추가

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

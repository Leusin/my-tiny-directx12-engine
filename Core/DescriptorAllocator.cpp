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
        _device, m_HeapType, m_NumDescriptorsPerHeap); // 새 DescriptorAllocatorPage 생성

    m_HeapPool.emplace_back(newPage);               // 새 DescriptorAllocatorPage 풀에 추가
    m_AvailableHeaps.insert(m_HeapPool.size() - 1); // DescriptorAllocatorPage의 인덱스 추가

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

    // free lists 초기화
    AddNewBlock(0, m_NumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
    return m_HeapType;
}

bool DescriptorAllocatorPage::HasSpace(uint32_t numDescriptors) const
{
    // std::map::lower_bound 메서드는 free list에서 요청된 numDescriptors보다
    // 작지 않은(같거나 더 큰) 첫 번째 항목을 찾는 데 사용됩니다.
    // numDescriptors보다 작지 않은 요소가 존재하지 않으면
    // past-the-end 이터레이터(마지막 원소의 다음 원소를 가르킴)가 반환됩니다.
    return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
}

uint32_t DescriptorAllocatorPage::NumFreeHandles() const
{
    return m_NumFreeHandles;
}

/// <summary>
/// free list에서 디스크립터를 할당하는 데 사용됩니다.
/// </summary>
/// <param name="numDescriptors">
/// free list에서 디스크립터 블록이 할당되면 기존 free 블록을 분할하고
/// 나머지 디스크립터를 free 목록으로 반환합니다.
/// </param>
/// <returns></returns>
DescriptorAllocation DescriptorAllocatorPage::Allocate(uint32_t numDescriptors)
{
    /*
     * 호출자가 하나의 디스크립터만 요청하고 free 목록에 100개의 디스크립터로
     * 구성된 free 블록이 있는 경우 힙에서 100개의 디스크립터로 구성된 free 블록이
     * 제거되고 해당 블록에서 1개의 디스크립터가 할당된 후 99개의 디스크립터로
     * 구성된 free 블록이 free 목록에 다시 추가됩니다.
     */

    std::lock_guard<std::mutex> lock(m_AllocationMutex); // 멀티 스레드의 레이스 컨디션을 방지하기 위해 잠금

    // 힙에 요청된 디스크립터 개수보다 적은 수의 디스크립터가 남아 있으면
    // NULL 디스크립터를 반환하고 다른 힙을 시도합니다.
    if (numDescriptors > m_NumFreeHandles)
    {
        return DescriptorAllocation();
    }

    // 요청을 충족할 만큼 충분히 큰 첫 번째 블록을 가져옵니다.
    auto smallestBlockIt = m_FreeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == m_FreeListBySize.end())
    {
        // 요청을 충족할 수 있는 free 블록이 없습니다.
        return DescriptorAllocation();
    }

    // 요청을 충족하는 가장 작은 블록의 크기입니다.
    auto blockSize = smallestBlockIt->first;

    // FreeListByOffset 맵의 동일한 항목에 대한 포인터입니다.
    auto offsetIt = smallestBlockIt->second;

    // 디스크립터 힙의 오프셋입니다.
    auto offset = offsetIt->first;

    /*
     * 발견된 free 블록을 free 목록에서 제거하고 free 블록을 분할하여
     * 생성된 새 블록을 free 목록에 다시 추가합니다.
     */

    // free 목록에서 기존 free 블록을 제거합니다.
    m_FreeListBySize.erase(smallestBlockIt);
    m_FreeListByOffset.erase(offsetIt);

    //  이 블록을 분할하여 생성되는 새 free 블록을 계산합니다.
    auto newOffset = offset + numDescriptors;
    auto newSize   = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // 할당이 요청된 크기와 정확히 일치하지 않는 경우,
        // 남은 부분을 free 목록으로 반환합니다.
        AddNewBlock(newOffset, newSize);
    }

    // free handles를 감소 시킵니다..
    m_NumFreeHandles -= numDescriptors;

    return DescriptorAllocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, m_DescriptorHandleIncrementSize),
        numDescriptors,
        m_DescriptorHandleIncrementSize,
        shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber)
{
    // 설명자 힙 내에서 설명자의 오프셋을 계산합니다.
    auto offset = ComputeOffset(descriptorHandle.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    // 프레임이 완료될 때까지 블록을 free 목록에 바로 추가하지 않습니다.
    m_StaleDescriptors.emplace(offset, descriptorHandle.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_AllocationMutex);

    while (!m_StaleDescriptors.empty() && m_StaleDescriptors.front().FrameNumber <= frameNumber)
    {
        auto& staleDescriptor = m_StaleDescriptors.front();

        // 힙에 있는 디스크립터의 오프셋입니다.
        auto offset = staleDescriptor.Offset;
        // 할당된 디스크립터 수입니다.
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
/// free list에 블록을 추가합니다.
/// 블록은 FreeListByOffset 맵과 FreeListBySize 맵에 모두 추가됩니다.
/// </summary>
/// <param name="offset"></param>
/// <param name="numDescriptors"></param>
void DescriptorAllocatorPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    // std::map::emplace 메서드는 첫 번째 요소가 삽입된 요소에 대한
    // 이터레이터인 std::pair 을 반환합니다.
    auto offsetIt = m_FreeListByOffset.emplace(offset, numDescriptors);
    auto sizeIt   = m_FreeListBySize.emplace(numDescriptors, offsetIt.first);

    // FreeBlockInfo의 FreeListBySizeIt 멤버 변수가 FreeListBySize 멀티맵의
    // 이터레이터를 가리키도록 합니다.
    offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // 오프셋이 지정된 오프셋보다 큰 첫 번째 요소를 찾습니다.
    // 이 블록은 해제되는 블록 뒤에 나타나야 하는 블록입니다.
    auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);

    // 해제되는 블록 앞에 나타나는 블록을 찾습니다.
    auto prevBlockIt = nextBlockIt;
    // 목록에서 첫 번째 블록이 아닌 경우.
    if (prevBlockIt != m_FreeListByOffset.begin())
    {
        // 목록에서 이전 블록으로 이동합니다.
        --prevBlockIt;
    }
    else
    {
        // 목록의 끝으로 설정하여 해제되는 블록 앞에 블록이 없음을 표시합니다.
        prevBlockIt = m_FreeListByOffset.end();
    }

    // 여유 핸들의 수를 힙에 다시 추가합니다.
    // 이 작업은 블록을 병합하기 전에 수행해야 하는데, 
    // 블록을 병합하면 numDescriptors 변수가 수정되기 때문입니다.
    m_NumFreeHandles += numDescriptors;

    if (prevBlockIt != m_FreeListByOffset.end() && offset == prevBlockIt->first + prevBlockIt->second.Size)
    {
        // 이전 블록이 해제할 블록 바로 뒤에 있는 경우
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

         // 이전 블록과 병합한 크기만큼 블록 크기를 늘립니다.
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.Size;

        // free list에서 이전 블록을 제거합니다
        m_FreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(prevBlockIt);
    }

    if (nextBlockIt != m_FreeListByOffset.end() && offset + numDescriptors == nextBlockIt->first)
    {
        // 다음 블록은 해제할 블록 바로 앞에 있습니다.
        //
        // Offset               NextBlock.Offset
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // 다음 블록과 병합하는 크기만큼 블록 크기를 늘립니다.
        numDescriptors += nextBlockIt->second.Size;

        // 무료 목록에서 다음 블록을 제거합니다.
        m_FreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
        m_FreeListByOffset.erase(nextBlockIt);
    }

    // 해제된 블록을 무료 목록에 추가합니다.
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

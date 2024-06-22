/*
 * 설명자 란?
 * - 리소스에 대한 뷰(RTV, DSV, CBV, SRV, UAV)를 생성하는데 사용됩니다.
 * - CBV, SRV, UAV 또는 샘플러를 셰이더에 사용하려면 먼저
 *  CPU 표시 설명자 힙(CPU visible descriptor heap)으로 복사하는 작업을 처리합니다.
 *
 * - CPU 표시 설명자(CPU visible descriptor)는 CPU 메모리의 리소스 설명자를
 *  스테이징(staging)하는데 유용하며, 셰이더에서 사용하기 위해 GPU 표시 설명자 힙에 복사됩니다.
 *
 * - CPU 표시 설명자를 생성해서 사용하는 곳: RTV, DSV, CBV, SRV, UAV, 샘플러
 */

// DescriptorAllocator
// - 설명자를 요청하는 애플리케이션의 기본 인터페이스입니다.
// - 클래스는 설명자 페이지를 관리합니다.
//
// DescriptorAllocatorPage
// - ID3D12DescriptorHeap에 대한 래퍼 클래스입니다.
// - 페이지의 사용 가능한 목록을 추적합니다.
//
// DescriptorAllocation
// - DescriptorAllocator::Allocate 메서드에서 반환된 할당을 래핑합니다.
// - 원래 페이지에 대한 포인터도 저장하고 설명자가 더 이상 필요하지 않은 경우
//  자동으로 해제됩니다.
//
// DynamicDescriptorHeap
// - CPU 표시 설명자를 GPU 표시 설명자 힙으로 복사하는 작업을 처리합니다.
//

#pragma once

// DescriptorAllocator 클리스와 그외 클래스가 공통으로 쓰는 헤더
#include "d3dx12.h"
#include <cstdint> // uint32_t, uint64_t 사용.

// DescriptorAllocatorPage 가 사용하는 헤더
#include <d3d12.h> // ID3D12DescriptorHeap 사용.
#include <wrl.h>   // ComPtr 사용.

// 공동으로 사용되는 STL 타입 클래스
#include <map>
#include <mutex> // std::mutex 사용. DescriptorAllocator::Allocate 메서드에서 멀티스레드가 안전한 할당을 하도록합니다.
#include <memory> // std::shared_ptr 사용.
#include <queue>
#include <set> // std::set 사용. DescriptorAllocator에서 사용할 페이지 풀을 정렬된 인덱스 목록으로 저장합니다.
#include <vector>

class DescriptorAllocation
{
public:
    // Creates a NULL descriptor.
    DescriptorAllocation();

    DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
        uint32_t numHandles,
        uint32_t descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page);

    // The destructor will automatically free the allocation.
    //~DescriptorAllocation();

    // Copies are not allowed.
    // DescriptorAllocation(const DescriptorAllocation&)            = delete;
    // DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

    // Move is allowed.
    // DescriptorAllocation(DescriptorAllocation&& allocation);
    // DescriptorAllocation& operator=(DescriptorAllocation&& other);

    // Check if this a valid descriptor.
    bool IsNull() const;

    // Get a descriptor at a particular offset in the allocation.
    // D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

    // Get the number of (consecutive) handles for this allocation.
    // uint32_t GetNumHandles() const;


    // Get the heap that this allocation came from.
    // (For internal use only).
    // std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;
};


/// <summary>
/// - 클래스의 목적은 ID3D12DescriptorHeap dprp free list allocator 전략을 제공하기 위함 입니다.
/// - ID3D12DescriptorHeap의 래퍼일 뿐만 아니라 힙의 디스크립터를 관리하는 free list allocator도 구현합니다.
/// - DescriptorAllocator 클래스 외부에서 사용하지 않습니다.
/// - 클래스는 설명자의 할당 요청을 충족할 수 있어야 하지만, 여유 핸들(free handles)의 수를 쿼리하고 요청을 충족하기에
/// 충분한 공간이 있는지 확인하기 위해 함수도 제공해야 합니다.
///     - HasSpace: DescriptorAllocatorPage에 요청을 충족할 수 있을 만큼 큰 연속적인 설명자 블록이 있는지 확인합니다.
///     - NumFreeHandles: 설명자 힙에서 사용 가능한 설명자 핸들 수를 반환합니다. 사용 가능한 목록의 파편화(fragmentation)로
///     인해 사용 가능한 핸들 수보다 작거나 같은 할당은 여전히 실패할 수 있습니다.
///     - Allocate: 설명자 힙에서 연속된 여러 설명자를 할당합니다. DescriptorAllocatorPage가 요청을 충족할 수 없는 경우, 이 함수는 null을 반환합니다.
///     - Free: DescriptorAllocation을 힙으로 반환합니다. 디스크립터를 참조하는 명령 목록이 명령 대기열에서 실행을 완료할
///     때까지 디스크립터를 재사용할 수 없으므로 렌더 프레임 실행이 완료될 때까지 디스크립터는 힙으로 직접 반환되지 않습니다.
///     - ReleaseStaleDescriptors: 해제된 디스크립터를 재사용할 수 있도록 디스크립터 힙으로 되돌립니다.
/// </summary>
class DescriptorAllocatorPage
    // DescriptorAllocatorPage 클래스가 자체에서 std::shared_ptr을 검색할 수 있도록 하는
    // shared_from_this 멤버 함수를 제공합니다(DescriptorAllocatorPage::Allocate 메서드에서 사용).
    : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
    /// <summary>
    /// 생성자
    /// </summary>
    /// <param name="type">생성할 설명자 힙의 유형</param>
    /// <param name="numDescriptors">설명자 힙에 할당할 디스크립터 수</param>
    DescriptorAllocatorPage(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    /// <summary>
    /// DescriptorAllocatorPage를 구성하는 데 사용된 설명자 힙 유형을 반환합니다.
    /// </summary>
    /// <returns>설명자 힙 타입</returns>
    D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

    /// <summary>
    /// DescriptorAllocatorPage에 요청을 충족할 만큼 충분히 큰
    /// 연속적인 설명자 블록이 설명자 힙에 있는지 확인하는 데 사용됩니다.
    ///
    /// 할당 요청을 하기 전에 먼저 할당 요청이 성공할지 여부를 확인한 다음
    /// 실패 여부를 확인하는 것이 더 효율적인 경우가 많습니다.
    /// </summary>
    /// <param name="numDescriptors"></param>
    /// <returns></returns>
    bool HasSpace(uint32_t numDescriptors) const;

    /// <summary>
    /// DescriptorAllocatorPage가 아직 사용할 수 있는 설명자 핸들 수를 확인합니다.
    ///
    /// 사용 가능한 목록(free list)의 파편화로 인해 사용 가능한 핸들(free handles) 수가
    /// 요청된 연속 디스크립터 블록보다 적어 실패할 수도 있습니다.
    /// </summary>
    /// <returns></returns>
    uint32_t NumFreeHandles() const;

    /// <summary>
    /// 이 디스크립터 힙에 여러 디스스크립터를 할당합니다.
    ///
    /// 할당이 실패하면, NULL 설명자를 반환합니다.
    /// 참고: 설명자가 유효한지 확인하려면 DescriptorAllocation::IsNull를 사용합니다.
    /// </summary>
    /// <param name="numDescriptors">할당할 디스크립터 수</param>
    /// <returns></returns>
    DescriptorAllocation Allocate(uint32_t numDescriptors);

    /// <summary>
    /// 설명자를 힙으로 다시 반환합니다.
    ///
    /// DescriptorAllocatorPage::Allocate 메서드를 사용하여 할당된
    /// DescriptorAllocation을 해제하는 데 사용됩니다.
    ///
    /// 사용하지 않는 경우 DescriptorAllocation 클래스가 자동으로
    /// 원래의 DescriptorAllocatorPage로 돌아가기 때문에
    /// 이 메서드를 직접 호출할 필요는 없습니다.
    ///
    /// 이 메서드는 DescriptorAllocation을 r-value 참조로 사용하므로
    /// 원래 DescriptorAllocation이 유효하지 않은 상태로 함수로 이동됩니다.
    /// </summary>
    /// <param name="descriptorHandle"></param>
    /// <param name="frameNumber">
    /// 오래된 디스크립터는 직접 해제되지 않고 오래된 할당 큐에 저장됩니다.
    /// 오래된 할당은 DescriptorAllocatorPage::ReleaseStaleAllocations 메서드를 통해
    /// 힙으로 반환됩니다.
    /// </param>
    void Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber);

    /// <summary>
    /// 오래된 설명자를 설명자 힙으로 다시 반환합니다.
    /// </summary>
    /// <param name="frameNumber">
    /// 완료된 프레임 번호.해당 프레임 동안 해제된 모든 디스크립터는 힙으로 반환됩니다.
    /// </param>
    void ReleaseStaleDescriptors(uint64_t frameNumber);

protected:
    /// <summary>
    /// 기본 설명자(base descriptor)부터 지정된 설명자 핸들까지의 설명자 수를 계산합니다.
    ///
    /// 이 메서드는 설명자가 해제되었을 때 힙에서 설명자를 재배치할 위치를
    /// 결정하는 데 사용됩니다.
    /// </summary>
    /// <param name="handle"></param>
    /// <returns></returns>
    uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    /// <summary>
    /// free list에 설명자 블록 하나를 추가합니다.
    ///
    /// 다음 상황에서 이 메서드가 사용됩니다.
    /// - 모든 설명자를 포함하는 free list를 하나의 블록으로 초기화할 때
    /// - 할당 중에 설명자 블록을 분할할 때
    /// - 설명자가 해제된 경우 인접한 블록을 병합할 때
    /// </summary>
    /// <param name="offset"></param>
    /// <param name="numDescriptors"></param>
    void AddNewBlock(uint32_t offset, uint32_t numDescriptors);

    // Free a block of descriptors.
    // This will also merge free blocks in the free list to form larger blocks
    // that can be reused.

    /// <summary>
    /// 하나의 블록 설명자를 할당 해제 합니다.
    /// 또한 free list의 free 블록들을 병합하여 재사용할 수 있는 더 큰 블록으로 만듭니다.
    ///
    /// 이 메서드는 릴리스된 설명자를 설명자 힙에 다시 커밋하기 위해
    /// ReleaseStaleDescriptors 메서드에서 사용됩니다. free list에서
    /// 인접한 블록을 병합할 수 있는지 확인합니다. free list에서 free 블록을 병합하면
    /// free list의 파편화가 줄어듭니다.
    /// </summary>
    /// <param name="offset"></param>
    /// <param name="numDescriptors"></param>
    void FreeBlock(uint32_t offset, uint32_t numDescriptors);

private:
    /// 코드 가독성을 개선하고 모호성을 줄이기 위해 정의한 유형 별징(alias)입니다.
    // 설명자 힙 내의 (설명자의)오프셋을.
    using OffsetType = uint32_t;
    // 사용 가능한 설명자 수.
    using SizeType = uint32_t;

    struct FreeBlockInfo;
    // 설명자 힙 내의 오프셋을 기준으로 여유(free) 블록을 나열하는 맵입니다.
    using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

    // 여유(free) 블록을 크기별로 나열하는 맵입니다.
    // 여러 블록의 크기가 같을 수 있으므로 멀티맵이어야 합니다.
    // (multimap은 map과 거의 동일하지만, key값이 중복 가능하다)
    using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

    /// <summary>
    /// 단순히 free list에 있는 블록의 크기와 해당 항목에 대한 참조(이터레이터)를 
    /// FreeListBySize 맵에 저장합니다. FreeBlockInfo 구조체는 free list에서 인접한 
    /// 블록을 병합할 때 해당 항목을 검색하지 않고 빠르게 제거할 수 있도록 
    /// FreeListBySize 맵에 해당 항목에 대한 이터레이터를 저장합니다.
    /// </summary>
    /// 
    struct FreeBlockInfo
    {
        FreeBlockInfo(SizeType size)
            : Size(size)
        {
        }

        SizeType Size;
        FreeListBySize::iterator FreeListBySizeIt;
    };

    /// <summary>
    /// 할당 해제된 프레임이 GPU에서 실행이 완료될 때까지 재사용할 수 없는 
    /// (할당 해제된)설명자 힙의 설명자를 추적하는 데 사용됩니다.
    /// </summary>
    struct StaleDescriptorInfo
    {
        /// <summary>
        /// </summary>
        /// <param name="offset">첫 번째 설명자의 오프셋</param>
        /// <param name="size">설명자 범위의 디스크립터 수</param>
        /// <param name="frame">설명자가 할당 해제된 프레임</param>
        StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
            : Offset(offset)
            , Size(size)
            , FrameNumber(frame)
        {
        }

        // 디스크립터 힙 내의 오프셋.
        OffsetType Offset;
        // 설명자 수
        SizeType Size;
        // 디스크립터가 해제된 프레임 번호.
        uint64_t FrameNumber;
    };

    // 오래된 설명자는 해제된 프레임이 완료될 때까지 해제 대기열에 대기합니다.
    using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

    FreeListByOffset m_FreeListByOffset;
    FreeListBySize m_FreeListBySize;
    StaleDescriptorQueue m_StaleDescriptors;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
    // 클래스에서 사용하는 설명자 힙의 유형.
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType; 
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
    // 설명자 힙 내 설명자의 증분(increment) 크기는 제조업체에 따라 다르므로 런타임에 쿼리해야 합니다.
    uint32_t m_DescriptorHandleIncrementSize; 
    uint32_t m_NumDescriptorsInHeap; //설명자 힙에 있는 총 설명자 개수.
    uint32_t m_NumFreeHandles;

    // 멀티 스레드에서 안전한 액세스 할당 및 할당 해제를 보장하는 데 사용됩니다.
    std::mutex m_AllocationMutex;
};

/// <summary>
/// 디스크립터를 할당하는 클래스 입니다. 리소스에게 필요한 모든 디스크립터를 관리합니다.
/// - 텍스처와 같이 새 리소스를 로드할 때 응용프로그램에 디스크립터를 할당하는데 사용합니다.
/// - 사용되지 않는 디스크립터는 재사용할 수 있도록 자동으로 디스크립터 힙에 반환합니다.
///
/// - DescriptorAllocatorPages 풀을 저장합니다. 요청을 만족시킬 수 있는 페이지가 없으면 새 페이지가 생성되어 풀에 추가됩니다.
///
/// - DescriptorAllocatorPage 포인터를 사용하고, DescriptorAllocation 클래스에 의존적입니다.
/// </summary>
class DescriptorAllocator
{
public:
    /// <summary>
    /// </summary>
    /// <param name="type">DescriptorAllocator가 할당할 설명자의 타입입니다.</param>
    /// <param name="numDescriptorsPerHeap"> 설명자 힙당 설명자 수입니다. 기본값으로 256개의 설명자가 생성됩니다.</param>
    DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

    /// <summary>
    /// CPU 표시 설명자 힙에서 여러 개의 연속 설명자를 할당합니다.
    /// </summary>
    /// <param name="numDescriptors">생성할 설명자 수.기본값으로 하나의 설명자만 할당됩니다. 둘 이상 필요한 경우 인수를
    /// 지정합니다.</param> <returns>설명자의 래퍼 클래스인 DescriptorAllocation을 반환합니다.</returns>
    DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

    /// <summary>
    /// 재사용을 위해 더는 사용하지 않는 설명자를 해제합니다.
    /// 커맨드 큐에가 참조되지 않을 때만 호출되어야 합니다.
    /// </summary>
    /// <param name="frameNumber"></param>
    void ReleaseStaleDescriptors(uint64_t frameNumber);

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    /// <summary>
    /// 페이지 풀에 할당 요청을 만족할 수 있는 페이지가 없는 경우 새 할당자 페이지를 생성
    /// </summary>
    /// <returns></returns>
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    /// <summary>
    /// 할당할 설명자 타입을 저장합니다. 새 설명자 힙을 만들때도 사용됩니다.
    /// </summary>
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;

    /// <summary>
    /// 설명자 힙당 생성할 설명자 수를 저장합니다.
    /// </summary>
    uint32_t m_NumDescriptorsPerHeap;

    /// <summary>
    /// DescriptorAllocatorPage 포인터 타입의 std::Vector 입니다.
    /// 할당된 모든 페이지를 추적합니다.
    /// </summary>
    DescriptorHeapPool m_HeapPool;

    /// <summary>
    /// _heapPoo에서 사용 가능한 페이지 인덱스 입니다.
    /// DescriptorAllocatorPage의 모든 설명자가 사용되었을 때 _heapPool
    /// 벡터에 있는 해당 페이지의 인덱스가 _availableHeaps 세트에서 제거됩니다.
    /// </summary>
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;

    ID3D12Device2* _device;
};

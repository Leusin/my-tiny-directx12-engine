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

#include "d3dx12.h"

#include <cstdint> // uint32_t, uint64_t 타입을 사용합니다.
#include <mutex> // std::mutex를 통해 DescriptorAllocator::Allocate 메서드에서 멀티스레드가 안전한 할당을 하도록합니다.
#include <memory> // std::shared_ptr를 사용합니다.
#include <set>    // std::set 를 통해사용할 페이지 풀을 정렬된 인덱스 목록으로 저장합니다.
#include <vector>

class DescriptorAllocation
{
public:
    // Creates a NULL descriptor.
    //DescriptorAllocation();

    /*DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
        uint32_t numHandles,
        uint32_t descriptorSize,
        std::shared_ptr<DescriptorAllocatorPage> page);*/

    // The destructor will automatically free the allocation.
    //~DescriptorAllocation();

    // Copies are not allowed.
    //DescriptorAllocation(const DescriptorAllocation&)            = delete;
    //DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

    // Move is allowed.
    //DescriptorAllocation(DescriptorAllocation&& allocation);
    //DescriptorAllocation& operator=(DescriptorAllocation&& other);

    // Check if this a valid descriptor.
    bool IsNull() const;

    // Get a descriptor at a particular offset in the allocation.
    //D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

    // Get the number of (consecutive) handles for this allocation.
    //uint32_t GetNumHandles() const;

    
    // Get the heap that this allocation came from.
    // (For internal use only).
    //std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocatorPage() const;
};

class DescriptorAllocatorPage: public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
    DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    //D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

    /**
     * Check to see if this descriptor page has a contiguous block of descriptors
     * large enough to satisfy the request.
     */
    //bool HasSpace(uint32_t numDescriptors) const;

    /**
     * Get the number of available handles in the heap.
     */
    uint32_t NumFreeHandles() const;

    /**
     * Allocate a number of descriptors from this descriptor heap.
     * If the allocation cannot be satisfied, then a NULL descriptor
     * is returned.
     */
    DescriptorAllocation Allocate(uint32_t numDescriptors);

     /**
     * Return a descriptor back to the heap.
     * @param frameNumber Stale descriptors are not freed directly, but put
     * on a stale allocations queue. Stale allocations are returned to the heap
     * using the DescriptorAllocatorPage::ReleaseStaleAllocations method.
     */
    //void Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber);

    /**
     * Returned the stale descriptors back to the descriptor heap.
     */
    void ReleaseStaleDescriptors(uint64_t frameNumber);
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
    ///
    /// </summary>
    /// <param name="type">DescriptorAllocator가 할당할 설명자의 타입입니다.</param>
    /// <param name="numDescriptorsPerHeap"> 설명자 힙당 설명자 수입니다. 기본값으로 256개의 설명자가 생성됩니다.</param>
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
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
};

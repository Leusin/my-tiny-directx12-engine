/*
 * ������ ��?
 * - ���ҽ��� ���� ��(RTV, DSV, CBV, SRV, UAV)�� �����ϴµ� ���˴ϴ�.
 * - CBV, SRV, UAV �Ǵ� ���÷��� ���̴��� ����Ϸ��� ����
 *  CPU ǥ�� ������ ��(CPU visible descriptor heap)���� �����ϴ� �۾��� ó���մϴ�.
 *
 * - CPU ǥ�� ������(CPU visible descriptor)�� CPU �޸��� ���ҽ� �����ڸ�
 *  ������¡(staging)�ϴµ� �����ϸ�, ���̴����� ����ϱ� ���� GPU ǥ�� ������ ���� ����˴ϴ�.
 *
 * - CPU ǥ�� �����ڸ� �����ؼ� ����ϴ� ��: RTV, DSV, CBV, SRV, UAV, ���÷�
 */

// DescriptorAllocator
// - �����ڸ� ��û�ϴ� ���ø����̼��� �⺻ �������̽��Դϴ�.
// - Ŭ������ ������ �������� �����մϴ�.
//
// DescriptorAllocatorPage
// - ID3D12DescriptorHeap�� ���� ���� Ŭ�����Դϴ�.
// - �������� ��� ������ ����� �����մϴ�.
//
// DescriptorAllocation
// - DescriptorAllocator::Allocate �޼��忡�� ��ȯ�� �Ҵ��� �����մϴ�.
// - ���� �������� ���� �����͵� �����ϰ� �����ڰ� �� �̻� �ʿ����� ���� ���
//  �ڵ����� �����˴ϴ�.
//
// DynamicDescriptorHeap
// - CPU ǥ�� �����ڸ� GPU ǥ�� ������ ������ �����ϴ� �۾��� ó���մϴ�.
//

#pragma once

#include "d3dx12.h"

#include <cstdint> // uint32_t, uint64_t Ÿ���� ����մϴ�.
#include <mutex> // std::mutex�� ���� DescriptorAllocator::Allocate �޼��忡�� ��Ƽ�����尡 ������ �Ҵ��� �ϵ����մϴ�.
#include <memory> // std::shared_ptr�� ����մϴ�.
#include <set>    // std::set �� ���ػ���� ������ Ǯ�� ���ĵ� �ε��� ������� �����մϴ�.
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
/// ��ũ���͸� �Ҵ��ϴ� Ŭ���� �Դϴ�. ���ҽ����� �ʿ��� ��� ��ũ���͸� �����մϴ�.
/// - �ؽ�ó�� ���� �� ���ҽ��� �ε��� �� �������α׷��� ��ũ���͸� �Ҵ��ϴµ� ����մϴ�.
/// - ������ �ʴ� ��ũ���ʹ� ������ �� �ֵ��� �ڵ����� ��ũ���� ���� ��ȯ�մϴ�.
///
/// - DescriptorAllocatorPages Ǯ�� �����մϴ�. ��û�� ������ų �� �ִ� �������� ������ �� �������� �����Ǿ� Ǯ�� �߰��˴ϴ�.
///
/// - DescriptorAllocatorPage �����͸� ����ϰ�, DescriptorAllocation Ŭ������ �������Դϴ�.
/// </summary>
class DescriptorAllocator
{
public:
    /// <summary>
    ///
    /// </summary>
    /// <param name="type">DescriptorAllocator�� �Ҵ��� �������� Ÿ���Դϴ�.</param>
    /// <param name="numDescriptorsPerHeap"> ������ ���� ������ ���Դϴ�. �⺻������ 256���� �����ڰ� �����˴ϴ�.</param>
    DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

    /// <summary>
    /// CPU ǥ�� ������ ������ ���� ���� ���� �����ڸ� �Ҵ��մϴ�.
    /// </summary>
    /// <param name="numDescriptors">������ ������ ��.�⺻������ �ϳ��� �����ڸ� �Ҵ�˴ϴ�. �� �̻� �ʿ��� ��� �μ���
    /// �����մϴ�.</param> <returns>�������� ���� Ŭ������ DescriptorAllocation�� ��ȯ�մϴ�.</returns>
    DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

    /// <summary>
    /// ������ ���� ���� ������� �ʴ� �����ڸ� �����մϴ�.
    /// Ŀ�ǵ� ť���� �������� ���� ���� ȣ��Ǿ�� �մϴ�.
    /// </summary>
    /// <param name="frameNumber"></param>
    void ReleaseStaleDescriptors(uint64_t frameNumber);

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    /// <summary>
    /// ������ Ǯ�� �Ҵ� ��û�� ������ �� �ִ� �������� ���� ��� �� �Ҵ��� �������� ����
    /// </summary>
    /// <returns></returns>
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    /// <summary>
    /// �Ҵ��� ������ Ÿ���� �����մϴ�. �� ������ ���� ���鶧�� ���˴ϴ�.
    /// </summary>
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;

    /// <summary>
    /// ������ ���� ������ ������ ���� �����մϴ�.
    /// </summary>
    uint32_t m_NumDescriptorsPerHeap;

    /// <summary>
    /// DescriptorAllocatorPage ������ Ÿ���� std::Vector �Դϴ�.
    /// �Ҵ�� ��� �������� �����մϴ�.
    /// </summary>
    DescriptorHeapPool m_HeapPool;

    /// <summary>
    /// _heapPoo���� ��� ������ ������ �ε��� �Դϴ�.
    /// DescriptorAllocatorPage�� ��� �����ڰ� ���Ǿ��� �� _heapPool
    /// ���Ϳ� �ִ� �ش� �������� �ε����� _availableHeaps ��Ʈ���� ���ŵ˴ϴ�.
    /// </summary>
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;
};

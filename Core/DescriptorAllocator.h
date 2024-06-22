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

// DescriptorAllocator Ŭ������ �׿� Ŭ������ �������� ���� ���
#include "d3dx12.h"
#include <cstdint> // uint32_t, uint64_t ���.

// DescriptorAllocatorPage �� ����ϴ� ���
#include <d3d12.h> // ID3D12DescriptorHeap ���.
#include <wrl.h>   // ComPtr ���.

// �������� ���Ǵ� STL Ÿ�� Ŭ����
#include <map>
#include <mutex> // std::mutex ���. DescriptorAllocator::Allocate �޼��忡�� ��Ƽ�����尡 ������ �Ҵ��� �ϵ����մϴ�.
#include <memory> // std::shared_ptr ���.
#include <queue>
#include <set> // std::set ���. DescriptorAllocator���� ����� ������ Ǯ�� ���ĵ� �ε��� ������� �����մϴ�.
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
/// - Ŭ������ ������ ID3D12DescriptorHeap dprp free list allocator ������ �����ϱ� ���� �Դϴ�.
/// - ID3D12DescriptorHeap�� ������ �Ӹ� �ƴ϶� ���� ��ũ���͸� �����ϴ� free list allocator�� �����մϴ�.
/// - DescriptorAllocator Ŭ���� �ܺο��� ������� �ʽ��ϴ�.
/// - Ŭ������ �������� �Ҵ� ��û�� ������ �� �־�� ������, ���� �ڵ�(free handles)�� ���� �����ϰ� ��û�� �����ϱ⿡
/// ����� ������ �ִ��� Ȯ���ϱ� ���� �Լ��� �����ؾ� �մϴ�.
///     - HasSpace: DescriptorAllocatorPage�� ��û�� ������ �� ���� ��ŭ ū �������� ������ ����� �ִ��� Ȯ���մϴ�.
///     - NumFreeHandles: ������ ������ ��� ������ ������ �ڵ� ���� ��ȯ�մϴ�. ��� ������ ����� ����ȭ(fragmentation)��
///     ���� ��� ������ �ڵ� ������ �۰ų� ���� �Ҵ��� ������ ������ �� �ֽ��ϴ�.
///     - Allocate: ������ ������ ���ӵ� ���� �����ڸ� �Ҵ��մϴ�. DescriptorAllocatorPage�� ��û�� ������ �� ���� ���, �� �Լ��� null�� ��ȯ�մϴ�.
///     - Free: DescriptorAllocation�� ������ ��ȯ�մϴ�. ��ũ���͸� �����ϴ� ��� ����� ��� ��⿭���� ������ �Ϸ���
///     ������ ��ũ���͸� ������ �� �����Ƿ� ���� ������ ������ �Ϸ�� ������ ��ũ���ʹ� ������ ���� ��ȯ���� �ʽ��ϴ�.
///     - ReleaseStaleDescriptors: ������ ��ũ���͸� ������ �� �ֵ��� ��ũ���� ������ �ǵ����ϴ�.
/// </summary>
class DescriptorAllocatorPage
    // DescriptorAllocatorPage Ŭ������ ��ü���� std::shared_ptr�� �˻��� �� �ֵ��� �ϴ�
    // shared_from_this ��� �Լ��� �����մϴ�(DescriptorAllocatorPage::Allocate �޼��忡�� ���).
    : public std::enable_shared_from_this<DescriptorAllocatorPage>
{
public:
    /// <summary>
    /// ������
    /// </summary>
    /// <param name="type">������ ������ ���� ����</param>
    /// <param name="numDescriptors">������ ���� �Ҵ��� ��ũ���� ��</param>
    DescriptorAllocatorPage(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

    /// <summary>
    /// DescriptorAllocatorPage�� �����ϴ� �� ���� ������ �� ������ ��ȯ�մϴ�.
    /// </summary>
    /// <returns>������ �� Ÿ��</returns>
    D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

    /// <summary>
    /// DescriptorAllocatorPage�� ��û�� ������ ��ŭ ����� ū
    /// �������� ������ ����� ������ ���� �ִ��� Ȯ���ϴ� �� ���˴ϴ�.
    ///
    /// �Ҵ� ��û�� �ϱ� ���� ���� �Ҵ� ��û�� �������� ���θ� Ȯ���� ����
    /// ���� ���θ� Ȯ���ϴ� ���� �� ȿ������ ��찡 �����ϴ�.
    /// </summary>
    /// <param name="numDescriptors"></param>
    /// <returns></returns>
    bool HasSpace(uint32_t numDescriptors) const;

    /// <summary>
    /// DescriptorAllocatorPage�� ���� ����� �� �ִ� ������ �ڵ� ���� Ȯ���մϴ�.
    ///
    /// ��� ������ ���(free list)�� ����ȭ�� ���� ��� ������ �ڵ�(free handles) ����
    /// ��û�� ���� ��ũ���� ��Ϻ��� ���� ������ ���� �ֽ��ϴ�.
    /// </summary>
    /// <returns></returns>
    uint32_t NumFreeHandles() const;

    /// <summary>
    /// �� ��ũ���� ���� ���� �𽺽�ũ���͸� �Ҵ��մϴ�.
    ///
    /// �Ҵ��� �����ϸ�, NULL �����ڸ� ��ȯ�մϴ�.
    /// ����: �����ڰ� ��ȿ���� Ȯ���Ϸ��� DescriptorAllocation::IsNull�� ����մϴ�.
    /// </summary>
    /// <param name="numDescriptors">�Ҵ��� ��ũ���� ��</param>
    /// <returns></returns>
    DescriptorAllocation Allocate(uint32_t numDescriptors);

    /// <summary>
    /// �����ڸ� ������ �ٽ� ��ȯ�մϴ�.
    ///
    /// DescriptorAllocatorPage::Allocate �޼��带 ����Ͽ� �Ҵ��
    /// DescriptorAllocation�� �����ϴ� �� ���˴ϴ�.
    ///
    /// ������� �ʴ� ��� DescriptorAllocation Ŭ������ �ڵ�����
    /// ������ DescriptorAllocatorPage�� ���ư��� ������
    /// �� �޼��带 ���� ȣ���� �ʿ�� �����ϴ�.
    ///
    /// �� �޼���� DescriptorAllocation�� r-value ������ ����ϹǷ�
    /// ���� DescriptorAllocation�� ��ȿ���� ���� ���·� �Լ��� �̵��˴ϴ�.
    /// </summary>
    /// <param name="descriptorHandle"></param>
    /// <param name="frameNumber">
    /// ������ ��ũ���ʹ� ���� �������� �ʰ� ������ �Ҵ� ť�� ����˴ϴ�.
    /// ������ �Ҵ��� DescriptorAllocatorPage::ReleaseStaleAllocations �޼��带 ����
    /// ������ ��ȯ�˴ϴ�.
    /// </param>
    void Free(DescriptorAllocation&& descriptorHandle, uint64_t frameNumber);

    /// <summary>
    /// ������ �����ڸ� ������ ������ �ٽ� ��ȯ�մϴ�.
    /// </summary>
    /// <param name="frameNumber">
    /// �Ϸ�� ������ ��ȣ.�ش� ������ ���� ������ ��� ��ũ���ʹ� ������ ��ȯ�˴ϴ�.
    /// </param>
    void ReleaseStaleDescriptors(uint64_t frameNumber);

protected:
    /// <summary>
    /// �⺻ ������(base descriptor)���� ������ ������ �ڵ������ ������ ���� ����մϴ�.
    ///
    /// �� �޼���� �����ڰ� �����Ǿ��� �� ������ �����ڸ� ���ġ�� ��ġ��
    /// �����ϴ� �� ���˴ϴ�.
    /// </summary>
    /// <param name="handle"></param>
    /// <returns></returns>
    uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    /// <summary>
    /// free list�� ������ ��� �ϳ��� �߰��մϴ�.
    ///
    /// ���� ��Ȳ���� �� �޼��尡 ���˴ϴ�.
    /// - ��� �����ڸ� �����ϴ� free list�� �ϳ��� ������� �ʱ�ȭ�� ��
    /// - �Ҵ� �߿� ������ ����� ������ ��
    /// - �����ڰ� ������ ��� ������ ����� ������ ��
    /// </summary>
    /// <param name="offset"></param>
    /// <param name="numDescriptors"></param>
    void AddNewBlock(uint32_t offset, uint32_t numDescriptors);

    // Free a block of descriptors.
    // This will also merge free blocks in the free list to form larger blocks
    // that can be reused.

    /// <summary>
    /// �ϳ��� ��� �����ڸ� �Ҵ� ���� �մϴ�.
    /// ���� free list�� free ��ϵ��� �����Ͽ� ������ �� �ִ� �� ū ������� ����ϴ�.
    ///
    /// �� �޼���� �������� �����ڸ� ������ ���� �ٽ� Ŀ���ϱ� ����
    /// ReleaseStaleDescriptors �޼��忡�� ���˴ϴ�. free list����
    /// ������ ����� ������ �� �ִ��� Ȯ���մϴ�. free list���� free ����� �����ϸ�
    /// free list�� ����ȭ�� �پ��ϴ�.
    /// </summary>
    /// <param name="offset"></param>
    /// <param name="numDescriptors"></param>
    void FreeBlock(uint32_t offset, uint32_t numDescriptors);

private:
    /// �ڵ� �������� �����ϰ� ��ȣ���� ���̱� ���� ������ ���� ��¡(alias)�Դϴ�.
    // ������ �� ���� (��������)��������.
    using OffsetType = uint32_t;
    // ��� ������ ������ ��.
    using SizeType = uint32_t;

    struct FreeBlockInfo;
    // ������ �� ���� �������� �������� ����(free) ����� �����ϴ� ���Դϴ�.
    using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

    // ����(free) ����� ũ�⺰�� �����ϴ� ���Դϴ�.
    // ���� ����� ũ�Ⱑ ���� �� �����Ƿ� ��Ƽ���̾�� �մϴ�.
    // (multimap�� map�� ���� ����������, key���� �ߺ� �����ϴ�)
    using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

    /// <summary>
    /// �ܼ��� free list�� �ִ� ����� ũ��� �ش� �׸� ���� ����(���ͷ�����)�� 
    /// FreeListBySize �ʿ� �����մϴ�. FreeBlockInfo ����ü�� free list���� ������ 
    /// ����� ������ �� �ش� �׸��� �˻����� �ʰ� ������ ������ �� �ֵ��� 
    /// FreeListBySize �ʿ� �ش� �׸� ���� ���ͷ����͸� �����մϴ�.
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
    /// �Ҵ� ������ �������� GPU���� ������ �Ϸ�� ������ ������ �� ���� 
    /// (�Ҵ� ������)������ ���� �����ڸ� �����ϴ� �� ���˴ϴ�.
    /// </summary>
    struct StaleDescriptorInfo
    {
        /// <summary>
        /// </summary>
        /// <param name="offset">ù ��° �������� ������</param>
        /// <param name="size">������ ������ ��ũ���� ��</param>
        /// <param name="frame">�����ڰ� �Ҵ� ������ ������</param>
        StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frame)
            : Offset(offset)
            , Size(size)
            , FrameNumber(frame)
        {
        }

        // ��ũ���� �� ���� ������.
        OffsetType Offset;
        // ������ ��
        SizeType Size;
        // ��ũ���Ͱ� ������ ������ ��ȣ.
        uint64_t FrameNumber;
    };

    // ������ �����ڴ� ������ �������� �Ϸ�� ������ ���� ��⿭�� ����մϴ�.
    using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

    FreeListByOffset m_FreeListByOffset;
    FreeListBySize m_FreeListBySize;
    StaleDescriptorQueue m_StaleDescriptors;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
    // Ŭ�������� ����ϴ� ������ ���� ����.
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType; 
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
    // ������ �� �� �������� ����(increment) ũ��� ������ü�� ���� �ٸ��Ƿ� ��Ÿ�ӿ� �����ؾ� �մϴ�.
    uint32_t m_DescriptorHandleIncrementSize; 
    uint32_t m_NumDescriptorsInHeap; //������ ���� �ִ� �� ������ ����.
    uint32_t m_NumFreeHandles;

    // ��Ƽ �����忡�� ������ �׼��� �Ҵ� �� �Ҵ� ������ �����ϴ� �� ���˴ϴ�.
    std::mutex m_AllocationMutex;
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
    /// </summary>
    /// <param name="type">DescriptorAllocator�� �Ҵ��� �������� Ÿ���Դϴ�.</param>
    /// <param name="numDescriptorsPerHeap"> ������ ���� ������ ���Դϴ�. �⺻������ 256���� �����ڰ� �����˴ϴ�.</param>
    DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
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

    ID3D12Device2* _device;
};

#pragma once

#include "MemorySizeDefines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

/// <summary>
/// GPU로 데이터를 업로드하는 데 사용됩니다. 특히 동적 상수, 정점, 인덱스 버퍼 
/// 데이터를 업로드하는 데 사용됩니다.
/// </summary>
class UploadBuffer
{
public:
    // GPU에 데이터를 업로드하는 데 사용합니다.
    struct Allocation
    {
        void* CPU;
        D3D12_GPU_VIRTUAL_ADDRESS GPU;
    };

    /// <param name="pageSize">GPU 메모리의 새 페이지에 할당될 사이즈</param>
    explicit UploadBuffer(ID3D12Device2* device, size_t pageSize = _2MB);

    virtual ~UploadBuffer();

    /// <returns>The maximum size of an allocation is the size of a single page.</returns>
    size_t GetPageSize() const
    {
        return m_PageSize;
    }

    /// <summary>
    /// 업로드 힙에 메모리를 할당합니다. 
    /// 할당은 페이지 크기를 초과할 수 없습니다. memcpy 또는 유사한 메서드를 사용해
    /// 이 함수에서 반환된 할당 구조의 CPU 포인터에 버퍼 데이터를 복사합니다.
    /// </summary>
    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    /// <summary>
    /// 재사용을 위해 모든 할당을 해제합니다.
    /// 커맨드 리스트의 실행을 마쳤을 떄 사용할 수 있습니다.
    /// </summary>
    void Reset();

private:
    // 할당자를 위한 페이지 정의
    struct Page
    {
        Page(size_t sizeInBytes);
        ~Page();

        /// <summary>
        /// 페이지에 할당할 공간이 있는지 확인합니다.
        /// </summary>
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        /// <summary>
        /// 페이지에 메모리를 할당합니다.
        /// 할당 크기가 너무 크면 std::bad_alloc을 발생시킵니다.
        /// </summary>
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        // 재사용을 위해 페이지를 재설정 합니다.
        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

        // 기반 포인터.
        void* m_CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

        // 할당된 페이지 사이즈.
        size_t m_PageSize;
        // 바이트 단위의 현재 할당 오프셋.
        size_t m_Offset;
    };

    // 메모리 페이지 풀.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    // 사용 가능한 페이지 풀에서 페이지를 요청하거나 
    // 사용 가능한 페이지가 없는 경우 새 페이지를 만듭니다.
    std::shared_ptr<Page> RequestPage();

    PagePool m_PagePool;
    PagePool m_AvailablePages;

    std::shared_ptr<Page> m_CurrentPage;

    // 각 페이지의 메모리 크기.
    size_t m_PageSize;
};

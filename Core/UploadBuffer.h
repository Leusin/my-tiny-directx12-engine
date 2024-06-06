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
    /// 메모리 페이지에 있는 메모리 청크(또는 블록)을 할당합니다.
    /// (청크(Chunk): 한 개의 묶여진 정보)
    /// </summary>
    /// <param name="sizeInBytes">할당 크기(바이트 단위)</param>
    /// <param name="alignment">할당 메모리 정렬(바이트 단위). e.g., 상수 버퍼에 대한 할당은 256바이트로 정렬되어야 합니다.</param>
    /// <returns></returns>
    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    /// <summary>
    /// 다음 프레임(혹은 다음 커맨드 리스트 기록을 위해)에 재사용할 수 있도록
    /// 모든 메모리 할당을 재설정 합니다.
    /// </summary>
    void Reset();

    // 커맨드 큐가 수행중이 아닌 경우에만 재설정해야 합니다.

private:
    // 할당자를 위한 페이지 정의
    struct Page
    {
        /// <summary>
        /// </summary>
        /// <param name="sizeInBytes">페이지의 크기</param>
        Page(size_t sizeInBytes);
        ~Page();

        /// <summary>
        ///페이지가 요청된 할당을 충족하는지 검사합니다.
        /// </summary>
        /// <param name="sizeInBytes"></param>
        /// <param name="alignment"></param>
        /// <returns>할당을 충족할 수 있으면 true, 할당을 충족할 수 없으면 false를 반환합니다.</returns>
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        /// <summary>
        /// 실제 할당이 발생하는 곳입니다.
        /// CPU 데이터를 GPU에 직접 복사(memecpy 사용)합니다.
        /// </summary>
        /// <param name="sizeInBytes"></param>
        /// <param name="alignment"></param>
        /// <returns>GPU 주소를 파이프라인에 바인딩하는 데 사용할 수 있는 구조체를 반환합니다.</returns>
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        /// <summary>
        /// 페이지의 포인터 오프셋을 0으로 재설정합니다.
        /// </summary>
        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> _d3d12Resource;

        // 기반 포인터.
        void* _CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS _GPUPtr;

        // 할당된 페이지 사이즈.
        size_t _pageSize;
        // 바이트 단위의 현재 할당 오프셋.
        size_t _offset;
    };

    // 메모리 페이지 풀.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    /// <summary>
    /// 사용할 수 있는 페이지 목록에서 
    /// 새 페이지를 검색하거나 새 페이지를 만듭니다.
    /// 할당자에게 할당할 페이지가 없거나
    /// 현재 페이지에 할당 요청을 충족할 수 있는 공간이 없는 경우 사용됩니다.
    /// </summary>
    /// <returns>할당 요청을 충족하는 메모리 페이지</returns>
    std::shared_ptr<Page> RequestPage();

    /// <summary>
    /// 할당자가 생성한 페이지를 저장합니다.
    /// _availablePages 에 저장된 페이지는 저장되지 않습니다.
    /// </summary>
    PagePool _pagePool;

    // Reset 메서드에서 _pagePool는 
    // _availablePages 큐를 재설정하는데 사용합니다.

    /// <summary>
    /// 할당 요청에 충족되는 페이지를 저장합니다.
    /// </summary>
    PagePool _availablePages;

    std::shared_ptr<Page> _currentPage;

    // 각 페이지의 메모리 크기.
    size_t m_PageSize;
};

#pragma once

#include "MemorySizeDefines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

/// <summary>
/// GPU�� �����͸� ���ε��ϴ� �� ���˴ϴ�. Ư�� ���� ���, ����, �ε��� ���� 
/// �����͸� ���ε��ϴ� �� ���˴ϴ�.
/// </summary>
class UploadBuffer
{
public:
    // GPU�� �����͸� ���ε��ϴ� �� ����մϴ�.
    struct Allocation
    {
        void* CPU;
        D3D12_GPU_VIRTUAL_ADDRESS GPU;
    };

    /// <param name="pageSize">GPU �޸��� �� �������� �Ҵ�� ������</param>
    explicit UploadBuffer(ID3D12Device2* device, size_t pageSize = _2MB);

    virtual ~UploadBuffer();

    /// <returns>The maximum size of an allocation is the size of a single page.</returns>
    size_t GetPageSize() const
    {
        return m_PageSize;
    }

    /// <summary>
    /// ���ε� ���� �޸𸮸� �Ҵ��մϴ�. 
    /// �Ҵ��� ������ ũ�⸦ �ʰ��� �� �����ϴ�. memcpy �Ǵ� ������ �޼��带 �����
    /// �� �Լ����� ��ȯ�� �Ҵ� ������ CPU �����Ϳ� ���� �����͸� �����մϴ�.
    /// </summary>
    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    /// <summary>
    /// ������ ���� ��� �Ҵ��� �����մϴ�.
    /// Ŀ�ǵ� ����Ʈ�� ������ ������ �� ����� �� �ֽ��ϴ�.
    /// </summary>
    void Reset();

private:
    // �Ҵ��ڸ� ���� ������ ����
    struct Page
    {
        Page(size_t sizeInBytes);
        ~Page();

        /// <summary>
        /// �������� �Ҵ��� ������ �ִ��� Ȯ���մϴ�.
        /// </summary>
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        /// <summary>
        /// �������� �޸𸮸� �Ҵ��մϴ�.
        /// �Ҵ� ũ�Ⱑ �ʹ� ũ�� std::bad_alloc�� �߻���ŵ�ϴ�.
        /// </summary>
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        // ������ ���� �������� �缳�� �մϴ�.
        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

        // ��� ������.
        void* m_CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

        // �Ҵ�� ������ ������.
        size_t m_PageSize;
        // ����Ʈ ������ ���� �Ҵ� ������.
        size_t m_Offset;
    };

    // �޸� ������ Ǯ.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    // ��� ������ ������ Ǯ���� �������� ��û�ϰų� 
    // ��� ������ �������� ���� ��� �� �������� ����ϴ�.
    std::shared_ptr<Page> RequestPage();

    PagePool m_PagePool;
    PagePool m_AvailablePages;

    std::shared_ptr<Page> m_CurrentPage;

    // �� �������� �޸� ũ��.
    size_t m_PageSize;
};

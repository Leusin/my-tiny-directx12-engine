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
    /// �޸� �������� �ִ� �޸� ûũ(�Ǵ� ���)�� �Ҵ��մϴ�.
    /// (ûũ(Chunk): �� ���� ������ ����)
    /// </summary>
    /// <param name="sizeInBytes">�Ҵ� ũ��(����Ʈ ����)</param>
    /// <param name="alignment">�Ҵ� �޸� ����(����Ʈ ����). e.g., ��� ���ۿ� ���� �Ҵ��� 256����Ʈ�� ���ĵǾ�� �մϴ�.</param>
    /// <returns></returns>
    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    /// <summary>
    /// ���� ������(Ȥ�� ���� Ŀ�ǵ� ����Ʈ ����� ����)�� ������ �� �ֵ���
    /// ��� �޸� �Ҵ��� �缳�� �մϴ�.
    /// </summary>
    void Reset();

    // Ŀ�ǵ� ť�� �������� �ƴ� ��쿡�� �缳���ؾ� �մϴ�.

private:
    // �Ҵ��ڸ� ���� ������ ����
    struct Page
    {
        /// <summary>
        /// </summary>
        /// <param name="sizeInBytes">�������� ũ��</param>
        Page(size_t sizeInBytes);
        ~Page();

        /// <summary>
        ///�������� ��û�� �Ҵ��� �����ϴ��� �˻��մϴ�.
        /// </summary>
        /// <param name="sizeInBytes"></param>
        /// <param name="alignment"></param>
        /// <returns>�Ҵ��� ������ �� ������ true, �Ҵ��� ������ �� ������ false�� ��ȯ�մϴ�.</returns>
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        /// <summary>
        /// ���� �Ҵ��� �߻��ϴ� ���Դϴ�.
        /// CPU �����͸� GPU�� ���� ����(memecpy ���)�մϴ�.
        /// </summary>
        /// <param name="sizeInBytes"></param>
        /// <param name="alignment"></param>
        /// <returns>GPU �ּҸ� ���������ο� ���ε��ϴ� �� ����� �� �ִ� ����ü�� ��ȯ�մϴ�.</returns>
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        /// <summary>
        /// �������� ������ �������� 0���� �缳���մϴ�.
        /// </summary>
        void Reset();

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> _d3d12Resource;

        // ��� ������.
        void* _CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS _GPUPtr;

        // �Ҵ�� ������ ������.
        size_t _pageSize;
        // ����Ʈ ������ ���� �Ҵ� ������.
        size_t _offset;
    };

    // �޸� ������ Ǯ.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    /// <summary>
    /// ����� �� �ִ� ������ ��Ͽ��� 
    /// �� �������� �˻��ϰų� �� �������� ����ϴ�.
    /// �Ҵ��ڿ��� �Ҵ��� �������� ���ų�
    /// ���� �������� �Ҵ� ��û�� ������ �� �ִ� ������ ���� ��� ���˴ϴ�.
    /// </summary>
    /// <returns>�Ҵ� ��û�� �����ϴ� �޸� ������</returns>
    std::shared_ptr<Page> RequestPage();

    /// <summary>
    /// �Ҵ��ڰ� ������ �������� �����մϴ�.
    /// _availablePages �� ����� �������� ������� �ʽ��ϴ�.
    /// </summary>
    PagePool _pagePool;

    // Reset �޼��忡�� _pagePool�� 
    // _availablePages ť�� �缳���ϴµ� ����մϴ�.

    /// <summary>
    /// �Ҵ� ��û�� �����Ǵ� �������� �����մϴ�.
    /// </summary>
    PagePool _availablePages;

    std::shared_ptr<Page> _currentPage;

    // �� �������� �޸� ũ��.
    size_t m_PageSize;
};

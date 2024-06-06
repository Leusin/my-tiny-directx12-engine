#include "pch.h"
#include "UploadBuffer.h"

#include "Helper.h"

ID3D12Device2* g_device;


UploadBuffer::UploadBuffer(ID3D12Device2* device, size_t pageSize)
    : m_PageSize(pageSize)
{ 
    g_device = device;
}

UploadBuffer::~UploadBuffer()
{
}


UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_PageSize) // �Ҵ� ũ�Ⱑ ���� ������ ũ�� ���� Ŭ �� 
    {
        throw std::bad_alloc(); // std::bad_alloc�� �����ϴ�.
    }

    // ���� �޸� ������ ������ ���ų� ���� �������� ��û�� ������ �� ���� ��
    // �� �������� �����˴ϴ�.
    if (!_currentPage || !_currentPage->HasSpace(sizeInBytes, alignment))
    {
        _currentPage = RequestPage();
    }

    return _currentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
    _currentPage = nullptr;
    // ��� ������ ���������� ��� ���� �մϴ�.
    _availablePages = _pagePool;

    for (auto page : _availablePages)
    {
        // �� �Ҵ��� ���� �������� �����մϴ�.
        page->Reset();
    }
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!_availablePages.empty()) // _availablePages ť�� ��� ������ �������� �ִٸ�
    {
        page = _availablePages.front(); // ť�� �� �� �������� ��ȯ�� ��,
        _availablePages.pop_front(); // pop �˴ϴ�.
    }
    else // ��� ������ �������� ���ٸ�
    {
        page = std::make_shared<Page>(m_PageSize); // �� �������� ������ ��
        _pagePool.push_back(page); // _pagePool ť �� �ڿ� push �˴ϴ�.
    }

    return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes)
    : _pageSize(sizeInBytes)
    , _offset(0)
    , _CPUPtr(nullptr)
    , _GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto desc         = CD3DX12_RESOURCE_DESC::Buffer(_pageSize);

    // ���ε� ���� Ŀ�Ե� ���ҽ��� ID3D12Resource�� �����մϴ�.
    ThrowIfFailed(g_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_d3d12Resource)));

    _GPUPtr = _d3d12Resource->GetGPUVirtualAddress(); // ID3D12Resource::GetGPUVirtualAddress �ż��带 ���� GPU�� �ּҸ� �޽��ϴ�.
    _d3d12Resource->Map(0, nullptr, &_CPUPtr); // ID3D12Resource::Map �޼��� ���� CPU �ּҸ� �޽��ϴ�.

    // ���ҽ��� ���� �ʿ����� ���� ������ ���ҽ��� ���ε� ���·� �δ� ���� �����մϴ�.
}

UploadBuffer::Page::~Page()
{
    _d3d12Resource->Unmap(0, nullptr); // ID3D12Resource::Unmap �޼��带 ���� ���ҽ� �޸� ������ �����մϴ�.
    _CPUPtr = nullptr; // CPU �����͸� nullptr�� �缳�� �մϴ�.
    _GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0); // GPU �����͸� 0���� �缳�� �մϴ�.

    // _d3d12Resource �� CompPtr�� ����ϱ� ������ ��������� ������ �ʿ� �����ϴ�.
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize   = Math::AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = Math::AlignUp(_offset, alignment);

    return alignedOffset + alignedSize <= _pageSize; // ��û�� ���� �Ҵ��� ������ �� ������ true�� ��ȯ�մϴ�.
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment)) // �Ҵ� ��û�� �����ϴ� ���� �� ���� ���
    {
        // ���� �Ҵ��� ������ �����ϴ�.
        throw std::bad_alloc(); // std::bad_alloc() ���ܸ� �����ϴ�.
    }

    // �Ҵ� ũ��� ���� �ּ� ��� ��û�� ���Ŀ� ���� align up �Ǿ�� �մϴ�.
    // ��κ��� ��� �Ҵ� ũ��� �̹� ��û�� ���Ŀ� ���� align up �Ǿ�������
    // (e.g., ���ؽ� �Ǵ� �ε��� ���ۿ� �޸� �Ҵ��ϴ� ���) 

    size_t alignedSize = Math::AlignUp(sizeInBytes, alignment); // ��Ȯ���� �����ϱ� ���� ��������� align up �� �� �����մϴ�.
    _offset           = Math::AlignUp(_offset, alignment); // offset ���� ��û�� ���Ŀ� ���� align up �մϴ�.

    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>(_CPUPtr) + _offset; // ���ĵ� CPU �ּҸ� ����ü�� ����մϴ�.
    allocation.GPU = _GPUPtr + _offset; // ���ĵ� GPU �ּҸ� ����ü�� ����մϴ�.

    _offset += alignedSize; // alignedSize ũ�� ��ŭ offset�� ���� ��ŵ�ϴ�.

    return allocation;
}

void UploadBuffer::Page::Reset()
{
    _offset = 0;
}

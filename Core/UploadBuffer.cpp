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
    if (sizeInBytes > m_PageSize) // 할당 크기가 남은 페이지 크기 보다 클 때 
    {
        throw std::bad_alloc(); // std::bad_alloc를 던집니다.
    }

    // 남은 메모리 페이지 공간이 없거나 현재 페이지가 요청을 충족할 수 없을 때
    // 새 페이지를 요정됩니다.
    if (!_currentPage || !_currentPage->HasSpace(sizeInBytes, alignment))
    {
        _currentPage = RequestPage();
    }

    return _currentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
    _currentPage = nullptr;
    // 사용 가능한 페이지들을 모두 리셋 합니다.
    _availablePages = _pagePool;

    for (auto page : _availablePages)
    {
        // 새 할당을 위해 페이지를 리셋합니다.
        page->Reset();
    }
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!_availablePages.empty()) // _availablePages 큐에 사용 가능한 페이지가 있다면
    {
        page = _availablePages.front(); // 큐의 맨 앞 페이지가 반환된 뒤,
        _availablePages.pop_front(); // pop 됩니다.
    }
    else // 사용 가능한 페이지가 없다면
    {
        page = std::make_shared<Page>(m_PageSize); // 새 페이지가 생성된 뒤
        _pagePool.push_back(page); // _pagePool 큐 맨 뒤에 push 됩니다.
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

    // 업로드 힙에 커밋된 리소스로 ID3D12Resource를 생성합니다.
    ThrowIfFailed(g_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_d3d12Resource)));

    _GPUPtr = _d3d12Resource->GetGPUVirtualAddress(); // ID3D12Resource::GetGPUVirtualAddress 매서드를 통해 GPU와 주소를 받습니다.
    _d3d12Resource->Map(0, nullptr, &_CPUPtr); // ID3D12Resource::Map 메서드 통해 CPU 주소를 받습니다.

    // 리소스가 더는 필요하지 않을 때까지 리소스를 매핑된 상태로 두는 것이 안전합니다.
}

UploadBuffer::Page::~Page()
{
    _d3d12Resource->Unmap(0, nullptr); // ID3D12Resource::Unmap 메서드를 통해 리소스 메모리 매핑을 해제합니다.
    _CPUPtr = nullptr; // CPU 포인터를 nullptr로 재설정 합니다.
    _GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0); // GPU 포인터를 0으로 재설정 합니다.

    // _d3d12Resource 는 CompPtr를 사용하기 때문에 명시적으로 해제할 필요 없습니다.
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize   = Math::AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = Math::AlignUp(_offset, alignment);

    return alignedOffset + alignedSize <= _pageSize; // 요청된 정렬 할당이 충족될 수 있으면 true를 반환합니다.
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment)) // 할당 요청을 충족하는 공간 이 없을 경우
    {
        // 더는 할당할 공간이 없습니다.
        throw std::bad_alloc(); // std::bad_alloc() 예외를 던집니다.
    }

    // 할당 크기와 시작 주소 모두 요청된 정렬에 맞춰 align up 되어야 합니다.
    // 대부분의 경우 할당 크기는 이미 요청된 정렬에 맞춰 align up 되어있지만
    // (e.g., 버텍스 또는 인덱스 버퍼에 메모리 할당하는 경우) 

    size_t alignedSize = Math::AlignUp(sizeInBytes, alignment); // 정확성을 보장하기 위해 명시적으로 align up 한 뒤 정의합니다.
    _offset           = Math::AlignUp(_offset, alignment); // offset 또한 요청한 정렬에 따라 align up 합니다.

    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>(_CPUPtr) + _offset; // 정렬된 CPU 주소를 구조체에 기록합니다.
    allocation.GPU = _GPUPtr + _offset; // 정렬된 GPU 주소를 구조체에 기록합니다.

    _offset += alignedSize; // alignedSize 크기 만큼 offset을 증가 시킵니다.

    return allocation;
}

void UploadBuffer::Page::Reset()
{
    _offset = 0;
}

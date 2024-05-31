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
    if (sizeInBytes > m_PageSize)
    {
        throw std::bad_alloc();
    }

    // 현재 페이지가 없거나 요청된 할당이 현재 페이지의 남은 공간을 초과하는 경우
    // 새 페이지를 요청합니다.
    if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
    {
        m_CurrentPage = RequestPage();
    }

    return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
    m_CurrentPage = nullptr;
    // 사용 가능한 페이지들을 모두 리셋 합니다.
    m_AvailablePages = m_PagePool;

    for (auto page : m_AvailablePages)
    {
        // 새 할당을 위해 페이지를 리셋합니다.
        page->Reset();
    }
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!m_AvailablePages.empty())
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>(m_PageSize);
        m_PagePool.push_back(page);
    }

    return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes)
    : m_PageSize(sizeInBytes)
    , m_Offset(0)
    , m_CPUPtr(nullptr)
    , m_GPUPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto desc         = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);

    ThrowIfFailed(g_device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_d3d12Resource)));

    m_GPUPtr = m_d3d12Resource->GetGPUVirtualAddress();
    m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
}

UploadBuffer::Page::~Page()
{
    m_d3d12Resource->Unmap(0, nullptr);
    m_CPUPtr = nullptr;
    m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    size_t alignedSize   = Math::AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = Math::AlignUp(m_Offset, alignment);

    return alignedOffset + alignedSize <= m_PageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        // 더는 할당할 공간이 없습니다.
        throw std::bad_alloc();
    }

    size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
    m_Offset           = Math::AlignUp(m_Offset, alignment);

    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>(m_CPUPtr) + m_Offset;
    allocation.GPU = m_GPUPtr + m_Offset;

    m_Offset += alignedSize;

    return allocation;
}

void UploadBuffer::Page::Reset()
{
    m_Offset = 0;
}

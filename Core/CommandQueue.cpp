#include "pch.h"
#include "CommandQueue.h"


CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    : m_fenceValue(0)
    , m_commandListType(type)
    , m_d3d12Device(device)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type                     = type;
    desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask                 = 0;

    ThrowIfFailed(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
    ThrowIfFailed(m_d3d12Device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

    m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_fenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(m_d3d12Device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
    ThrowIfFailed(
        m_d3d12Device->CreateCommandList(0, m_commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    return commandList;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
    // ť�� ��� �ϳ��� �׸��� �ְ� �ش� Ŀ�ǵ� ��������Ϳ� ����� �潺 ���� �����ߴٸ�
    if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_commandAllocatorQueue.front().commandAllocator;
        m_commandAllocatorQueue.pop();
        // ť�� �� �տ��� ������ �缳���˴ϴ�.
        ThrowIfFailed(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }
    //  ť�� ��� �ϳ��� Ŀ�ǵ� ����Ʈ�� �ִٸ�,
    if (!m_commandListQueue.empty())
    {
        commandList = m_commandListQueue.front();
        m_commandListQueue.pop();
        // ť �� �տ��� ���� ���� �ܰ迡�� ������ Ŀ�ǵ� ��������͸� �̿��� �缳���մϴ�.
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
    }
    else // ����� �� �ִ� Ŀ�ǵ� ����Ʈ�� ���ٸ�, ���� �����մϴ�.
    {
        commandList = CreateCommandList(commandAllocator);
    }

    // Ŀ�ǵ� ����Ʈ�� ����� �� Ŀ�ǵ� ��������͸� �˻��� �� �ֵ���
    // Ŀ�ǵ� ��������͸� Ŀ�ǵ� ����Ʈ�� �����մϴ�.
    ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

    return commandList;
}

/// <summary>
/// Ŀ�ǵ� ����Ʈ�� �����մϴ�.
/// </summary>
/// <returns>Ŀ�ǵ� ����Ʈ�� ��ٸ� �潺 ���� ��ȯ�˴ϴ�.</returns>
uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    commandList->Close();

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const ppCommandLists[] = { commandList.Get() };

    m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    uint64_t fenceValue = Signal();

    m_commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
    m_commandListQueue.push(commandList);


    // Ŀ�ǵ� ����������� �������� Ŀ�ǵ� ��������� ť�� ComPtr�� �����Ǿ����ϴ�.
    // ���⼭ �ӽ� COM �������� ������ �����ϰ� ������ �� �ֽ��ϴ�.
    commandAllocator->Release();

    return fenceValue;
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValue = ++m_fenceValue;
    m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValue);
    return fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        m_d3d12Fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        ::WaitForSingleObject(m_fenceEvent, DWORD_MAX);
    }
}

void CommandQueue::Flush()
{
    WaitForFenceValue(Signal());
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
    return m_d3d12CommandQueue;
}

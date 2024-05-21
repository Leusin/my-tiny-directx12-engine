#pragma once

#include <d3d12.h> // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>   // For Microsoft::WRL::ComPtr

#include <cstdint> // For uint64_t
#include <queue>   // For std::queue

/// <summary>
///  ID3D12CommandQueue �������̽��� CPU�� GPU ���� ����ȭ�� �����ϴ� �� ���Ǵ� 
/// ����ȭ ������Ƽ�긦 ĸ��ȭ�� Ŭ���� �Դϴ�.
/// </summary>
class CommandQueue
{
public:
    CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandQueue();


protected:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

    /// <summary>
    /// Ŀ�ǵ� ť�κ��� ����� �� �ִ� Ŀ�ǵ� ����Ʈ�� �����ɴϴ�.
    /// </summary>
    /// <returns>�׸��� ����� ȣ���ϴ� �� ����� �� �ִ� Ŀ�ǵ� ����Ʈ�� ��ȯ�մϴ�. </returns>
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

    /// <summary>
    /// Ŀ�ǵ� ����Ʈ�� �����մϴ�.
    /// </summary>
    /// <returns>Ŀ�ǵ� ����Ʈ�� �ִ� ����� Ŀ�ǵ� ť���� ������ �Ϸ��ߴ��� 
    /// Ȯ���� �� �ִ� �潺 ��(�÷���)�� ��ȯ�մϴ�.</returns>
    uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
    /// <summary>
    /// Ŀ�ǵ� ��������Ͱ� Ŀ�ǵ� ť���� �� �̻� ��� ���� �ƴ��� �����ϱ� ���� 
    /// Ŀ�ǵ� ť�� �潺 ���� ��ȣ�� ������ �ش� �潺 ��(�� ������ Ŀ�ǵ� ���������)�� 
    /// ������ �� �ֵ��� �����մϴ�.
    /// </summary>
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue      = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

    D3D12_COMMAND_LIST_TYPE m_commandListType;
    ComPtr<ID3D12Device2> m_d3d12Device; // ���� ������
    ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue; // ���� ������

    /// <summary>
    /// Ŀ�ǵ� ť�� �����ǰ� �ִ� ���ҽ��� ��������� �ʵ��� �����ϱ� ���� ���˴ϴ�.
    /// </summary>
    ComPtr<ID3D12Fence> m_d3d12Fence;
    uint64_t m_fenceValue;

    /// <summary>
    /// �潺�� Ư�� ���� ���������� �޴� �ڵ��Դϴ�. ���� �潺 ���� ��ü�� �Ϸ�� ���� �����ӿ�
    /// ������ �潺 ���� �������� ������, CPU ������� �潺 ���� ������ ������ ����մϴ�.
    /// </summary>
    HANDLE m_fenceEvent;

    CommandAllocatorQueue m_commandAllocatorQueue;
    CommandListQueue m_commandListQueue;
};

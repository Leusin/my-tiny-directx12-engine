#pragma once

#include <d3d12.h> // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>   // For Microsoft::WRL::ComPtr

#include <cstdint> // For uint64_t
#include <queue>   // For std::queue

/// <summary>
///  ID3D12CommandQueue 인터페이스와 CPU와 GPU 간의 동기화를 수행하는 데 사용되는 
/// 동기화 프리미티브를 캡슐화한 클래스 입니다.
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
    /// 커맨드 큐로부터 사용할 수 있는 커맨드 리스트를 가져옵니다.
    /// </summary>
    /// <returns>그리기 명령을 호출하는 데 사용할 수 있는 커맨드 리스트를 반환합니다. </returns>
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

    /// <summary>
    /// 커맨드 리스트를 실행합니다.
    /// </summary>
    /// <returns>커맨드 리스트에 있는 명령이 커맨드 큐에서 실행을 완료했는지 
    /// 확인할 수 있는 펜스 값(플래그)을 반환합니다.</returns>
    uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
    /// <summary>
    /// 커맨드 얼로케이터가 커맨드 큐에서 더 이상 사용 중이 아님을 보장하기 위해 
    /// 커맨드 큐에 펜스 값을 신호로 보내고 해당 펜스 값(및 연관된 커맨드 얼로케이터)을 
    /// 재사용할 수 있도록 저장합니다.
    /// </summary>
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue      = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;

    D3D12_COMMAND_LIST_TYPE m_commandListType;
    ComPtr<ID3D12Device2> m_d3d12Device; // 참조 포인터
    ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue; // 참조 포인터

    /// <summary>
    /// 커맨드 큐에 참조되고 있는 리소스가 덮어쓰여지지 않도록 보장하기 위해 사용됩니다.
    /// </summary>
    ComPtr<ID3D12Fence> m_d3d12Fence;
    uint64_t m_fenceValue;

    /// <summary>
    /// 펜스가 특정 값에 도달했음을 받는 핸들입니다. 만약 펜스 객스 객체의 완료된 값이 프레임에
    /// 지정된 펜스 값에 도달하지 않으면, CPU 스레드는 펜스 값이 도달할 때까지 대기합니다.
    /// </summary>
    HANDLE m_fenceEvent;

    CommandAllocatorQueue m_commandAllocatorQueue;
    CommandListQueue m_commandListQueue;
};

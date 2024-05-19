#pragma once

#include "pch.h"


#include <wrl.h>
using namespace Microsoft::WRL;

class Graphics
{
public:
    Graphics();

    void Initialize(HWND m_hWnd, uint32_t& m_width, uint32_t& m_hight);
    void Shutdown(void);

    void Update();
    void Render();
    void Resize(uint32_t width, uint32_t height);

private:
    /// <summary>
    /// ���� ����� �� Ŀ�ǵ���� �μ��� ���� Ư�� �����͸� ���� �� �ֽ��ϴ�.
    /// </summary>
    void ParseCommandLineArguments();

    void EnableDebugLayer();
    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    bool CheckTearingSupport();
    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
    HANDLE CreateEventHandle();

    uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);

private:
    // ������� ���� Ŭ�������� ����� �ΰ� �̸�
    std::string m_loggerName = "graphics";

    //
    // �������α׷� �ʱ�ȭ ���� ����
    //

    /// <summary>
    /// �� ������ ǥ�� �� �Դϴ�. flip presentation ���� ����� ���� 2���� �۾Ƽ��� �ȵ˴ϴ�.
    /// </summary>
    static const uint8_t m_numFrames = 3;

    /// <summary>
    /// ����Ʈ���� ������Ʈ�������� ������� �����Դϴ�. 
    /// ����Ʈ���� ������Ʈ�������� ��� ������ ����� ����� �� �ְ� �մϴ�.
    /// �ϵ��� �����Ǿ��ų� ��� �׽�Ʈ�� �� ����� �� �ֽ��ϴ�.
    /// </summary>
    bool m_useWarp = false;

   uint32_t m_clientWidth = 1280;
   uint32_t m_clientHeight = 720;

    /// <summary>
    /// ����̽��� ����ü���� ������ ������ ������ Ư�� �޽����� ó������ �ʵ��� �մϴ�.
    /// </summary>
    bool m_isInitialized = false;

    //
    // Windows �� DirectX ����
    //

    HWND m_hWnd;

    /// <summary>
    /// ��ü ȭ�� â���� ��ȯ�� �� ���� â ũ�⸦ ������ �ݴϴ�.
    /// </summary>
    RECT m_windowRect;
    
    ComPtr<ID3D12Device2> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;

    /// <summary>
    /// �������� �̹����� â�� ǥ���ϴ� ������ �մϴ�.
    /// </summary>
    ComPtr<IDXGISwapChain4> m_swapChain;

    /// <summary>
    /// ���� ü���� ���� ���� ����� ���ҽ��� �����˴ϴ�.
    /// ����۸� �ùٸ� ���·� ��ȯ�ϱ����� ����� ���ҽ��� ���� �����͸� �����ϴ� �迭�Դϴ�.
    /// </summary>
    ComPtr<ID3D12Resource> m_backBuffers[m_numFrames];

    /// <summary>
    /// GPU ����� ���� ���� ����մϴ�. ���� �����带 ����� ��� �ϳ��� ����մϴ�.
    /// </summary>
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    /// <summary>
    /// Ŀ�ǵ� ����Ʈ�� �ִ� CPU ����� ����ϱ� ���� ��� �޸� ���� �Դϴ�.
    /// GPU�� ��ϵ� ��� ����� ó���ϱ� ������ ������ �� �����ϴ�.
    /// ���� ü���� �� ���۸��� ��� �ϳ��� Ŀ�ǵ� ��������Ͱ� �ʿ��մϴ�.
    /// </summary>
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[m_numFrames];

    /// <summary>
    /// ����ü���� ����� �ؽ�ó�� ���� Ÿ�� �並 ����� ����˴ϴ�.
    /// GPU �޸𸮿��� �ؽ�ó ���ҽ��� ��ġ, �ؽ�ó ũ�� �� �ؽ�ó ������ �����մϴ�.
    /// DirectX 12���ʹ� RTV�� ��ũ���� ���� ����˴ϴ�. 
    /// </summary>
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;

    /// <summary>
    /// �ʱ�ȭ �� ��ũ���� ���� ���� ��� ũ�⸦ ������ �� ����˴ϴ�.
    /// ��ũ���� ���� ��ũ���� ũ��� GPU �����縶�� �ٸ��� ������ 
    /// ��ũ���� ���� �ε����� �ùٸ��� �������ϱ� ���� ����մϴ�.
    /// </summary>
    UINT m_rtvDescriptorSize;

    /// <summary>
    /// ���� ü���� �ø� �𵨿� ���� ���� ������� �ε����� ���������� ���� �� �ֽ��ϴ�.
    /// ����ü���� ���� �� ���� �ε����� �����մϴ�.
    /// </summary>
    UINT m_currentBackBufferIndex;

    //
    // GPU ����ȭ ���� ����
    //

    /// <summary>
    /// Ŀ�ǵ� ť�� �����ǰ� �ִ� ���ҽ��� ��������� �ʵ��� �����ϱ� ���� ���˴ϴ�.
    /// </summary>
    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValue                    = 0;

    /// <summary>
    /// Ư�� �����ӿ� ���� Ŀ�ǵ� ť�� ��ȣ�� ���� �� ���� �潺 ������ �����ϴ� �� ���˴ϴ�.
    /// </summary>
    uint64_t m_frameFenceValues[m_numFrames] = {};

    /// <summary>
    /// �潺�� Ư�� ���� ���������� �޴� �ڵ��Դϴ�. ���� �潺 ���� ��ü�� �Ϸ�� ���� �����ӿ� 
    /// ������ �潺 ���� �������� ������, CPU ������� �潺 ���� ������ ������ ����մϴ�.
    /// </summary>
    HANDLE m_fenceEvent;

    //
    // ���� ü���� present �ż���� ���õ� ����
    //

    /// <summary>
    /// �������� �̹����� ȭ�鿡 ǥ���ϱ� ���� ���� �ֱ⸦ ��ٷ��� �ϴ��� ���θ� �����մϴ�.
    /// m_vSync = true �� �� ���� ü���� ���� ���� �ֱ���� present ���� �ʽ��ϴ�. 
    /// �׷��� ���� ������ �ӵ��� ���� �ֻ����� ���ѵ˴ϴ�.
    /// m_vSync = false �� ��� ����ü���� �ִ� �ӵ��� ȭ�鿡 present �ǹǷ� ������ �ӵ��� 
    /// ���ѵ��� ������ ��ũ�� Ƽ� ������ �߻��� �� �ֽ��ϴ�.
    /// </summary>
    bool m_vSync = true;

    /// <summary>
    /// ���� �ֻ���(VVR)�� �����ϴ� ���÷��̴� ���� ���� �ֱⰡ �߻��ϴ� ������ �����ϵ��� 
    /// ����� ��ũ�� Ƽ��� ���� �ݴϴ�.
    /// </summary>
    bool m_tearingSupported = false;
};

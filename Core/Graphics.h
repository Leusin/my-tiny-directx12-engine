#pragma once

#include "pch.h"
#include "CommandQueue.h"

#include <wrl.h>
using namespace Microsoft::WRL;

/// <summary>
/// �� ������ ǥ�� �� �Դϴ�. flip presentation ���� ����� ���� 2���� �۾Ƽ��� �ȵ˴ϴ�.
/// </summary>
static const uint8_t s_bufferCount = 3;

class Graphics
{
public:
    Graphics() = default;

    void Initialize(HWND m_hWnd, uint32_t& m_width, uint32_t& m_hight);
    void Shutdown(void);

    void Resize(uint32_t width, uint32_t height);

    void Flush();

    std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
    ID3D12Device2* GetDevice();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
    Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;
    UINT GetCurrentBackBufferIndex() const;
    UINT Present();

    bool IsVSync() const;
    void SetVSync(bool vSync);
    void ToggleVSync();

private:
    /// <summary>
    /// ���� ����� �� Ŀ�ǵ���� �μ��� ���� Ư�� �����͸� ���� �� �ֽ��ϴ�.
    /// </summary>
    void ParseCommandLineArguments();

    void EnableDebugLayer();
    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
    bool CheckTearingSupport();
    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);

private:
    // ������� ���� Ŭ�������� ����� �ΰ� �̸�
    std::string m_loggerName = "graphics";

    //
    // �������α׷� �ʱ�ȭ ���� ����
    //

    /// <summary>
    /// ����Ʈ���� ������Ʈ�������� ������� �����Դϴ�.
    /// ����Ʈ���� ������Ʈ�������� ��� ������ ����� ����� �� �ְ� �մϴ�.
    /// �ϵ��� �����Ǿ��ų� ��� �׽�Ʈ�� �� ����� �� �ֽ��ϴ�.
    /// </summary>
    bool m_useWarp = false;

    uint32_t m_clientWidth  = 1280;
    uint32_t m_clientHeight = 720;

    /// <summary>
    /// ����̽��� ����ü���� ������ ������ ������ Ư�� �޽����� ó������ �ʵ��� �մϴ�.
    /// </summary>
    bool m_isInitialized = false;

    //
    // Windows �� DirectX ����
    //

    HWND m_hWnd;

    ComPtr<ID3D12Device2> m_d3d12Device;

    /// <summary>
    /// �������� �̹����� â�� ǥ���ϴ� ������ �մϴ�.
    /// </summary>
    ComPtr<IDXGISwapChain4> m_dxgiSwapChain;

    /// <summary>
    /// ���� ü���� ���� ���� ����� ���ҽ��� �����˴ϴ�.
    /// ����۸� �ùٸ� ���·� ��ȯ�ϱ����� ����� ���ҽ��� ���� �����͸� �����ϴ� �迭�Դϴ�.
    /// </summary>
    ComPtr<ID3D12Resource> m_backBuffers[s_bufferCount];

    /// <summary>
    /// GPU ����� ���� ���� ����մϴ�. ���� �����带 ����� ��� �ϳ��� ����մϴ�.
    /// </summary>
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    /// <summary>
    /// Ŀ�ǵ� ����Ʈ�� �ִ� CPU ����� ����ϱ� ���� ��� �޸� ���� �Դϴ�.
    /// GPU�� ��ϵ� ��� ����� ó���ϱ� ������ ������ �� �����ϴ�.
    /// ���� ü���� �� ���۸��� ��� �ϳ��� Ŀ�ǵ� ��������Ͱ� �ʿ��մϴ�.
    /// </summary>
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[s_bufferCount];

    /// <summary>
    /// ����ü���� ����� �ؽ�ó�� ���� Ÿ�� �並 ����� ����˴ϴ�.
    /// GPU �޸𸮿��� �ؽ�ó ���ҽ��� ��ġ, �ؽ�ó ũ�� �� �ؽ�ó ������ �����մϴ�.
    /// DirectX 12���ʹ� RTV�� ��ũ���� ���� ����˴ϴ�.
    /// </summary>
    ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap;

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
    bool m_isTearingSupported;

    std::shared_ptr<CommandQueue> m_DirectCommandQueue;
    std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
    std::shared_ptr<CommandQueue> m_CopyCommandQueue;

    /// <summary>
    /// ���� �ֻ���(VVR)�� �����ϴ� ���÷��̴� ���� ���� �ֱⰡ �߻��ϴ� ������ �����ϵ���
    /// ����� ��ũ�� Ƽ��� ���� �ݴϴ�.
    /// </summary>
    bool m_tearingSupported = false;
};

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
    /// 앱이 실행될 때 커맨드라인 인수를 통해 특정 데이터를 받을 수 있습니다.
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
    // 디버깅을 위해 클래스에서 사용할 로거 이름
    std::string m_loggerName = "graphics";

    //
    // 응용프로그램 초기화 관련 변수
    //

    /// <summary>
    /// 백 버퍼의 표면 수 입니다. flip presentation 모델을 사용할 때는 2보다 작아서는 안됩니다.
    /// </summary>
    static const uint8_t m_numFrames = 3;

    /// <summary>
    /// 소프트웨어 래지스트라이저를 사용할지 여부입니다. 
    /// 소프트웨어 레지스트라이저는 고급 랜더링 기능을 사용할 수 있게 합니다.
    /// 하드웨어가 오래되었거나 기술 테스트할 떄 사용할 수 있습니다.
    /// </summary>
    bool m_useWarp = false;

   uint32_t m_clientWidth = 1280;
   uint32_t m_clientHeight = 720;

    /// <summary>
    /// 디바이스와 스왑체인이 완전히 생성될 때까지 특정 메시지가 처리하지 않도록 합니다.
    /// </summary>
    bool m_isInitialized = false;

    //
    // Windows 와 DirectX 변수
    //

    HWND m_hWnd;

    /// <summary>
    /// 전체 화면 창으로 전환할 때 이전 창 크기를 저장해 뒵니다.
    /// </summary>
    RECT m_windowRect;
    
    ComPtr<ID3D12Device2> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;

    /// <summary>
    /// 렌더링된 이미지를 창에 표시하는 역할을 합니다.
    /// </summary>
    ComPtr<IDXGISwapChain4> m_swapChain;

    /// <summary>
    /// 스왑 체인은 여러 개의 백버퍼 리소스로 생성됩니다.
    /// 백버퍼를 올바른 상태로 전환하기위해 백버퍼 리소스에 대한 포인터를 저장하는 배열입니다.
    /// </summary>
    ComPtr<ID3D12Resource> m_backBuffers[m_numFrames];

    /// <summary>
    /// GPU 명령을 가장 먼저 기록합니다. 단일 스레드를 사용할 경우 하나만 사용합니다.
    /// </summary>
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    /// <summary>
    /// 커맨드 리스트에 있는 CPU 명령을 기록하기 위한 백업 메모리 역할 입니다.
    /// GPU가 기록된 모든 명령을 처리하기 전까진 재사용할 수 없습니다.
    /// 스왑 체인의 각 버퍼마다 적어도 하나의 커맨드 얼로케이터가 필요합니다.
    /// </summary>
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[m_numFrames];

    /// <summary>
    /// 스왑체인의 백버퍼 텍스처는 렌더 타겟 뷰를 사용해 설명됩니다.
    /// GPU 메모리에서 텍스처 리소스의 위치, 텍스처 크기 및 텍스처 형식을 설명합니다.
    /// DirectX 12부터는 RTV가 디스크립터 힙에 저장됩니다. 
    /// </summary>
    ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;

    /// <summary>
    /// 초기화 시 디스크립터 힙의 단일 요소 크기를 쿼리한 뒤 저장됩니다.
    /// 디스크립터 힙의 디스크립터 크기는 GPU 제조사마다 다르기 때문에 
    /// 디스크립터 힙의 인덱스를 올바르게 오프셋하기 위해 사용합니다.
    /// </summary>
    UINT m_rtvDescriptorSize;

    /// <summary>
    /// 스왑 체인의 플립 모델에 따라 현재 백버퍼의 인덱스가 연속적이지 않을 수 있습니다.
    /// 스왑체인의 현재 백 버퍼 인덱스를 저장합니다.
    /// </summary>
    UINT m_currentBackBufferIndex;

    //
    // GPU 동기화 관련 변수
    //

    /// <summary>
    /// 커맨드 큐에 참조되고 있는 리소스가 덮어쓰여지지 않도록 보장하기 위해 사용됩니다.
    /// </summary>
    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValue                    = 0;

    /// <summary>
    /// 특정 프레임에 대한 커맨드 큐에 신호를 보낼 때 사용된 펜스 값들을 추적하는 데 사용됩니다.
    /// </summary>
    uint64_t m_frameFenceValues[m_numFrames] = {};

    /// <summary>
    /// 펜스가 특정 값에 도달했음을 받는 핸들입니다. 만약 펜스 객스 객체의 완료된 값이 프레임에 
    /// 지정된 펜스 값에 도달하지 않으면, CPU 스레드는 펜스 값이 도달할 때까지 대기합니다.
    /// </summary>
    HANDLE m_fenceEvent;

    //
    // 스왑 체인의 present 매서드와 관련된 변수
    //

    /// <summary>
    /// 렌더링된 이미지를 화면에 표시하기 전에 수직 주기를 기다려야 하는지 여부를 제어합니다.
    /// m_vSync = true 일 때 스왑 체인은 다음 수직 주기까지 present 되지 않습니다. 
    /// 그러면 앱의 프레임 속도가 고정 주사율로 제한됩니다.
    /// m_vSync = false 인 경우 스왑체인이 최대 속도로 화면에 present 되므로 프레임 속도가 
    /// 제한되지 않지만 스크린 티어링 현상이 발생할 수 있습니다.
    /// </summary>
    bool m_vSync = true;

    /// <summary>
    /// 가변 주사율(VVR)을 지원하는 디스플레이는 앱이 수직 주기가 발생하는 시점을 결정하도록 
    /// 허용해 스크린 티어링을 없애 줍니다.
    /// </summary>
    bool m_tearingSupported = false;
};

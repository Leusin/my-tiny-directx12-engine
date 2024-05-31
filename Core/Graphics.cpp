#include "pch.h"
#include "Graphics.h"
#include "Logger.h"

#include <shellapi.h> // CommandLineToArgvW 함수


void Graphics::Initialize(HWND hWnd, uint32_t& width, uint32_t& hight)
{
    m_hWnd = hWnd;

    // 로거 등록
    spdlog::stdout_color_mt(m_loggerName);

    ParseCommandLineArguments();

    EnableDebugLayer();

    auto g_TearingSupported = CheckTearingSupport();

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(m_useWarp);

    spdlog::get(m_loggerName)->debug("1. 디바이스 생성");

    m_d3d12Device = CreateDevice(dxgiAdapter4);

    spdlog::get(m_loggerName)->debug("2. 커맨드 큐 생성");

    m_DirectCommandQueue  = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CopyCommandQueue    = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);

    spdlog::get(m_loggerName)->debug("3. 스왑 체인 생성");

    m_dxgiSwapChain = CreateSwapChain(hWnd, m_DirectCommandQueue->GetD3D12CommandQueue(), width, hight, s_bufferCount);

    // 백 버퍼의 인덱스는 스왑 체인에서 직접 쿼리해서 얻을 수 있습니다.
    m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    spdlog::get(m_loggerName)->debug("4. 디스크립터 힙 생성");

    m_d3d12RTVDescriptorHeap = CreateDescriptorHeap(m_d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_bufferCount);
    m_rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews(m_d3d12Device, m_dxgiSwapChain, m_d3d12RTVDescriptorHeap);

    // 커맨드 얼로케이터를 프레임 수만큼 생성합니다.
    for (int i = 0; i < s_bufferCount; ++i)
    {
        m_commandAllocators[i] = CreateCommandAllocator(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    spdlog::get(m_loggerName)->debug("5. 커맨드 리스트 생성");

    // 하나의 커맨드 리스트를 생성합니다.
    m_commandList = CreateCommandList(
        m_d3d12Device, m_commandAllocators[m_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    spdlog::get(m_loggerName)->debug("5. 펜스와 이벤트 핸들러 생성");

    m_isInitialized = true;
}

void Graphics::Shutdown(void)
{
    // GPU에 대기 중인 명령 목록을 플러시합니다. (중요)
    Flush();
}

//void Graphics::Render()
//{
//    auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex];
//    auto backBuffer       = m_backBuffers[m_currentBackBufferIndex];
//
//    commandAllocator->Reset();
//    m_commandList->Reset(commandAllocator.Get(), nullptr);
//
/*
 * DirectX 12에서는 그래픽 프로그래머가 자원을 사용하기 전에 올바른 상태에 있는지
 * 확인해야 합니다. 자원은 하나의 상태에서 다른 상태로 전환해야 하며, 이 작업은
 * 리소스 배리어를 사용하여 수행하고 해당 리소스 배리어를 명령 리스트에 삽입해야 합니다.
 * 예를 들어, 스왑 체인의 백 버퍼를 렌더 타겟으로 사용하기 전에, 이를 RENDER_TARGET
 * 상태로 전환해야 하며, 렌더링된 이미지를 화면에 표시하기 위해서는 이를 PRESENT 상태로
 * 전환해야 합니다.
 *
 * 리소스 배리어에는 여러 종류가 있습니다.
 * - Transition: 리소스를 한 상태에서 다른 상태로 전환
 * - Aliasing: 리소스가 동일한 메모리 공간을 공유할 때 사용
 * - UAV: 동일한 리소스에 대한 모든 읽기 및 쓰기 명령이 완료될 때까지 대기
 *   - 읽기 > 쓰기: 다른 셰이더에서 쓰기 전에 모든 이전의 읽기 작업이 완료되었는지 보장
 *   - 쓰기 > 읽기: 다른 셰이더에서 읽기 전에 모든 이전의 쓰기 작업이 완료되었는지 보장
 *   - 쓰기 > 쓰기: 다른 드로우 또는 디스패치에서 같은 리소스를 쓰려고 시도하는
 *   서로 다른 셰이더로 인한 레이스 컨디션을 방지(같은 드로우 또는 디스패치 호출에서
 *   발생할 수 있는 레이스 컨디션은 방지하지 않습니다)
 */
//
//
//    // 1. 백버퍼 지우기
//    {
//        // 백 버퍼를 RENDER_TARGET 상태로 전환합니다.
//        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
//            // 리소스의 상태를 추적하는 방법이 따로 있지만 지금은 하드코딩합니다.
//            D3D12_RESOURCE_STATE_PRESENT,        // 전이 이전 상태
//            D3D12_RESOURCE_STATE_RENDER_TARGET); // 전이 이후 상태
//        m_commandList->ResourceBarrier(1, &barrier);
//
//        // 백 버퍼를 지웁니다.
//        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
//        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
//            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);
//
//        m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
//    }
//
//    // 2. 랜더링된 프레임 표시
//    {
//        // 백 버퍼를 PRESENT 상태로 전환합니다.
//        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//            backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
//        m_commandList->ResourceBarrier(1, &barrier);
//
//        // 커맨드 리스트를 종료하고, 커맨드 큐에 제출합니다.
//        ThrowIfFailed(m_commandList->Close());
//
//        // 커맨드 리스트 실행합니다.
//        ID3D12CommandList* const commandLists[] = { m_commandList.Get() };
//        m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
//
//        // 스왑 체인의 현재 백 버퍼를 화면에 표시합니다.
//        UINT syncInterval = m_vSync ? 1 : 0;
//        UINT presentFlags = m_tearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
//        ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
//
//        // DXGI_SWAP_EFFECT_FLIP_DISCARD 플립 모델을 사용할 때, 백 버퍼 인덱스의 순서는
//        // 연속적이지 않을 수 있습니다. 스왑 체인의 현재 백 버퍼의 인덱스를 가져옵니다.
//        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
//
//        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//        for (int i = 0; i < s_bufferCount; ++i)
//        {
//            ComPtr<ID3D12Resource> backBuffer;
//            ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
//
//            m_d3d12Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
//
//            m_backBuffers[i] = backBuffer;
//
//            rtvHandle.Offset(m_rtvDescriptorSize);
//        }
//
//    }
//}

void Graphics::Resize(uint32_t width, uint32_t height)
{
    if (m_clientWidth != width || m_clientHeight != height) // 클라이언트 영역의 너비와 높이 변경 사항을 확인합니다. 
    {
        spdlog::get(m_loggerName)->info("Resize 매서드 호출({}, {})", width, height);

        m_clientWidth  = std::max(static_cast<uint32_t>(1), width);
        m_clientHeight = std::max(static_cast<uint32_t>(1), height);

        // GPU에는 스왑 체인의 백 버퍼를 참조하는 처리 중인 커맨드 리스트가 있을 수 있기 때문에
        // 스왑 체인의 백 버퍼에 대한 모든 참조를 해제해야 합니다.
        Flush();

        for (int i = 0; i < s_bufferCount; ++i)
        {
            m_backBuffers[i].Reset();
        };

        // 현재 스왑 체인 설명을 조회해 같은 색상 형식 및 스왑 체인 플래그를 사용하여 
        // 스왑 체인 버퍼를 다시 만듭니다. 
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(
            m_dxgiSwapChain->ResizeBuffers(s_bufferCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        // 백 버퍼의 인덱스가 동일하지 않을 수 있으므로, 애플리케이션에서 알고 있는 
        // 현재 백 버퍼 인덱스를 업데이트하는 것이 중요합니다.
        m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

        // 해당 버퍼를 참조하는 디스크립터를 업데이트해야 합니다.
        UpdateRenderTargetViews(m_d3d12Device, m_dxgiSwapChain, m_d3d12RTVDescriptorHeap);
    }
}

void Graphics::ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        // 창 (픽셀당) 너비 가져오기
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            m_clientWidth = ::wcstol(argv[++i], nullptr, 10);
        }
        // 창 (픽셀당) 높이 가져오기
        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            m_clientHeight = ::wcstol(argv[++i], nullptr, 10);
        }
        // Windows Advanced Rasterization Platform (WARP) 사용 여부 가져오기
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            m_useWarp = true;
        }
    }

    // CommandLineToArgvW에서 할당된 메모리 해제
    ::LocalFree(argv);
}

void Graphics::EnableDebugLayer()
{
#if defined(_DEBUG)
    // 모든 DX12 관련 작업을 수행하기 전에 항상 디버그 레이어를 활성화합니다.
    // 이렇게 하면 DX12 객체를 생성하는 동안 생성된 모든 가능한 오류가
    // 디버그 레이어에서 감지됩니다.
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

ComPtr<IDXGIAdapter4> Graphics::GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // 어댑터가 실제로 D3D12 디바이스를 생성하지 않고도 D3D12 장치를 생성할 수 있는지
            // 확인합니다. 전용 비디오 메모리가 가장 큰 어댑터가 우선적으로 선택됩니다.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }

    return dxgiAdapter4;
}

ComPtr<ID3D12Device2> Graphics::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        //
        // DirectX12 경고 및 오류를 해결해야 하지만 모든 경고를 해결하는 것은 실용적이지
        // 않을 수 있습니다. 특정 경고 메시지를 무시하도록 카테고리, 심각도, 메시지 ID에
        // 따라 스토리지 큐 필터를 지정할 수 있습니다.
        //

        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            // 리소스 생성 중에 지정된 최적화된 투명 색상이 아닌 투명 색상을 사용하여
            // 렌더링 대상을 지울 때 발생합니다. 임의의 투명 색상을 사용하여 렌더 대상을
            // 지우려면 이 경고를 비활성화해야 합니다.
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            // Visual Studio에 통합된 그래픽 디버거를 사용하여 프레임을 캡처할 때 발생합니다.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            // Visual Studio에 통합된 그래픽 디버거를 사용하여 프레임을 캡처할 때 발생합니다.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        // NewFilter.DenyList.NumCategories = _countof(Categories);
        // NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs        = _countof(DenyIds);
        NewFilter.DenyList.pIDList       = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

/// <summary>
/// 가변 주사율(vrr) 디스플레이를 지원하 앱을 만들려면 스왑 체인을 생성할 때
/// DXGI_FEATURE_PRESENT_ALLOW_TEARING 을 지정합니다. 또한, sync-interval이 0인
/// 상태에서 스왑 체인을 표시할 때 DXGI_PRESENT_ALLOW_TEARING 플래그를 사용해야 합니다.
/// </summary>
/// <returns></returns>
bool Graphics::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

ComPtr<IDXGISwapChain4> Graphics::CreateSwapChain(HWND hWnd,
    ComPtr<ID3D12CommandQueue> commandQueue,
    uint32_t width,
    uint32_t height,
    uint32_t bufferCount)
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width                 = width;
    swapChainDesc.Height                = height;
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo                = FALSE;
    // 멀티 샘플링 매개변수를 설명하는 구조체입니다. 비트 블록 전송 모델 스입체인에 유혀하며
    // 플립 모델 스왑체인을 사용할 때 {1, 0}으로 지정합니다.
    swapChainDesc.SampleDesc = { 1, 0 };
    // CPU 엑세스 옵션을 설명하는 DXGI_USAGE-typed 값입니다.
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // 왑 체인의 버퍼 수를 설명하는 값입니다. 전체 화면 스왑 체인을 생성할 때 일반적으로
    // 이 값에 프런트 버퍼를 포함합니다. 플립 프레젠테이션 모델을 사용할 때 최소 버퍼 수는
    // 두 개입니다.
    swapChainDesc.BufferCount = bufferCount;
    // 백버퍼 크기가 출력 대상과 같지 않는 경우 때의 동작을 설정하는 값입니다.
    // DXGI_SCALING_STRETCH 는 백버퍼가 대상 크기에 맞게 스케일하는 설정입니다.
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    // 스 체인에서 버퍼를 present 한 후 동작을 처리하는 설정입니다.
    // DXGI_SWAP_EFFECT_FLIP_DISCARD 는 present 한 뒤 DXGI 가 버퍼의 내용을 폐기합니다.
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // 백버퍼의 투명도를 설정하는 값입니다. DXGI_ALPHA_MODE_UNSPECIFIED 는
    // 투명도 설정을 하지 않도록 합니다.
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // 스크린 티어링 지원이 가능한 경우 DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING 플래그를 지정합니다.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        // 전체 화면 스왑체인에 대한 DXGI_SWAP_CHAIN_FULLSCREEN_DESC 구조체에
        // 포인터 입니다. 윈도우 모드로 스왑체인을 생성하려면 NULL로 설정합니다.
        nullptr,
        // 콘텐츠를 제한하는 출력을 위한 DXGIOutput 인터페이스 포인터입니다.
        // 콘텐츠 제한하는 출력을 하지 않으려면 NULL로 설정합니다.
        nullptr,
        &swapChain1));

    // Alt+Enter 전체 화면 토글 기능을 비활성화합니다.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> Graphics::CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type                       = type;
    desc.NumDescriptors             = numDescriptors;
    // desc.Flags;
    //  단일 어뎁터 작업인 경우 0으로 설정합니다.
    // desc.NodeMask;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Graphics::UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
    ComPtr<IDXGISwapChain4> swapChain,
    ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 각 백 버퍼의 수만큼 RTV를 생성합니다.
    for (int i = 0; i < s_bufferCount; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        m_backBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

ComPtr<ID3D12CommandAllocator> Graphics::CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> Graphics::CreateCommandList(ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

    ThrowIfFailed(commandList->Close());

    return commandList;
}

std::shared_ptr<CommandQueue> Graphics::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
    std::shared_ptr<CommandQueue> commandQueue;
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            commandQueue = m_DirectCommandQueue;
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            commandQueue = m_ComputeCommandQueue;
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            commandQueue = m_CopyCommandQueue;
            break;
        default:
            assert(false && "Invalid command queue type.");
    }

    return commandQueue;
}

/// <summary>
/// GPU에서 이전에 실행된 모든 명령이 완료된 후에 CPU 스레드가 계속 처리할 수 있도록
/// 보장하는 데 사용됩니다. 이는 GPU에서 처리 중인 명령이 참조하고 있는 모든 백 버퍼
/// 리소스가 리사이즈되기 전에 완료되도록 보장하는 데 유용합니다. 또한, 애플리케이션을
/// 종료하기 전에 명령 큐에 존재하는 명령 목록이 참조할 수 있는 모든 리소스를 해제하기
/// 전에 GPU 명령 큐를 플러시하는 것이 강력히 권장됩니다.
/// </summary>
void Graphics::Flush()
{
    m_DirectCommandQueue->Flush();
    m_ComputeCommandQueue->Flush();
    m_CopyCommandQueue->Flush();
}

ID3D12Device2* Graphics::GetDevice()
{
    return m_d3d12Device.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphics::GetCurrentRenderTargetView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_d3d12RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);
}

Microsoft::WRL::ComPtr<ID3D12Resource> Graphics::GetCurrentBackBuffer() const
{
    return m_backBuffers[m_currentBackBufferIndex];
}


UINT Graphics::GetCurrentBackBufferIndex() const
{
    return m_currentBackBufferIndex;
}

UINT Graphics::Present()
{
    UINT syncInterval = m_vSync ? 1 : 0;
    UINT presentFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_dxgiSwapChain->Present(syncInterval, presentFlags));
    m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    return m_currentBackBufferIndex;
}

bool Graphics::IsVSync() const
{
    return m_vSync;
}

void Graphics::SetVSync(bool vSync)
{
    m_vSync = vSync;
}

void Graphics::ToggleVSync()
{
    SetVSync(!m_vSync);
}

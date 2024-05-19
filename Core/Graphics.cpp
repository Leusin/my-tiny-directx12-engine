#include "pch.h"
#include "Graphics.h"
#include "Logger.h"

#include <shellapi.h> // CommandLineToArgvW �Լ�

Graphics::Graphics()
{
}

void Graphics::Initialize(HWND hWnd, uint32_t& width, uint32_t& hight)
{
    m_hWnd = hWnd;

    // �ΰ� ���
    spdlog::stdout_color_mt(m_loggerName);

    ParseCommandLineArguments();

    EnableDebugLayer();

    auto g_TearingSupported = CheckTearingSupport();

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(m_useWarp);

    m_device = CreateDevice(dxgiAdapter4);

    spdlog::get(m_loggerName)->info("1. ����̽� ����");

    m_commandQueue = CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    spdlog::get(m_loggerName)->info("2. Ŀ�ǵ� ť ����");

    m_swapChain = CreateSwapChain(hWnd, m_commandQueue, width, hight, m_numFrames);

    spdlog::get(m_loggerName)->info("3. ���� ü�� ����");

    // �� ������ �ε����� ���� ü�ο��� ���� �����ؼ� ���� �� �ֽ��ϴ�.
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_rtvDescriptorHeap = CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_numFrames);
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    spdlog::get(m_loggerName)->info("4. ��ũ���� �� ����");

    UpdateRenderTargetViews(m_device, m_swapChain, m_rtvDescriptorHeap);

    // Ŀ�ǵ� ��������͸� ������ ����ŭ �����մϴ�.
    for (int i = 0; i < m_numFrames; ++i)
    {
        m_commandAllocators[i] = CreateCommandAllocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    // �ϳ��� Ŀ�ǵ� ����Ʈ�� �����մϴ�.
    m_commandList = CreateCommandList(
        m_device, m_commandAllocators[m_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_fence      = CreateFence(m_device);
    m_fenceEvent = CreateEventHandle();

    spdlog::get(m_loggerName)->info("5. �潺�� �̺�Ʈ �ڵ鷯 ����");

    m_isInitialized = true;
}

void Graphics::Shutdown(void)
{
    // GPU�� ��� ���� ��� ����� �÷����մϴ�. (�߿�)
    Flush(m_commandQueue, m_fence, m_fenceValue, m_fenceEvent);

    ::CloseHandle(m_fenceEvent);
}

void Graphics::Update()
{
    // �ܼ� ������ ������Ʈ

    static uint64_t s_frameCounter = 0;
    static double elapsedSeconds   = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    s_frameCounter++;
    auto t1        = clock.now();
    auto deltaTime = t1 - t0;
    t0             = t1;

    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        char buffer[500];
        auto fps = s_frameCounter / elapsedSeconds;
        sprintf_s(buffer, 500, "FPS: %f", fps);
        spdlog::info(buffer);

        s_frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}

void Graphics::Render()
{
    auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex];
    auto backBuffer       = m_backBuffers[m_currentBackBufferIndex];

    commandAllocator->Reset();
    m_commandList->Reset(commandAllocator.Get(), nullptr);

    /*
     * DirectX 12������ �׷��� ���α׷��Ӱ� �ڿ��� ����ϱ� ���� �ùٸ� ���¿� �ִ���
     * Ȯ���ؾ� �մϴ�. �ڿ��� �ϳ��� ���¿��� �ٸ� ���·� ��ȯ�ؾ� �ϸ�, �� �۾���
     * ���ҽ� �踮� ����Ͽ� �����ϰ� �ش� ���ҽ� �踮� ��� ����Ʈ�� �����ؾ� �մϴ�.
     * ���� ���, ���� ü���� �� ���۸� ���� Ÿ������ ����ϱ� ����, �̸� RENDER_TARGET
     * ���·� ��ȯ�ؾ� �ϸ�, �������� �̹����� ȭ�鿡 ǥ���ϱ� ���ؼ��� �̸� PRESENT ���·�
     * ��ȯ�ؾ� �մϴ�.
     *
     * ���ҽ� �踮��� ���� ������ �ֽ��ϴ�.
     * - Transition: ���ҽ��� �� ���¿��� �ٸ� ���·� ��ȯ
     * - Aliasing: ���ҽ��� ������ �޸� ������ ������ �� ���
     * - UAV: ������ ���ҽ��� ���� ��� �б� �� ���� ����� �Ϸ�� ������ ���
     *   - �б� > ����: �ٸ� ���̴����� ���� ���� ��� ������ �б� �۾��� �Ϸ�Ǿ����� ����
     *   - ���� > �б�: �ٸ� ���̴����� �б� ���� ��� ������ ���� �۾��� �Ϸ�Ǿ����� ����
     *   - ���� > ����: �ٸ� ��ο� �Ǵ� ����ġ���� ���� ���ҽ��� ������ �õ��ϴ�
     *   ���� �ٸ� ���̴��� ���� ���̽� ������� ����(���� ��ο� �Ǵ� ����ġ ȣ�⿡��
     *   �߻��� �� �ִ� ���̽� ������� �������� �ʽ��ϴ�)
     */


    // 1. ����� �����
    {
        // �� ���۸� RENDER_TARGET ���·� ��ȯ�մϴ�.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
            // ���ҽ��� ���¸� �����ϴ� ����� ���� ������ ������ �ϵ��ڵ��մϴ�.
            D3D12_RESOURCE_STATE_PRESENT,        // ���� ���� ����
            D3D12_RESOURCE_STATE_RENDER_TARGET); // ���� ���� ����
        m_commandList->ResourceBarrier(1, &barrier);

        // �� ���۸� ����ϴ�.
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);

        m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // 2. �������� ������ ǥ��
    {
        // �� ���۸� PRESENT ���·� ��ȯ�մϴ�.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);

        // Ŀ�ǵ� ����Ʈ�� �����ϰ�, Ŀ�ǵ� ť�� �����մϴ�.
        ThrowIfFailed(m_commandList->Close());

        // Ŀ�ǵ� ����Ʈ �����մϴ�.
        ID3D12CommandList* const commandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        m_frameFenceValues[m_currentBackBufferIndex] = Signal(m_commandQueue, m_fence, m_fenceValue);

        // ���� ü���� ���� �� ���۸� ȭ�鿡 ǥ���մϴ�.
        UINT syncInterval = m_vSync ? 1 : 0;
        UINT presentFlags = m_tearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));

        // DXGI_SWAP_EFFECT_FLIP_DISCARD �ø� ���� ����� ��, �� ���� �ε����� ������
        // ���������� ���� �� �ֽ��ϴ�. ���� ü���� ���� �� ������ �ε����� �����ɴϴ�.
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        // ���� �������� �������� ���� �� ������ ������ ����� ����, CPU �����带 ������ŵ�ϴ�.
        WaitForFenceValue(m_fence, m_frameFenceValues[m_currentBackBufferIndex], m_fenceEvent);
    }
}

void Graphics::Resize(uint32_t width, uint32_t height)
{
    if (m_clientWidth != width || m_clientHeight != height) // Ŭ���̾�Ʈ ������ �ʺ�� ���� ���� ������ Ȯ���մϴ�. 
    {
        spdlog::get(m_loggerName)->info("Resize �ż��� ȣ��({}, {})", width, height);

        // GPU���� ���� ü���� �� ���۸� �����ϴ� ó�� ���� Ŀ�ǵ� ����Ʈ�� ���� �� �ֱ� ������
        // ���� ü���� �� ���ۿ� ���� ��� ������ �����ؾ� �մϴ�.
        Flush(m_commandQueue, m_fence, m_fenceValue, m_fenceEvent);

        for (int i = 0; i < m_numFrames; ++i)
        {
            // ���� ü���� �� ���ۿ� ���� ���� ������ �����մϴ�.
            m_backBuffers[i].Reset();

            // �����Ӹ����� �潺 ���� ���� �� ���� �ε����� �潺 ������ �缳���˴ϴ�.
            m_frameFenceValues[i] = m_frameFenceValues[m_currentBackBufferIndex];
        }

        // ���� ���� ü�� ������ ��ȸ�� ���� ���� ���� �� ���� ü�� �÷��׸� ����Ͽ� 
        // ���� ü�� ���۸� �ٽ� ����ϴ�. 
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_swapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(
            m_swapChain->ResizeBuffers(m_numFrames, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        // �� ������ �ε����� �������� ���� �� �����Ƿ�, ���ø����̼ǿ��� �˰� �ִ� 
        // ���� �� ���� �ε����� ������Ʈ�ϴ� ���� �߿��մϴ�.
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        // �ش� ���۸� �����ϴ� ��ũ���͸� ������Ʈ�ؾ� �մϴ�.
        UpdateRenderTargetViews(m_device, m_swapChain, m_rtvDescriptorHeap);
    }
}

void Graphics::ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    for (size_t i = 0; i < argc; ++i)
    {
        // â (�ȼ���) �ʺ� ��������
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            m_clientWidth = ::wcstol(argv[++i], nullptr, 10);
        }
        // â (�ȼ���) ���� ��������
        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            m_clientHeight = ::wcstol(argv[++i], nullptr, 10);
        }
        // Windows Advanced Rasterization Platform (WARP) ��� ���� ��������
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            m_useWarp = true;
        }
    }

    // CommandLineToArgvW���� �Ҵ�� �޸� ����
    ::LocalFree(argv);
}

void Graphics::EnableDebugLayer()
{
#if defined(_DEBUG)
    // ��� DX12 ���� �۾��� �����ϱ� ���� �׻� ����� ���̾ Ȱ��ȭ�մϴ�.
    // �̷��� �ϸ� DX12 ��ü�� �����ϴ� ���� ������ ��� ������ ������
    // ����� ���̾�� �����˴ϴ�.
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

            // ����Ͱ� ������ D3D12 ����̽��� �������� �ʰ� D3D12 ��ġ�� ������ �� �ִ���
            // Ȯ���մϴ�. ���� ���� �޸𸮰� ���� ū ����Ͱ� �켱������ ���õ˴ϴ�.
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
        // DirectX12 ��� �� ������ �ذ��ؾ� ������ ��� ��� �ذ��ϴ� ���� �ǿ�������
        // ���� �� �ֽ��ϴ�. Ư�� ��� �޽����� �����ϵ��� ī�װ�, �ɰ���, �޽��� ID��
        // ���� ���丮�� ť ���͸� ������ �� �ֽ��ϴ�.
        //

        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            // ���ҽ� ���� �߿� ������ ����ȭ�� ���� ������ �ƴ� ���� ������ ����Ͽ�
            // ������ ����� ���� �� �߻��մϴ�. ������ ���� ������ ����Ͽ� ���� �����
            // ������� �� ��� ��Ȱ��ȭ�ؾ� �մϴ�.
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            // Visual Studio�� ���յ� �׷��� ����Ÿ� ����Ͽ� �������� ĸó�� �� �߻��մϴ�.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            // Visual Studio�� ���յ� �׷��� ����Ÿ� ����Ͽ� �������� ĸó�� �� �߻��մϴ�.
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

ComPtr<ID3D12CommandQueue> Graphics::CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    // �켱 ����
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    // �߰� �÷���
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    // ���� GPU �۾��� ��� �� ���� 0���� �����մϴ�.
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

    return d3d12CommandQueue;
}

/// <summary>
/// ���� �ֻ���(vrr) ���÷��̸� ������ ���� ������� ���� ü���� ������ ��
/// DXGI_FEATURE_PRESENT_ALLOW_TEARING �� �����մϴ�. ����, sync-interval�� 0��
/// ���¿��� ���� ü���� ǥ���� �� DXGI_PRESENT_ALLOW_TEARING �÷��׸� ����ؾ� �մϴ�.
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
    // ��Ƽ ���ø� �Ű������� �����ϴ� ����ü�Դϴ�. ��Ʈ ��� ���� �� ����ü�ο� �����ϸ�
    // �ø� �� ����ü���� ����� �� {1, 0}���� �����մϴ�.
    swapChainDesc.SampleDesc = { 1, 0 };
    // CPU ������ �ɼ��� �����ϴ� DXGI_USAGE-typed ���Դϴ�.
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    // �� ü���� ���� ���� �����ϴ� ���Դϴ�. ��ü ȭ�� ���� ü���� ������ �� �Ϲ�������
    // �� ���� ����Ʈ ���۸� �����մϴ�. �ø� ���������̼� ���� ����� �� �ּ� ���� ����
    // �� ���Դϴ�.
    swapChainDesc.BufferCount = bufferCount;
    // ����� ũ�Ⱑ ��� ���� ���� �ʴ� ��� ���� ������ �����ϴ� ���Դϴ�.
    // DXGI_SCALING_STRETCH �� ����۰� ��� ũ�⿡ �°� �������ϴ� �����Դϴ�.
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    // �� ü�ο��� ���۸� present �� �� ������ ó���ϴ� �����Դϴ�.
    // DXGI_SWAP_EFFECT_FLIP_DISCARD �� present �� �� DXGI �� ������ ������ ����մϴ�.
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // ������� ������ �����ϴ� ���Դϴ�. DXGI_ALPHA_MODE_UNSPECIFIED ��
    // ���� ������ ���� �ʵ��� �մϴ�.
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // ��ũ�� Ƽ� ������ ������ ��� DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING �÷��׸� �����մϴ�.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        // ��ü ȭ�� ����ü�ο� ���� DXGI_SWAP_CHAIN_FULLSCREEN_DESC ����ü��
        // ������ �Դϴ�. ������ ���� ����ü���� �����Ϸ��� NULL�� �����մϴ�.
        nullptr,
        // �������� �����ϴ� ����� ���� DXGIOutput �������̽� �������Դϴ�.
        // ������ �����ϴ� ����� ���� �������� NULL�� �����մϴ�.
        nullptr,
        &swapChain1));

    // Alt+Enter ��ü ȭ�� ��� ����� ��Ȱ��ȭ�մϴ�.
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
    //  ���� ��� �۾��� ��� 0���� �����մϴ�.
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

    // ���� ü���� �� �� ������ ����ŭ RTV�� �����մϴ�.
    for (int i = 0; i < m_numFrames; ++i)
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

ComPtr<ID3D12Fence> Graphics::CreateFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

HANDLE Graphics::CreateEventHandle()
{
    HANDLE fenceEvent;

    fenceEvent = ::CreateEvent(
        // NULL �̶�� �ڽ� ���μ������� ����� �� �����ϴ�.
        NULL,
        // TRUE ��� ���� ���� ������Ʈ�� �����մϴ�. FALSE ��� �ڵ� ���� �̺�Ʈ
        // ������Ʈ�� �����ϰ� ���� �����尡 ������ �� �ý����� �ڵ����� �̺�Ʈ ���¸�
        // ��ȣ ���� ���·� �����մϴ�.
        FALSE,
        // TRUE ��� �̺�Ʈ ������Ʈ�� ��ȣ�� ���� �� �ִ� ���·� �ʱ�ȭ�ǰ�,
        // FALSE ��� ��ȣ�� ���� �ʽ��ϴ�.
        FALSE,
        // NULL �̶�� �̺�Ʈ ������Ʈ�� �̸� ���� �����˴ϴ�.
        NULL);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

uint64_t Graphics::Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

/// <summary>
/// �潺�� Ư�� ���� �������� ���� ��� CPU �����带 ����Ű�� �� ���˴ϴ�.
/// ���� ü�� ���۸� ���������Ϸ��� ���ۿ� ���� ��� ������ �����Ǿ�� �մϴ�.
/// �̸� ���� Flush �Լ��� GPU�� ��� ����� �Ϸ��ߴ��� Ȯ���ϴ� �� ���˴ϴ�.
/// </summary>
void Graphics::WaitForFenceValue(ComPtr<ID3D12Fence> fence,
    uint64_t fenceValue,
    HANDLE fenceEvent,
    std::chrono::milliseconds duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

/// <summary>
/// GPU���� ������ ����� ��� ����� �Ϸ�� �Ŀ� CPU �����尡 ��� ó���� �� �ֵ���
/// �����ϴ� �� ���˴ϴ�. �̴� GPU���� ó�� ���� ����� �����ϰ� �ִ� ��� �� ����
/// ���ҽ��� ��������Ǳ� ���� �Ϸ�ǵ��� �����ϴ� �� �����մϴ�. ����, ���ø����̼���
/// �����ϱ� ���� ��� ť�� �����ϴ� ��� ����� ������ �� �ִ� ��� ���ҽ��� �����ϱ�
/// ���� GPU ��� ť�� �÷����ϴ� ���� ������ ����˴ϴ�.
/// </summary>
void Graphics::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
{
    uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

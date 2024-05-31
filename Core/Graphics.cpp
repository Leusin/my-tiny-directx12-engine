#include "pch.h"
#include "Graphics.h"
#include "Logger.h"

#include <shellapi.h> // CommandLineToArgvW �Լ�


void Graphics::Initialize(HWND hWnd, uint32_t& width, uint32_t& hight)
{
    m_hWnd = hWnd;

    // �ΰ� ���
    spdlog::stdout_color_mt(m_loggerName);

    ParseCommandLineArguments();

    EnableDebugLayer();

    auto g_TearingSupported = CheckTearingSupport();

    ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(m_useWarp);

    spdlog::get(m_loggerName)->debug("1. ����̽� ����");

    m_d3d12Device = CreateDevice(dxgiAdapter4);

    spdlog::get(m_loggerName)->debug("2. Ŀ�ǵ� ť ����");

    m_DirectCommandQueue  = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_ComputeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CopyCommandQueue    = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);

    spdlog::get(m_loggerName)->debug("3. ���� ü�� ����");

    m_dxgiSwapChain = CreateSwapChain(hWnd, m_DirectCommandQueue->GetD3D12CommandQueue(), width, hight, s_bufferCount);

    // �� ������ �ε����� ���� ü�ο��� ���� �����ؼ� ���� �� �ֽ��ϴ�.
    m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    spdlog::get(m_loggerName)->debug("4. ��ũ���� �� ����");

    m_d3d12RTVDescriptorHeap = CreateDescriptorHeap(m_d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_bufferCount);
    m_rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews(m_d3d12Device, m_dxgiSwapChain, m_d3d12RTVDescriptorHeap);

    // Ŀ�ǵ� ��������͸� ������ ����ŭ �����մϴ�.
    for (int i = 0; i < s_bufferCount; ++i)
    {
        m_commandAllocators[i] = CreateCommandAllocator(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    spdlog::get(m_loggerName)->debug("5. Ŀ�ǵ� ����Ʈ ����");

    // �ϳ��� Ŀ�ǵ� ����Ʈ�� �����մϴ�.
    m_commandList = CreateCommandList(
        m_d3d12Device, m_commandAllocators[m_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    spdlog::get(m_loggerName)->debug("5. �潺�� �̺�Ʈ �ڵ鷯 ����");

    m_isInitialized = true;
}

void Graphics::Shutdown(void)
{
    // GPU�� ��� ���� ��� ����� �÷����մϴ�. (�߿�)
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
//
//
//    // 1. ����� �����
//    {
//        // �� ���۸� RENDER_TARGET ���·� ��ȯ�մϴ�.
//        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
//            // ���ҽ��� ���¸� �����ϴ� ����� ���� ������ ������ �ϵ��ڵ��մϴ�.
//            D3D12_RESOURCE_STATE_PRESENT,        // ���� ���� ����
//            D3D12_RESOURCE_STATE_RENDER_TARGET); // ���� ���� ����
//        m_commandList->ResourceBarrier(1, &barrier);
//
//        // �� ���۸� ����ϴ�.
//        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
//        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
//            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);
//
//        m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
//    }
//
//    // 2. �������� ������ ǥ��
//    {
//        // �� ���۸� PRESENT ���·� ��ȯ�մϴ�.
//        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
//            backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
//        m_commandList->ResourceBarrier(1, &barrier);
//
//        // Ŀ�ǵ� ����Ʈ�� �����ϰ�, Ŀ�ǵ� ť�� �����մϴ�.
//        ThrowIfFailed(m_commandList->Close());
//
//        // Ŀ�ǵ� ����Ʈ �����մϴ�.
//        ID3D12CommandList* const commandLists[] = { m_commandList.Get() };
//        m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
//
//        // ���� ü���� ���� �� ���۸� ȭ�鿡 ǥ���մϴ�.
//        UINT syncInterval = m_vSync ? 1 : 0;
//        UINT presentFlags = m_tearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
//        ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
//
//        // DXGI_SWAP_EFFECT_FLIP_DISCARD �ø� ���� ����� ��, �� ���� �ε����� ������
//        // ���������� ���� �� �ֽ��ϴ�. ���� ü���� ���� �� ������ �ε����� �����ɴϴ�.
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
    if (m_clientWidth != width || m_clientHeight != height) // Ŭ���̾�Ʈ ������ �ʺ�� ���� ���� ������ Ȯ���մϴ�. 
    {
        spdlog::get(m_loggerName)->info("Resize �ż��� ȣ��({}, {})", width, height);

        m_clientWidth  = std::max(static_cast<uint32_t>(1), width);
        m_clientHeight = std::max(static_cast<uint32_t>(1), height);

        // GPU���� ���� ü���� �� ���۸� �����ϴ� ó�� ���� Ŀ�ǵ� ����Ʈ�� ���� �� �ֱ� ������
        // ���� ü���� �� ���ۿ� ���� ��� ������ �����ؾ� �մϴ�.
        Flush();

        for (int i = 0; i < s_bufferCount; ++i)
        {
            m_backBuffers[i].Reset();
        };

        // ���� ���� ü�� ������ ��ȸ�� ���� ���� ���� �� ���� ü�� �÷��׸� ����Ͽ� 
        // ���� ü�� ���۸� �ٽ� ����ϴ�. 
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(
            m_dxgiSwapChain->ResizeBuffers(s_bufferCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        // �� ������ �ε����� �������� ���� �� �����Ƿ�, ���ø����̼ǿ��� �˰� �ִ� 
        // ���� �� ���� �ε����� ������Ʈ�ϴ� ���� �߿��մϴ�.
        m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

        // �ش� ���۸� �����ϴ� ��ũ���͸� ������Ʈ�ؾ� �մϴ�.
        UpdateRenderTargetViews(m_d3d12Device, m_dxgiSwapChain, m_d3d12RTVDescriptorHeap);
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
/// GPU���� ������ ����� ��� ����� �Ϸ�� �Ŀ� CPU �����尡 ��� ó���� �� �ֵ���
/// �����ϴ� �� ���˴ϴ�. �̴� GPU���� ó�� ���� ����� �����ϰ� �ִ� ��� �� ����
/// ���ҽ��� ��������Ǳ� ���� �Ϸ�ǵ��� �����ϴ� �� �����մϴ�. ����, ���ø����̼���
/// �����ϱ� ���� ��� ť�� �����ϴ� ��� ����� ������ �� �ִ� ��� ���ҽ��� �����ϱ�
/// ���� GPU ��� ť�� �÷����ϴ� ���� ������ ����˴ϴ�.
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

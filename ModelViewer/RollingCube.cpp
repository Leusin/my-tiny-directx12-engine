#include "RollingCube.h"
#include "Logger.h"

struct VertexPosColor
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

static VertexPosColor g_vertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },  // 1
    { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },   // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },  // 3
    { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },  // 4
    { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) },   // 5
    { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },    // 6
    { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }    // 7
};

static WORD
    g_indicies[36] = { 0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0, 3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7 };

RollingCube::RollingCube(void)
    : m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , m_FoV(45.0f)
    , m_contentLoaded(false)
{
}

void RollingCube::Startup(void)
{
    // �ΰ� ���
    spdlog::stdout_color_mt(m_loggerName);

    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(GetClientWidth()), static_cast<float>(GetClientHeight()));
}

RollingCube::~RollingCube()
{
}

void RollingCube::LoadContent(void)
{
    auto device       = GetGraphics().GetDevice();
    auto commandQueue = GetGraphics().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList  = commandQueue->GetCommandList();

    spdlog::get(m_loggerName)->debug("1. ť�� �޽ÿ� ���� ���ؽ� ���� ������ ���ε�...");

    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(
        commandList, &m_vertexBuffer, &intermediateVertexBuffer, _countof(g_vertices), sizeof(VertexPosColor), g_vertices);

    // ���ؽ� ���� �� ����
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes    = sizeof(g_vertices);
    m_vertexBufferView.StrideInBytes  = sizeof(VertexPosColor);

    spdlog::get(m_loggerName)->debug("2. ť�� �޽ÿ� ���� �ε��� ���� ������ ���ε�...");

    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(
        commandList, &m_indexBuffer, &intermediateIndexBuffer, _countof(g_indicies), sizeof(WORD), g_indicies);

    // �ε��� ���� �� ����
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format         = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes    = sizeof(g_indicies);

    spdlog::get(m_loggerName)->debug("3. ���� ���ٽ� �信 ���� ��ũ���� �� ����...");

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors             = 1;
    dsvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    /*
     * ��ũ���� ���̶�, ��ũ����(���ҽ� ��)�� �迭 �Դϴ�. ��ũ���� �迭�� ���ҷ�
     * DSV �ϳ��� �ʿ��ϴ� �ϴ��� ��ũ���� ���� �־�� �մϴ�.
     */

    spdlog::get(m_loggerName)->debug("4. ���ؽ� ���̴� �ε�...");
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    spdlog::get(m_loggerName)->debug("5. �ȼ� ���̴� �ε�...");
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    /*
     * �⺻������ ���̴� �����Ϸ�(FXC.exe)�� ���� ���͸��� ���̴��� ���� .hlsl
     * ���� �̸��� ���� ������ ���̴� ������Ʈ(cso) ������ ����� �� �Դϴ�.
     * cso ������ D3DReadFileToBlob �Լ��� ���� ���̳ʸ� ��ü�� �ε��� �� �ֽ��ϴ�.
     */

    spdlog::get(m_loggerName)->debug("6. ���ؽ� ���̴��� ���� ��ǲ ���̾ƿ� ����...");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    /*
     * ��ǲ ���̾ƿ��� ���� �Ӽ��� GPU�� ��� ���޵Ǵ��� �����ϴ� ������ �����Դϴ�.
     * ���� ����(��ŷ �Ǵ� ���͸���)�� �� �Ӽ��� ���̴��� ��� ���εǴ����� �����մϴ�.
     * HLSL������ �̸� ������ ����� ����, ��� �Է� ���̾ƿ��� ����Ͽ� ���� ������ ����
     * �մϴ�.
     *
     * �� ��ǲ ���̾ƿ� ������ ���̴������� ���� �ڵ�� �����˴ϴ�.
     * struct VertexPosColor
     * {
     *    float3 Position : POSITION;
     *    float3 Color    : COLOR;
     * };
     * - ù��° �Ķ���ʹ� POSITION �ø�ƽ�� ���ε��� 3�� ��Ҹ� ���� float ���Դϴ�.
     * - �ι�° �Ķ���ʹ� COLOR �ø�ƽ�� ���ε��� 3�� ��Ҹ� ���� float ���Դϴ�.
     */

    spdlog::get(m_loggerName)->debug("7. �罺 �ñ״�ó ����...");

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // ��ǲ ���̾ƿ��� ����ϰ� Ư�� ���������� �ܰ迡 ���� ���ʿ��� ������ �ź��մϴ�.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // ���� ���̴����� ���Ǵ� ���� 32��Ʈ ��� ��Ʈ �Ű������Դϴ�.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // ��Ʈ �ñ״�ó ����ȭ.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // ��Ʈ �ñ״�ó ����.
    ThrowIfFailed(device->CreateRootSignature(
        0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    spdlog::get(m_loggerName)->debug("8. �׷��Ƚ� ���������� ���� ��ü(PSO) ����...");
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets      = 1;
    rtvFormats.RTFormats[0]          = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature        = m_rootSignature.Get();
    pipelineStateStream.InputLayout           = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS                    = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS                    = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats            = rtvFormats;

    /*
     * PSO�� PipelineStateStream �� ����Ͽ� ����˴ϴ�. �� ����ü�� PSO�� �����ϴ� �����
     * �����ϴ� �Ϸ��� ��Ʈ�� ��ū�� �����մϴ�. ��Ʈ�� ��ū�� ���δ� Vertex Shader(VS),
     * Geometry Shader(GS), Pixel Shader(PS) ���� �ֽ��ϴ�. ��Ʈ�� ��ū�� �⺻ ���� �������ؾ�
     * �ϴ� ��쿡�� �����ϸ� �˴ϴ�. ���� ���, �׼����̼� ���̴�(Hull �� Domain Shaders)��
     * PSO�� �ʿ����� �ʴٸ� ���������� ���� ��Ʈ�� ����ü�� ������ �ʿ䰡 �����ϴ�.
     */

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    // Ŀ�ǵ� ����Ʈ�� ������ �ϱ� �� �ε����� ���ؽ� ���۰� GPU ���ҽ��� ���ε� Ȯ���ϵ���
    // Ŀ�ǵ� ť�� ���� ��ŵ�ϴ�.
    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue); // CPU ������� �潺 ���� ������ ������ �����˴ϴ�.

    m_contentLoaded = true;

    spdlog::get(m_loggerName)->debug("9. ���� ���� ����...");

    ResizeDepthBuffer(GetClientWidth(), GetClientHeight());
}

void RollingCube::UnloadContent(void)
{
    m_contentLoaded = false;
}

void RollingCube::Cleanup(void)
{
}

void RollingCube::Update(UpdateEventArgs& e)
{
    static uint64_t frameCount = 0;
    static double totalTime    = 0.0;

    IGameApp::Update(e);

    totalTime += e.ElapsedTime;
    frameCount++;

    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", fps);
        OutputDebugStringA(buffer);

        spdlog::get(m_loggerName)->info("FPS: {}", fps);

        frameCount = 0;
        totalTime  = 0.0;
    }

    // �� ��� ������Ʈ.
    float angle                 = static_cast<float>(e.TotalTime * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_modelMatrix               = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // �� ��� ������Ʈ.
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint  = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_viewMatrix               = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // ���� ��� ������Ʈ.
    float aspectRatio  = GetClientWidth() / static_cast<float>(GetClientHeight());
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void RollingCube::Render(RenderEventArgs& e)
{
    /*
    * ������ �� �ؾ� �� ��
    * 
    * �÷��� ���� ���� Ŭ����
    * ���� �ִ� �� ������Ʈ ����:
    *    ���ҽ��� �ʿ���ϴ� ���¿� �°� ����
    *    PSO ���ε�
    *    ��Ʈ �ñ״�ó ���ε�
    *    ��Ÿ ���ҽ�(CBV, UAV, SRV, Sampler ...)�� ���̴� ���������� ���ε�
    *    IA �� ������Ƽ�� �������� ����
    *    IA �� ���ؽ� ���� ���ε�
    *    IA �� �ε��� ���� ���ε�
    *    RS ������Ʈ ����
    *    RS ���� ���ʱ� ����
    *    OM �� �÷��� ����-���ٽ� ���� Ÿ�� ���ε�
    *    ���ϵ��� �׸���
    * ȭ�鿡 ������ �̹��� ǥ���ϱ�
    */

    IGameApp::Render(e);

    auto commandQueue = GetGraphics().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList  = commandQueue->GetCommandList();

    UINT currentBackBufferIndex = GetGraphics().GetCurrentBackBufferIndex();
    auto backBuffer             = GetGraphics().GetCurrentBackBuffer();
    auto rtv                    = GetGraphics().GetCurrentRenderTargetView();
    auto dsv                    = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    // ����� ����� (���� Ÿ�� Ŭ����).
    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandList, dsv);
    }

    commandList->SetPipelineState(m_PipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    /*
    * ���ҽ�(CBV, UAV, SRV, ���÷� ��ũ����)�� ���ε��ϱ� ����  Ŀ�ǵ� ����Ʈ�� ��Ʈ �ñ״�ó�� 
    * �������� ������ ���ҽ��� ���ε��ϴ� ���� ��Ÿ�� ������ �߻��մϴ�.
    */

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    // MVP ��� ������Ʈ (��Ʈ �Ķ���� ������Ʈ)
    XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
    mvpMatrix          = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    // �׸���
    commandList->DrawIndexedInstanced(_countof(g_indicies), 1, 0, 0, 0);

    // ȭ�鿡 ǥ���ϱ�
    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

        currentBackBufferIndex = GetGraphics().Present(); // �� ���� ������ ȭ�鿡 ǥ���մϴ�. 

        // ���� �����ӿ� �� ���۸� �����ϱ� ���� �ش� �ε����� �潺 ���� �Ϸ�Ǿ����� Ȯ���մϴ�.
        // �潺�� ���� �������� ���� ���, �潺�� ������ ������ CPU �����带 �����մϴ�. 
        // CommandQueue::WaitForFenceValue�� ��ȯ�� �Ŀ��� ���� �������� �������ص� �����մϴ�
        commandQueue->WaitForFenceValue(m_fenceValues[currentBackBufferIndex]);
    }
}

void RollingCube::OnKeyPressed(KeyEventArgs& e)
{
    switch (e.Key)
    {
        case KeyCode::Escape:
            ToggleIsDone();
            break;
        case KeyCode::Enter:
            if (e.Alt)
            {
                case KeyCode::F11:
                    ToggleFullscreen();
                    break;
            }
        case KeyCode::V:
            GetGraphics().ToggleVSync();
            break;
    }
}

void RollingCube::OnMouseWheel(MouseWheelEventArgs& e)
{
    m_FoV -= e.WheelDelta;
    m_FoV = std::clamp(m_FoV, 12.0f, 90.0f);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", m_FoV);
    OutputDebugStringA(buffer);
}

void RollingCube::OnResize(ResizeEventArgs& e)
{
    if (e.Width != GetClientWidth() || e.Height != GetClientHeight())
    {
        IGameApp::OnResize(e);

        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(e.Width), static_cast<float>(e.Height));

        ResizeDepthBuffer(e.Width, e.Height);
    }
}

void RollingCube::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void RollingCube::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    FLOAT* clearColor)
{
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void RollingCube::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv,
    FLOAT depth)
{
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void RollingCube::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements,
    size_t elementSize,
    const void* bufferData,
    D3D12_RESOURCE_FLAGS flags)
{
    auto device = GetGraphics().GetDevice();

    size_t bufferSize = numElements * elementSize;

    // �⺻ ���� �ִ� GPU ���ҽ��� ���� Ŀ�Ե� ���ҽ��� ����.
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    // ���ε��� Ŀ�Ե� ���ҽ��� ����.
    if (bufferData)
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData                  = bufferData;
        subresourceData.RowPitch               = bufferSize;
        subresourceData.SlicePitch             = subresourceData.RowPitch;

        ::UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
    }
}

void RollingCube::ResizeDepthBuffer(int width, int height)
{

    if (m_contentLoaded)
    {
        // ���� ���۸� ������ �� �ִ� ��� GPU Ŀ�ǵ带 �÷����մϴ�.
        GetGraphics().Flush();

        /*
         * ���۸� �������� �ϱ� ���� Ŀ�ǵ� ����Ʈ�� ������ ���� ���۸� �������� �ʾƾ� �մϴ�.
         * ���� �ش� �Լ��� ȣ���ϱ� ���� Ŀ�ǵ� ť�� �� �÷��� �ؾ� �մϴ�.
         */

        // �����찡 �ּ�ȭ�Ǹ� �������� Ŭ���̾� ȭ���� �ʺ� ���̰� 0 �� �˴ϴ�.
        // ũ�Ⱑ 0 �� ���۸� �����ϴ� ���� �����ϱ� ���� �ʼ� 1�� ũ��� �����մϴ�.
        width  = std::max(1, width);
        height = std::max(1, height);

        auto device = GetGraphics().GetDevice();

        // ȭ�� ������� �������� ���ҽ�.
        // ���� ���۸� �����մϴ�.
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil      = { 1.0f, 0 };


        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto desc           = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&m_depthBuffer)));

        // ����-���ٽ� �� ������Ʈ
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format                        = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv.Texture2D.MipSlice            = 0;
        dsv.Flags                         = D3D12_DSV_FLAG_NONE;

        device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

ComPtr<ID3DBlob> RollingCube::CompileShader(const wchar_t* filename, const char* target)
{
    ComPtr<ID3DBlob> code;
    ComPtr<ID3DBlob> errorMsgs;
    HRESULT hr = D3DCompileFromFile(filename,
        nullptr,
        nullptr,
        "main",
        target,
        0, //e e.g. D3DCOMPILE_DEBUG, D3DCOMPILE_SKIP_OPTIMIZATION
        0,
        &code,
        &errorMsgs);

    if (FAILED(hr))
    {
        if (errorMsgs.Get())
        {
            spdlog::get(m_loggerName)->error("���̴� ������ ���� - {}", (const char*)errorMsgs->GetBufferPointer());
        }
    }
    else
    {
        spdlog::get(m_loggerName)->debug("{} ���̴� ������ ���� ... ", target);
    }

    return code;
}

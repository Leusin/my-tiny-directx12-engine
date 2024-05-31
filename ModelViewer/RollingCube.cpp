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
    // 로거 등록
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

    spdlog::get(m_loggerName)->debug("1. 큐브 메시에 사용될 버텍스 버퍼 데이터 업로드...");

    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(
        commandList, &m_vertexBuffer, &intermediateVertexBuffer, _countof(g_vertices), sizeof(VertexPosColor), g_vertices);

    // 버텍스 버퍼 뷰 생성
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes    = sizeof(g_vertices);
    m_vertexBufferView.StrideInBytes  = sizeof(VertexPosColor);

    spdlog::get(m_loggerName)->debug("2. 큐브 메시에 사용될 인덱사 버퍼 데이터 업로드...");

    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(
        commandList, &m_indexBuffer, &intermediateIndexBuffer, _countof(g_indicies), sizeof(WORD), g_indicies);

    // 인덱스 버퍼 뷰 생성
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format         = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes    = sizeof(g_indicies);

    spdlog::get(m_loggerName)->debug("3. 뎁스 스텐실 뷰에 대한 디스크립터 힙 생성...");

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors             = 1;
    dsvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    /*
     * 디스크립터 힙이란, 디스크립터(리소스 뷰)의 배열 입니다. 디스크립터 배열의 원소로
     * DSV 하나만 필요하다 하더라도 디스크립터 힙이 있어야 합니다.
     */

    spdlog::get(m_loggerName)->debug("4. 버텍스 셰이더 로드...");
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob));

    spdlog::get(m_loggerName)->debug("5. 픽셀 셰이더 로드...");
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob));

    /*
     * 기본값으로 셰이더 컴파일러(FXC.exe)가 같은 디렉터리에 셰이더는 원본 .hlsl
     * 파일 이름을 가진 컴파일 셰이더 오브젝트(cso) 파일을 출력할 것 입니다.
     * cso 파일은 D3DReadFileToBlob 함수를 통해 바이너리 객체로 로드할 수 있습니다.
     */

    spdlog::get(m_loggerName)->debug("6. 버텍스 셰이더에 대한 인풋 레이아웃 생성...");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    /*
     * 인풋 레이아웃은 정점 속성이 GPU로 어떻게 전달되는지 설명하는 데이터 구조입니다.
     * 버퍼 형식(패킹 또는 인터리브)과 각 속성이 셰이더에 어떻게 매핑되는지를 정의합니다.
     * HLSL에서는 이를 지정할 방법이 없고, 대신 입력 레이아웃을 사용하여 버퍼 형식을 지정
     * 합니다.
     *
     * 위 인풋 레이아웃 형식은 셰이더에서는 다음 코드와 대응됩니다.
     * struct VertexPosColor
     * {
     *    float3 Position : POSITION;
     *    float3 Color    : COLOR;
     * };
     * - 첫번째 파라미터는 POSITION 시멘틱에 바인딩된 3개 요소를 가진 float 값입니다.
     * - 두번째 파라미터는 COLOR 시멘틱에 바인딩된 3개 요소를 가진 float 값입니다.
     */

    spdlog::get(m_loggerName)->debug("7. 루스 시그니처 생성...");

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // 인풋 레이아웃을 허용하고 특정 파이프라인 단계에 대한 불필요한 접근을 거부합니다.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // 정점 셰이더에서 사용되는 단일 32비트 상수 루트 매개변수입니다.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // 루트 시그니처 직렬화.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // 루트 시그니처 생성.
    ThrowIfFailed(device->CreateRootSignature(
        0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    spdlog::get(m_loggerName)->debug("8. 그래픽스 파이프라인 상태 객체(PSO) 생성...");
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
     * PSO는 PipelineStateStream 를 사용하여 설명됩니다. 이 구조체는 PSO를 구성하는 방법을
     * 설명하는 일련의 스트림 토큰을 포함합니다. 스트림 토큰의 예로는 Vertex Shader(VS),
     * Geometry Shader(GS), Pixel Shader(PS) 등이 있습니다. 스트림 토큰은 기본 값을 재정의해야
     * 하는 경우에만 지정하면 됩니다. 예를 들어, 테셀레이션 셰이더(Hull 및 Domain Shaders)가
     * PSO에 필요하지 않다면 파이프라인 상태 스트림 구조체에 정의할 필요가 없습니다.
     */

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

    // 커맨드 리스트가 랜더링 하기 전 인덱스와 버텍스 버퍼가 GPU 리소스로 업로드 확인하도록
    // 커맨드 큐를 실행 시킵니다.
    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue); // CPU 스레드는 펜스 값에 도달할 때까지 정지됩니다.

    m_contentLoaded = true;

    spdlog::get(m_loggerName)->debug("9. 뎁스 버퍼 생성...");

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

    // 모델 행렬 업데이트.
    float angle                 = static_cast<float>(e.TotalTime * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_modelMatrix               = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // 뷰 행렬 업데이트.
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint  = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_viewMatrix               = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // 투영 행렬 업데이트.
    float aspectRatio  = GetClientWidth() / static_cast<float>(GetClientHeight());
    m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void RollingCube::Render(RenderEventArgs& e)
{
    /*
    * 랜더할 떄 해야 할 일
    * 
    * 컬러와 뎁스 버퍼 클리어
    * 씬에 있는 각 오브젝트 마다:
    *    리소스가 필요로하는 상태에 맞게 전이
    *    PSO 바인딩
    *    루트 시그니처 바인딩
    *    기타 리소스(CBV, UAV, SRV, Sampler ...)를 셰이더 스테이지에 바인딩
    *    IA 에 프리미티브 토폴로지 세팅
    *    IA 에 버텍스 버퍼 바인딩
    *    IA 에 인덱스 버퍼 바인딩
    *    RS 에뷰포트 세팅
    *    RS 씨저 렉탱글 세팅
    *    OM 에 컬러와 뎁스-스텐실 랜더 타겟 바인딩
    *    기하도형 그리기
    * 화면에 랜더된 이미지 표시하기
    */

    IGameApp::Render(e);

    auto commandQueue = GetGraphics().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList  = commandQueue->GetCommandList();

    UINT currentBackBufferIndex = GetGraphics().GetCurrentBackBufferIndex();
    auto backBuffer             = GetGraphics().GetCurrentBackBuffer();
    auto rtv                    = GetGraphics().GetCurrentRenderTargetView();
    auto dsv                    = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

    // 백버퍼 지우기 (랜더 타깃 클리어).
    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        ClearRTV(commandList, rtv, clearColor);
        ClearDepth(commandList, dsv);
    }

    commandList->SetPipelineState(m_PipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    /*
    * 리소스(CBV, UAV, SRV, 샘플러 디스크립터)를 바인딩하기 전에  커맨드 리스트에 루트 시그니처를 
    * 설정하지 않으면 리소스를 바인딩하는 동안 런타임 오류가 발생합니다.
    */

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    // MVP 행렬 업데이트 (루트 파라미터 업데이트)
    XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
    mvpMatrix          = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    // 그리기
    commandList->DrawIndexedInstanced(_countof(g_indicies), 1, 0, 0, 0);

    // 화면에 표시하기
    {
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

        currentBackBufferIndex = GetGraphics().Present(); // 백 버퍼 내용을 화면에 표시합니다. 

        // 다음 프레임에 백 버퍼를 재사용하기 전에 해당 인덱스의 펜스 값이 완료되었는지 확인합니다.
        // 펜스에 아직 도달하지 않은 경우, 펜스에 도달할 때까지 CPU 스레드를 정지합니다. 
        // CommandQueue::WaitForFenceValue가 반환된 후에는 다음 프레임을 렌더링해도 안전합니다
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

    // 기본 힙에 있는 GPU 리소스에 대한 커밋된 리소스를 생성.
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    // 업로드할 커밋된 리소스를 생성.
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
        // 깊이 버퍼를 참조할 수 있는 모든 GPU 커맨드를 플러시합니다.
        GetGraphics().Flush();

        /*
         * 버퍼를 리사이즈 하기 전에 커맨들 리스트가 이전의 뎁스 버퍼를 참조하지 않아야 합니다.
         * 따라서 해당 함수를 호출하기 전에 커맨드 큐를 꼭 플러시 해야 합니다.
         */

        // 윈도우가 최소화되면 윈도우의 클라이언 화면의 너비나 높이가 0 이 됩니다.
        // 크기가 0 인 버퍼를 생성하는 것을 방지하기 위해 초소 1의 크기로 고정합니다.
        width  = std::max(1, width);
        height = std::max(1, height);

        auto device = GetGraphics().GetDevice();

        // 화면 리사이즈에 의존적인 리소스.
        // 뎁스 버퍼를 생성합니다.
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

        // 뎁스-스텐실 뷰 업데이트
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
            spdlog::get(m_loggerName)->error("셰이더 컴파일 실패 - {}", (const char*)errorMsgs->GetBufferPointer());
        }
    }
    else
    {
        spdlog::get(m_loggerName)->debug("{} 셰이더 컴파일 성공 ... ", target);
    }

    return code;
}

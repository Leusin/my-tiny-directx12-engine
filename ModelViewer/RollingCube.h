#pragma once

#include "GameCore.h"

using namespace gcore;
using namespace DirectX;

class RollingCube: public IGameApp
{
public:
    RollingCube(void);
    ~RollingCube();

    void Startup(void) override;
    void LoadContent(void) override;

    void UnloadContent(void) override;
    void Cleanup(void) override;

    void Update(UpdateEventArgs& e) override;
    void Render(RenderEventArgs& e) override;

    void OnKeyPressed(KeyEventArgs& e) override;
    void OnMouseWheel(MouseWheelEventArgs& e) override;

    void OnResize(ResizeEventArgs& e) override;

private:
    // 리소스 전이 핼퍼 함수.
    void TransitionResourceTransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState);

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

    // 랜더 타깃 클리어.
    void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        FLOAT* clearColor);

    // 뎁스-스텐실 뷰의 뎁스 클리어.
    void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        FLOAT depth = 1.0f);
    
    /// <summary>
    /// PU buffer 생성.
    /// </summary>
    /// <param name="commandList">버퍼 데이터를 destination resource 로 전송하기 위해 필요합니다.</param>
    /// <param name="pDestinationResource">이 메서드에서 생성된 대상 리소스를 저장합니다.</param>
    /// <param name="pIntermediateResource">이 메서드에서 생성된 중간 리소스를 저장합니다.</param>
    /// <param name="numElements">cpu 데이터가 gpu 리소스로 전송될 떄 사용합니다.</param>
    /// <param name="elementSize">cpu 데이터가 gpu 리소스로 전송될 떄 사용합니다.</param>
    /// <param name="bufferData">cpu 데이터가 gpu 리소스로 전송될 떄 사용합니다.</param>
    /// <param name="flags">버퍼 리소스를 생성하는 데 필요한 추가 플래그를 지정하는데 사용됩니다.</param>
    void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements,
        size_t elementSize,
        const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    // 클라이언트 화면 크기에 맞는 뎁스 버퍼 크기 재조정
    void ResizeDepthBuffer(int width, int height);

    /* 
    * 깊이 버퍼는 정규화된 장치 좌표 공간(NDC)에서 픽셀의 깊이를 저장합니다. 
    * 현재 깊이 버퍼의 값을 기준으로 가려지지 않은 픽셀만 그려집니다. 이를 
    * 통해 뷰어와 더 가까운 도형 위에 더 멀리 있는 도형이 그려지지 않도록 
    * 보장합니다.
    */

    ComPtr<ID3DBlob> CompileShader(const wchar_t* filename, const char* target);

private:
    // 동기화를 위해 매 랜더 프레임마다 펜스 값을 추적합니다.
    uint64_t m_fenceValues[s_bufferCount] = {};

    // 큐브의 버텍스 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    // 큐브의 인덱스 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    // 뎁스 버퍼
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
    // 뎁스 버퍼에 대한 디스크립터 힙
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    // 루트 시그니처. 
    // 파이프라인의 여러 단계로 전달되는 매개 변수를 설명합니다.
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // 파이프라인 스테인트 오브젝트. 렌더링 파이프라인 자체를 설명합니다.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

    /*
    * 파이프라인 상태 객체는 그래픽스 또는 컴퓨팅 파이프라인의 모든 상태를 
    * 캡슐화하는 객체입니다. 여기에는 셰이더 프로그램, 루트 서명, 블렌드 상태, 
    * 래스터라이저 상태, 깊이/스텐실 상태 등이 포함됩니다. PSO는 파이프라인의 
    * 설정을 한 번에 변경할 수 있도록 합니다.
    */

    // 렌더링할 화면의 특정 부분을 지정합니다.
    D3D12_VIEWPORT m_viewport;

    /*
    * 렌더링할 화면의 보이는 부분을 지정합니다. 뷰포트는 화면 크기보다 작을 
    * 수 있으며(일 반적으로 스플릿 스크린 렌더링할 때), 출력 병합 단계에 
    * 바인딩된 렌더 타겟보다 커서는 안 됩니다. 
    * 뷰포트는 래스터라이저 단계에서 지정되며, 래스터라이저에게 정규화된 
    * 장치 좌표 공간(NDC)에서 화면 공간으로 정점을 변환하는 방법을 알려줍니다. 
    * 이는 픽셀 셰이더에 필요합니다.
    * 
    * - 스플릿 스크린 렌더링(Split-Screen Rendering)
    *    : 여러 뷰포트를 사용하여 화면을 여러 영역으로 나누어 각 영역에 대해 
    *    별도로 렌더링을 수행하는 기법입니다.
    */

    D3D12_RECT m_scissorRect;


    // 카메라의 화각 값을 담습니다.
    float m_FoV;

    // 버텍스 셰이더의 매개 변수 MVP 행렬에 사용됩니다.
    DirectX::XMMATRIX m_modelMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    bool m_contentLoaded;

    // 디버깅을 위해 클래스에서 사용할 로거 이름.
    std::string m_loggerName = "viewer";
};

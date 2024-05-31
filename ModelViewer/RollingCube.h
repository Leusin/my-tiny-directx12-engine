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
    // ���ҽ� ���� ���� �Լ�.
    void TransitionResourceTransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState);

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

    // ���� Ÿ�� Ŭ����.
    void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        FLOAT* clearColor);

    // ����-���ٽ� ���� ���� Ŭ����.
    void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        FLOAT depth = 1.0f);
    
    /// <summary>
    /// PU buffer ����.
    /// </summary>
    /// <param name="commandList">���� �����͸� destination resource �� �����ϱ� ���� �ʿ��մϴ�.</param>
    /// <param name="pDestinationResource">�� �޼��忡�� ������ ��� ���ҽ��� �����մϴ�.</param>
    /// <param name="pIntermediateResource">�� �޼��忡�� ������ �߰� ���ҽ��� �����մϴ�.</param>
    /// <param name="numElements">cpu �����Ͱ� gpu ���ҽ��� ���۵� �� ����մϴ�.</param>
    /// <param name="elementSize">cpu �����Ͱ� gpu ���ҽ��� ���۵� �� ����մϴ�.</param>
    /// <param name="bufferData">cpu �����Ͱ� gpu ���ҽ��� ���۵� �� ����մϴ�.</param>
    /// <param name="flags">���� ���ҽ��� �����ϴ� �� �ʿ��� �߰� �÷��׸� �����ϴµ� ���˴ϴ�.</param>
    void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements,
        size_t elementSize,
        const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    // Ŭ���̾�Ʈ ȭ�� ũ�⿡ �´� ���� ���� ũ�� ������
    void ResizeDepthBuffer(int width, int height);

    /* 
    * ���� ���۴� ����ȭ�� ��ġ ��ǥ ����(NDC)���� �ȼ��� ���̸� �����մϴ�. 
    * ���� ���� ������ ���� �������� �������� ���� �ȼ��� �׷����ϴ�. �̸� 
    * ���� ���� �� ����� ���� ���� �� �ָ� �ִ� ������ �׷����� �ʵ��� 
    * �����մϴ�.
    */

    ComPtr<ID3DBlob> CompileShader(const wchar_t* filename, const char* target);

private:
    // ����ȭ�� ���� �� ���� �����Ӹ��� �潺 ���� �����մϴ�.
    uint64_t m_fenceValues[s_bufferCount] = {};

    // ť���� ���ؽ� ����
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    // ť���� �ε��� ����
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    // ���� ����
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
    // ���� ���ۿ� ���� ��ũ���� ��
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    // ��Ʈ �ñ״�ó. 
    // ������������ ���� �ܰ�� ���޵Ǵ� �Ű� ������ �����մϴ�.
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // ���������� ������Ʈ ������Ʈ. ������ ���������� ��ü�� �����մϴ�.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

    /*
    * ���������� ���� ��ü�� �׷��Ƚ� �Ǵ� ��ǻ�� ������������ ��� ���¸� 
    * ĸ��ȭ�ϴ� ��ü�Դϴ�. ���⿡�� ���̴� ���α׷�, ��Ʈ ����, ���� ����, 
    * �����Ͷ����� ����, ����/���ٽ� ���� ���� ���Ե˴ϴ�. PSO�� ������������ 
    * ������ �� ���� ������ �� �ֵ��� �մϴ�.
    */

    // �������� ȭ���� Ư�� �κ��� �����մϴ�.
    D3D12_VIEWPORT m_viewport;

    /*
    * �������� ȭ���� ���̴� �κ��� �����մϴ�. ����Ʈ�� ȭ�� ũ�⺸�� ���� 
    * �� ������(�� �������� ���ø� ��ũ�� �������� ��), ��� ���� �ܰ迡 
    * ���ε��� ���� Ÿ�ٺ��� Ŀ���� �� �˴ϴ�. 
    * ����Ʈ�� �����Ͷ����� �ܰ迡�� �����Ǹ�, �����Ͷ��������� ����ȭ�� 
    * ��ġ ��ǥ ����(NDC)���� ȭ�� �������� ������ ��ȯ�ϴ� ����� �˷��ݴϴ�. 
    * �̴� �ȼ� ���̴��� �ʿ��մϴ�.
    * 
    * - ���ø� ��ũ�� ������(Split-Screen Rendering)
    *    : ���� ����Ʈ�� ����Ͽ� ȭ���� ���� �������� ������ �� ������ ���� 
    *    ������ �������� �����ϴ� ����Դϴ�.
    */

    D3D12_RECT m_scissorRect;


    // ī�޶��� ȭ�� ���� ����ϴ�.
    float m_FoV;

    // ���ؽ� ���̴��� �Ű� ���� MVP ��Ŀ� ���˴ϴ�.
    DirectX::XMMATRIX m_modelMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    bool m_contentLoaded;

    // ������� ���� Ŭ�������� ����� �ΰ� �̸�.
    std::string m_loggerName = "viewer";
};

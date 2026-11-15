// ============================================================
// ファイルの役割: 立方体描画リソースの管理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

// Engine/CubeRenderer.h
#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

// クラスの責務: 立方体描画リソースの管理
class CubeRenderer {
public:
    bool Initialize(ID3D12Device* device);
    void Draw(ID3D12GraphicsCommandList* commandList,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj,
        const DirectX::XMFLOAT4& color);

private:
    struct ConstantBufferData {
        DirectX::XMFLOAT4X4 wvp;
        DirectX::XMFLOAT4 color;
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
};
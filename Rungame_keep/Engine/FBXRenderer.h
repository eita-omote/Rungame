// ============================================================
// ファイルの役割: FBX描画リソースの管理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <string>
#include "../Components/FBXModel.h"
#include "../Components/FBXLoaderComponent.h"

//
// クラスの責務: FBX描画リソースの管理
class FBXRenderer {
public:
    bool Initialize(ID3D12Device* device,
        const std::vector<SkinnedVertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::vector<FBXLoaderComponent::MaterialData>& materials,
        const std::vector<FBXLoaderComponent::SubmeshData>& submeshes);
    void Draw(ID3D12GraphicsCommandList* cmd,
        const DirectX::XMMATRIX& world,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& projection,
        const std::vector<DirectX::XMMATRIX>& bones);
private:
    static constexpr UINT MaxBones = 256;
    struct Constants {
        DirectX::XMFLOAT4X4 wvp;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT4 lightDirectionAmbient;
    };
    bool InitializeTextures(ID3D12Device* device,
        const std::vector<FBXLoaderComponent::MaterialData>& materials);
    std::vector<FBXLoaderComponent::SubmeshData> m_submeshes;
    std::vector<DirectX::XMFLOAT4> m_materialColors;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

    bool m_hasTexture = false;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertices;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_indices;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_boneBuffer;
    unsigned char* m_boneMapped = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_vbv{};
    D3D12_INDEX_BUFFER_VIEW m_ibv{};
    UINT m_indexCount = 0;
    UINT m_srvDescriptorSize = 0;
};

// ============================================================
// ファイルの役割: FBXモデルの描画処理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "FBXLoaderComponent.h"
#include "AnimatorComponent.h"
#include "../Engine/GameObject.h"
#include "../Engine/FBXRenderer.h"

// Step 1: draws FBX in its imported (unanimated) pose.
// クラスの責務: FBXモデルの描画処理
class FBXMeshRendererComponent : public Component {
public:
    bool InitializeMesh(ID3D12Device* device) {
        if (!m_owner) return false;
        auto loader = m_owner->GetComponent<FBXLoaderComponent>();
        return loader && m_renderer.Initialize(device, loader->GetVertices(), loader->GetIndices(),
            loader->GetMaterials(), loader->GetSubmeshes());
    }

    void Draw(ID3D12GraphicsCommandList* cmd, CubeRenderer&,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& proj) override {
        if (!m_owner) return;
        auto transform = m_owner->GetComponent<TransformComponent>();
        if (!transform) return;
        auto animator = m_owner->GetComponent<AnimatorComponent>();
        static const std::vector<DirectX::XMMATRIX> emptyBones;
        m_renderer.Draw(cmd, transform->GetWorldMatrix(), view, proj,
            animator ? animator->GetFinalBoneTransforms() : emptyBones);
    }
private:
    FBXRenderer m_renderer;
};

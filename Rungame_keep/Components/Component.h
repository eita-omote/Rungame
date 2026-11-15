// ============================================================
// ファイルの役割: コンポーネント共通インターフェース
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <DirectXMath.h>
#include <d3d12.h>

// 前方宣言:循環参照を防ぐためにクラスが存在することを宣言する
// クラスの責務: コンポーネント共通インターフェース
class GameObject;
// クラスの責務: コンポーネント共通インターフェース
class CubeRenderer;

// ==========================================================================================
// コンポーネントの基底クラス
// GameObjectにアタッチする機能(移動、描画、衝突判定など)を定義するための基本インターフェース
// ==========================================================================================

// クラスの責務: コンポーネント共通インターフェース
class Component {
public:
    virtual ~Component() = default;
    virtual void Initialize(GameObject* owner) { m_owner = owner; }
    virtual void Update(float deltaTime) {}
    virtual void Draw(ID3D12GraphicsCommandList* cmdList, CubeRenderer& renderer,
        const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj) {
    }

protected:
    GameObject* m_owner = nullptr;
};
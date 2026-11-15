// ============================================================
// ファイルの役割: 基本メッシュの描画処理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "../Engine/GameObject.h"
#include "../Engine/CubeRenderer.h"


// =================================================================
// 【メッシュ描画コンポーネント】
//　所有者(GameObject)の TransformComponent のワールド変換行列を取得し、CubeRenderer を使って描画する
// =================================================================
// クラスの責務: 基本メッシュの描画処理
class MeshRendererComponent : public Component
{
public:
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 描画色(RGBA)

	void Draw(ID3D12GraphicsCommandList* cmdList, CubeRenderer& renderer,
		const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj)override
	{
		// 所有者(GameObject)の TransformComponent を取得してワールド変換行列を取得
		auto transform = m_owner->GetComponent<TransformComponent>();
		if (transform)
		{
			//位置情報を行列化して、CubeRenderer で描画する
			renderer.Draw(cmdList, transform->GetWorldMatrix(), view, proj, color);
		}
	}
};
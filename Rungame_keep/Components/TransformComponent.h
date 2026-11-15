// ============================================================
// ファイルの役割: 位置・回転・拡大率の保持
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"

// クラスの責務: 位置・回転・拡大率の保持
class TransformComponent : public Component
{

	// =================================================================
	// 【トランスフォーム(位置・拡大縮小)コンポーネント】
	// ゲームオブジェクトの位置や拡大縮小を管理するコンポーネント
	// =================================================================
public :
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };	//ワールド座標上の位置
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };		//ワールド座標上の拡大縮小


	//スケールと平行移動を組み合わせてワールド変換行列を計算・取得する関数
	DirectX::XMMATRIX GetWorldMatrix() const
	{
		return DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
			DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	}
};
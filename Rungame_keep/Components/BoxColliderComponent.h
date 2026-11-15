// ============================================================
// ファイルの役割: 直方体による衝突判定
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "../Engine/GameObject.h"

//軸平行境界ボックスAABB(Axis-Aligned Bounding Box)を表す構造体
struct AABB
{
	DirectX::XMFLOAT3 minPos;
	DirectX::XMFLOAT3 maxPos;
};

// =================================================================
// 【ボックスコライダー(当たり判定)コンポーネント】
// AABBを使って衝突判定を行う
// =================================================================
// クラスの責務: 直方体による衝突判定
class BoxColliderComponent : public Component
{
public:
	DirectX::XMFLOAT3 size = { 1.0f, 1.0f, 1.0f };	//コライダーのサイズ(幅・高さ・奥行き)

	// 所有者のTransformComponentから位置を取得し、現在のAABBを計算して返す
	AABB GetAABB() const {
		auto transform = m_owner->GetComponent<TransformComponent>();
		DirectX::XMFLOAT3 pos = transform ? transform->position : DirectX::XMFLOAT3{ 0, 0, 0 };
		return{
			{pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, pos.z - size.z * 0.5f},
			{pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, pos.z + size.z * 0.5f}
		};
	}
	// 他のBoxColliderComponentとの衝突判定を行う関数
	bool Intersects(const BoxColliderComponent& other)const {
		AABB a = GetAABB();
		AABB b = other.GetAABB();
		// AABB同士の衝突判定: 各軸で重なりがあるかを確認
		return(a.minPos.x <= b.maxPos.x && a.maxPos.x >= b.minPos.x) &&
			(a.minPos.y <= b.maxPos.y && a.maxPos.y >= b.minPos.y) &&
			(a.minPos.z <= b.maxPos.z && a.maxPos.z >= b.minPos.z);
	}

};
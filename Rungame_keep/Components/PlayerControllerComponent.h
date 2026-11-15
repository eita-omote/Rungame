// ============================================================
// ファイルの役割: レーン移動・ジャンプ・着地の制御
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include <algorithm>
#include <cmath>
#include "../Engine/GameObject.h"

// =================================================================
// 【プレイヤー操作コンポーネント】
// レーン移動やジャンプ処理、物理（重力）更新を行う
// =================================================================

// クラスの責務: レーン移動・ジャンプ・着地の制御
class PlayerControllerComponent : public Component
{
public:
	//初期状態にリセット
	void Reset()
	{
		m_currentLane = 1;
		m_targetX = 0.0f;
		m_velocityY = 0.0f;
		m_isGrounded = true;
		// リセット時には位置も中央レーン・地面へ戻す。
		if (m_owner) {
			if (auto* transform = m_owner->GetComponent<TransformComponent>()) {
				transform->position.x = 0.0f;
				transform->position.y = 0.0f;
			}
		}
	}

	//左レーンへ移動
	void MoveLeft()
	{
		OutputDebugStringA("[Input] MoveLeft Called\n");
		if (m_currentLane > 0)
		{
			m_currentLane--;
			UpdateTargetX();
		}
	}

	//右レーンへ移動
	void MoveRight()
	{

		if (m_currentLane < 2)
		{
			OutputDebugStringA("[Input] MoveRight Called\n");
			m_currentLane++;
			UpdateTargetX();
		}
	}

	// ジャンプ処理（接地中のみ発動）
	void Jump() {
		if (m_isGrounded) {
			m_velocityY = m_jumpSpeed; // 上方向の初速を設定
			m_isGrounded = false;
		}
	}

	void Update(float deltaTime) override
	{
		auto transform = m_owner->GetComponent<TransformComponent>();
		if (!transform) return;

		// X軸：地上・空中の区別なく目標レーンへ滑らかに移動する。
		// 指数補間を使用するため、フレームレートによる移動速度の差を抑えられる。
		const float laneDt = (std::max)(0.0f, deltaTime);
		const float blend = 1.0f - std::exp(-m_laneResponsiveness * laneDt);
		transform->position.x += (m_targetX - transform->position.x) * blend;
		if (std::fabs(m_targetX - transform->position.x) < 0.001f)
			transform->position.x = m_targetX;

		//Y軸:空中(ジャンプ中)の場合は重力加速度を適用
		if (!m_isGrounded)
		{
			// 一定加速度の式を使い、フレームレートによるジャンプ高さの差を抑える。
			const float dt = (std::max)(0.0f, (std::min)(deltaTime, 0.05f));
			transform->position.y += m_velocityY * dt - 0.5f * m_gravity * dt * dt;
			m_velocityY -= m_gravity * dt;

			//着地判定(地面の高さ Y = 0.0)
			if (transform->position.y <= 0.0f)
			{
				transform->position.y = 0.0f;
				m_velocityY = 0.0f;
				m_isGrounded = true;
			}
		}
	}



public:
	// 物理パラメータは操作コンポーネントだけが管理する。
	void SetJumpPhysics(float initialSpeed, float gravity) {
		m_jumpSpeed = (std::max)(0.1f, initialSpeed);
		m_gravity = (std::max)(0.1f, gravity);
	}
	// 0:左、1:中央、2:右。入力ごとに隣のレーンを目標にする。
	int GetTargetLane() const { return m_currentLane; }
	void SetLaneResponsiveness(float value) { m_laneResponsiveness = (std::max)(0.1f, value); }
	float GetExpectedAirTime() const { return 2.0f * m_jumpSpeed / m_gravity; }
	float GetExpectedJumpHeight() const { return m_jumpSpeed * m_jumpSpeed / (2.0f * m_gravity); }
	bool IsGrounded() const { return m_isGrounded; }
	float GetVerticalVelocity() const { return m_velocityY; }

private:
	int m_currentLane = 1;		//現在のレーン (0:左, 1:中央, 2:右)
	float m_targetX = 0.0f;		//移動目標となるX座標
	float m_velocityY = 0.0f;	//Y軸方向の速度
	bool m_isGrounded = true;	//接地フラグ
	float m_laneResponsiveness = 12.0f; // レーン移動の追従速度（地上・空中共通）
	float m_jumpSpeed = 8.0f;  // ジャンプ初速度（ワールド単位/秒）
	float m_gravity = 20.0f;   // 重力加速度（ワールド単位/秒の2乗）

	//レーン番号(0, 1, 2)から X座標 (-2.0f, 0.0f, 2.0f)を計算する共通関数
	void UpdateTargetX()
	{
		m_targetX = static_cast<float>(m_currentLane - 1) * 2.0f;
	}
};
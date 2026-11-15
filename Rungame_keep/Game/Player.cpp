// ============================================================
// ファイルの役割: プレイヤーの基本動作
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

// Player.cpp
#include "Player.h"

using namespace DirectX;

// Player::Initialize : 必要な初期状態とリソースを準備します。
void Player::Initialize() {
    m_position = { 0.0f, 0.0f, 0.0f };
    m_currentLane = 1;
    m_targetX = 0.0f;
    m_velocityY = 0.0f;
    m_isGrounded = true;
}

// Player::MoveLeft : この関数の処理内容と呼び出し順序を確認してください。
void Player::MoveLeft() {
    if (m_currentLane > 0) {
        m_currentLane--;
        m_targetX = (m_currentLane - 1) * 2.0f;
    }
}

// Player::MoveRight : この関数の処理内容と呼び出し順序を確認してください。
void Player::MoveRight() {
    if (m_currentLane < 2) {
        m_currentLane++;
        m_targetX = (m_currentLane - 1) * 2.0f;
    }
}

// Player::Jump : この関数の処理内容と呼び出し順序を確認してください。
void Player::Jump() {
    if (m_isGrounded) {
        m_velocityY = 8.0f;
        m_isGrounded = false;
    }
}

// Player::Update : 前フレームからの時間に応じて状態を進めます。
void Player::Update(float deltaTime) {
    // X軸：Smooth Interpolation（レーン間のスムーズ移動）
    m_position.x += (m_targetX - m_position.x) * 12.0f * deltaTime;

    // Y軸：重力演算
    if (!m_isGrounded) {
        m_velocityY -= 20.0f * deltaTime;
        m_position.y += m_velocityY * deltaTime;

        if (m_position.y <= 0.0f) {
            m_position.y = 0.0f;
            m_velocityY = 0.0f;
            m_isGrounded = true;
        }
    }
}

XMMATRIX Player::GetWorldMatrix() const {
    return XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
}
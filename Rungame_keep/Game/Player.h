// ============================================================
// ファイルの役割: プレイヤーの状態と操作
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma
#include <DirectXMath.h>
#include "StageManager.h"

// クラスの責務: プレイヤーの状態と操作
class Player {
public:
    void Initialize();
    void Update(float deltaTime);
    void MoveLeft();
    void MoveRight();
    void Jump();

    AABB GetAABB() const {
        // プレイヤーサイズ 1.0x1.0x1.0 として計算
        return {
            { m_position.x - 0.5f, m_position.y - 0.5f, m_position.z - 0.5f },
            { m_position.x + 0.5f, m_position.y + 0.5f, m_position.z + 0.5f }
        };
    }
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    DirectX::XMMATRIX GetWorldMatrix() const;

private:
    DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    int m_currentLane = 1;      // 0: 左 (-2.0f), 1: 中央 (0.0f), 2: 右 (+2.0f)
    float m_targetX = 0.0f;
    float m_velocityY = 0.0f;
    bool m_isGrounded = true;
};

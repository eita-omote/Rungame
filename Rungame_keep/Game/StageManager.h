// ============================================================
// ファイルの役割: 障害物とステージの管理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "../Engine/GameObject.h"
#include "../Engine/CubeRenderer.h"
#include "../Components/BoxColliderComponent.h"

//=================================================================
// 【ステージ管理クラス】
// 障害物の生成・移動・描画を管理するクラス
//=================================================================
// クラスの責務: 障害物とステージの管理
class StageManager {
public:
    void Initialize(); // ステージやタイマーの初期化
    void Update(float deltaTime); //難易度計算・障害物の生成・移動と削除の更新処理
    void Draw(ID3D12GraphicsCommandList* cmdList, CubeRenderer& renderer, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);

    // プレイヤーのコライダーと障害物の衝突判定を行う関数
    bool CheckCollision(const BoxColliderComponent& playerCollider);
    void SetDifficultyLevel(int level) { m_difficultyLevel = (std::max)(1, (std::min)(5, level)); }
    float GetSpeed() const { return m_speed; }
    float GetSpawnInterval() const { return m_spawnInterval; }

private:
    //GameObject のスマートポインタ配列を使って障害物を管理
    std::vector<std::unique_ptr<GameObject>> m_obstacles;

    float m_spawnTimer = 0.0f;     // 障害物生成のタイマー
    int m_difficultyLevel = 1;  // DifficultyManagerから受け取る難易度
    float m_speed = 15.0f;         // 現在のスピード
    float m_spawnInterval = 1.2f;  // 現在の生成間隔

    // 難易度の初期値・限界値（上限/下限）
    const float INITIAL_SPEED = 15.0f;      // スピードの初期値
    const float MAX_SPEED = 35.0f;          // スピードの上限
    const float INITIAL_INTERVAL = 1.2f;    // 生成間隔の初期値
    const float MIN_INTERVAL = 0.45f;       // 生成間隔の下限

};
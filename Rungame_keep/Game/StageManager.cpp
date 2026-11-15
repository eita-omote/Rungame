// ============================================================
// ファイルの役割: 障害物の生成・移動・衝突判定
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#include "StageManager.h"
#include "../Components/TransformComponent.h"
#include "../Components/MeshRendererComponent.h"
#include <algorithm>
#include <cstdlib>

using namespace DirectX;

// StageManager::Initialize : 必要な初期状態とリソースを準備します。
void StageManager::Initialize() {
    m_obstacles.clear();
    m_spawnTimer = 0.0f;
    m_speed = INITIAL_SPEED;
    m_spawnInterval = INITIAL_INTERVAL;
}

// StageManager::Update : 前フレームからの時間に応じて状態を進めます。
void StageManager::Update(float deltaTime) {
    // 難易度はDifficultyManagerから渡されたレベルを使用する。
    // Lv.5以降は速度・生成間隔を上限/下限に固定する。
    const int steps = m_difficultyLevel - 1;
    m_speed = (std::min)(INITIAL_SPEED + steps * 5.0f, MAX_SPEED);
    m_spawnInterval = (std::max)(INITIAL_INTERVAL - steps * 0.1875f, MIN_INTERVAL);

    // -------------------------------------------------------------
    // 【障害物の自動生成】
    // -------------------------------------------------------------
    m_spawnTimer += deltaTime;
    if (m_spawnTimer >= m_spawnInterval) {
        m_spawnTimer = 0.0f;

        // ランダムに3つのレーンから1つ選択
        int lane = rand() % 3;
        float xPos = (lane - 1) * 2.0f;

        // 障害物 GameObject の構築とコンポーネント組み立て
        auto obs = std::make_unique<GameObject>();

        // ① 位置・サイズ設定
        auto trans = obs->AddComponent<TransformComponent>();
        trans->position = { xPos, 0.0f, 50.0f }; // プレイヤーの前方 50 の位置に生成
        trans->scale = { 1.2f, 1.2f, 1.2f };

        // ② 色（黄色系）を設定して描画コンポーネント追加
        auto mesh = obs->AddComponent<MeshRendererComponent>();
        mesh->color = XMFLOAT4(0.9f, 0.8f, 0.1f, 1.0f);

        // ③ 当たり判定コンポーネント追加
        auto col = obs->AddComponent<BoxColliderComponent>();
        col->size = trans->scale;

        // リストに追加
        m_obstacles.push_back(std::move(obs));
    }

    // -------------------------------------------------------------
    // 【障害物の移動と画面外削除】
    // -------------------------------------------------------------
    for (auto it = m_obstacles.begin(); it != m_obstacles.end();) {
        (*it)->Update(deltaTime);

        auto trans = (*it)->GetComponent<TransformComponent>();
        if (trans) {
            // プレイヤーに向かって迫ってくる移動処理
            trans->position.z -= m_speed * deltaTime;

            // プレイヤーを通り過ぎて画面外 (-10.0) に行ったらメモリから削除
            if (trans->position.z < -10.0f) {
                it = m_obstacles.erase(it);
                continue;
            }
        }
        ++it;
    }
}

// StageManager::Draw : 現在の状態を描画します。
void StageManager::Draw(ID3D12GraphicsCommandList* cmdList, CubeRenderer& renderer,
    const XMMATRIX& view, const XMMATRIX& proj) {
    // 全障害物の描画呼び出し
    for (auto& obs : m_obstacles) {
        obs->Draw(cmdList, renderer, view, proj);
    }
}

// StageManager::CheckCollision : この関数の処理内容と呼び出し順序を確認してください。
bool StageManager::CheckCollision(const BoxColliderComponent& playerCollider) {
    // 生存している全障害物とプレイヤーの当たり判定を一括チェック
    for (auto& obs : m_obstacles) {
        auto col = obs->GetComponent<BoxColliderComponent>();
        if (col && playerCollider.Intersects(*col)) {
            return true; // 1つでも衝突したら true
        }
    }
    return false;
}

// ============================================================
// ファイルの役割: プレイ中の入力・更新
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Scene.h"
#include <functional>
#include <utility>

// プレイシーン：ゲームロジックの実体は既存GameAppに残し、段階的に移行する。
// クラスの責務: プレイ中の入力・更新
class GameScene final : public Scene {
public:
    GameScene(std::function<void(float)> updateGame, std::function<void(WPARAM)> inputGame)
        : m_update(std::move(updateGame)), m_input(std::move(inputGame)) {
    }
    void Update(float deltaTime) override { if (m_update) m_update(deltaTime); }
    void OnKeyDown(WPARAM key) override { if (m_input) m_input(key); }
private:
    std::function<void(float)> m_update;
    std::function<void(WPARAM)> m_input;
};

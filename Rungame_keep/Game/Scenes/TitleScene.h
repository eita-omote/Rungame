// ============================================================
// ファイルの役割: タイトル画面の入力受付
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Scene.h"
#include <functional>
#include <utility>

// タイトルシーン：第12段階でタイトルUIを追加する。
// クラスの責務: タイトル画面の入力受付
class TitleScene final : public Scene {
public:
    explicit TitleScene(std::function<void()> startGame) : m_startGame(std::move(startGame)) {}
    void OnKeyDown(WPARAM key) override {
        if (key == VK_RETURN && m_startGame) m_startGame();
    }
private:
    std::function<void()> m_startGame;
};

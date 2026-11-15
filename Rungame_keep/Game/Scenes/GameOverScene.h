// ============================================================
// ファイルの役割: ゲームオーバー時の入力受付
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Scene.h"
#include <functional>
#include <utility>

// ゲームオーバーシーン：R/Enterで再挑戦、Tでタイトルに戻る。
// クラスの責務: ゲームオーバー時の入力受付
class GameOverScene final : public Scene {
public:
    GameOverScene(std::function<void()> retry, std::function<void()> toTitle)
        : m_retry(std::move(retry)), m_toTitle(std::move(toTitle)) {
    }
    void OnKeyDown(WPARAM key) override {
        if ((key == 'R' || key == VK_RETURN) && m_retry) m_retry();
        else if (key == 'T' && m_toTitle) m_toTitle();
    }
private:
    std::function<void()> m_retry;
    std::function<void()> m_toTitle;
};

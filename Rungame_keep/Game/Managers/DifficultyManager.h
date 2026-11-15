// ============================================================
// ファイルの役割: 経過時間に応じた難易度の更新
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <algorithm>

// 生存時間から難易度を一元管理する。
// クラスの責務: 経過時間に応じた難易度の更新
class DifficultyManager final {
public:
    void Reset() { m_elapsed = 0.0f; }
    void Update(float dt) { m_elapsed += (std::max)(0.0f, dt); }
    // 境界を跨ぐフレームの得点計算に使用する。
    float SecondsUntilNextLevel() const {
        if (GetLevel() >= 5) return 1.0e9f;
        return GetLevel() * 30.0f - m_elapsed;
    }
    float GetElapsedSeconds() const { return m_elapsed; }
    int GetLevel() const {
        return (std::min)(5, 1 + static_cast<int>(m_elapsed / 30.0f));
    }
    float GetScoreMultiplier() const { return 1.0f + 0.5f * (GetLevel() - 1); }
private:
    float m_elapsed = 0.0f;
};

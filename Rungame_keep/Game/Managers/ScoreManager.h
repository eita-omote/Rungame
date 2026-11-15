// ============================================================
// ファイルの役割: 時間と倍率に応じたスコアの計算
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <algorithm>
#include <cstdint>

// 生存時間に応じて加点。小数部分を保持してフレームレートに依存しない。
// クラスの責務: 時間と倍率に応じたスコアの計算
class ScoreManager final {
public:
    void Reset() { m_score = 0.0; }
    void Update(float dt, float multiplier) {
        m_score += static_cast<double>((std::max)(0.0f, dt)) * 10.0 * multiplier;
    }
    std::uint64_t GetScore() const { return static_cast<std::uint64_t>(m_score); }
private:
    double m_score = 0.0;
};

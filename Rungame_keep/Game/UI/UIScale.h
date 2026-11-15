// ============================================================
// ファイルの役割: 画面サイズに応じたUI倍率の計算
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <algorithm>
#include <cmath>
namespace UIScale {
    // Reference resolution: 1280 x 720 client pixels.
    inline float Factor(int width, int height) {
        if (width <= 0 || height <= 0) return 1.0f;
        return (std::min)(width / 1280.0f, height / 720.0f);
    }
    inline int Px(float value, float factor) {
        return static_cast<int>(std::lround(value * factor));
    }
    inline int Font(float value, float factor) {
        return (std::max)(1, Px(value, factor));
    }
}

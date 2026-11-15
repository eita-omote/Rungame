// ============================================================
// ファイルの役割: シーン共通の操作
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>

// シーンの共通インターフェース。シーンごとの入力・更新・開始処理を多態的に呼び出す。
// クラスの責務: シーン共通の操作
class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void Update(float deltaTime) { (void)deltaTime; }
    virtual void OnKeyDown(WPARAM key) { (void)key; }
};

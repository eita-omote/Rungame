// ============================================================
// ファイルの役割: シーンの所有と切り替え
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Scene.h"
#include <memory>
#include <array>
#include <cstddef>

// 3種類のシーンを所有し、現在のシーンだけに入力・更新を渡す。
enum class SceneId { Title = 0, Game, GameOver };

// クラスの責務: シーンの所有と切り替え
class SceneManager {
public:
    void Register(SceneId id, std::unique_ptr<Scene> scene) {
        m_scenes[static_cast<size_t>(id)] = std::move(scene);
    }

    void Change(SceneId id) {
        Scene* next = m_scenes[static_cast<size_t>(id)].get();
        if (!next || (m_current == id && m_active)) return;
        if (m_active) m_active->OnExit();
        m_current = id;
        m_active = next;
        m_active->OnEnter();
    }

    SceneId Current() const { return m_current; }
    void Update(float deltaTime) { if (m_active) m_active->Update(deltaTime); }
    void OnKeyDown(WPARAM key) { if (m_active) m_active->OnKeyDown(key); }

private:
    std::array<std::unique_ptr<Scene>, 3> m_scenes{};
    Scene* m_active = nullptr;
    SceneId m_current = SceneId::Title;
};

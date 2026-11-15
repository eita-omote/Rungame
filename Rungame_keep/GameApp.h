// ============================================================
// ファイルの役割: ゲーム全体の構成要素と状態の管理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>
#include <memory>
#include "Engine/GraphicsDevice.h"
#include "Engine/Camera.h"
#include "Engine/CubeRenderer.h"
#include "Game/Player.h"
#include "Game/StageManager.h"
#include "Game/Scenes/SceneManager.h"
#include "Game/UI/TitleOverlay.h"
#include "Game/UI/ScoreOverlay.h"
#include "Game/UI/GameOverOverlay.h"
#include "Engine/FadeRenderer.h"
#include "Game/Managers/DifficultyManager.h"
#include "Game/Managers/ScoreManager.h"
//=================================================================
// 【ゲームアプリケーションクラス】
// ウィンドウ生成、ゲームの初期化・更新・描画を管理するクラス
//=================================================================
// クラスの責務: ゲーム全体の構成要素と状態の管理
class GameApp {
public:
    bool Initialize(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height);
    void Run();
    void OnKeyDown(WPARAM key);

private:
    HWND m_hwnd = nullptr;
    UINT m_width = 1280;
    UINT m_height = 720;
    bool m_resizePending = false;
    bool m_isMinimized = false;

    //OOP設計に基づき、GraphicsDevice, Camera, CubeRenderer, Player, StageManager の各クラスをメンバ変数として保持
    GraphicsDevice m_graphics;
    Camera m_camera;
    CubeRenderer m_cubeRenderer;

    // プレイヤーと床の GameObject をスマートポインタで管理
    std::unique_ptr<GameObject> m_player;
    std::unique_ptr <GameObject> m_floor;
    StageManager m_stageManager;

    // シーンの所有と遷移を管理する。
    SceneManager m_sceneManager;
    TitleOverlay m_titleOverlay;
    ScoreOverlay m_scoreOverlay;
    GameOverOverlay m_gameOverOverlay;
    FadeRenderer m_fadeRenderer;
    float m_fadeOpacity = 0.0f;

    enum class FadePhase { None, FadeOutGame };
    FadePhase m_fadePhase = FadePhase::None;
    float m_fadeElapsed = 0.0f;
    static constexpr float kFadeOutSeconds = 0.35f;
    void UpdateGameOverTransition(float deltaTime);
    DifficultyManager m_difficultyManager;
    ScoreManager m_scoreManager;

    void ResetGame();
    void StartGame();
    void UpdateGame(float deltaTime);
    void HandleGameInput(WPARAM key);
    void UpdateWindowCaption();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
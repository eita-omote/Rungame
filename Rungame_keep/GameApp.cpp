// ============================================================
// ファイルの役割: ゲーム全体の初期化・更新・シーン遷移・ウィンドウ処理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#include "GameApp.h"
#include "Components/TransformComponent.h"
#include "Components/PlayerControllerComponent.h"
#include "Components/BoxColliderComponent.h"
#include "Components/MeshRendererComponent.h"
#include "Components/FBXLoaderComponent.h"
#include "Components/AnimatorComponent.h"
#include "Components/PlayerAnimationStateComponent.h"
#include "Components/FBXMeshRendererComponent.h"
#include "Game/Scenes/TitleScene.h"
#include "Game/Scenes/GameScene.h"
#include "Game/Scenes/GameOverScene.h"

using namespace DirectX;

// GameApp::Initialize : 必要な初期状態とリソースを準備します。
bool GameApp::Initialize(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
{
    m_width = width;
    m_height = height;

    // ウィンドウクラスの登録
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"RunGameClass", nullptr };
    RegisterClassEx(&wc);

    m_hwnd = CreateWindow(wc.lpszClassName, L"3D Run Game", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 100, 100, width, height, nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    // CreateWindowのwidth/heightは外枠込み。実際の描画領域を使用する。
    RECT initialClient{};
    GetClientRect(m_hwnd, &initialClient);
    m_width = static_cast<UINT>(initialClient.right - initialClient.left);
    m_height = static_cast<UINT>(initialClient.bottom - initialClient.top);
    if (m_width == 0 || m_height == 0) return false;

    // DirectX 12 の初期化
// DirectX 12描画基盤を最初に初期化します。失敗した場合は起動を中断します。
    if (!m_graphics.Initialize(m_hwnd, m_width, m_height)) return false;
    // レンダラーとカメラの初期化
    if (!m_cubeRenderer.Initialize(m_graphics.GetDevice())) return false;

    m_camera.Initialize(static_cast<float>(m_width) / m_height);



    //==========================================================
    // 床の GameObject を生成し、コンポーネントを追加
    //==========================================================
    m_floor = std::make_unique<GameObject>();
    auto floorTrans = m_floor->AddComponent<TransformComponent>();
    floorTrans->position = { 0.0f, -0.6f, 30.0f };
    floorTrans->scale = { 6.0f, 0.2f, 60.0f };
    auto floorMesh = m_floor->AddComponent<MeshRendererComponent>();
    floorMesh->color = XMFLOAT4{ 0.4f, 0.4f, 0.4f, 1.0f };

    //==========================================================
    // プレイヤーの GameObject を生成し、コンポーネントを追加
    //==========================================================
    m_player = std::make_unique<GameObject>();
    auto playerTransform = m_player->AddComponent<TransformComponent>();
    playerTransform->scale = { 0.01f, 0.01f, 0.01f }; // Adjust for your FBX units
    m_player->AddComponent<BoxColliderComponent>()->size = { 1.0f, 1.0f, 1.0f };
    m_player->AddComponent<PlayerControllerComponent>();

    // Mixamoからダウンロードしたモデル（メッシュ・スケルトン）を読み込み
    auto fbxLoader = m_player->AddComponent<FBXLoaderComponent>();
    if (!fbxLoader->LoadModel("Assets/Ch06_nonPBR.fbx")) return false;

    // Step 2: load run animation before creating the skinned renderer.
    auto animator = m_player->AddComponent<AnimatorComponent>();
    if (!animator->LoadClip("Run", "Assets/Fast Run.fbx")) return false;
    if (!animator->LoadClip("Jump", "Assets/Jump.fbx")) return false;
    // 第9段階：ジャンプ時間とアニメーションの長さを同期させる。
    // 物理計算はPlayerControllerComponent、再生速度は状態コンポーネントが管理する。
    animator->SetBlendDuration(0.15f); // seconds; adjust 0.10-0.25 to taste
    animator->Play("Run");
    m_player->AddComponent<PlayerAnimationStateComponent>()->Reset();

    auto fbxMesh = m_player->AddComponent<FBXMeshRendererComponent>();
    if (!fbxMesh->InitializeMesh(m_graphics.GetDevice())) return false;

    // 第11段階：各シーンを登録。SceneManagerが所有し、GameAppは遷移を要求する。
// シーンの実体はSceneManagerが所有し、GameAppは開始・更新処理を渡します。
    m_sceneManager.Register(SceneId::Title,
        std::make_unique<TitleScene>([this]() { StartGame(); }));
    m_sceneManager.Register(SceneId::Game,
        std::make_unique<GameScene>(
            [this](float dt) { UpdateGame(dt); },
            [this](WPARAM key) { HandleGameInput(key); }));
    m_sceneManager.Register(SceneId::GameOver,
        std::make_unique<GameOverScene>(
            [this]() { StartGame(); },
            [this]() { m_sceneManager.Change(SceneId::Title); UpdateWindowCaption(); }));

    // タイトル表示用の子ウィンドウを作成する。
    if (!m_titleOverlay.Initialize(m_hwnd)) return false;
    if (!m_scoreOverlay.Initialize(m_hwnd)) return false;
    if (!m_gameOverOverlay.Initialize(m_hwnd)) return false;
    if (!m_fadeRenderer.Initialize(m_graphics.GetDevice())) return false;

    ResetGame();
    m_sceneManager.Change(SceneId::Title);
    UpdateWindowCaption();

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    // 初期化中に届いたWM_SIZEも描画開始前に処理する。
    m_resizePending = true;

    return true;
}

// ゲームのリセット処理
// GameApp::ResetGame : ゲーム状態を初期値に戻します。
void GameApp::ResetGame()
{
    auto trans = m_player->GetComponent<TransformComponent>();
    if (trans) trans->position = { 0.0f, 0.0f, 0.0f };

    auto mesh = m_player->GetComponent<MeshRendererComponent>();
    if (mesh) mesh->color = XMFLOAT4{ 0.9f, 0.2f, 0.2f, 1.0f };

    auto controller = m_player->GetComponent<PlayerControllerComponent>();
    if (controller) controller->Reset();
    if (auto state = m_player->GetComponent<PlayerAnimationStateComponent>()) state->Reset();

    m_stageManager.Initialize();
    m_difficultyManager.Reset();
    m_scoreManager.Reset();
    m_stageManager.SetDifficultyLevel(1);
    m_scoreOverlay.SetValues(0, 0.0f, 1, 1.0f);

}

// ゲーム開始・リトライ時に、プレイヤーとステージを初期化する。
// GameApp::StartGame : ゲーム状態をリセットしてプレイを開始します。
void GameApp::StartGame()
{
    m_fadePhase = FadePhase::None;
    m_fadeElapsed = 0.0f;
    m_fadeOpacity = 0.0f;
    ResetGame();
    m_sceneManager.Change(SceneId::Game);
    UpdateWindowCaption();
}

// 第11段階は仮の画面表示としてウィンドウタイトルで状態を確認する。
// 第12〜14段階で専用の2D UI描画へ置き換える。
// GameApp::UpdateWindowCaption : シーンに応じてタイトルバーとUI表示を切り替えます。
void GameApp::UpdateWindowCaption()
{
    if (!m_hwnd) return;
    // タイトルのときだけUIを前面に表示する。
    m_titleOverlay.SetVisible(m_sceneManager.Current() == SceneId::Title);
    m_scoreOverlay.SetVisible(m_sceneManager.Current() == SceneId::Game && m_fadePhase == FadePhase::None);
    m_gameOverOverlay.SetVisible(m_sceneManager.Current() == SceneId::GameOver && m_fadePhase == FadePhase::None);
    switch (m_sceneManager.Current()) {
    case SceneId::Title:
        SetWindowTextW(m_hwnd, L"3D Run Game | TITLE - Enter: Start"); break;
    case SceneId::Game:
        SetWindowTextW(m_hwnd, L"3D Run Game | PLAYING - A/D: Lane, Space: Jump"); break;
    case SceneId::GameOver:
        SetWindowTextW(m_hwnd, L"3D Run Game | GAME OVER - R/Enter: Retry, T: Title"); break;
    }
}

// 入力は現在のシーンにのみ届ける。タイトル中の移動入力は無効。
// GameApp::OnKeyDown : キー入力を現在のシーンに渡します。
void GameApp::OnKeyDown(WPARAM key)
{
    // 衝突演出中の入力による意図しないシーン遷移を防ぐ。
    if (m_fadePhase != FadePhase::None) return;
    m_sceneManager.OnKeyDown(key);
}

// プレイシーン固有の操作。空中レーン移動を維持する。
// GameApp::HandleGameInput : ゲーム中のキー操作を移動・ジャンプに変換します。
void GameApp::HandleGameInput(WPARAM key)
{
    if (!m_player) return;
    auto* controller = m_player->GetComponent<PlayerControllerComponent>();
    if (!controller) return;
    switch (key) {
    case VK_LEFT: case 'A': controller->MoveLeft(); break;
    case VK_RIGHT: case 'D': controller->MoveRight(); break;
    case VK_SPACE: case 'W': controller->Jump(); break;
    }
}

// プレイシーンのみゲームの時間を進め、衝突したらゲームオーバーへ遷移する。
// GameApp::UpdateGame : スコア・難易度・プレイヤー・障害物・衝突判定を更新します。
void GameApp::UpdateGame(float deltaTime)
{
    if (!m_player || m_fadePhase != FadePhase::None) return;
    // 境界時刻を跨ぐフレームでは、前後の倍率を正確に適用する。
    float remaining = deltaTime;
    while (remaining > 0.0f) {
        const float untilNext = m_difficultyManager.SecondsUntilNextLevel();
        const float step = (std::min)(remaining, (std::max)(0.00001f, untilNext));
        m_scoreManager.Update(step, m_difficultyManager.GetScoreMultiplier());
        m_difficultyManager.Update(step);
        remaining -= step;
    }
    m_stageManager.SetDifficultyLevel(m_difficultyManager.GetLevel());
    m_scoreOverlay.SetValues(m_scoreManager.GetScore(),
        m_difficultyManager.GetElapsedSeconds(), m_difficultyManager.GetLevel(),
        m_difficultyManager.GetScoreMultiplier());

    m_player->Update(deltaTime);
    if (auto* state = m_player->GetComponent<PlayerAnimationStateComponent>())
        state->Tick(deltaTime);
    m_stageManager.Update(deltaTime);

    auto* playerCol = m_player->GetComponent<BoxColliderComponent>();
// プレイヤーと障害物の衝突時にフェード遷移を開始します。
    if (playerCol && m_stageManager.CheckCollision(*playerCol)) {
        // ゲームオーバー直前の結果を確定し、画面に渡す。
        m_gameOverOverlay.SetResults(m_scoreManager.GetScore(),
            m_difficultyManager.GetElapsedSeconds(), m_difficultyManager.GetLevel(),
            m_difficultyManager.GetScoreMultiplier());
        // 衝突したフレームでゲームの更新を停止し、暗転を開始する。
        m_fadePhase = FadePhase::FadeOutGame;
        m_fadeElapsed = 0.0f;
        m_fadeOpacity = 0.01f;
        m_scoreOverlay.SetVisible(false);
    }
    if (auto* trans = m_player->GetComponent<TransformComponent>())
        m_camera.UpdateFollow(trans->position);
}

// GameApp::UpdateGameOverTransition : 暗転時間を進め、終了後にゲームオーバーへ遷移します。
void GameApp::UpdateGameOverTransition(float deltaTime)
{
    if (m_fadePhase != FadePhase::FadeOutGame) return;

    m_fadeElapsed += deltaTime;
    const float progress = (std::min)(1.0f, m_fadeElapsed / kFadeOutSeconds);
    m_fadeOpacity = progress;

    if (progress >= 1.0f) {
        // Fade-out is complete: switch directly to the opaque GameOver UI.
        // Do not reveal the 3D game scene in between.
        m_sceneManager.Change(SceneId::GameOver);
        m_fadePhase = FadePhase::None;
        m_fadeElapsed = 0.0f;
        m_fadeOpacity = 0.0f;
        UpdateWindowCaption();
    }
}

// GameApp::Run : メッセージ処理・更新・描画を繰り返します。
void GameApp::Run()
{
    MSG msg{};
    LARGE_INTEGER frequency{}, lastTime{}, currentTime{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    bool running = true;

// メッセージ処理とフレーム更新を分離し、キー長押し中も描画を継続します。
    while (running) {
        // キー長押し中も更新・描画が止まらないよう、メッセージと描画を分離する。
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) break;
        if (m_isMinimized) {
            // 最小化中は0サイズのバックバッファを生成しない。
            WaitMessage();
            QueryPerformanceCounter(&lastTime);
            continue;
        }
        if (m_resizePending) {
            m_resizePending = false;
            RECT rc{};
            GetClientRect(m_hwnd, &rc);
            const UINT newWidth = static_cast<UINT>(rc.right - rc.left);
            const UINT newHeight = static_cast<UINT>(rc.bottom - rc.top);
            if (newWidth > 0 && newHeight > 0) {
                if (!m_graphics.Resize(newWidth, newHeight)) {
                    MessageBoxW(m_hwnd, L"Failed to resize DirectX 12.", L"Resize Error", MB_OK | MB_ICONERROR);
                    break;
                }
                m_width = newWidth;
                m_height = newHeight;
                m_camera.Initialize(static_cast<float>(newWidth) / newHeight);
                m_titleOverlay.Resize();
                m_scoreOverlay.Resize();
                m_gameOverOverlay.Resize();
            }
            // リサイズ中の経過時間をゲーム進行に加算しない。
            QueryPerformanceCounter(&lastTime);
        }

        QueryPerformanceCounter(&currentTime);
        float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart)
            / static_cast<float>(frequency.QuadPart);
        lastTime = currentTime;
        if (deltaTime > 0.05f) deltaTime = 0.05f;
        if (deltaTime < 0.0f) deltaTime = 0.0f;

        m_sceneManager.Update(deltaTime);
        UpdateGameOverTransition(deltaTime);
// 1フレームの描画開始。ここからEndFrameまでGPUへ描画命令を積みます。
        m_graphics.BeginFrame();
        auto* cmdList = m_graphics.GetCommandList();
        XMMATRIX view = m_camera.GetViewMatrix();
        XMMATRIX proj = m_camera.GetProjectionMatrix();
        // Draw 3D only during gameplay. GameOver uses its own opaque UI.
        if (m_sceneManager.Current() == SceneId::Game) {
            if (m_floor) m_floor->Draw(cmdList, m_cubeRenderer, view, proj);
            if (m_player) m_player->Draw(cmdList, m_cubeRenderer, view, proj);
            m_stageManager.Draw(cmdList, m_cubeRenderer, view, proj);
        }
        // Draw the DX12 fade after the 3D scene, before Present.
        m_fadeRenderer.Draw(cmdList, m_fadeOpacity);
// 描画コマンドを確定し、スワップチェーンを表示します。
        m_graphics.EndFrame();
    }
}

LRESULT CALLBACK GameApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GameApp* app = reinterpret_cast<GameApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (uMsg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    if (app) {
        if (uMsg == WM_KEYDOWN) {
            // OSのキーリピートは無視し、1押下につき1レーン変更する。
            if ((lParam & (1LL << 30)) == 0) app->OnKeyDown(wParam);
            return 0;
        }
        if (uMsg == WM_SIZE) {
            app->m_isMinimized = (wParam == SIZE_MINIMIZED);
            if (!app->m_isMinimized) {
                // Win32の子UIは即座に追従させる。
                app->m_titleOverlay.Resize();
                app->m_scoreOverlay.Resize();
                app->m_gameOverOverlay.Resize();
                // GPUリソースは描画ループ側で安全に更新する。
                app->m_resizePending = true;
            }
            return 0;
        }
        if (uMsg == WM_EXITSIZEMOVE) {
            app->m_titleOverlay.Resize();
            app->m_scoreOverlay.Resize();
            app->m_gameOverOverlay.Resize();
            app->m_resizePending = true;
            return 0;
        }
        if (uMsg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
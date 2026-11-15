// ============================================================
// ファイルの役割: Windowsアプリケーションの開始処理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#include "GameApp.h"
#include <windows.h>

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    GameApp app;

    if (!app.Initialize(hInstance, nCmdShow, 1280, 720))
    {
        MessageBoxW(
            nullptr,
            L"GameApp initialization failed.",
            L"Initialization Error",
            MB_OK | MB_ICONERROR
        );

        return -1;
    }

    MessageBoxW(
        nullptr,
        L"Initialization succeeded.",
        L"Debug",
        MB_OK
    );

    app.Run();

    return 0;
}
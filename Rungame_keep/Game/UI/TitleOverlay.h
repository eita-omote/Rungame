// ============================================================
// ファイルの役割: タイトル画面の表示
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>
#include <algorithm>
#include "UIScale.h"

// 第12段階：タイトル専用のWin32描画ビュー。
// DirectX 12の3D描画や既存のコンポーネントには影響を与えない。
// 将来DirectX 12の2D描画を導入する際は、このクラスだけ置き換えられる。
// クラスの責務: タイトル画面の表示
class TitleOverlay final {
public:
    bool Initialize(HWND parent) {
        m_parent = parent;
        HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
        WNDCLASSW wc{};
        wc.lpfnWndProc = &TitleOverlay::WndProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"RunGameTitleOverlayV12";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        // 同じクラスが登録済みでも続行する。
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        RECT rc{};
        GetClientRect(parent, &rc);
        m_window = CreateWindowExW(0, wc.lpszClassName, L"", WS_CHILD | WS_CLIPSIBLINGS,
            0, 0, rc.right, rc.bottom, parent, nullptr, instance, nullptr);
        if (!m_window) return false;
        Resize();
        ShowWindow(m_window, SW_HIDE);
        return true;
    }

    void SetVisible(bool visible) {
        if (!m_window) return;
        if (visible) {
            Resize();
            SetWindowPos(m_window, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            InvalidateRect(m_window, nullptr, FALSE);
            UpdateWindow(m_window);
        }
        else {
            ShowWindow(m_window, SW_HIDE);
        }
    }

    void Resize()
    {
        if (!m_window || !m_parent)
        {
            return;
        }

        RECT rc{};
        GetClientRect(m_parent, &rc);

        // 親ウィンドウの現在のクライアントサイズ
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;

        if (width <= 0 || height <= 0)
        {
            return;
        }

        // タイトルUIを親ウィンドウ全体に合わせる
        SetWindowPos(
            m_window,
            HWND_TOP,
            0,
            0,
            width,
            height,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER
        );

        // 子ウィンドウの実際のサイズを取得
        RECT overlayRect{};
        GetClientRect(m_window, &overlayRect);

        // デバッグ出力
        wchar_t debugText[256]{};

        swprintf_s(
            debugText,
            L"Parent: %d x %d / Overlay: %ld x %ld\n",
            width,
            height,
            overlayRect.right - overlayRect.left,
            overlayRect.bottom - overlayRect.top
        );

        OutputDebugStringW(debugText);

        // サイズ変更後の再描画を要求
        RedrawWindow(
            m_window,
            nullptr,
            nullptr,
            RDW_INVALIDATE | RDW_ERASE
        );
    }

private:
    HWND m_parent = nullptr;
    HWND m_window = nullptr;

    static void DrawCentered(HDC dc, const wchar_t* text, int y, int height,
        int fontSize, int weight, COLORREF color, int width, float scale) {
        HFONT font = CreateFontW(-UIScale::Font(fontSize, scale), 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(dc, font);
        SetTextColor(dc, color);
        RECT area{ UIScale::Px(16, scale), y, width - UIScale::Px(16, scale), y + height };
        DrawTextW(dc, text, -1, &area, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT) {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int width = rc.right - rc.left;
            const int height = rc.bottom - rc.top;

            //現在のウィンドウの中央
            const int centerY = height / 2;
            const float scale = UIScale::Factor(width, height);
            const auto S = [scale](int value) { return UIScale::Px(value, scale); };

            HBRUSH background = CreateSolidBrush(RGB(14, 23, 42));
            FillRect(dc, &rc, background);
            DeleteObject(background);
            SetBkMode(dc, TRANSPARENT);

            // 装飾ラインと見出し（ゲーム内容とは独立したタイトルUI）
            HBRUSH accent = CreateSolidBrush(RGB(45, 212, 191));
            RECT line{ width / 2 - S(100), centerY - S(65), width / 2 + S(100), centerY - S(61) };
            FillRect(dc, &line, accent);
            DeleteObject(accent);
            // タイトル文字
            DrawCentered(
                dc, L"3D RUN GAME", centerY - S(180), S(100), 52, FW_BOLD, RGB(242, 247, 255), width, scale);

            // サブタイトル
            DrawCentered(
                dc, L"SURVIVE / DODGE / RUN", centerY - S(50), S(34), 19, FW_SEMIBOLD, RGB(45, 212, 191), width, scale);

            // ゲーム開始案内
            DrawCentered(
                dc,
                L"PRESS ENTER TO START", centerY + S(18), S(45), 27, FW_BOLD, RGB(242, 247, 255), width, scale);

            // 操作説明
            DrawCentered(
                dc, L"A / D or LEFT / RIGHT : CHANGE LANE", centerY + S(100), S(28), 17, FW_NORMAL, RGB(180, 194, 215), width, scale);

            DrawCentered(
                dc, L"SPACE or W : JUMP", centerY + S(133), S(28), 17, FW_NORMAL, RGB(180, 194, 215), width, scale);
            EndPaint(hwnd, &ps);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
};

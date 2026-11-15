// ============================================================
// ファイルの役割: ゲームオーバー結果画面の表示
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>
#include <cstdint>
#include <algorithm>
#include <string>
#include "UIScale.h"

// ゲームオーバー専用のWin32子ウィンドウ。
// タイトル画面と同じ色調で、親のクライアント領域に追従する。
// クラスの責務: ゲームオーバー結果画面の表示
class GameOverOverlay final {
public:
    bool Initialize(HWND parent) {
        m_parent = parent;
        HINSTANCE instance = reinterpret_cast<HINSTANCE>(
            GetWindowLongPtrW(parent, GWLP_HINSTANCE));
        WNDCLASSW wc{};
        wc.lpfnWndProc = &GameOverOverlay::WndProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"RunGameOverOverlayV14";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        if (!RegisterClassW(&wc) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

        RECT rc{};
        GetClientRect(parent, &rc);
        m_window = CreateWindowExW(
            0, wc.lpszClassName, L"",
            WS_CHILD | WS_CLIPSIBLINGS,
            0, 0, rc.right, rc.bottom,
            parent, nullptr, instance, this);
        if (!m_window) return false;
        Resize();
        ShowWindow(m_window, SW_HIDE);
        return true;
    }

    void SetResults(std::uint64_t score, float seconds, int level, float multiplier) {
        m_score = score;
        m_seconds = (std::max)(0.0f, seconds);
        m_level = level;
        m_multiplier = multiplier;
        if (m_window) InvalidateRect(m_window, nullptr, FALSE);
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

    void Resize() {
        if (!m_window || !m_parent) return;
        RECT rc{};
        GetClientRect(m_parent, &rc);
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;
        if (width <= 0 || height <= 0) return;
        SetWindowPos(m_window, HWND_TOP, 0, 0, width, height,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        InvalidateRect(m_window, nullptr, FALSE);
    }

private:
    HWND m_parent = nullptr;
    HWND m_window = nullptr;
    std::uint64_t m_score = 0;
    float m_seconds = 0.0f;
    int m_level = 1;
    float m_multiplier = 1.0f;

    static void CenteredText(HDC dc, const wchar_t* value, int y, int lineHeight,
        int fontSize, int weight, COLORREF color, int width, float scale) {
        HFONT font = CreateFontW(-UIScale::Font(fontSize, scale), 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        if (!font) return;
        HGDIOBJ previous = SelectObject(dc, font);
        SetTextColor(dc, color);
        RECT rect{ UIScale::Px(12, scale), y, width - UIScale::Px(12, scale), y + lineHeight };
        DrawTextW(dc, value, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(dc, previous);
        DeleteObject(font);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return TRUE;
        }
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT) {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int width = rc.right - rc.left;
            const int height = rc.bottom - rc.top;
            const int centerY = height / 2;
            const float scale = UIScale::Factor(width, height);
            const auto S = [scale](int value) { return UIScale::Px(value, scale); };
            auto* self = reinterpret_cast<GameOverOverlay*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            HBRUSH background = CreateSolidBrush(RGB(14, 23, 42));
            FillRect(dc, &rc, background);
            DeleteObject(background);
            SetBkMode(dc, TRANSPARENT);

            HBRUSH accent = CreateSolidBrush(RGB(45, 212, 191));
            RECT line{ width / 2 - S(100), centerY - S(115), width / 2 + S(100), centerY - S(111) };
            FillRect(dc, &line, accent);
            DeleteObject(accent);

            CenteredText(dc, L"GAME OVER", centerY - S(235), S(82), 54,
                FW_BOLD, RGB(242, 247, 255), width, scale);
            CenteredText(dc, L"YOUR RUN HAS ENDED", centerY - S(100), S(32), 19,
                FW_SEMIBOLD, RGB(45, 212, 191), width, scale);

            if (self) {
                const std::wstring score = L"FINAL SCORE  " + std::to_wstring(self->m_score);
                const int totalSeconds = static_cast<int>(self->m_seconds);
                const std::wstring time = L"SURVIVAL TIME  " +
                    std::to_wstring(totalSeconds / 60) + L"m " +
                    std::to_wstring(totalSeconds % 60) + L"s";
                const std::wstring difficulty = L"LEVEL  " +
                    std::to_wstring(self->m_level) + L"    x" +
                    std::to_wstring(static_cast<int>(self->m_multiplier * 10.0f + 0.5f) / 10) +
                    L"." + std::to_wstring(static_cast<int>(self->m_multiplier * 10.0f + 0.5f) % 10);
                CenteredText(dc, score.c_str(), centerY - S(48), S(58), 32,
                    FW_BOLD, RGB(242, 247, 255), width, scale);
                CenteredText(dc, time.c_str(), centerY + S(12), S(36), 21,
                    FW_NORMAL, RGB(180, 194, 215), width, scale);
                CenteredText(dc, difficulty.c_str(), centerY + S(49), S(34), 18,
                    FW_NORMAL, RGB(180, 194, 215), width, scale);
            }
            CenteredText(dc, L"PRESS ENTER OR R TO RETRY", centerY + S(119), S(44), 24,
                FW_BOLD, RGB(45, 212, 191), width, scale);
            CenteredText(dc, L"PRESS T TO RETURN TO TITLE", centerY + S(164), S(34), 17,
                FW_NORMAL, RGB(180, 194, 215), width, scale);

            EndPaint(hwnd, &ps);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
};

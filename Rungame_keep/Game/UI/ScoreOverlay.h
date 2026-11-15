// ============================================================
// ファイルの役割: スコア・時間・レベルHUDの表示
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <algorithm>
#include "UIScale.h"

// プレイ中のみ表示するHUD。入力フォーカスは親ウィンドウが保持する。
// クラスの責務: スコア・時間・レベルHUDの表示
class ScoreOverlay final {
public:
    bool Initialize(HWND parent) {
        m_parent = parent;
        const HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
        WNDCLASSW wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = inst;
        wc.lpszClassName = L"RunGameScoreOverlayV13";
        if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        m_window = CreateWindowExW(0, wc.lpszClassName, L"", WS_CHILD | WS_CLIPSIBLINGS,
            0, 0, 1, 1, parent, nullptr, inst, this);
        if (!m_window) return false;
        Resize();
        ShowWindow(m_window, SW_HIDE);
        return true;
    }
    // HUDはゲーム画面全体を覆わず、左上の必要な領域だけを使う。
    void Resize() {
        if (!m_window || !m_parent) return;
        RECT rc{};
        GetClientRect(m_parent, &rc);
        const int clientWidth = rc.right - rc.left;
        const int clientHeight = rc.bottom - rc.top;
        if (clientWidth <= 0 || clientHeight <= 0) return;

        const float scale = UIScale::Factor(clientWidth, clientHeight);
        const int margin = UIScale::Px(16, scale);
        const int hudWidth = (std::max)(1, UIScale::Px(390, scale));
        const int hudHeight = (std::max)(1, UIScale::Px(75, scale));
        const int x = (std::min)(margin, clientWidth - 1);
        const int y = (std::min)(margin, clientHeight - 1);
        const int width = (std::min)(hudWidth, clientWidth - x);
        const int height = (std::min)(hudHeight, clientHeight - y);
        SetWindowPos(m_window, HWND_TOP, x, y, width, height,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        InvalidateRect(m_window, nullptr, FALSE);
    }

    void SetVisible(bool visible) {
        if (!m_window) return;
        if (visible) {
            Resize();
            SetWindowPos(m_window, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            InvalidateRect(m_window, nullptr, FALSE);
        }
        else ShowWindow(m_window, SW_HIDE);
    }
    void SetValues(std::uint64_t score, float seconds, int level, float multiplier) {
        // スコアや秒数の表示が変化したときだけ再描画を要求する。
        const int tenths = static_cast<int>(seconds * 10.0f);
        if (score == m_score && tenths == m_tenths && level == m_level && multiplier == m_multiplier) return;
        m_score = score; m_tenths = tenths; m_level = level; m_multiplier = multiplier;
        if (m_window && IsWindowVisible(m_window)) InvalidateRect(m_window, nullptr, FALSE);
    }
private:
    HWND m_parent = nullptr;
    HWND m_window = nullptr;
    std::uint64_t m_score = 0;
    int m_tenths = 0;
    int m_level = 1;
    float m_multiplier = 1.0f;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
        if (message == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        }
        auto* self = reinterpret_cast<ScoreOverlay*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_PAINT) {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{}; GetClientRect(hwnd, &rc);

            RECT parentRect{};
            if (self && self->m_parent) GetClientRect(self->m_parent, &parentRect);
            const float scale = UIScale::Factor(parentRect.right - parentRect.left,
                parentRect.bottom - parentRect.top);
            const auto S = [scale](int value) { return UIScale::Px(value, scale); };
            HBRUSH bg = CreateSolidBrush(RGB(14, 23, 42)); FillRect(dc, &rc, bg); DeleteObject(bg);
            SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(242, 247, 255));
            HFONT font = CreateFontW(-UIScale::Font(21, scale), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HGDIOBJ old = SelectObject(dc, font);
            if (self) {
                const std::wstring score = L"SCORE  " + std::to_wstring(self->m_score);
                const std::wstring info = L"TIME  " + std::to_wstring(self->m_tenths / 10)
                    + L"s     LV " + std::to_wstring(self->m_level)
                    + L"     x" + std::to_wstring(static_cast<int>(self->m_multiplier)) + L"."
                    + std::to_wstring(static_cast<int>(self->m_multiplier * 10.0f) % 10);
                TextOutW(dc, S(12), S(5), score.c_str(), static_cast<int>(score.size()));
                TextOutW(dc, S(12), S(32), info.c_str(), static_cast<int>(info.size()));
            }
            SelectObject(dc, old); DeleteObject(font);
            EndPaint(hwnd, &ps); return 0;
        }
        return DefWindowProcW(hwnd, message, wp, lp);
    }
};

// ============================================================
// ファイルの役割: 旧方式のWin32フェード表示（現在の参照状況はGameAppで確認）
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdio>

// 画面全体を覆う半透明のWin32子ウィンドウ。
// DirectX 12の描画処理には変更を加えず、UIより前面でフェードする。
// クラスの責務: 旧方式のWin32フェード表示（現在の参照状況はGameAppで確認）
class FadeOverlay final {
public:
    bool Initialize(HWND parent)
    {
        m_parent = parent;

        HINSTANCE instance = reinterpret_cast<HINSTANCE>(
            GetWindowLongPtrW(parent, GWLP_HINSTANCE)
            );

        WNDCLASSW wc{};
        wc.lpfnWndProc = &FadeOverlay::WndProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"RunGameFadeOverlayV15";

        if (!RegisterClassW(&wc))
        {
            DWORD error = GetLastError();

            if (error != ERROR_CLASS_ALREADY_EXISTS)
            {
                wchar_t message[256];

                swprintf_s(
                    message,
                    L"RegisterClassW failed. Error: %lu",
                    error
                );

                MessageBoxW(
                    parent,
                    message,
                    L"FadeOverlay Error",
                    MB_OK | MB_ICONERROR
                );

                return false;
            }
        }

        RECT rc{};
        GetClientRect(parent, &rc);

        m_window = CreateWindowExW(
            0,
            wc.lpszClassName,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS,
            0,
            0,
            rc.right - rc.left,
            rc.bottom - rc.top,
            parent,
            nullptr,
            instance,
            this
        );

        if (!m_window)
        {
            DWORD error = GetLastError();

            wchar_t message[256];

            swprintf_s(
                message,
                L"CreateWindowExW failed. Error: %lu",
                error
            );

            MessageBoxW(
                parent,
                message,
                L"FadeOverlay Error",
                MB_OK | MB_ICONERROR
            );

            return false;
        }

        SetLayeredWindowAttributes(
            m_window,
            0,
            0,
            LWA_ALPHA
        );

        ShowWindow(m_window, SW_HIDE);

        return true;
    }

    void Resize() {
        if (!m_parent || !m_window) return;
        RECT rc{};
        GetClientRect(m_parent, &rc);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w > 0 && h > 0) {
            SetWindowPos(m_window, HWND_TOP, 0, 0, w, h,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }

    void SetOpacity(float opacity) {
        if (!m_window) return;
        opacity = (std::max)(0.0f, (std::min)(1.0f, opacity));
        if (opacity <= 0.0f) {
            ShowWindow(m_window, SW_HIDE);
            return;
        }
        const BYTE alpha = static_cast<BYTE>(opacity * 255.0f + 0.5f);
        SetLayeredWindowAttributes(m_window, 0, alpha, LWA_ALPHA);
        SetWindowPos(m_window, HWND_TOP, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

private:
    HWND m_parent = nullptr;
    HWND m_window = nullptr;
    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wp,
        LPARAM lp)
    {
        switch (msg)
        {
        case WM_NCCREATE:
        {
            CREATESTRUCTW* cs =
                reinterpret_cast<CREATESTRUCTW*>(lp);

            SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(
                    cs->lpCreateParams
                    )
            );

            return TRUE;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);

            RECT rc{};
            GetClientRect(hwnd, &rc);

            HBRUSH brush =
                CreateSolidBrush(RGB(7, 12, 24));

            FillRect(dc, &rc, brush);
            DeleteObject(brush);

            EndPaint(hwnd, &ps);

            return 0;
        }
        }

        return DefWindowProcW(
            hwnd,
            msg,
            wp,
            lp
        );
    }
};

/*
 * ClamWin Free Antivirus — CWWindow implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_window.h"
#include <string.h>

CWWindow::CWWindow()
    : m_hwnd(NULL)
    , m_hInst(NULL)
{
    m_hInst = GetModuleHandleA(NULL);
}

CWWindow::~CWWindow()
{
    if (m_hwnd)
    {
        /* Detach us from the HWND before destroy to prevent double-free */
        SetWindowLongPtrA(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

void CWWindow::show(int nCmdShow)
{
    if (m_hwnd) ShowWindow(m_hwnd, nCmdShow);
}

void CWWindow::hide()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void CWWindow::invalidate(bool erase)
{
    if (m_hwnd) InvalidateRect(m_hwnd, NULL, erase ? TRUE : FALSE);
}

/* ─── fill default WNDCLASS fields ─────────────────────────── */

void CWWindow::fillWndClass(WNDCLASSA& wc)
{
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = staticWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = m_hInst;
    wc.hIcon         = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    /* lpszClassName set by create() before registration */
}

/* ─── create ────────────────────────────────────────────────── */

bool CWWindow::create(const std::string& className,
                      const std::string& windowName,
                      DWORD style,
                      DWORD exStyle,
                      HWND  parent,
                      int   x, int y, int w, int h)
{
    /* Register class (idempotent if already registered) */
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    fillWndClass(wc);
    wc.lpszClassName = className.c_str();
    RegisterClassA(&wc);   /* ignore failure if already registered */

    /* Pass `this` via lpCreateParams so WM_CREATE can set GWLP_USERDATA */
    m_hwnd = CreateWindowExA(exStyle,
                              className.c_str(),
                              windowName.c_str(),
                              style,
                              x, y, w, h,
                              parent, NULL, m_hInst,
                              static_cast<LPVOID>(this));
    return (m_hwnd != NULL);
}

/* ─── Default message handler ────────────────────────────────── */

LRESULT CWWindow::onMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
            if (!onCreate())
                return -1;   /* abort CreateWindow */
            return 0;

        case WM_DESTROY:
            onDestroy();
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            onPaint(hdc);
            EndPaint(m_hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            onCommand(LOWORD(wp), (HWND)lp);
            return 0;

        case WM_SIZE:
            onSize(LOWORD(lp), HIWORD(lp));
            return 0;
    }
    return DefWindowProcA(m_hwnd, msg, wp, lp);
}

/* ─── Static WndProc ─────────────────────────────────────────── */

LRESULT CALLBACK CWWindow::staticWndProc(HWND hwnd, UINT msg,
                                          WPARAM wp, LPARAM lp)
{
    CWWindow* self = NULL;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTA* cs = reinterpret_cast<CREATESTRUCTA*>(lp);
        self = static_cast<CWWindow*>(cs->lpCreateParams);
        if (self)
        {
            self->m_hwnd = hwnd;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(self));
        }
    }
    else
    {
        self = reinterpret_cast<CWWindow*>(
                   GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->onMessage(msg, wp, lp);

    return DefWindowProcA(hwnd, msg, wp, lp);
}

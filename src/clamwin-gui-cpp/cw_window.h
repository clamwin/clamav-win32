/*
 * ClamWin Free Antivirus — CWWindow base class
 *
 * Standard Win32 C++ window pattern: static WndProc stores `this`
 * via GWLP_USERDATA, dispatches to virtual handlers.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>
#include <string>

class CWWindow
{
public:
    CWWindow();
    virtual ~CWWindow();

    /* Access underlying HWND */
    HWND hwnd() const { return m_hwnd; }

    void show(int nCmdShow = SW_SHOW);
    void hide();

    /* Repaint the client area */
    void invalidate(bool erase = false);

protected:
    HWND      m_hwnd;
    HINSTANCE m_hInst;

    /* Register window class and create window.
     * className:  Win32 class name (ANSI)
     * windowName: window title (ANSI)
     * style:      WS_* flags
     * ex:         WS_EX_* flags */
    bool create(const std::string& className,
                const std::string& windowName,
                DWORD style,
                DWORD exStyle  = 0,
                HWND  parent   = NULL,
                int   x        = CW_USEDEFAULT,
                int   y        = CW_USEDEFAULT,
                int   w        = CW_USEDEFAULT,
                int   h        = CW_USEDEFAULT);

    /* Override to customize class registration before calling create(). */
    virtual void fillWndClass(WNDCLASSA& wc);

    /* ── Virtual message handlers ─────────────────────────── */

    /* Return false to abort creation. */
    virtual bool onCreate() { return true; }

    virtual void onDestroy()                    {}
    virtual void onPaint(HDC hdc)               { (void)hdc; }
    virtual void onSize(int w, int h)           { (void)w; (void)h; }
    virtual void onCommand(int id, HWND src)    { (void)id; (void)src; }
    virtual LRESULT onMessage(UINT msg, WPARAM wp, LPARAM lp);

private:
    /* No copy */
    CWWindow(const CWWindow&);
    CWWindow& operator=(const CWWindow&);

    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp);
};

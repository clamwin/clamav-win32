/*
 * ClamWin Free Antivirus — CWTray
 *
 * System Tray module encapsulated into a class.
 * Manages the notification area icon, context menu, and balloon tips.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include <windows.h>
#include <shellapi.h>

class CWTray
{
public:
    CWTray();
    ~CWTray();

    bool create(HWND hwnd, HICON hIcon, const char* tooltip);
    void destroy();

    void setIcon(HICON hIcon, const char* tooltip);
    void showBalloon(const char* title, const char* msg, DWORD flags);
    void showContextMenu(bool enableScanReport, bool enableUpdateReport);
    bool isCreated() const { return m_created; }

private:
    /* No copy */
    CWTray(const CWTray&);
    CWTray& operator=(const CWTray&);

    NOTIFYICONDATAA m_nid;
    bool m_created;
    bool m_version4;   /* true if shell accepted NOTIFYICON_VERSION_4 */
};

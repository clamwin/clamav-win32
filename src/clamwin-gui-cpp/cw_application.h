/*
 * ClamWin Free Antivirus — CWApplication
 *
 * Singleton application class: owns config, tray, scheduler.
 * WinMain creates one CWApplication and calls run().
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>
#include "cw_gui_shared.h"   /* CW_Config, IDM_*, WM_TRAYICON */
#include "cw_dashboard.h"
#include "cw_tray.h"
#include "cw_scheduler.h"
#include "cw_update_checker.h"

class CWApplication
{
public:
    CWApplication();
    ~CWApplication();

    /* Run the application. Returns WM_QUIT wParam. */
    int run(HINSTANCE hInst, LPSTR cmdLine);

    /* Access the config (for use by callbacks / dialogs). */
    CWConfig& config() { return m_config; }

    /* Static accessor for use in WndProc callbacks. */
    static CWApplication* instance() { return s_instance; }

    /* Returns tray HWND for use as target for tray command events. */
    HWND getTrayHwnd() const { return m_hwndTray; }

private:
    /* No copy */
    CWApplication(const CWApplication&);
    CWApplication& operator=(const CWApplication&);

    static CWApplication* s_instance;

    HINSTANCE    m_hInst;
    HWND         m_hwndTray;    /* hidden message-only window */
    CWDashboard  m_dash;        /* persistent dashboard window */
    HICON        m_hIcon;
    UINT         m_taskbarCreatedMsg;
    CWTray       m_tray;
    CWScheduler  m_scheduler;
    CWConfig     m_config;
    CWUpdateChecker m_updateChecker;

    /* Returns dashboard HWND if created, else tray HWND. */
    HWND dialogParent() const;
    /* Hide dashboard before modal, restore it after. */
    void hideDashForModal();
    void restoreDash();
    HWND activeDash() const { return m_dash.hwnd(); }

    /* Registration / creation */
    bool registerHiddenClass();
    bool createHiddenWindow();

    /* Tray and menu actions */
    void createTray();
    void destroyTray();
    void updateTrayTip();
    void showContextMenu();
    void showReportsMenu();

    void doScan();
    void doScanMemory();
    void doUpdate();
    void doScheduledScan();
    void doScheduledUpdate();
    void doPreferences();
    void doSchedule();
    void doScanReport();
    void doUpdateReport();
    void doOpenDashboard();
    void doHelp();
    void onVersionCheckResult(WPARAM wp, LPARAM lp);

    /* Balloon tip helper — respects trayNotify config */
    void showBalloonNotify(const char* msg, DWORD flags);

    static LRESULT CALLBACK staticWndProc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp);
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
};

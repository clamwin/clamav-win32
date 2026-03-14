/*
 * ClamWin Free Antivirus — CWDashboard
 *
 * Main dashboard window: status banner + action cards.
 * Inherits CWWindow for Win32 message dispatch.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_window.h"
#include "cw_gui_shared.h"   /* CW_Config, IDC_CARD_*, IDM_*, CW_DBInfo, CW_ProtectionStatus */
#include <vector>

class CWDashboard : public CWWindow
{
public:
    explicit CWDashboard(CWConfig& config);
    virtual ~CWDashboard();

    /* Create and show the window. Returns false on failure. */
    bool open(HWND parent = NULL);

    /* Refresh status banner (call after update completes). */
    void refreshStatus();

    /* Set when a newer ClamWin version is available (from CWUpdateChecker). */
    void setUpdateAvailable(const char* versionStr);

protected:
    /* CWWindow overrides */
    virtual bool onCreate();
    virtual void onPaint(HDC hdc);
    virtual void onCommand(int id, HWND src);
    virtual LRESULT onMessage(UINT msg, WPARAM wp, LPARAM lp);
    virtual void fillWndClass(WNDCLASSA& wc);

private:
    struct CardInfo {
        int          id;
        const char*  title;
        const char*  desc;
    };

    static const CardInfo s_cards[];
    static const int      s_cardCount;

    CWConfig&           m_config;
    CW_ProtectionStatus m_status;
    CW_DBInfo           m_dbInfo;
    int                 m_hoverCard;   /* -1 = none */
    bool                m_showMnemonics;
    HWND                m_hwndTooltip;
    bool                m_updateAvailable;
    bool                m_updateLayoutAdjusted;
    char                m_newVersion[64];

    /* Fonts — owned, cleaned up in onDestroy */
    HFONT m_fontTitle;
    HFONT m_fontDesc;
    HFONT m_fontBanner;
    HFONT m_fontBannerSub;
    HFONT m_fontStatus;

    void createFonts();
    void destroyFonts();
    void getCardRect(int index, const RECT& client, RECT& out) const;
    void paintBanner(HDC hdc, const RECT& client);
    void paintCards(HDC hdc, const RECT& client);
    void paintStatusBar(HDC hdc, const RECT& client);
    void paintUpdateBanner(HDC hdc, const RECT& client);
    int  cardAtPoint(POINT pt) const;
    void initCardTooltips();
    void updateCardTooltipRects();
    void destroyCardTooltips();
    void postTrayCommand(int menuId);
};

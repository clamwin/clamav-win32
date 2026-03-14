/*
 * ClamWin Free Antivirus — CWAboutDialog
 *
 * Modern About dialog with logo panels and theme support.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_dialog.h"
#include "cw_gui_shared.h"
#include "cw_config.h"

class CWAboutDialog : public CWDialog
{
public:
    explicit CWAboutDialog(CWConfig& cfg);
    virtual ~CWAboutDialog();

protected:
    virtual bool onInit() override;
    virtual bool onCommand(int id, HWND src) override;
    virtual INT_PTR handleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    CWConfig& m_cfg;
    HFONT     m_hFontTitle;
    HFONT     m_hFontSection;
    HFONT     m_hFontNormal;
    HFONT     m_hFontSmall;
    HFONT     m_hFontLink;
    HWND      m_hwndBtnOk;
    HWND      m_hwndLinkWeb;
    HWND      m_hwndLinkClamAV;
    HWND      m_hwndLinkNetfarm;
    void*     m_pImgClamAV;
    void*     m_pImgClamWin;
    void*     m_pImgNetfarm;

    void createFonts();
    void destroyFonts();
    void destroyImages();
    void* loadPngResource(int resourceId);
    HWND addLabel(const char* text, int x, int y, int w, int h, HFONT font, int id = 0);
    HWND addLink(const char* text, int x, int y, int w, int h, HFONT font, int id);
    HWND addBitmap(int id, int x, int y, int w, int h);
};

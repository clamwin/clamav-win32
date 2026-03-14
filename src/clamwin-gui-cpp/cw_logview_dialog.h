/*
 * ClamWin Free Antivirus — CWLogViewDialog
 *
 * Log viewer dialog replicating wxDialogLogView.py
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_dialog.h"

class CWLogViewDialog : public CWDialog
{
public:
    CWLogViewDialog(const char* logfile, const char* title);
    virtual ~CWLogViewDialog();

protected:
    virtual bool onInit() override;
    virtual bool onCommand(int id, HWND src) override;
    virtual INT_PTR handleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    friend LRESULT CALLBACK CW_LogViewScrollbarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    void layoutControls(int w, int h);
    void refreshCustomScrollbars();
    void paintCustomScrollbar(HWND hwnd, HDC hdc);
    void onCustomScrollbarMouseDown(HWND hwnd, int x, int y);
    void onCustomScrollbarMouseMove(HWND hwnd, int x, int y);
    void onCustomScrollbarMouseUp(HWND hwnd);
    int  computeMaxLineWidthPx(const char* text) const;
    int  lineHeightPx() const;
    void setTopVisibleLine(int topLine);
    void setHorizontalScrollPx(int x);
    
    const char* m_logfile;
    const char* m_title;
    
    HWND m_hwndText;
    HWND m_hwndBtnOk;
    HWND m_hwndVScroll;
    HWND m_hwndHScroll;
    HFONT m_hFont;
    bool m_flatScrollbarInit;
    bool m_useCustomScrollbars;
    bool m_dragV;
    bool m_dragH;
    int  m_dragOffset;
    int  m_maxLineWidthPx;
};

/*
 * ClamWin Free Antivirus — CWScheduleDialog
 *
 * Full implementation of wxDialogScheduledScan.py.
 * Edits the single scheduled-scan entry stored in CWConfig.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_dialog.h"
#include "cw_config.h"

class CWScheduleDialog : public CWDialog
{
public:
    explicit CWScheduleDialog(CWConfig& cfg);
    virtual ~CWScheduleDialog();

protected:
    virtual bool     onInit() override;
    virtual bool     onCommand(int id, HWND src) override;
    virtual INT_PTR  handleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    CWConfig& m_cfg;

    /* Controls */
    HWND m_cmbFrequency;
    HWND m_edtHour;
    HWND m_spinHour;
    HWND m_edtMinute;
    HWND m_spinMinute;
    HWND m_cmbDay;
    HWND m_chkActive;
    HWND m_chkScanMemory;
    HWND m_chkRunMissed;
    HWND m_edtFolder;
    HWND m_btnBrowse;
    HWND m_edtDescription;
    HWND m_btnOk;
    HWND m_btnCancel;
    HWND m_hwndTooltip;

    HFONT m_hFont;
    HFONT m_hFontBold;
    bool  m_showMnemonics;

    void setFont(HWND hwnd, bool bold = false);
    void createLayout();
    void createTooltips();
    void destroyTooltips();
    void addTooltip(HWND target, const char* text);
    void loadFromConfig();
    void saveToConfig();
    void browseForFolder();
    void updateDayEnableState();
    void configureComboHeight(HWND combo);
    void configureEditRect(HWND edit);
    bool drawComboItem(DRAWITEMSTRUCT* dis);
};

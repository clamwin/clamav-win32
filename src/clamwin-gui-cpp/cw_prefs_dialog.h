/*
 * ClamWin Free Antivirus — CWPrefsDialog
 *
 * References wxDialogPreferences.py from legacy codebase.
 * Full layout with 10 tabs will be implemented in Phase 3.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_dialog.h"
#include "cw_config.h"

class CWPrefsDialog : public CWDialog
{
public:
    explicit CWPrefsDialog(CWConfig& cfg);
    virtual ~CWPrefsDialog();

protected:
    virtual bool onInit() override;
    virtual bool onCommand(int id, HWND src) override;
    virtual INT_PTR handleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    enum { PAGE_COUNT = 8 };

    enum PageIndex {
        PAGE_GENERAL = 0,
        PAGE_FILTERS,
        PAGE_UPDATES,
        PAGE_PROXY,
        PAGE_SCHEDULE,
        PAGE_LIMITS,
        PAGE_FILES,
        PAGE_ADVANCED
    };

    CWConfig& m_cfg;

    HFONT m_hFont;
    HFONT m_hFontBold;

    HWND m_hwndSidebar;
    WNDPROC m_sidebarOldProc;
    HWND m_hwndPages[PAGE_COUNT];
    WNDPROC m_pageOldProc[PAGE_COUNT];
    HWND m_hwndBtnOk;
    HWND m_hwndBtnCancel;
    HWND m_hwndTooltip;
    int  m_activePage;
    bool m_showMnemonics;

    /* Filters */
    HWND m_edtExclPattern;
    HWND m_lstExclude;
    HWND m_btnExclAdd;
    HWND m_btnExclRemove;
    HWND m_edtInclPattern;
    HWND m_lstInclude;
    HWND m_btnInclAdd;
    HWND m_btnInclRemove;

    /* General */
    HWND m_chkRecursive;
    HWND m_chkScanMail;
    HWND m_radActionReport;
    HWND m_radActionRemove;
    HWND m_radActionQuarantine;
    HWND m_edtQuarantine;
    HWND m_btnQuarantineBrowse;

    /* Updates */
    HWND m_chkUpdateScheduled;
    HWND m_edtMirror;
    HWND m_cmbUpdateFrequency;
    HWND m_edtUpdateHour;
    HWND m_spinUpdateHour;
    HWND m_edtUpdateMinute;
    HWND m_spinUpdateMinute;
    HWND m_cmbUpdateDay;
    HWND m_chkUpdateOnStartup;
    HWND m_chkCheckVersion;

    /* Proxy */
    HWND m_chkProxyEnabled;
    HWND m_edtProxyHost;
    HWND m_edtProxyPort;
    HWND m_edtProxyUser;
    HWND m_edtProxyPass;

    /* Schedule */
    HWND m_chkScanScheduled;
    HWND m_cmbScanFrequency;
    HWND m_edtScanHour;
    HWND m_spinScanHour;
    HWND m_edtScanMinute;
    HWND m_spinScanMinute;
    HWND m_cmbScanDay;
    HWND m_btnScheduleDetails;

    /* Limits */
    HWND m_chkScanArchives;
    HWND m_edtMaxScanSize;
    HWND m_edtMaxFileSize;
    HWND m_edtMaxFiles;
    HWND m_edtMaxDepth;
    HWND m_spinMaxScanSize;
    HWND m_spinMaxFileSize;
    HWND m_spinMaxFiles;
    HWND m_spinMaxDepth;

    /* Files */
    HWND m_edtDatabasePath;
    HWND m_btnDatabaseBrowse;
    HWND m_edtScanLog;
    HWND m_btnScanLogBrowse;
    HWND m_edtUpdateLog;
    HWND m_btnUpdateLogBrowse;

    /* Advanced */
    HWND m_chkScanOle2;
    HWND m_cmbPriority;

    void createFonts();
    void createLayout();
    void createTooltips();
    void destroyTooltips();
    void addTooltip(HWND target, const char* text);
    void createSidebar();
    void createPages();
    void createGeneralPage(HWND page);
    void createFiltersPage(HWND page);
    void createUpdatesPage(HWND page);
    void createProxyPage(HWND page);
    void createSchedulePage(HWND page);
    void createLimitsPage(HWND page);
    void createFilesPage(HWND page);
    void createAdvancedPage(HWND page);

    void showPage(int index);
    void setControlFont(HWND hwnd, bool bold = false);
    void updateEnableStates();
    void loadFromConfig();
    bool saveToConfig();

    void browseForFolder(HWND editTarget, const char* title);
    void browseForFile(HWND editTarget, const char* title, const char* filter, bool saveDialog);
    int readIntFromEdit(HWND hwndEdit, int fallback) const;
    void writeIntToEdit(HWND hwndEdit, int value) const;
    void moveLabeledEditRow(HWND label, HWND edit, HWND btn, int y, int labelW, int editX, int editW, int btnW);
    bool drawSidebarItem(DRAWITEMSTRUCT* dis);
    bool drawMnemonicStaticItem(DRAWITEMSTRUCT* dis);
    bool drawToggleItem(DRAWITEMSTRUCT* dis);
    bool drawComboItem(DRAWITEMSTRUCT* dis);

    static LRESULT CALLBACK pageSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK sidebarSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    int findPageIndex(HWND hwnd) const;
};

/*
 * ClamWin Free Antivirus — CWScheduleDialog
 *
 * Full implementation of wxDialogScheduledScan.py.
 * Google Drive–style flat design, matches CWPrefsDialog aesthetic.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_schedule_dialog.h"
#include "cw_toggle.h"
#include "cw_time_edit.h"
#include "cw_mnemonic.h"
#include "cw_dpi.h"
#include <shlobj.h>
#include <commctrl.h>
#include <cstdlib>

/* ─── Local control IDs ───────────────────────────────────────── */
enum
{
    IDC_SCHED_FREQ        = 3100,
    IDC_SCHED_HOUR,
    IDC_SCHED_MINUTE,
    IDC_SCHED_DAY,
    IDC_SCHED_ACTIVE,
    IDC_SCHED_SCANMEM,
    IDC_SCHED_RUNMISSED,
    IDC_SCHED_SPIN_HOUR,
    IDC_SCHED_SPIN_MIN,
    IDC_SCHED_FOLDER,
    IDC_SCHED_BROWSE,
    IDC_SCHED_DESC,
};

static const UINT_PTR s_scheduleMnemonicTimerId = 0xCA12;

/* ─── Constructor / Destructor ────────────────────────────────── */

CWScheduleDialog::CWScheduleDialog(CWConfig& cfg)
    : m_cfg(cfg)
    , m_cmbFrequency(NULL)
    , m_edtHour(NULL)
    , m_spinHour(NULL)
    , m_edtMinute(NULL)
    , m_spinMinute(NULL)
    , m_cmbDay(NULL)
    , m_chkActive(NULL)
    , m_chkScanMemory(NULL)
    , m_chkRunMissed(NULL)
    , m_edtFolder(NULL)
    , m_btnBrowse(NULL)
    , m_edtDescription(NULL)
    , m_btnOk(NULL)
    , m_btnCancel(NULL)
    , m_hwndTooltip(NULL)
    , m_hFont(NULL)
    , m_hFontBold(NULL)
    , m_showMnemonics(false)
{
}

CWScheduleDialog::~CWScheduleDialog()
{
    destroyTooltips();
    if (m_hFont)     DeleteObject(m_hFont);
    if (m_hFontBold) DeleteObject(m_hFontBold);
}

void CWScheduleDialog::addTooltip(HWND target, const char* text)
{
    if (!m_hwndTooltip || !target || !text || !text[0])
        return;

    TOOLINFOA ti;
    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    ti.hwnd = m_hwnd;
    ti.uId = (UINT_PTR)target;
    ti.lpszText = const_cast<LPSTR>(text);
    SendMessageA(m_hwndTooltip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

void CWScheduleDialog::createTooltips()
{
    m_hwndTooltip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, "",
                                    WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    m_hwnd, NULL, GetModuleHandleA(NULL), NULL);
    if (!m_hwndTooltip)
        return;

    SetWindowPos(m_hwndTooltip, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SendMessageA(m_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, CW_Scale(360));

    addTooltip(m_cmbFrequency, "How often the schedule is executed");
    addTooltip(m_edtHour, "When the schedule should be started");
    addTooltip(m_edtMinute, "When the schedule should be started");
    addTooltip(m_spinHour, "When the schedule should be started");
    addTooltip(m_spinMinute, "When the schedule should be started");
    addTooltip(m_cmbDay, "When schedule frequency is weekly select day of the week");
    addTooltip(m_edtFolder, "Specify a folder to be scanned");
    addTooltip(m_btnBrowse, "Click to browse for a folder");
    addTooltip(m_edtDescription, "Specify a friendly description for the scheduled scan");
    addTooltip(m_chkActive, "Select if you wish to enable this schedule");
    addTooltip(m_chkScanMemory, "Select if you wish to include programs computer memory during every scan");
    addTooltip(m_chkRunMissed, "Select if you wish to run the scheduled task if the computer was turned off at the time");
    addTooltip(m_btnOk, "Closes the dialog and applies the settings");
    addTooltip(m_btnCancel, "Closes the dialog and discards the changes");
}

void CWScheduleDialog::destroyTooltips()
{
    if (m_hwndTooltip)
    {
        DestroyWindow(m_hwndTooltip);
        m_hwndTooltip = NULL;
    }
}

/* ─── Font helpers ────────────────────────────────────────────── */

void CWScheduleDialog::setFont(HWND hwnd, bool bold)
{
    if (!hwnd) return;
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)(bold ? m_hFontBold : m_hFont), TRUE);
    /* Apply EM_SETRECTNP centering on EDIT controls */
    configureEditRect(hwnd);
}

void CWScheduleDialog::configureEditRect(HWND edit)
{
    char cls[32] = {0};
    GetClassNameA(edit, cls, sizeof(cls));
    if (lstrcmpiA(cls, "EDIT") != 0)
        return;

    RECT rc;
    GetClientRect(edit, &rc);

    HFONT hf = (HFONT)SendMessageA(edit, WM_GETFONT, 0, 0);
    HDC hdc = GetDC(edit);
    if (!hdc) return;
    HGDIOBJ old = hf ? SelectObject(hdc, hf) : NULL;
    TEXTMETRICA tm = {0};
    GetTextMetricsA(hdc, &tm);
    if (old) SelectObject(hdc, old);
    ReleaseDC(edit, hdc);

    int textH = tm.tmHeight > 0 ? tm.tmHeight : CW_Scale(13);
    int topPad = ((rc.bottom - rc.top) - textH) / 2;
    if (topPad < 1) topPad = 1;

    RECT textRc;
    textRc.left   = CW_Scale(8);
    textRc.right  = rc.right - CW_Scale(8);
    textRc.top    = topPad;
    textRc.bottom = topPad + textH + 1;
    SendMessageA(edit, EM_SETRECTNP, 0, (LPARAM)&textRc);
    InvalidateRect(edit, NULL, TRUE);
}

void CWScheduleDialog::configureComboHeight(HWND combo)
{
    if (!combo) return;
    SendMessageA(combo, CB_SETITEMHEIGHT, (WPARAM)-1, CW_Scale(26));
    SendMessageA(combo, CB_SETITEMHEIGHT, (WPARAM)0,  CW_Scale(26));
}

/* ─── Layout ──────────────────────────────────────────────────── */

void CWScheduleDialog::createLayout()
{
    HMODULE hInst = GetModuleHandleA(NULL);

    m_hFont = CreateFontW(-CW_Scale(13), 0, 0, 0, FW_NORMAL,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    m_hFontBold = CreateFontW(-CW_Scale(13), 0, 0, 0, FW_SEMIBOLD,
                              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    if (!m_hFont)     m_hFont     = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!m_hFontBold) m_hFontBold = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    const int x0       = CW_Scale(16);
    const int colLabel = CW_Scale(150);
    const int colCtrl  = x0 + colLabel + CW_Scale(8);
    const int ctrlW    = CW_Scale(180);
    const int rowH     = CW_Scale(30);
    const int rowStep  = CW_Scale(42);
    const int btnH     = CW_Scale(30);

    int y = CW_Scale(16);

    /* ── "Schedule" section label ── */
    HWND hSecSched = CreateWindowExA(0, "STATIC", "Schedule",
                                     WS_CHILD | WS_VISIBLE,
                                     x0, y, CW_Scale(300), CW_Scale(20),
                                     m_hwnd, NULL, hInst, NULL);
    setFont(hSecSched, true);
    y += CW_Scale(28);

    /* Frequency */
    HWND hLblFreq = CreateWindowExA(0, "STATIC", "Scanning Frequency:",
                                    WS_CHILD | WS_VISIBLE,
                                    x0, y + CW_Scale(4), colLabel, CW_Scale(20),
                                    m_hwnd, NULL, hInst, NULL);
    setFont(hLblFreq);

    m_cmbFrequency = CreateWindowExA(0, "COMBOBOX", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                     CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED |
                                     CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                     colCtrl, y, ctrlW, CW_Scale(220),
                                     m_hwnd, (HMENU)IDC_SCHED_FREQ, hInst, NULL);
    setFont(m_cmbFrequency);
    configureComboHeight(m_cmbFrequency);
    static const char* freqs[] = { "Daily", "Workdays", "Weekly", "Hourly" };
    for (int i = 0; i < 4; ++i)
        SendMessageA(m_cmbFrequency, CB_ADDSTRING, 0, (LPARAM)freqs[i]);
    y += rowStep;

    /* Day of week */
    HWND hLblDay = CreateWindowExA(0, "STATIC", "Day of Week:",
                                   WS_CHILD | WS_VISIBLE,
                                   x0, y + CW_Scale(4), colLabel, CW_Scale(20),
                                   m_hwnd, NULL, hInst, NULL);
    setFont(hLblDay);

    m_cmbDay = CreateWindowExA(0, "COMBOBOX", "",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                               CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED |
                               CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                               colCtrl, y, ctrlW, CW_Scale(260),
                               m_hwnd, (HMENU)IDC_SCHED_DAY, hInst, NULL);
    setFont(m_cmbDay);
    configureComboHeight(m_cmbDay);
    static const char* days[] = { "Monday","Tuesday","Wednesday",
                                  "Thursday","Friday","Saturday","Sunday" };
    for (int i = 0; i < 7; ++i)
        SendMessageA(m_cmbDay, CB_ADDSTRING, 0, (LPARAM)days[i]);
    y += rowStep;

    /* Time */
    HWND hLblTime = CreateWindowExA(0, "STATIC", "Time (HH:MM):",
                                    WS_CHILD | WS_VISIBLE,
                                    x0, y + CW_Scale(4), colLabel, CW_Scale(20),
                                    m_hwnd, NULL, hInst, NULL);
    setFont(hLblTime);

    m_edtHour = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                ES_AUTOHSCROLL,
                                colCtrl, y, CW_Scale(84), rowH,
                                m_hwnd, (HMENU)IDC_SCHED_HOUR, hInst, NULL);
    setFont(m_edtHour);

    m_spinHour = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                 WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                 colCtrl + CW_Scale(86), y, CW_Scale(18), rowH,
                                 m_hwnd, (HMENU)IDC_SCHED_SPIN_HOUR, hInst, NULL);
    SendMessageA(m_spinHour, UDM_SETRANGE, 0, MAKELONG(23, 0));

    HWND hColon = CreateWindowExA(0, "STATIC", ":",
                                  WS_CHILD | WS_VISIBLE | SS_CENTER,
                                  colCtrl + CW_Scale(107), y + CW_Scale(4),
                                  CW_Scale(12), CW_Scale(20),
                                  m_hwnd, NULL, hInst, NULL);
    setFont(hColon);

    m_edtMinute = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                  ES_NUMBER | ES_AUTOHSCROLL,
                                  colCtrl + CW_Scale(122), y, CW_Scale(38), rowH,
                                  m_hwnd, (HMENU)IDC_SCHED_MINUTE, hInst, NULL);
    setFont(m_edtMinute);

    m_spinMinute = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                   WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                   colCtrl + CW_Scale(162), y, CW_Scale(18), rowH,
                                   m_hwnd, (HMENU)IDC_SCHED_SPIN_MIN, hInst, NULL);
    SendMessageA(m_spinMinute, UDM_SETRANGE, 0, MAKELONG(59, 0));
    y += rowStep + CW_Scale(6);

    /* Divider (thin static line) */
    HWND hDiv = CreateWindowExA(0, "STATIC", "",
                                WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                                x0, y, CW_Scale(368), CW_Scale(2),
                                m_hwnd, NULL, hInst, NULL);
    (void)hDiv;
    y += CW_Scale(12);

    /* Checkboxes */
    m_chkActive = CreateWindowExA(0, "BUTTON", "&Activate This Schedule",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                  x0, y, CW_Scale(320), CW_Scale(24),
                                  m_hwnd, (HMENU)IDC_SCHED_ACTIVE, hInst, NULL);
    setFont(m_chkActive);
    y += CW_Scale(32);

    m_chkScanMemory = CreateWindowExA(0, "BUTTON", "Scan Programs Loaded in &Memory",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                      x0, y, CW_Scale(320), CW_Scale(24),
                                      m_hwnd, (HMENU)IDC_SCHED_SCANMEM, hInst, NULL);
    setFont(m_chkScanMemory);
    y += CW_Scale(32);

    m_chkRunMissed = CreateWindowExA(0, "BUTTON", "&Catch up on missed tasks",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                     x0, y, CW_Scale(320), CW_Scale(24),
                                     m_hwnd, (HMENU)IDC_SCHED_RUNMISSED, hInst, NULL);
    setFont(m_chkRunMissed);
    y += CW_Scale(36);

    /* Scan Folder */
    HWND hLblFolder = CreateWindowExA(0, "STATIC", "Scan Folder:",
                                      WS_CHILD | WS_VISIBLE,
                                      x0, y, CW_Scale(200), CW_Scale(20),
                                      m_hwnd, NULL, hInst, NULL);
    setFont(hLblFolder);
    y += CW_Scale(24);

    m_edtFolder = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                  ES_AUTOHSCROLL,
                                  x0, y, CW_Scale(282), rowH,
                                  m_hwnd, (HMENU)IDC_SCHED_FOLDER, hInst, NULL);
    setFont(m_edtFolder);

    m_btnBrowse = CreateWindowExA(0, "BUTTON", "&Browse...",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  x0 + CW_Scale(290), y, CW_Scale(94), rowH,
                                  m_hwnd, (HMENU)IDC_SCHED_BROWSE, hInst, NULL);
    setFont(m_btnBrowse);
    y += rowStep;

    /* Description */
    HWND hLblDesc = CreateWindowExA(0, "STATIC", "Description:",
                                    WS_CHILD | WS_VISIBLE,
                                    x0, y, CW_Scale(200), CW_Scale(20),
                                    m_hwnd, NULL, hInst, NULL);
    setFont(hLblDesc);
    y += CW_Scale(24);

    m_edtDescription = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                       ES_AUTOHSCROLL,
                                       x0, y, CW_Scale(384), rowH,
                                       m_hwnd, (HMENU)IDC_SCHED_DESC, hInst, NULL);
    setFont(m_edtDescription);
    y += rowStep + CW_Scale(4);

    /* OK / Cancel */
    RECT dlgRc;
    GetClientRect(m_hwnd, &dlgRc);
    int btnW  = CW_Scale(90);
    int btnX2 = dlgRc.right - CW_Scale(16) - btnW;
    int btnX1 = btnX2 - CW_Scale(8) - btnW;

    m_btnOk = CreateWindowExA(0, "BUTTON", "&OK",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                              btnX1, y, btnW, btnH,
                              m_hwnd, (HMENU)IDOK, hInst, NULL);
    setFont(m_btnOk);

    m_btnCancel = CreateWindowExA(0, "BUTTON", "&Cancel",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  btnX2, y, btnW, btnH,
                                  m_hwnd, (HMENU)IDCANCEL, hInst, NULL);
    setFont(m_btnCancel);
}

/* ─── Load / Save ────────────────────────────────────────────── */

static void writeIntToEditSched(HWND edit, int value)
{
    if (!edit) return;
    char buf[16];
    wsprintfA(buf, "%02d", value);
    SetWindowTextA(edit, buf);
}

void CWScheduleDialog::loadFromConfig()
{
    SendMessageA(m_cmbFrequency, CB_SETCURSEL,
                 (WPARAM)m_cfg.scanFrequency, 0);
    CW_WriteHourToEdit(m_edtHour,   m_cfg.scanHour);
    writeIntToEditSched(m_edtMinute, m_cfg.scanMinute);
    SendMessageA(m_cmbDay, CB_SETCURSEL, (WPARAM)m_cfg.scanDay, 0);
    CW_ToggleSetChecked(m_chkActive,     m_cfg.scanScheduled);
    CW_ToggleSetChecked(m_chkScanMemory, m_cfg.scanMemory);
    CW_ToggleSetChecked(m_chkRunMissed,  m_cfg.scanRunMissed);
    SetWindowTextA(m_edtFolder,      m_cfg.scanPath.c_str());
    SetWindowTextA(m_edtDescription, m_cfg.scanDescription.c_str());
    updateDayEnableState();
}

static int readIntFromEditSched(HWND edit, int fallback)
{
    if (!edit) return fallback;
    char buf[16] = {0};
    GetWindowTextA(edit, buf, sizeof(buf));
    int val = fallback;
    if (buf[0]) val = atoi(buf);
    return val;
}

void CWScheduleDialog::saveToConfig()
{
    int freq = (int)SendMessageA(m_cmbFrequency, CB_GETCURSEL, 0, 0);
    if (freq < 0) freq = 0;
    m_cfg.scanFrequency = freq;

    int h = CW_ReadHourFromEdit(m_edtHour, 0);
    int m = readIntFromEditSched(m_edtMinute, 0);
    if (h < 0) h = 0; if (h > 23) h = 23;
    if (m < 0) m = 0; if (m > 59) m = 59;
    m_cfg.scanHour   = h;
    m_cfg.scanMinute = m;

    int day = (int)SendMessageA(m_cmbDay, CB_GETCURSEL, 0, 0);
    if (day < 0) day = 0;
    m_cfg.scanDay = day;

    m_cfg.scanScheduled  = CW_ToggleGetChecked(m_chkActive);
    m_cfg.scanMemory     = CW_ToggleGetChecked(m_chkScanMemory);
    m_cfg.scanRunMissed  = CW_ToggleGetChecked(m_chkRunMissed);

    char buf[MAX_PATH] = {0};
    GetWindowTextA(m_edtFolder, buf, MAX_PATH);
    m_cfg.scanPath = buf;

    char desc[256] = {0};
    GetWindowTextA(m_edtDescription, desc, sizeof(desc));
    m_cfg.scanDescription = desc;
}

/* ─── Browse ─────────────────────────────────────────────────── */

void CWScheduleDialog::browseForFolder()
{
    char path[MAX_PATH] = {0};
    GetWindowTextA(m_edtFolder, path, MAX_PATH);

    BROWSEINFOA bi = {0};
    bi.hwndOwner  = m_hwnd;
    bi.pszDisplayName = path;
    bi.lpszTitle  = "Select folder to scan";
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl)
    {
        if (SHGetPathFromIDListA(pidl, path))
            SetWindowTextA(m_edtFolder, path);
        CoTaskMemFree(pidl);
    }
}

/* ─── Enable state ───────────────────────────────────────────── */

void CWScheduleDialog::updateDayEnableState()
{
    /* Day-of-week combo is only meaningful for Weekly frequency (index 2) */
    int freq = (int)SendMessageA(m_cmbFrequency, CB_GETCURSEL, 0, 0);
    EnableWindow(m_cmbDay, freq == 2 ? TRUE : FALSE);
}

/* ─── Combo owner-draw ───────────────────────────────────────── */

bool CWScheduleDialog::drawComboItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlType != ODT_COMBOBOX) return false;
    if (dis->CtlID != IDC_SCHED_FREQ && dis->CtlID != IDC_SCHED_DAY)
        return false;

    CWTheme* theme = CW_GetTheme();
    if (!theme) return false;

    HDC   hdc = dis->hDC;
    RECT  rc  = dis->rcItem;

    UINT itemId = dis->itemID;
    if (itemId == (UINT)-1)
    {
        int sel = (int)SendMessageA(dis->hwndItem, CB_GETCURSEL, 0, 0);
        if (sel >= 0) itemId = (UINT)sel;
    }

    bool disabled  = (dis->itemState & ODS_DISABLED) != 0;
    bool selected  = (dis->itemState & ODS_SELECTED)  != 0;

    COLORREF bg = selected ? theme->colorAccent() : theme->colorSurface();
    COLORREF fg = selected  ? RGB(255,255,255)
                : disabled  ? theme->colorTextMuted()
                :             theme->colorText();

    HBRUSH hbr = CreateSolidBrush(bg);
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);

    if (itemId != (UINT)-1)
    {
        char text[128] = {0};
        SendMessageA(dis->hwndItem, CB_GETLBTEXT, (WPARAM)itemId, (LPARAM)text);

        RECT tr = rc;
        tr.left  += CW_Scale(8);
        tr.right -= CW_Scale(8);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, fg);
        HFONT oldFont = (HFONT)SelectObject(hdc, m_hFont);
        DrawTextA(hdc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    if (dis->itemState & ODS_FOCUS)
    {
        RECT fr = rc;
        InflateRect(&fr, -CW_Scale(1), -CW_Scale(1));
        DrawFocusRect(hdc, &fr);
    }
    return true;
}

/* ─── onInit ─────────────────────────────────────────────────── */

bool CWScheduleDialog::onInit()
{
    SetWindowTextA(m_hwnd, "Scheduled Scan");
    setDialogMnemonicCues(m_hwnd, false);
    SetTimer(m_hwnd, s_scheduleMnemonicTimerId, 30, NULL);
    createLayout();
    createTooltips();
    loadFromConfig();
    if (m_edtFolder) SetFocus(m_edtFolder);
    return false;   /* we set focus manually */
}

/* ─── onCommand ──────────────────────────────────────────────── */

bool CWScheduleDialog::onCommand(int id, HWND /*src*/)
{
    switch (id)
    {
        case IDC_SCHED_ACTIVE:
            CW_ToggleSetChecked(m_chkActive, !CW_ToggleGetChecked(m_chkActive));
            InvalidateRect(m_chkActive, NULL, TRUE);
            return true;

        case IDC_SCHED_SCANMEM:
            CW_ToggleSetChecked(m_chkScanMemory, !CW_ToggleGetChecked(m_chkScanMemory));
            InvalidateRect(m_chkScanMemory, NULL, TRUE);
            return true;

        case IDC_SCHED_RUNMISSED:
            CW_ToggleSetChecked(m_chkRunMissed, !CW_ToggleGetChecked(m_chkRunMissed));
            InvalidateRect(m_chkRunMissed, NULL, TRUE);
            return true;

        case IDC_SCHED_FREQ:
            updateDayEnableState();
            return true;

        case IDC_SCHED_BROWSE:
            browseForFolder();
            return true;

        case IDOK:
            saveToConfig();
            endDialog(IDOK);
            return true;

        case IDCANCEL:
            endDialog(IDCANCEL);
            return true;
    }
    return false;
}

/* ─── handleMessage ──────────────────────────────────────────── */

INT_PTR CWScheduleDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_NOTIFY:
        {
            NMHDR* hdr = (NMHDR*)lp;
            if (hdr && hdr->code == UDN_DELTAPOS)
            {
                NMUPDOWN* nmud = (NMUPDOWN*)lp;
                int ctlId = (int)hdr->idFrom;
                if (ctlId == IDC_SCHED_SPIN_HOUR)
                {
                    int h = CW_ReadHourFromEdit(m_edtHour, 0);
                    h = (h + nmud->iDelta + 24) % 24;
                    CW_WriteHourToEdit(m_edtHour, h);
                    return 0;
                }
                if (ctlId == IDC_SCHED_SPIN_MIN)
                {
                    char mbuf[8] = {0};
                    GetWindowTextA(m_edtMinute, mbuf, sizeof(mbuf));
                    int m = atoi(mbuf);
                    m = (m + nmud->iDelta + 60) % 60;
                    CW_WriteMinuteToEdit(m_edtMinute, m);
                    return 0;
                }
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
            if (drawComboItem(dis))
                return TRUE;
            if (dis && dis->CtlType == ODT_BUTTON &&
                (dis->CtlID == IDC_SCHED_ACTIVE || dis->CtlID == IDC_SCHED_SCANMEM || dis->CtlID == IDC_SCHED_RUNMISSED))
            {
                CW_DrawOwnerToggle(dis, m_hFont, false, m_showMnemonics);
                return TRUE;
            }
            break;
        }

        case WM_SYSKEYDOWN:
        {
            const bool showNow = ((GetKeyState(VK_MENU) & 0x8000) != 0) || (wp == VK_MENU);
            if (showNow != m_showMnemonics)
            {
                m_showMnemonics = showNow;
                setDialogMnemonicCues(m_hwnd, m_showMnemonics);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            if (wp == VK_MENU)
                return TRUE;
            break;
        }

        case WM_SYSKEYUP:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                setDialogMnemonicCues(m_hwnd, false);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            if (wp == VK_MENU)
                return TRUE;
            break;

        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_KEYMENU)
                return TRUE;
            break;

        case WM_KILLFOCUS:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                setDialogMnemonicCues(m_hwnd, false);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            break;

        case WM_COMMAND:
        {
            int ctlId = (int)LOWORD(wp);
            int notif  = (int)HIWORD(wp);
            /* Suppress BN_SETFOCUS / BN_KILLFOCUS — act only on BN_CLICKED */
            if ((ctlId == IDC_SCHED_ACTIVE || ctlId == IDC_SCHED_SCANMEM || ctlId == IDC_SCHED_RUNMISSED) &&
                notif != BN_CLICKED)
                return FALSE;
            if (ctlId == IDC_SCHED_FREQ && notif == CBN_SELCHANGE)
            {
                updateDayEnableState();
                return TRUE;
            }
            break;
        }

        case WM_TIMER:
            if ((UINT_PTR)wp == s_scheduleMnemonicTimerId)
            {
                const bool showNow = (GetKeyState(VK_MENU) & 0x8000) != 0;
                if (showNow != m_showMnemonics)
                {
                    m_showMnemonics = showNow;
                    setDialogMnemonicCues(m_hwnd, m_showMnemonics);
                    InvalidateRect(m_hwnd, NULL, TRUE);
                }
                return TRUE;
            }
            break;

        case WM_DESTROY:
            KillTimer(m_hwnd, s_scheduleMnemonicTimerId);
            destroyTooltips();
            break;
    }

    return CWDialog::handleMessage(msg, wp, lp);
}

/* ─── C Wrapper ─────────────────────────────────────────────── */
int CW_ScheduleDialogRun(HWND hwndParent, CWConfig *cfg)
{
    CWScheduleDialog dlg(*cfg);
    return (int)dlg.runModal(hwndParent, 420, 530);
}


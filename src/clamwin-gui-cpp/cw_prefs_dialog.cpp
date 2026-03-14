/*
 * ClamWin Free Antivirus — CWPrefsDialog
 *
 * Themed multi-page preferences dialog with sidebar navigation.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_prefs_dialog.h"
#include "cw_theme.h"
#include "cw_toggle.h"
#include "cw_time_edit.h"
#include "cw_prefs_validation.h"
#include "cw_mnemonic.h"
#include "cw_dpi.h"
#include "cw_gui_shared.h"
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <ctype.h>
#include <string>
#include <vector>

enum
{
    IDC_PREFS_SIDEBAR = 4100,

    IDC_PREFS_GENERAL_RECURSIVE,
    IDC_PREFS_GENERAL_SCANMAIL,
    IDC_PREFS_GENERAL_ACT_REPORT,
    IDC_PREFS_GENERAL_ACT_REMOVE,
    IDC_PREFS_GENERAL_ACT_QUAR,
    IDC_PREFS_GENERAL_QUAR_PATH,
    IDC_PREFS_GENERAL_QUAR_BROWSE,

    IDC_PREFS_UPDATES_ENABLE,
    IDC_PREFS_UPDATES_MIRROR,
    IDC_PREFS_UPDATES_FREQ,
    IDC_PREFS_UPDATES_HOUR,
    IDC_PREFS_UPDATES_MINUTE,
    IDC_PREFS_UPDATES_DAY,
    IDC_PREFS_UPDATES_ONSTART,
    IDC_PREFS_UPDATES_CHECKVER,

    IDC_PREFS_PROXY_ENABLE,
    IDC_PREFS_PROXY_HOST,
    IDC_PREFS_PROXY_PORT,
    IDC_PREFS_PROXY_USER,
    IDC_PREFS_PROXY_PASS,

    IDC_PREFS_SCHED_ENABLE,
    IDC_PREFS_SCHED_FREQ,
    IDC_PREFS_SCHED_HOUR,
    IDC_PREFS_SCHED_MINUTE,
    IDC_PREFS_SCHED_DAY,
    IDC_PREFS_SCHED_DETAILS,

    IDC_PREFS_LIMITS_ARCHIVES,
    IDC_PREFS_LIMITS_MAXSCAN,
    IDC_PREFS_LIMITS_MAXFILE,
    IDC_PREFS_LIMITS_MAXFILES,
    IDC_PREFS_LIMITS_MAXDEPTH,

    IDC_PREFS_FILES_DB,
    IDC_PREFS_FILES_DB_BROWSE,
    IDC_PREFS_FILES_SCANLOG,
    IDC_PREFS_FILES_SCANLOG_BROWSE,
    IDC_PREFS_FILES_UPDATELOG,
    IDC_PREFS_FILES_UPDATELOG_BROWSE,

    IDC_PREFS_ADV_OLE2,
    IDC_PREFS_ADV_PRIORITY,

    IDC_PREFS_FILTERS_LST_EXCL,
    IDC_PREFS_FILTERS_EDT_EXCL,
    IDC_PREFS_FILTERS_BTN_EXCL_ADD,
    IDC_PREFS_FILTERS_BTN_EXCL_REM,
    IDC_PREFS_FILTERS_LST_INCL,
    IDC_PREFS_FILTERS_EDT_INCL,
    IDC_PREFS_FILTERS_BTN_INCL_ADD,
    IDC_PREFS_FILTERS_BTN_INCL_REM,

    IDC_PREFS_UPDATES_SPIN_HOUR,
    IDC_PREFS_UPDATES_SPIN_MIN,
    IDC_PREFS_SCHED_SPIN_HOUR,
    IDC_PREFS_SCHED_SPIN_MIN,
    IDC_PREFS_LIMITS_SPIN_MAXSCAN,
    IDC_PREFS_LIMITS_SPIN_MAXFILE,
    IDC_PREFS_LIMITS_SPIN_MAXFILES,
    IDC_PREFS_LIMITS_SPIN_MAXDEPTH
};

static const char* s_pageNames[8] = {
    "&General",
    "&Filters",
    "Internet &Updates",
    "&Proxy",
    "&Scheduled Scans",
    "&Limits",
    "File L&ocations",
    "&Advanced"
};

static const char* s_prefsPageOwnerProp = "CWPrefsPageOwner";
static const char* s_prefsSidebarOwnerProp = "CWPrefsSidebarOwner";
static const char* s_prefsEditOldProcProp = "CWPrefsEditOldProc";
static const UINT_PTR s_prefsMnemonicTimerId = 0xCA11;

static LRESULT CALLBACK prefsEditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    WNDPROC oldProc = (WNDPROC)GetPropA(hwnd, s_prefsEditOldProcProp);
    if (!oldProc)
        return DefWindowProcA(hwnd, msg, wp, lp);

    if (msg == WM_KEYDOWN && wp == VK_TAB)
    {
        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (root)
        {
            bool prev = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            HWND next = GetNextDlgTabItem(root, hwnd, prev ? TRUE : FALSE);
            if (next)
            {
                SetFocus(next);
                return 0;
            }
        }
    }

    if (msg == WM_GETDLGCODE)
    {
        LRESULT code = CallWindowProcA(oldProc, hwnd, msg, wp, lp);
        code &= ~(DLGC_WANTTAB | DLGC_WANTALLKEYS);
        return code;
    }

    return CallWindowProcA(oldProc, hwnd, msg, wp, lp);
}

static int findSidebarMnemonicPageIndex(char ch)
{
    if (!ch)
        return -1;

    unsigned char key = (unsigned char)tolower((unsigned char)ch);
    const int pageCount = (int)(sizeof(s_pageNames) / sizeof(s_pageNames[0]));
    for (int i = 0; i < pageCount; ++i)
    {
        std::string plain;
        int accelIndex = -1;
        CW_ParseMnemonicText(s_pageNames[i], plain, accelIndex);
        if (accelIndex >= 0 && accelIndex < (int)plain.size())
        {
            unsigned char accel = (unsigned char)tolower((unsigned char)plain[(size_t)accelIndex]);
            if (accel == key)
                return i;
        }
    }
    return -1;
}

static int findSidebarMnemonicPageIndexFromVKey(WPARAM key)
{
    if (key < 'A' || key > 'Z')
        return -1;

    char ch = (char)tolower((unsigned char)key);
    return findSidebarMnemonicPageIndex(ch);
}

static void enableNotifyStyle(HWND hwnd)
{
    if (!hwnd) return;
    LONG_PTR style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    SetWindowLongPtrA(hwnd, GWL_STYLE, style | BS_NOTIFY);
}

static bool isCheckOrRadio(HWND hwnd)
{
    if (!hwnd) return false;
    char cls[32] = {0};
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (lstrcmpiA(cls, "Button") != 0)
        return false;

    LONG_PTR style = GetWindowLongPtrA(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    return type == BS_CHECKBOX ||
           type == BS_AUTOCHECKBOX ||
           type == BS_RADIOBUTTON ||
           type == BS_AUTORADIOBUTTON;
}

static bool isToggleControlId(int id)
{
    switch (id)
    {
        case IDC_PREFS_GENERAL_RECURSIVE:
        case IDC_PREFS_GENERAL_SCANMAIL:
        case IDC_PREFS_GENERAL_ACT_REPORT:
        case IDC_PREFS_GENERAL_ACT_REMOVE:
        case IDC_PREFS_GENERAL_ACT_QUAR:
        case IDC_PREFS_UPDATES_ENABLE:
        case IDC_PREFS_UPDATES_ONSTART:
        case IDC_PREFS_UPDATES_CHECKVER:
        case IDC_PREFS_PROXY_ENABLE:
        case IDC_PREFS_SCHED_ENABLE:
        case IDC_PREFS_LIMITS_ARCHIVES:
        case IDC_PREFS_ADV_OLE2:
            return true;
    }
    return false;
}

static bool isRadioControlId(int id)
{
    return id == IDC_PREFS_GENERAL_ACT_REPORT ||
           id == IDC_PREFS_GENERAL_ACT_REMOVE ||
           id == IDC_PREFS_GENERAL_ACT_QUAR;
}

static bool isThemedComboId(int id)
{
    return id == IDC_PREFS_UPDATES_FREQ ||
           id == IDC_PREFS_UPDATES_DAY ||
           id == IDC_PREFS_SCHED_FREQ ||
           id == IDC_PREFS_SCHED_DAY ||
           id == IDC_PREFS_ADV_PRIORITY;
}

static void configureThemedCombo(HWND combo)
{
    if (!combo) return;
    int fieldHeight = CW_Scale(26);
    int listHeight = CW_Scale(26);
    SendMessageA(combo, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)fieldHeight);
    SendMessageA(combo, CB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)listHeight);
}

static void configureCenteredEditTextRect(HWND edit)
{
    if (!edit)
        return;

    char className[32] = {0};
    GetClassNameA(edit, className, sizeof(className));
    if (lstrcmpiA(className, "EDIT") != 0)
        return;

    LONG_PTR style = GetWindowLongPtrA(edit, GWL_STYLE);
    if ((style & ES_MULTILINE) == 0)
    {
        SetWindowLongPtrA(edit, GWL_STYLE, style | ES_MULTILINE);
    }

    if (!GetPropA(edit, s_prefsEditOldProcProp))
    {
        WNDPROC oldProc = (WNDPROC)SetWindowLongPtrA(edit, GWLP_WNDPROC, (LONG_PTR)prefsEditSubclassProc);
        if (oldProc)
            SetPropA(edit, s_prefsEditOldProcProp, (HANDLE)oldProc);
    }

    RECT rc;
    GetClientRect(edit, &rc);

    HFONT hFont = (HFONT)SendMessageA(edit, WM_GETFONT, 0, 0);
    HDC hdc = GetDC(edit);
    if (!hdc)
        return;

    HGDIOBJ oldFont = NULL;
    if (hFont)
        oldFont = SelectObject(hdc, hFont);

    TEXTMETRICA tm;
    ZeroMemory(&tm, sizeof(tm));
    GetTextMetricsA(hdc, &tm);

    if (oldFont)
        SelectObject(hdc, oldFont);
    ReleaseDC(edit, hdc);

    int clientH = rc.bottom - rc.top;
    int textH = tm.tmHeight;
    if (textH <= 0)
        textH = CW_Scale(13);

    int topPad = (clientH - textH) / 2;
    if (topPad < 1)
        topPad = 1;

    const int leftPad = CW_Scale(8);
    RECT textRc;
    textRc.left   = leftPad;
    textRc.right  = rc.right - CW_Scale(8);
    textRc.top    = topPad;
    textRc.bottom = topPad + textH + 1;

    SendMessageA(edit, EM_SETRECTNP, 0, (LPARAM)&textRc);
    InvalidateRect(edit, NULL, TRUE);
}

/* Prop name matches CW_TOGGLE_PROP in cw_toggle.h */
static const char* s_toggleProp = CW_TOGGLE_PROP;
static const char* s_invalidFieldProp = "CW_PREFS_INVALID_FIELD";

static HBRUSH getValidationEditBrush(CWTheme* theme)
{
    static HBRUSH s_brush = NULL;
    static COLORREF s_color = CLR_INVALID;

    COLORREF desired = RGB(255, 235, 238);
    if (theme && theme->isDark())
        desired = RGB(68, 38, 38);

    if (!s_brush || s_color != desired)
    {
        if (s_brush)
            DeleteObject(s_brush);
        s_brush = CreateSolidBrush(desired);
        s_color = desired;
    }

    return s_brush;
}

static bool getToggleChecked(HWND hwnd)
{
    if (!hwnd) return false;
    return GetPropA(hwnd, s_toggleProp) != NULL;
}

static void setToggleChecked(HWND hwnd, bool checked)
{
    if (!hwnd) return;
    if (checked)
        SetPropA(hwnd, s_toggleProp, (HANDLE)1);
    else
        RemovePropA(hwnd, s_toggleProp);
}

CWPrefsDialog::CWPrefsDialog(CWConfig& cfg)
    : m_cfg(cfg)
    , m_hFont(NULL)
    , m_hFontBold(NULL)
    , m_hwndSidebar(NULL)
    , m_sidebarOldProc(NULL)
    , m_hwndBtnOk(NULL)
    , m_hwndBtnCancel(NULL)
    , m_activePage(PAGE_GENERAL)
    , m_showMnemonics(false)
    , m_chkRecursive(NULL)
    , m_chkScanMail(NULL)
    , m_radActionReport(NULL)
    , m_radActionRemove(NULL)
    , m_radActionQuarantine(NULL)
    , m_edtQuarantine(NULL)
    , m_btnQuarantineBrowse(NULL)
    , m_chkUpdateScheduled(NULL)
    , m_edtMirror(NULL)
    , m_cmbUpdateFrequency(NULL)
    , m_edtUpdateHour(NULL)
    , m_spinUpdateHour(NULL)
    , m_edtUpdateMinute(NULL)
    , m_spinUpdateMinute(NULL)
    , m_cmbUpdateDay(NULL)
    , m_chkUpdateOnStartup(NULL)
    , m_chkCheckVersion(NULL)
    , m_chkProxyEnabled(NULL)
    , m_edtProxyHost(NULL)
    , m_edtProxyPort(NULL)
    , m_edtProxyUser(NULL)
    , m_edtProxyPass(NULL)
    , m_chkScanScheduled(NULL)
    , m_cmbScanFrequency(NULL)
    , m_edtScanHour(NULL)
    , m_spinScanHour(NULL)
    , m_edtScanMinute(NULL)
    , m_spinScanMinute(NULL)
    , m_cmbScanDay(NULL)
    , m_btnScheduleDetails(NULL)
    , m_chkScanArchives(NULL)
    , m_edtMaxScanSize(NULL)
    , m_edtMaxFileSize(NULL)
    , m_edtMaxFiles(NULL)
    , m_edtMaxDepth(NULL)
    , m_spinMaxScanSize(NULL)
    , m_spinMaxFileSize(NULL)
    , m_spinMaxFiles(NULL)
    , m_spinMaxDepth(NULL)
    , m_edtDatabasePath(NULL)
    , m_btnDatabaseBrowse(NULL)
    , m_edtScanLog(NULL)
    , m_btnScanLogBrowse(NULL)
    , m_edtUpdateLog(NULL)
    , m_btnUpdateLogBrowse(NULL)
    , m_edtExclPattern(NULL)
    , m_lstExclude(NULL)
    , m_btnExclAdd(NULL)
    , m_btnExclRemove(NULL)
    , m_edtInclPattern(NULL)
    , m_lstInclude(NULL)
    , m_btnInclAdd(NULL)
    , m_btnInclRemove(NULL)
    , m_chkScanOle2(NULL)
    , m_cmbPriority(NULL)
{
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        m_hwndPages[i] = NULL;
        m_pageOldProc[i] = NULL;
    }
}

CWPrefsDialog::~CWPrefsDialog()
{
    if (m_hFont && m_hFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(m_hFont);
    if (m_hFontBold && m_hFontBold != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(m_hFontBold);
}

void CWPrefsDialog::createFonts()
{
    if (m_hFont && m_hFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
    if (m_hFontBold && m_hFontBold != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
    {
        DeleteObject(m_hFontBold);
        m_hFontBold = NULL;
    }

    m_hFont = CreateFontW(-CW_Scale(13), 0, 0, 0, FW_NORMAL,
                          FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    m_hFontBold = CreateFontW(-CW_Scale(13), 0, 0, 0, FW_SEMIBOLD,
                              FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    if (!m_hFont)
        m_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!m_hFontBold)
        m_hFontBold = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

void CWPrefsDialog::setControlFont(HWND hwnd, bool bold)
{
    if (!hwnd) return;
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)(bold ? m_hFontBold : m_hFont), TRUE);

    char className[32] = {0};
    GetClassNameA(hwnd, className, sizeof(className));
    if (lstrcmpiA(className, "EDIT") == 0)
        configureCenteredEditTextRect(hwnd);
}

void CWPrefsDialog::createSidebar()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int pad = CW_Scale(12);
    int btnH = CW_Scale(30);
    int contentBottom = rc.bottom - pad - btnH - CW_Scale(10);
    int sidebarW = CW_Scale(170);

    m_hwndSidebar = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY |
                                    LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_VSCROLL,
                                    pad, pad, sidebarW, contentBottom - pad,
                                    m_hwnd, (HMENU)IDC_PREFS_SIDEBAR,
                                    GetModuleHandleA(NULL), NULL);
    setControlFont(m_hwndSidebar);

    if (m_hwndSidebar)
    {
        SetPropA(m_hwndSidebar, s_prefsSidebarOwnerProp, (HANDLE)this);
        m_sidebarOldProc = (WNDPROC)SetWindowLongPtrA(
            m_hwndSidebar,
            GWLP_WNDPROC,
            (LONG_PTR)&CWPrefsDialog::sidebarSubclassProc);
    }

    for (int i = 0; i < PAGE_COUNT; ++i)
        SendMessageA(m_hwndSidebar, LB_ADDSTRING, 0, (LPARAM)s_pageNames[i]);

    SendMessageA(m_hwndSidebar, LB_SETCURSEL, PAGE_GENERAL, 0);
}

void CWPrefsDialog::createGeneralPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    int w = CW_Scale(420);
    const int rowStep = CW_Scale(44);

    HWND hTitle = CreateWindowExA(0, "STATIC", "Scanning Options",
                                  WS_CHILD | WS_VISIBLE, x, y, w, CW_Scale(20),
                                  page, NULL, NULL, NULL);
    setControlFont(hTitle, true);
    y += rowStep;

    m_chkRecursive = CreateWindowExA(0, "BUTTON", "Scan in sub&directories",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                     x, y, w, CW_Scale(22), page,
                                     (HMENU)IDC_PREFS_GENERAL_RECURSIVE, NULL, NULL);
    setControlFont(m_chkRecursive);
    enableNotifyStyle(m_chkRecursive);
    y += rowStep;

    m_chkScanMail = CreateWindowExA(0, "BUTTON", "Sca&n mail files",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                    x, y, w, CW_Scale(22), page,
                                    (HMENU)IDC_PREFS_GENERAL_SCANMAIL, NULL, NULL);
    setControlFont(m_chkScanMail);
    enableNotifyStyle(m_chkScanMail);
    y += rowStep;

    HWND hAction = CreateWindowExA(0, "STATIC", "Infected files action",
                                   WS_CHILD | WS_VISIBLE, x, y, w, CW_Scale(20),
                                   page, NULL, NULL, NULL);
    setControlFont(hAction, true);
    y += rowStep;

    m_radActionReport = CreateWindowExA(0, "BUTTON", "&Report only",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY | WS_GROUP,
                                        x, y, w, CW_Scale(22), page,
                                        (HMENU)IDC_PREFS_GENERAL_ACT_REPORT, NULL, NULL);
    setControlFont(m_radActionReport);
    enableNotifyStyle(m_radActionReport);
    y += rowStep;

    m_radActionRemove = CreateWindowExA(0, "BUTTON", "R&emove infected files",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                        x, y, w, CW_Scale(22), page,
                                        (HMENU)IDC_PREFS_GENERAL_ACT_REMOVE, NULL, NULL);
    setControlFont(m_radActionRemove);
    enableNotifyStyle(m_radActionRemove);
    y += rowStep;

    m_radActionQuarantine = CreateWindowExA(0, "BUTTON", "&Move to quarantine",
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                            x, y, w, CW_Scale(22), page,
                                            (HMENU)IDC_PREFS_GENERAL_ACT_QUAR, NULL, NULL);
    setControlFont(m_radActionQuarantine);
    enableNotifyStyle(m_radActionQuarantine);
    y += rowStep;

    m_edtQuarantine = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                      x, y, CW_Scale(300), CW_Scale(30), page,
                                      (HMENU)IDC_PREFS_GENERAL_QUAR_PATH, NULL, NULL);
    setControlFont(m_edtQuarantine);

    m_btnQuarantineBrowse = CreateWindowExA(0, "BUTTON", "&Browse...",
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                            x + CW_Scale(310), y, CW_Scale(100), CW_Scale(30), page,
                                            (HMENU)IDC_PREFS_GENERAL_QUAR_BROWSE, NULL, NULL);
    setControlFont(m_btnQuarantineBrowse);
}

static const char* const s_patSep    = "|CLAMWIN_SEP|";
static const int         s_patSepLen = 14;

static void populatePatternList(HWND lb, const std::string& raw)
{
    SendMessageA(lb, LB_RESETCONTENT, 0, 0);
    size_t pos = 0;
    while (pos <= raw.size())
    {
        size_t next = raw.find(s_patSep, pos);
        if (next == std::string::npos)
        {
            std::string item = raw.substr(pos);
            if (!item.empty())
            {
                std::string padded = "  " + item;
                SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)padded.c_str());
            }
            break;
        }
        std::string item = raw.substr(pos, next - pos);
        if (!item.empty())
        {
            std::string padded = "  " + item;
            SendMessageA(lb, LB_ADDSTRING, 0, (LPARAM)padded.c_str());
        }
        pos = next + s_patSepLen;
    }
}

static std::string joinPatternList(HWND lb)
{
    int count = (int)SendMessageA(lb, LB_GETCOUNT, 0, 0);
    std::string result;
    for (int i = 0; i < count; ++i)
    {
        char buf[512] = {0};
        SendMessageA(lb, LB_GETTEXT, i, (LPARAM)buf);
        const char* p = buf;
        while (*p == ' ') ++p;  /* strip leading-space margin */
        if (!result.empty()) result += s_patSep;
        result += p;
    }
    return result;
}

static std::string trimAscii(const std::string& s)
{
    size_t first = 0;
    while (first < s.size() && (s[first] == ' ' || s[first] == '\t' || s[first] == '\r' || s[first] == '\n'))
        ++first;

    size_t last = s.size();
    while (last > first && (s[last - 1] == ' ' || s[last - 1] == '\t' || s[last - 1] == '\r' || s[last - 1] == '\n'))
        --last;

    return s.substr(first, last - first);
}

static bool showValidationError(HWND owner, HWND target, const char* msg)
{
    MessageBoxA(owner, msg, "ClamWin", MB_ICONEXCLAMATION | MB_OK);
    if (target)
    {
        SetPropA(target, s_invalidFieldProp, (HANDLE)1);
        SetFocus(target);
        InvalidateRect(target, NULL, TRUE);
    }
    return false;
}

static void clearValidationMark(HWND target)
{
    if (!target)
        return;

    RemovePropA(target, s_invalidFieldProp);
    InvalidateRect(target, NULL, TRUE);
}

void CWPrefsDialog::createFiltersPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int colW  = CW_Scale(210);
    const int gap   = CW_Scale(10);
    const int col2  = x + colW + gap;
    const int rowH  = CW_Scale(30);
    const int listH = CW_Scale(190);
    const int btnW  = CW_Scale(46);
    const int edtW  = colW - btnW - CW_Scale(4);

    HWND hDesc = CreateWindowExA(0, "STATIC",
                                 "Specify filename patterns to include or exclude from scanning.\r\n"
                                 "Wildcards: * matches any characters, ? matches one character.",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 x, y, CW_Scale(430), CW_Scale(36),
                                 page, NULL, NULL, NULL);
    setControlFont(hDesc);
    y += CW_Scale(44);

    HWND hExclLbl = CreateWindowExA(0, "STATIC", "&Exclude Matching Filenames:",
                                    WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                    x, y, colW, CW_Scale(20), page, NULL, NULL, NULL);
    setControlFont(hExclLbl);
    HWND hInclLbl = CreateWindowExA(0, "STATIC", "Include &Matching Filenames:",
                                    WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                    col2, y, colW, CW_Scale(20), page, NULL, NULL, NULL);
    setControlFont(hInclLbl);
    y += CW_Scale(24);

    m_edtExclPattern = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                       x, y, edtW, rowH,
                                       page, (HMENU)IDC_PREFS_FILTERS_EDT_EXCL, NULL, NULL);
    setControlFont(m_edtExclPattern);
    m_btnExclAdd = CreateWindowExA(0, "BUTTON", "&Add",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                   x + edtW + CW_Scale(4), y, btnW, rowH,
                                   page, (HMENU)IDC_PREFS_FILTERS_BTN_EXCL_ADD, NULL, NULL);
    setControlFont(m_btnExclAdd);

    m_edtInclPattern = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                       col2, y, edtW, rowH,
                                       page, (HMENU)IDC_PREFS_FILTERS_EDT_INCL, NULL, NULL);
    setControlFont(m_edtInclPattern);
    m_btnInclAdd = CreateWindowExA(0, "BUTTON", "A&dd",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                   col2 + edtW + CW_Scale(4), y, btnW, rowH,
                                   page, (HMENU)IDC_PREFS_FILTERS_BTN_INCL_ADD, NULL, NULL);
    setControlFont(m_btnInclAdd);
    y += CW_Scale(36);

    m_lstExclude = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                   LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL,
                                   x, y, colW, listH,
                                   page, (HMENU)IDC_PREFS_FILTERS_LST_EXCL, NULL, NULL);
    setControlFont(m_lstExclude);
    m_lstInclude = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                                   LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL,
                                   col2, y, colW, listH,
                                   page, (HMENU)IDC_PREFS_FILTERS_LST_INCL, NULL, NULL);
    setControlFont(m_lstInclude);
    y += listH + CW_Scale(6);

    m_btnExclRemove = CreateWindowExA(0, "BUTTON", "&Remove Selected",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                      x, y, colW, CW_Scale(26),
                                      page, (HMENU)IDC_PREFS_FILTERS_BTN_EXCL_REM, NULL, NULL);
    setControlFont(m_btnExclRemove);
    m_btnInclRemove = CreateWindowExA(0, "BUTTON", "R&emove Selected",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                      col2, y, colW, CW_Scale(26),
                                      page, (HMENU)IDC_PREFS_FILTERS_BTN_INCL_REM, NULL, NULL);
    setControlFont(m_btnInclRemove);
}

void CWPrefsDialog::createUpdatesPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);
    const int timeX = x + CW_Scale(122);
    const int hourW = CW_Scale(54);
    const int spinW = CW_Scale(18);
    const int colonW = CW_Scale(12);
    const int minW = CW_Scale(38);

    m_chkUpdateScheduled = CreateWindowExA(0, "BUTTON", "Enable scheduled database &updates",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                           x, y, CW_Scale(420), CW_Scale(22), page,
                                           (HMENU)IDC_PREFS_UPDATES_ENABLE, NULL, NULL);
    setControlFont(m_chkUpdateScheduled);
    enableNotifyStyle(m_chkUpdateScheduled);
    y += rowStep;

    HWND hMirror = CreateWindowExA(0, "STATIC", "&Download site:",
                                   WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                   x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                   page, NULL, NULL, NULL);
    setControlFont(hMirror);

    m_edtMirror = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                  x + CW_Scale(122), y, CW_Scale(280), CW_Scale(30),
                                  page, (HMENU)IDC_PREFS_UPDATES_MIRROR, NULL, NULL);
    setControlFont(m_edtMirror);
    y += rowStep;

    HWND hFreq = CreateWindowExA(0, "STATIC", "Freque&ncy:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hFreq);

    m_cmbUpdateFrequency = CreateWindowExA(0, "COMBOBOX", "",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                           x + CW_Scale(122), y, CW_Scale(150), CW_Scale(220),
                                           page, (HMENU)IDC_PREFS_UPDATES_FREQ, NULL, NULL);
    setControlFont(m_cmbUpdateFrequency);
    configureThemedCombo(m_cmbUpdateFrequency);
    SendMessageA(m_cmbUpdateFrequency, CB_ADDSTRING, 0, (LPARAM)"Daily");
    SendMessageA(m_cmbUpdateFrequency, CB_ADDSTRING, 0, (LPARAM)"Weekly");
    SendMessageA(m_cmbUpdateFrequency, CB_ADDSTRING, 0, (LPARAM)"Workdays");
    y += rowStep;

    HWND hDay = CreateWindowExA(0, "STATIC", "Day of &week:",
                                WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                page, NULL, NULL, NULL);
    setControlFont(hDay);

    m_cmbUpdateDay = CreateWindowExA(0, "COMBOBOX", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                     x + CW_Scale(122), y, CW_Scale(150), CW_Scale(220),
                                     page, (HMENU)IDC_PREFS_UPDATES_DAY, NULL, NULL);
    setControlFont(m_cmbUpdateDay);
    configureThemedCombo(m_cmbUpdateDay);

    static const char* days[7] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
    for (int i = 0; i < 7; ++i)
        SendMessageA(m_cmbUpdateDay, CB_ADDSTRING, 0, (LPARAM)days[i]);
    y += rowStep;

    HWND hTime = CreateWindowExA(0, "STATIC", "&Time (HH:MM):",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hTime);

    m_edtUpdateHour = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                      timeX, y, hourW, CW_Scale(30),
                                      page, (HMENU)IDC_PREFS_UPDATES_HOUR, NULL, NULL);
    setControlFont(m_edtUpdateHour);

    m_spinUpdateHour = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                       WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                       timeX + hourW, y, spinW, CW_Scale(30),
                                       page, (HMENU)IDC_PREFS_UPDATES_SPIN_HOUR, NULL, NULL);
    SendMessageA(m_spinUpdateHour, UDM_SETRANGE, 0, MAKELONG(23, 0));

    HWND hColonU = CreateWindowExA(0, "STATIC", ":",
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    timeX + hourW + spinW, y + CW_Scale(5), colonW, CW_Scale(20),
                    page, NULL, NULL, NULL);
    setControlFont(hColonU);

    m_edtUpdateMinute = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                        timeX + hourW + spinW + colonW, y, minW, CW_Scale(30),
                                        page, (HMENU)IDC_PREFS_UPDATES_MINUTE, NULL, NULL);
    setControlFont(m_edtUpdateMinute);

    m_spinUpdateMinute = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                         WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                         timeX + hourW + spinW + colonW + minW, y, spinW, CW_Scale(30),
                                         page, (HMENU)IDC_PREFS_UPDATES_SPIN_MIN, NULL, NULL);
    SendMessageA(m_spinUpdateMinute, UDM_SETRANGE, 0, MAKELONG(59, 0));
    y += rowStep;

    m_chkUpdateOnStartup = CreateWindowExA(0, "BUTTON", "&Update database on startup",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                           x, y, CW_Scale(260), CW_Scale(22), page,
                                           (HMENU)IDC_PREFS_UPDATES_ONSTART, NULL, NULL);
    setControlFont(m_chkUpdateOnStartup);
    enableNotifyStyle(m_chkUpdateOnStartup);
    y += CW_Scale(28);

    m_chkCheckVersion = CreateWindowExA(0, "BUTTON", "&Notify about new ClamWin releases",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                        x, y, CW_Scale(320), CW_Scale(22), page,
                                        (HMENU)IDC_PREFS_UPDATES_CHECKVER, NULL, NULL);
    setControlFont(m_chkCheckVersion);
    enableNotifyStyle(m_chkCheckVersion);
}

void CWPrefsDialog::createProxyPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);

    m_chkProxyEnabled = CreateWindowExA(0, "BUTTON", "Use pro&xy server",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                        x, y, CW_Scale(250), CW_Scale(22),
                                        page, (HMENU)IDC_PREFS_PROXY_ENABLE, NULL, NULL);
    setControlFont(m_chkProxyEnabled);
    enableNotifyStyle(m_chkProxyEnabled);
    y += rowStep;

    HWND hHost = CreateWindowExA(0, "STATIC", "Proxy s&erver:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hHost);
    m_edtProxyHost = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     x + CW_Scale(122), y, CW_Scale(260), CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_PROXY_HOST, NULL, NULL);
    setControlFont(m_edtProxyHost);
    y += rowStep;

    HWND hPort = CreateWindowExA(0, "STATIC", "P&ort:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hPort);
    m_edtProxyPort = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                     x + CW_Scale(122), y, CW_Scale(90), CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_PROXY_PORT, NULL, NULL);
    setControlFont(m_edtProxyPort);
    y += rowStep;

    HWND hUser = CreateWindowExA(0, "STATIC", "User&name:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hUser);
    m_edtProxyUser = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     x + CW_Scale(122), y, CW_Scale(260), CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_PROXY_USER, NULL, NULL);
    setControlFont(m_edtProxyUser);
    y += rowStep;

    HWND hPass = CreateWindowExA(0, "STATIC", "Pass&word:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hPass);
    m_edtProxyPass = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_PASSWORD,
                                     x + CW_Scale(122), y, CW_Scale(260), CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_PROXY_PASS, NULL, NULL);
    setControlFont(m_edtProxyPass);
}

void CWPrefsDialog::createSchedulePage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);
    const int buttonGap = CW_Scale(56);
    const int timeX = x + CW_Scale(122);
    const int hourW = CW_Scale(54);
    const int spinW = CW_Scale(18);
    const int colonW = CW_Scale(12);
    const int minW = CW_Scale(38);

    m_chkScanScheduled = CreateWindowExA(0, "BUTTON", "&Enable scheduled scans",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                         x, y, CW_Scale(250), CW_Scale(22),
                                         page, (HMENU)IDC_PREFS_SCHED_ENABLE, NULL, NULL);
    setControlFont(m_chkScanScheduled);
    enableNotifyStyle(m_chkScanScheduled);
    y += rowStep;

    HWND hFreq = CreateWindowExA(0, "STATIC", "F&requency:",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hFreq);

    m_cmbScanFrequency = CreateWindowExA(0, "COMBOBOX", "",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                         x + CW_Scale(122), y, CW_Scale(150), CW_Scale(220),
                                         page, (HMENU)IDC_PREFS_SCHED_FREQ, NULL, NULL);
    setControlFont(m_cmbScanFrequency);
    configureThemedCombo(m_cmbScanFrequency);
    SendMessageA(m_cmbScanFrequency, CB_ADDSTRING, 0, (LPARAM)"Daily");
    SendMessageA(m_cmbScanFrequency, CB_ADDSTRING, 0, (LPARAM)"Weekly");
    SendMessageA(m_cmbScanFrequency, CB_ADDSTRING, 0, (LPARAM)"Workdays");
    y += rowStep;

    HWND hDay = CreateWindowExA(0, "STATIC", "Day of &week:",
                                WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                page, NULL, NULL, NULL);
    setControlFont(hDay);

    m_cmbScanDay = CreateWindowExA(0, "COMBOBOX", "",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                   x + CW_Scale(122), y, CW_Scale(150), CW_Scale(220),
                                   page, (HMENU)IDC_PREFS_SCHED_DAY, NULL, NULL);
    setControlFont(m_cmbScanDay);
    configureThemedCombo(m_cmbScanDay);

    static const char* days[7] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
    for (int i = 0; i < 7; ++i)
        SendMessageA(m_cmbScanDay, CB_ADDSTRING, 0, (LPARAM)days[i]);
    y += rowStep;

    HWND hTime = CreateWindowExA(0, "STATIC", "&Time (HH:MM):",
                                 WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                 x, y + CW_Scale(4), CW_Scale(120), CW_Scale(20),
                                 page, NULL, NULL, NULL);
    setControlFont(hTime);

    m_edtScanHour = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                    timeX, y, hourW, CW_Scale(30),
                                    page, (HMENU)IDC_PREFS_SCHED_HOUR, NULL, NULL);
    setControlFont(m_edtScanHour);

    m_spinScanHour = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                     WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                     timeX + hourW, y, spinW, CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_SCHED_SPIN_HOUR, NULL, NULL);
    SendMessageA(m_spinScanHour, UDM_SETRANGE, 0, MAKELONG(23, 0));

    HWND hColonS = CreateWindowExA(0, "STATIC", ":",
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    timeX + hourW + spinW, y + CW_Scale(5), colonW, CW_Scale(20),
                    page, NULL, NULL, NULL);
    setControlFont(hColonS);

    m_edtScanMinute = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                      timeX + hourW + spinW + colonW, y, minW, CW_Scale(30),
                                      page, (HMENU)IDC_PREFS_SCHED_MINUTE, NULL, NULL);
    setControlFont(m_edtScanMinute);

    m_spinScanMinute = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                       WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_WRAP,
                                       timeX + hourW + spinW + colonW + minW, y, spinW, CW_Scale(30),
                                       page, (HMENU)IDC_PREFS_SCHED_SPIN_MIN, NULL, NULL);
    SendMessageA(m_spinScanMinute, UDM_SETRANGE, 0, MAKELONG(59, 0));
    y += buttonGap;

    m_btnScheduleDetails = CreateWindowExA(0, "BUTTON", "Open &Detailed Schedule...",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                           x, y, CW_Scale(272), CW_Scale(28),
                                           page, (HMENU)IDC_PREFS_SCHED_DETAILS, NULL, NULL);
    setControlFont(m_btnScheduleDetails);
}

void CWPrefsDialog::createLimitsPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);
    const int editW = CW_Scale(72);
    const int spinW = CW_Scale(18);
    const int editX = x + CW_Scale(250);
    const int spinX = editX + editW;

    m_chkScanArchives = CreateWindowExA(0, "BUTTON", "Extract files from arc&hives",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                        x, y, CW_Scale(280), CW_Scale(22),
                                        page, (HMENU)IDC_PREFS_LIMITS_ARCHIVES, NULL, NULL);
    setControlFont(m_chkScanArchives);
    enableNotifyStyle(m_chkScanArchives);
    y += rowStep;

    HWND hMaxFile = CreateWindowExA(0, "STATIC", "Do not sca&n files larger than (MB):",
                                    WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                    x, y + CW_Scale(4), CW_Scale(240), CW_Scale(20),
                                    page, NULL, NULL, NULL);
    setControlFont(hMaxFile);
    m_edtMaxFileSize = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                       editX, y, editW, CW_Scale(30),
                                       page, (HMENU)IDC_PREFS_LIMITS_MAXFILE, NULL, NULL);
    setControlFont(m_edtMaxFileSize);
    m_spinMaxFileSize = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                        WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS,
                                        spinX, y, spinW, CW_Scale(30),
                                        page, (HMENU)IDC_PREFS_LIMITS_SPIN_MAXFILE, NULL, NULL);
    SendMessageA(m_spinMaxFileSize, UDM_SETBUDDY, (WPARAM)m_edtMaxFileSize, 0);
    SendMessageA(m_spinMaxFileSize, UDM_SETRANGE, 0, MAKELONG(32767, 0));
    y += rowStep;

    HWND hMaxScan = CreateWindowExA(0, "STATIC", "Do not extract &more than (MB):",
                                    WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                    x, y + CW_Scale(4), CW_Scale(240), CW_Scale(20),
                                    page, NULL, NULL, NULL);
    setControlFont(hMaxScan);
    m_edtMaxScanSize = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                       editX, y, editW, CW_Scale(30),
                                       page, (HMENU)IDC_PREFS_LIMITS_MAXSCAN, NULL, NULL);
    setControlFont(m_edtMaxScanSize);
    m_spinMaxScanSize = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                        WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS,
                                        spinX, y, spinW, CW_Scale(30),
                                        page, (HMENU)IDC_PREFS_LIMITS_SPIN_MAXSCAN, NULL, NULL);
    SendMessageA(m_spinMaxScanSize, UDM_SETBUDDY, (WPARAM)m_edtMaxScanSize, 0);
    SendMessageA(m_spinMaxScanSize, UDM_SETRANGE, 0, MAKELONG(32767, 0));
    y += rowStep;

    HWND hMaxFiles = CreateWindowExA(0, "STATIC", "Do not e&xtract more than files:",
                                     WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                     x, y + CW_Scale(4), CW_Scale(240), CW_Scale(20),
                                     page, NULL, NULL, NULL);
    setControlFont(hMaxFiles);
    m_edtMaxFiles = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                    editX, y, editW, CW_Scale(30),
                                    page, (HMENU)IDC_PREFS_LIMITS_MAXFILES, NULL, NULL);
    setControlFont(m_edtMaxFiles);
    m_spinMaxFiles = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                     WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS,
                                     spinX, y, spinW, CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_LIMITS_SPIN_MAXFILES, NULL, NULL);
    SendMessageA(m_spinMaxFiles, UDM_SETBUDDY, (WPARAM)m_edtMaxFiles, 0);
    SendMessageA(m_spinMaxFiles, UDM_SETRANGE, 0, MAKELONG(32767, 0));
    y += rowStep;

    HWND hMaxDepth = CreateWindowExA(0, "STATIC", "Do not extract more than su&b-archives:",
                                     WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                     x, y + CW_Scale(4), CW_Scale(240), CW_Scale(20),
                                     page, NULL, NULL, NULL);
    setControlFont(hMaxDepth);
    m_edtMaxDepth = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL,
                                    editX, y, editW, CW_Scale(30),
                                    page, (HMENU)IDC_PREFS_LIMITS_MAXDEPTH, NULL, NULL);
    setControlFont(m_edtMaxDepth);
    m_spinMaxDepth = CreateWindowExA(0, UPDOWN_CLASSA, "",
                                     WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_SETBUDDYINT | UDS_NOTHOUSANDS,
                                     spinX, y, spinW, CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_LIMITS_SPIN_MAXDEPTH, NULL, NULL);
    SendMessageA(m_spinMaxDepth, UDM_SETBUDDY, (WPARAM)m_edtMaxDepth, 0);
    SendMessageA(m_spinMaxDepth, UDM_SETRANGE, 0, MAKELONG(32767, 0));
}

void CWPrefsDialog::createFilesPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);

    HWND l1 = CreateWindowExA(0, "STATIC", "&Virus database folder:",
                              WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                              x, y + CW_Scale(4), CW_Scale(150), CW_Scale(20),
                              page, NULL, NULL, NULL);
    setControlFont(l1);
    m_edtDatabasePath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                        x + CW_Scale(156), y, CW_Scale(200), CW_Scale(30),
                                        page, (HMENU)IDC_PREFS_FILES_DB, NULL, NULL);
    setControlFont(m_edtDatabasePath);
    m_btnDatabaseBrowse = CreateWindowExA(0, "BUTTON", "&Browse...",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                          x + CW_Scale(362), y, CW_Scale(80), CW_Scale(30),
                                          page, (HMENU)IDC_PREFS_FILES_DB_BROWSE, NULL, NULL);
    setControlFont(m_btnDatabaseBrowse);
    y += rowStep;

    HWND l2 = CreateWindowExA(0, "STATIC", "Sca&n report file:",
                              WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                              x, y + CW_Scale(4), CW_Scale(150), CW_Scale(20),
                              page, NULL, NULL, NULL);
    setControlFont(l2);
    m_edtScanLog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                   x + CW_Scale(156), y, CW_Scale(200), CW_Scale(30),
                                   page, (HMENU)IDC_PREFS_FILES_SCANLOG, NULL, NULL);
    setControlFont(m_edtScanLog);
    m_btnScanLogBrowse = CreateWindowExA(0, "BUTTON", "B&rowse...",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                         x + CW_Scale(362), y, CW_Scale(80), CW_Scale(30),
                                         page, (HMENU)IDC_PREFS_FILES_SCANLOG_BROWSE, NULL, NULL);
    setControlFont(m_btnScanLogBrowse);
    y += rowStep;

    HWND l3 = CreateWindowExA(0, "STATIC", "Up&date report file:",
                              WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                              x, y + CW_Scale(4), CW_Scale(150), CW_Scale(20),
                              page, NULL, NULL, NULL);
    setControlFont(l3);
    m_edtUpdateLog = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     x + CW_Scale(156), y, CW_Scale(200), CW_Scale(30),
                                     page, (HMENU)IDC_PREFS_FILES_UPDATELOG, NULL, NULL);
    setControlFont(m_edtUpdateLog);
    m_btnUpdateLogBrowse = CreateWindowExA(0, "BUTTON", "Bro&wse...",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                           x + CW_Scale(362), y, CW_Scale(80), CW_Scale(30),
                                           page, (HMENU)IDC_PREFS_FILES_UPDATELOG_BROWSE, NULL, NULL);
    setControlFont(m_btnUpdateLogBrowse);
}

void CWPrefsDialog::createAdvancedPage(HWND page)
{
    int x = CW_Scale(16);
    int y = CW_Scale(16);
    const int rowStep = CW_Scale(44);

    m_chkScanOle2 = CreateWindowExA(0, "BUTTON", "Extract and scan O&LE2 (Office) objects",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_NOTIFY,
                                    x, y, CW_Scale(400), CW_Scale(22),
                                    page, (HMENU)IDC_PREFS_ADV_OLE2, NULL, NULL);
    setControlFont(m_chkScanOle2);
    enableNotifyStyle(m_chkScanOle2);
    y += rowStep;

    HWND lPriority = CreateWindowExA(0, "STATIC", "Scanner priori&ty:",
                                     WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                     x, y + CW_Scale(4), CW_Scale(130), CW_Scale(20),
                                     page, NULL, NULL, NULL);
    setControlFont(lPriority);

    m_cmbPriority = CreateWindowExA(0, "COMBOBOX", "",
                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT,
                                    x + CW_Scale(136), y, CW_Scale(120), CW_Scale(160),
                                    page, (HMENU)IDC_PREFS_ADV_PRIORITY, NULL, NULL);
    setControlFont(m_cmbPriority);
    configureThemedCombo(m_cmbPriority);
    SendMessageA(m_cmbPriority, CB_ADDSTRING, 0, (LPARAM)"Low");
    SendMessageA(m_cmbPriority, CB_ADDSTRING, 0, (LPARAM)"Normal");
}

void CWPrefsDialog::createPages()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    int pad = CW_Scale(12);
    int btnH = CW_Scale(30);
    int contentBottom = rc.bottom - pad - btnH - CW_Scale(10);
    int sidebarW = CW_Scale(170);

    int x = pad + sidebarW + CW_Scale(10);
    int y = pad;
    int w = rc.right - x - pad;
    int h = contentBottom - y;

    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        m_hwndPages[i] = CreateWindowExA(WS_EX_CONTROLPARENT, "STATIC", "",
                                         WS_CHILD | (i == 0 ? WS_VISIBLE : 0),
                                         x, y, w, h, m_hwnd,
                                         (HMENU)(INT_PTR)(5000 + i), NULL, NULL);
        if (m_hwndPages[i])
        {
            SetPropA(m_hwndPages[i], s_prefsPageOwnerProp, (HANDLE)this);
            m_pageOldProc[i] = (WNDPROC)SetWindowLongPtrA(
                m_hwndPages[i],
                GWLP_WNDPROC,
                (LONG_PTR)&CWPrefsDialog::pageSubclassProc);
        }
    }

    createGeneralPage(m_hwndPages[PAGE_GENERAL]);
    createFiltersPage(m_hwndPages[PAGE_FILTERS]);
    createUpdatesPage(m_hwndPages[PAGE_UPDATES]);
    createProxyPage(m_hwndPages[PAGE_PROXY]);
    createSchedulePage(m_hwndPages[PAGE_SCHEDULE]);
    createLimitsPage(m_hwndPages[PAGE_LIMITS]);
    createFilesPage(m_hwndPages[PAGE_FILES]);
    createAdvancedPage(m_hwndPages[PAGE_ADVANCED]);
}

int CWPrefsDialog::findPageIndex(HWND hwnd) const
{
    for (int i = 0; i < PAGE_COUNT; ++i)
    {
        if (m_hwndPages[i] == hwnd)
            return i;
    }
    return -1;
}

LRESULT CALLBACK CWPrefsDialog::pageSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CWPrefsDialog* self = (CWPrefsDialog*)GetPropA(hwnd, s_prefsPageOwnerProp);
    HWND parent = GetParent(hwnd);

    switch (msg)
    {
        case WM_COMMAND:
        case WM_DRAWITEM:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
        case WM_NOTIFY:
            if (self && parent)
                return SendMessageA(parent, msg, wp, lp);
            break;
    }

    if (self)
    {
        int idx = self->findPageIndex(hwnd);
        if (idx >= 0 && self->m_pageOldProc[idx])
            return CallWindowProcA(self->m_pageOldProc[idx], hwnd, msg, wp, lp);
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

LRESULT CALLBACK CWPrefsDialog::sidebarSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CWPrefsDialog* self = (CWPrefsDialog*)GetPropA(hwnd, s_prefsSidebarOwnerProp);
    if (!self)
        return DefWindowProcA(hwnd, msg, wp, lp);

    if (msg == WM_KEYDOWN && (wp == VK_RIGHT || wp == VK_TAB))
    {
        HWND page = NULL;
        if (self->m_activePage >= 0 && self->m_activePage < PAGE_COUNT)
            page = self->m_hwndPages[self->m_activePage];

        if (page)
        {
            HWND firstInPage = GetNextDlgTabItem(page, NULL, FALSE);
            if (firstInPage)
            {
                SetFocus(firstInPage);
                return 0;
            }
        }

        if (self->focusNextTabStop(hwnd, false))
            return 0;
    }

    if (msg == WM_SYSCHAR)
    {
        int page = findSidebarMnemonicPageIndex((char)wp);
        if (page >= 0)
        {
            SendMessageA(hwnd, LB_SETCURSEL, page, 0);
            self->showPage(page);
            return 0;
        }
    }

    if (msg == WM_SYSKEYDOWN)
    {
        int page = findSidebarMnemonicPageIndexFromVKey(wp);
        if (page >= 0)
        {
            SendMessageA(hwnd, LB_SETCURSEL, page, 0);
            self->showPage(page);
            return 0;
        }
    }

    if (self->m_sidebarOldProc)
        return CallWindowProcA(self->m_sidebarOldProc, hwnd, msg, wp, lp);

    return DefWindowProcA(hwnd, msg, wp, lp);
}

void CWPrefsDialog::createLayout()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    int pad = CW_Scale(12);
    int btnW = CW_Scale(110);
    int btnH = CW_Scale(30);

    m_hwndBtnCancel = CreateWindowExA(0, "BUTTON", "&Cancel",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                      rc.right - pad - btnW,
                                      rc.bottom - pad - btnH,
                                      btnW, btnH,
                                      m_hwnd, (HMENU)IDCANCEL, NULL, NULL);
    setControlFont(m_hwndBtnCancel);

    m_hwndBtnOk = CreateWindowExA(0, "BUTTON", "&Save",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
                                  rc.right - pad - btnW - CW_Scale(120),
                                  rc.bottom - pad - btnH,
                                  btnW, btnH,
                                  m_hwnd, (HMENU)IDOK, NULL, NULL);
    setControlFont(m_hwndBtnOk, true);

    createSidebar();
    createPages();
}

void CWPrefsDialog::showPage(int index)
{
    if (index < 0 || index >= PAGE_COUNT)
        return;

    m_activePage = index;
    for (int i = 0; i < PAGE_COUNT; ++i)
        ShowWindow(m_hwndPages[i], i == index ? SW_SHOW : SW_HIDE);

    InvalidateRect(m_hwndSidebar, NULL, TRUE);
}

int CWPrefsDialog::readIntFromEdit(HWND hwndEdit, int fallback) const
{
    if (!hwndEdit) return fallback;
    char buf[64] = {0};
    GetWindowTextA(hwndEdit, buf, sizeof(buf));
    int v = atoi(buf);
    return v;
}

void CWPrefsDialog::writeIntToEdit(HWND hwndEdit, int value) const
{
    if (!hwndEdit) return;
    char buf[64];
    wsprintfA(buf, "%d", value);
    SetWindowTextA(hwndEdit, buf);
}

void CWPrefsDialog::updateEnableStates()
{
    bool isQuarantine = getToggleChecked(m_radActionQuarantine);
    EnableWindow(m_edtQuarantine, isQuarantine);
    EnableWindow(m_btnQuarantineBrowse, isQuarantine);

    bool updEnabled = getToggleChecked(m_chkUpdateScheduled);
    EnableWindow(m_edtMirror, updEnabled);
    EnableWindow(m_cmbUpdateFrequency, updEnabled);
    EnableWindow(m_edtUpdateHour, updEnabled);
    EnableWindow(m_spinUpdateHour, updEnabled);
    EnableWindow(m_edtUpdateMinute, updEnabled);
    EnableWindow(m_spinUpdateMinute, updEnabled);
    EnableWindow(m_chkUpdateOnStartup, updEnabled);
    int ufreq = (int)SendMessageA(m_cmbUpdateFrequency, CB_GETCURSEL, 0, 0);
    EnableWindow(m_cmbUpdateDay, updEnabled && ufreq == 1);

    bool proxyOn = getToggleChecked(m_chkProxyEnabled);
    EnableWindow(m_edtProxyHost, proxyOn);
    EnableWindow(m_edtProxyPort, proxyOn);
    EnableWindow(m_edtProxyUser, proxyOn);
    EnableWindow(m_edtProxyPass, proxyOn);

    bool scanEnabled = getToggleChecked(m_chkScanScheduled);
    EnableWindow(m_cmbScanFrequency, scanEnabled);
    EnableWindow(m_edtScanHour, scanEnabled);
    EnableWindow(m_spinScanHour, scanEnabled);
    EnableWindow(m_edtScanMinute, scanEnabled);
    EnableWindow(m_spinScanMinute, scanEnabled);
    int sfreq = (int)SendMessageA(m_cmbScanFrequency, CB_GETCURSEL, 0, 0);
    EnableWindow(m_cmbScanDay, scanEnabled && sfreq == 1);

    bool archivesOn = getToggleChecked(m_chkScanArchives);
    EnableWindow(m_edtMaxScanSize, archivesOn);
    EnableWindow(m_edtMaxFiles, archivesOn);
    EnableWindow(m_edtMaxDepth, archivesOn);
}

void CWPrefsDialog::loadFromConfig()
{
    setToggleChecked(m_chkRecursive, m_cfg.scanRecursive);
    setToggleChecked(m_chkScanMail, m_cfg.scanMail);

    setToggleChecked(m_radActionReport, m_cfg.infectedAction == 0);
    setToggleChecked(m_radActionRemove, m_cfg.infectedAction == 1);
    setToggleChecked(m_radActionQuarantine, m_cfg.infectedAction == 2);
    InvalidateRect(m_radActionReport, NULL, TRUE);
    InvalidateRect(m_radActionRemove, NULL, TRUE);
    InvalidateRect(m_radActionQuarantine, NULL, TRUE);
    SetWindowTextA(m_edtQuarantine, m_cfg.quarantinePath.c_str());

    setToggleChecked(m_chkUpdateScheduled, m_cfg.updateScheduled);
    SetWindowTextA(m_edtMirror, m_cfg.dbMirror.c_str());
    SendMessageA(m_cmbUpdateFrequency, CB_SETCURSEL, m_cfg.updateFrequency, 0);
    CW_WriteHourToEdit(m_edtUpdateHour, m_cfg.updateHour);
    CW_WriteMinuteToEdit(m_edtUpdateMinute, m_cfg.updateMinute);
    SendMessageA(m_cmbUpdateDay, CB_SETCURSEL, m_cfg.scanDay, 0);
    setToggleChecked(m_chkUpdateOnStartup, m_cfg.updateOnStartup);
    setToggleChecked(m_chkCheckVersion, m_cfg.checkVersion);

    setToggleChecked(m_chkProxyEnabled, m_cfg.proxyEnabled);
    SetWindowTextA(m_edtProxyHost, m_cfg.proxyHost.c_str());
    writeIntToEdit(m_edtProxyPort, m_cfg.proxyPort);
    SetWindowTextA(m_edtProxyUser, m_cfg.proxyUser.c_str());
    SetWindowTextA(m_edtProxyPass, m_cfg.proxyPass.c_str());

    setToggleChecked(m_chkScanScheduled, m_cfg.scanScheduled);
    SendMessageA(m_cmbScanFrequency, CB_SETCURSEL, m_cfg.scanFrequency, 0);
    CW_WriteHourToEdit(m_edtScanHour, m_cfg.scanHour);
    CW_WriteMinuteToEdit(m_edtScanMinute, m_cfg.scanMinute);
    SendMessageA(m_cmbScanDay, CB_SETCURSEL, m_cfg.scanDay, 0);

    setToggleChecked(m_chkScanArchives, m_cfg.scanArchives);
    writeIntToEdit(m_edtMaxScanSize, m_cfg.maxScanSizeMb);
    writeIntToEdit(m_edtMaxFileSize, m_cfg.maxFileSizeMb);
    writeIntToEdit(m_edtMaxFiles, m_cfg.maxFiles);
    writeIntToEdit(m_edtMaxDepth, m_cfg.maxDepth);

    SetWindowTextA(m_edtDatabasePath, m_cfg.databasePath.c_str());
    SetWindowTextA(m_edtScanLog, m_cfg.scanLogFile.c_str());
    SetWindowTextA(m_edtUpdateLog, m_cfg.updateLogFile.c_str());

    populatePatternList(m_lstExclude, m_cfg.excludePatterns);
    populatePatternList(m_lstInclude, m_cfg.includePatterns);

    setToggleChecked(m_chkScanOle2, m_cfg.scanOle2);
    SendMessageA(m_cmbPriority, CB_SETCURSEL,
                 (m_cfg.priority == "l" || m_cfg.priority == "L") ? 0 : 1, 0);

    InvalidateRect(m_chkRecursive, NULL, TRUE);
    InvalidateRect(m_chkScanMail, NULL, TRUE);
    InvalidateRect(m_chkUpdateScheduled, NULL, TRUE);
    InvalidateRect(m_chkUpdateOnStartup, NULL, TRUE);
    InvalidateRect(m_chkCheckVersion, NULL, TRUE);
    InvalidateRect(m_chkProxyEnabled, NULL, TRUE);
    InvalidateRect(m_chkScanScheduled, NULL, TRUE);
    InvalidateRect(m_chkScanArchives, NULL, TRUE);
    InvalidateRect(m_chkScanOle2, NULL, TRUE);

    updateEnableStates();
}

bool CWPrefsDialog::saveToConfig()
{
    m_cfg.scanRecursive = getToggleChecked(m_chkRecursive);
    m_cfg.scanMail = getToggleChecked(m_chkScanMail);

    if (getToggleChecked(m_radActionRemove))
        m_cfg.infectedAction = 1;
    else if (getToggleChecked(m_radActionQuarantine))
        m_cfg.infectedAction = 2;
    else
        m_cfg.infectedAction = 0;

    char buf[CW_MAX_PATH];

    GetWindowTextA(m_edtQuarantine, buf, sizeof(buf));
    m_cfg.quarantinePath = buf;
    if (getToggleChecked(m_radActionQuarantine) && trimAscii(m_cfg.quarantinePath).empty())
    {
        showPage(PAGE_GENERAL);
        return showValidationError(m_hwnd, m_edtQuarantine, "Quarantine folder cannot be empty.");
    }

    m_cfg.updateScheduled = getToggleChecked(m_chkUpdateScheduled);
    GetWindowTextA(m_edtMirror, buf, sizeof(buf));
    m_cfg.dbMirror = buf;
    if (m_cfg.updateScheduled && trimAscii(m_cfg.dbMirror).empty())
    {
        showPage(PAGE_UPDATES);
        return showValidationError(m_hwnd, m_edtMirror, "Download site cannot be empty when scheduled updates are enabled.");
    }
    m_cfg.updateFrequency = (int)SendMessageA(m_cmbUpdateFrequency, CB_GETCURSEL, 0, 0);
    if (m_cfg.updateFrequency < 0)
    {
        showPage(PAGE_UPDATES);
        return showValidationError(m_hwnd, m_cmbUpdateFrequency, "Please select an update frequency.");
    }
    m_cfg.updateHour = CW_ReadHourFromEdit(m_edtUpdateHour, m_cfg.updateHour);
    m_cfg.updateMinute = readIntFromEdit(m_edtUpdateMinute, m_cfg.updateMinute);
    m_cfg.scanDay = (int)SendMessageA(m_cmbUpdateDay, CB_GETCURSEL, 0, 0);
    if (m_cfg.updateFrequency == 1 && m_cfg.scanDay < 0)
    {
        showPage(PAGE_UPDATES);
        return showValidationError(m_hwnd, m_cmbUpdateDay, "Please select an update day of week.");
    }
    m_cfg.updateOnStartup = getToggleChecked(m_chkUpdateOnStartup);
    m_cfg.checkVersion = getToggleChecked(m_chkCheckVersion);

    m_cfg.proxyEnabled = getToggleChecked(m_chkProxyEnabled);
    GetWindowTextA(m_edtProxyHost, buf, sizeof(buf));
    m_cfg.proxyHost = buf;
    m_cfg.proxyPort = readIntFromEdit(m_edtProxyPort, m_cfg.proxyPort);
    if (m_cfg.proxyEnabled)
    {
        if (trimAscii(m_cfg.proxyHost).empty())
        {
            showPage(PAGE_PROXY);
            return showValidationError(m_hwnd, m_edtProxyHost, "Proxy server cannot be empty when proxy is enabled.");
        }
        if (m_cfg.proxyPort < 0 || m_cfg.proxyPort > 65535)
        {
            showPage(PAGE_PROXY);
            return showValidationError(m_hwnd, m_edtProxyPort, "Proxy port must be between 0 and 65535.");
        }
    }
    GetWindowTextA(m_edtProxyUser, buf, sizeof(buf));
    m_cfg.proxyUser = buf;
    GetWindowTextA(m_edtProxyPass, buf, sizeof(buf));
    m_cfg.proxyPass = buf;

    m_cfg.scanScheduled = getToggleChecked(m_chkScanScheduled);
    m_cfg.scanFrequency = (int)SendMessageA(m_cmbScanFrequency, CB_GETCURSEL, 0, 0);
    if (m_cfg.scanFrequency < 0)
    {
        showPage(PAGE_SCHEDULE);
        return showValidationError(m_hwnd, m_cmbScanFrequency, "Please select a scan frequency.");
    }
    m_cfg.scanHour = CW_ReadHourFromEdit(m_edtScanHour, m_cfg.scanHour);
    m_cfg.scanMinute = readIntFromEdit(m_edtScanMinute, m_cfg.scanMinute);
    m_cfg.scanDay = (int)SendMessageA(m_cmbScanDay, CB_GETCURSEL, 0, 0);
    if (m_cfg.scanFrequency == 1 && m_cfg.scanDay < 0)
    {
        showPage(PAGE_SCHEDULE);
        return showValidationError(m_hwnd, m_cmbScanDay, "Please select a scan day of week.");
    }

    m_cfg.scanArchives = getToggleChecked(m_chkScanArchives);
    m_cfg.maxScanSizeMb = readIntFromEdit(m_edtMaxScanSize, m_cfg.maxScanSizeMb);
    m_cfg.maxFileSizeMb = readIntFromEdit(m_edtMaxFileSize, m_cfg.maxFileSizeMb);
    m_cfg.maxFiles = readIntFromEdit(m_edtMaxFiles, m_cfg.maxFiles);
    m_cfg.maxDepth = readIntFromEdit(m_edtMaxDepth, m_cfg.maxDepth);
    if (m_cfg.maxScanSizeMb < 0) m_cfg.maxScanSizeMb = 0;
    if (m_cfg.maxFileSizeMb < 0) m_cfg.maxFileSizeMb = 0;
    if (m_cfg.maxFiles < 0) m_cfg.maxFiles = 0;
    if (m_cfg.maxDepth < 0) m_cfg.maxDepth = 0;
    {
        CWLimitsValidationResult limits = CW_ValidateLimitsValues(
            m_cfg.maxFileSizeMb,
            m_cfg.scanArchives,
            m_cfg.maxScanSizeMb,
            m_cfg.maxFiles,
            m_cfg.maxDepth);

        if (!limits.ok)
        {
            HWND target = m_edtMaxFileSize;
            if (limits.field == CW_LIMITS_FIELD_MAX_SCAN_SIZE) target = m_edtMaxScanSize;
            if (limits.field == CW_LIMITS_FIELD_MAX_FILES) target = m_edtMaxFiles;
            if (limits.field == CW_LIMITS_FIELD_MAX_DEPTH) target = m_edtMaxDepth;

            showPage(PAGE_LIMITS);
            return showValidationError(m_hwnd, target, limits.message);
        }
    }

    GetWindowTextA(m_edtDatabasePath, buf, sizeof(buf));
    m_cfg.databasePath = buf;
    GetWindowTextA(m_edtScanLog, buf, sizeof(buf));
    m_cfg.scanLogFile = buf;
    GetWindowTextA(m_edtUpdateLog, buf, sizeof(buf));
    m_cfg.updateLogFile = buf;
    if (trimAscii(m_cfg.databasePath).empty())
    {
        showPage(PAGE_FILES);
        return showValidationError(m_hwnd, m_edtDatabasePath, "Virus database folder cannot be empty.");
    }
    if (trimAscii(m_cfg.scanLogFile).empty())
    {
        showPage(PAGE_FILES);
        return showValidationError(m_hwnd, m_edtScanLog, "Scan report file cannot be empty.");
    }
    if (trimAscii(m_cfg.updateLogFile).empty())
    {
        showPage(PAGE_FILES);
        return showValidationError(m_hwnd, m_edtUpdateLog, "Update report file cannot be empty.");
    }

    m_cfg.scanOle2 = getToggleChecked(m_chkScanOle2);
    int prioSel = (int)SendMessageA(m_cmbPriority, CB_GETCURSEL, 0, 0);
    if (prioSel < 0)
    {
        showPage(PAGE_ADVANCED);
        return showValidationError(m_hwnd, m_cmbPriority, "Please select scanner priority.");
    }
    m_cfg.priority = (prioSel == 0) ? "l" : "n";

    m_cfg.excludePatterns = joinPatternList(m_lstExclude);
    m_cfg.includePatterns = joinPatternList(m_lstInclude);

    if (m_cfg.updateHour < 0) m_cfg.updateHour = 0;
    if (m_cfg.updateHour > 23) m_cfg.updateHour = 23;
    if (m_cfg.updateMinute < 0) m_cfg.updateMinute = 0;
    if (m_cfg.updateMinute > 59) m_cfg.updateMinute = 59;
    if (m_cfg.scanHour < 0) m_cfg.scanHour = 0;
    if (m_cfg.scanHour > 23) m_cfg.scanHour = 23;
    if (m_cfg.scanMinute < 0) m_cfg.scanMinute = 0;
    if (m_cfg.scanMinute > 59) m_cfg.scanMinute = 59;

    if (!m_cfg.save())
        return showValidationError(m_hwnd, NULL, "Failed to save configuration.");

    return true;
}

void CWPrefsDialog::browseForFolder(HWND editTarget, const char* title)
{
    char current[CW_MAX_PATH] = {0};
    GetWindowTextA(editTarget, current, sizeof(current));

    BROWSEINFOA bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl)
    {
        char path[MAX_PATH] = {0};
        if (SHGetPathFromIDListA(pidl, path))
            SetWindowTextA(editTarget, path);

        IMalloc* pMalloc = NULL;
        if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
        {
            pMalloc->Free(pidl);
            pMalloc->Release();
        }
    }
}

void CWPrefsDialog::browseForFile(HWND editTarget, const char* title, const char* filter, bool saveDialog)
{
    char path[CW_MAX_PATH] = {0};
    GetWindowTextA(editTarget, path, sizeof(path));

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | (saveDialog ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    ofn.lpstrDefExt = "txt";

    BOOL ok = saveDialog ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    if (ok)
        SetWindowTextA(editTarget, path);
}

bool CWPrefsDialog::drawSidebarItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlID != IDC_PREFS_SIDEBAR)
        return false;
    if ((int)dis->itemID < 0)
        return true;

    CWTheme* theme = CW_GetTheme();
    if (!theme)
        return false;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    bool selected = (dis->itemState & ODS_SELECTED) != 0;

    HBRUSH fill = CreateSolidBrush(selected ? theme->colorAccent() : theme->colorSurface());
    FillRect(hdc, &rc, fill);
    DeleteObject(fill);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, selected ? RGB(255, 255, 255) : theme->colorText());
    HFONT oldFont = (HFONT)SelectObject(hdc, m_hFontBold);

    char text[128] = {0};
    SendMessageA(dis->hwndItem, LB_GETTEXT, dis->itemID, (LPARAM)text);
    RECT tr = rc;
    tr.left += CW_Scale(10);
    CW_DrawMnemonicTextAlways(hdc,
                              m_hFontBold,
                              tr,
                              text,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                              m_showMnemonics);

    SelectObject(hdc, oldFont);
    return true;
}

bool CWPrefsDialog::drawMnemonicStaticItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlType != ODT_STATIC)
        return false;

    CWTheme* theme = CW_GetTheme();
    if (!theme)
        return false;

    char text[256] = {0};
    GetWindowTextA(dis->hwndItem, text, sizeof(text));
    if (!strchr(text, '&'))
        return false;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    FillRect(hdc, &rc, theme->brushBg());

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme->colorText());

    HFONT hFont = (HFONT)SendMessageA(dis->hwndItem, WM_GETFONT, 0, 0);
    if (!hFont)
        hFont = m_hFont;

    CW_DrawMnemonicTextStaticLabel(hdc,
                                   hFont,
                                   rc,
                                   text,
                                   DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                                   m_showMnemonics);
    return true;
}

bool CWPrefsDialog::drawToggleItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlType != ODT_BUTTON || !isToggleControlId((int)dis->CtlID))
        return false;

    CW_DrawOwnerToggle(dis,
                       m_hFont,
                       isRadioControlId((int)dis->CtlID),
                       m_showMnemonics);
    return true;
}

bool CWPrefsDialog::drawComboItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlType != ODT_COMBOBOX || !isThemedComboId((int)dis->CtlID))
        return false;

    CWTheme* theme = CW_GetTheme();
    if (!theme)
        return false;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    UINT itemId = dis->itemID;
    if (itemId == (UINT)-1)
    {
        int sel = (int)SendMessageA(dis->hwndItem, CB_GETCURSEL, 0, 0);
        if (sel >= 0)
            itemId = (UINT)sel;
    }

    bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    bool selected = (dis->itemState & ODS_SELECTED) != 0;
    bool invalid = GetPropA(dis->hwndItem, s_invalidFieldProp) != NULL;

    COLORREF invalidBg = theme->isDark() ? RGB(68, 38, 38) : RGB(255, 235, 238);
    COLORREF bg = selected ? theme->colorAccent() : (invalid ? invalidBg : theme->colorSurface());
    COLORREF fg = selected ? RGB(255, 255, 255)
                           : (disabled ? theme->colorTextMuted() : theme->colorText());

    HBRUSH hbr = CreateSolidBrush(bg);
    FillRect(hdc, &rc, hbr);
    DeleteObject(hbr);

    if (itemId != (UINT)-1)
    {
        char text[256] = {0};
        SendMessageA(dis->hwndItem, CB_GETLBTEXT, (WPARAM)itemId, (LPARAM)text);

        RECT tr = rc;
        tr.left += CW_Scale(8);
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

bool CWPrefsDialog::onInit()
{
    SetWindowTextA(m_hwnd, "ClamWin Preferences");
    setDialogMnemonicCues(m_hwnd, false);
    SetTimer(m_hwnd, s_prefsMnemonicTimerId, 30, NULL);
    createFonts();
    createLayout();
    loadFromConfig();
    showPage(PAGE_GENERAL);
    SetFocus(m_hwndSidebar);
    return false;
}

bool CWPrefsDialog::onCommand(int id, HWND src)
{
    if (isToggleControlId(id))
    {
        if (isRadioControlId(id))
        {
            setToggleChecked(m_radActionReport, id == IDC_PREFS_GENERAL_ACT_REPORT);
            setToggleChecked(m_radActionRemove, id == IDC_PREFS_GENERAL_ACT_REMOVE);
            setToggleChecked(m_radActionQuarantine, id == IDC_PREFS_GENERAL_ACT_QUAR);
            InvalidateRect(m_radActionReport, NULL, TRUE);
            InvalidateRect(m_radActionRemove, NULL, TRUE);
            InvalidateRect(m_radActionQuarantine, NULL, TRUE);
        }
        else
        {
            HWND ctrl = src ? src : GetDlgItem(m_hwnd, id);
            if (ctrl)
            {
                bool checked = getToggleChecked(ctrl);
                setToggleChecked(ctrl, !checked);
                InvalidateRect(ctrl, NULL, TRUE);
            }
        }

        updateEnableStates();
        return true;
    }

    switch (id)
    {
        case IDC_PREFS_UPDATES_FREQ:
        case IDC_PREFS_SCHED_FREQ:
            updateEnableStates();
            return true;

        case IDC_PREFS_GENERAL_QUAR_BROWSE:
            browseForFolder(m_edtQuarantine, "Select quarantine folder");
            return true;

        case IDC_PREFS_FILES_DB_BROWSE:
            browseForFolder(m_edtDatabasePath, "Select virus database folder");
            return true;

        case IDC_PREFS_FILES_SCANLOG_BROWSE:
            browseForFile(m_edtScanLog, "Select scan report file", "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0", true);
            return true;

        case IDC_PREFS_FILES_UPDATELOG_BROWSE:
            browseForFile(m_edtUpdateLog, "Select update report file", "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0", true);
            return true;

        case IDC_PREFS_FILTERS_BTN_EXCL_ADD:
        {
            char buf[512] = {0};
            GetWindowTextA(m_edtExclPattern, buf, sizeof(buf));
            if (buf[0])
            {
                std::string padded = std::string("  ") + buf;
                if (SendMessageA(m_lstExclude, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)padded.c_str()) == LB_ERR)
                    SendMessageA(m_lstExclude, LB_ADDSTRING, 0, (LPARAM)padded.c_str());
            }
            SetWindowTextA(m_edtExclPattern, "");
            SetFocus(m_edtExclPattern);
            return true;
        }
        case IDC_PREFS_FILTERS_BTN_EXCL_REM:
        {
            int n = (int)SendMessageA(m_lstExclude, LB_GETSELCOUNT, 0, 0);
            if (n > 0)
            {
                std::vector<int> sel(n);
                SendMessageA(m_lstExclude, LB_GETSELITEMS, n, (LPARAM)sel.data());
                for (int i = n - 1; i >= 0; --i)
                    SendMessageA(m_lstExclude, LB_DELETESTRING, sel[i], 0);
            }
            return true;
        }
        case IDC_PREFS_FILTERS_BTN_INCL_ADD:
        {
            char buf[512] = {0};
            GetWindowTextA(m_edtInclPattern, buf, sizeof(buf));
            if (buf[0])
            {
                std::string padded = std::string("  ") + buf;
                if (SendMessageA(m_lstInclude, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)padded.c_str()) == LB_ERR)
                    SendMessageA(m_lstInclude, LB_ADDSTRING, 0, (LPARAM)padded.c_str());
            }
            SetWindowTextA(m_edtInclPattern, "");
            SetFocus(m_edtInclPattern);
            return true;
        }
        case IDC_PREFS_FILTERS_BTN_INCL_REM:
        {
            int n = (int)SendMessageA(m_lstInclude, LB_GETSELCOUNT, 0, 0);
            if (n > 0)
            {
                std::vector<int> sel(n);
                SendMessageA(m_lstInclude, LB_GETSELITEMS, n, (LPARAM)sel.data());
                for (int i = n - 1; i >= 0; --i)
                    SendMessageA(m_lstInclude, LB_DELETESTRING, sel[i], 0);
            }
            return true;
        }

        case IDC_PREFS_SCHED_DETAILS:
            CW_ScheduleDialogRun(m_hwnd, &m_cfg);
            return true;

        case IDOK:
            if (saveToConfig())
                endDialog(IDOK);
            return true;

        case IDCANCEL:
            endDialog(IDCANCEL);
            return true;
    }

    return false;
}

INT_PTR CWPrefsDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
#ifdef WM_DPICHANGED
        case WM_DPICHANGED:
        {
            struct UiState
            {
                bool valid;
                int activePage;

                bool scanRecursive;
                bool scanMail;
                int infectedAction;
                std::string quarantinePath;

                bool updateScheduled;
                std::string dbMirror;
                int updateFrequency;
                int updateHour;
                int updateMinute;
                int updateDay;
                bool updateOnStartup;
                bool checkVersion;

                bool proxyEnabled;
                std::string proxyHost;
                int proxyPort;
                std::string proxyUser;
                std::string proxyPass;

                bool scanScheduled;
                int scanFrequency;
                int scanHour;
                int scanMinute;
                int scanDay;

                bool scanArchives;
                int maxScanSizeMb;
                int maxFileSizeMb;
                int maxFiles;
                int maxDepth;

                std::string databasePath;
                std::string scanLogFile;
                std::string updateLogFile;

                bool scanOle2;
                int priority;

                UiState()
                    : valid(false)
                    , activePage(PAGE_GENERAL)
                    , scanRecursive(false)
                    , scanMail(false)
                    , infectedAction(0)
                    , updateScheduled(false)
                    , updateFrequency(0)
                    , updateHour(0)
                    , updateMinute(0)
                    , updateDay(0)
                    , updateOnStartup(false)
                    , checkVersion(false)
                    , proxyEnabled(false)
                    , proxyPort(0)
                    , scanScheduled(false)
                    , scanFrequency(0)
                    , scanHour(0)
                    , scanMinute(0)
                    , scanDay(0)
                    , scanArchives(false)
                    , maxScanSizeMb(0)
                    , maxFileSizeMb(0)
                    , maxFiles(0)
                    , maxDepth(0)
                    , scanOle2(false)
                    , priority(0)
                {
                }
            };

            auto readText = [](HWND hwnd) -> std::string
            {
                if (!hwnd)
                    return std::string();

                int len = GetWindowTextLengthA(hwnd);
                if (len <= 0)
                    return std::string();

                std::string text;
                text.resize((size_t)len);
                GetWindowTextA(hwnd, &text[0], len + 1);
                return text;
            };

            auto captureState = [&]() -> UiState
            {
                UiState state;
                state.valid = (m_chkRecursive != NULL && m_edtMirror != NULL && m_cmbScanFrequency != NULL);
                state.activePage = m_activePage;

                if (!state.valid)
                    return state;

                state.scanRecursive = getToggleChecked(m_chkRecursive);
                state.scanMail = getToggleChecked(m_chkScanMail);
                state.infectedAction = getToggleChecked(m_radActionRemove) ? 1 : (getToggleChecked(m_radActionQuarantine) ? 2 : 0);
                state.quarantinePath = readText(m_edtQuarantine);

                state.updateScheduled = getToggleChecked(m_chkUpdateScheduled);
                state.dbMirror = readText(m_edtMirror);
                state.updateFrequency = (int)SendMessageA(m_cmbUpdateFrequency, CB_GETCURSEL, 0, 0);
                state.updateHour = CW_ReadHourFromEdit(m_edtUpdateHour, 0);
                state.updateMinute = readIntFromEdit(m_edtUpdateMinute, 0);
                state.updateDay = (int)SendMessageA(m_cmbUpdateDay, CB_GETCURSEL, 0, 0);
                state.updateOnStartup = getToggleChecked(m_chkUpdateOnStartup);
                state.checkVersion = getToggleChecked(m_chkCheckVersion);

                state.proxyEnabled = getToggleChecked(m_chkProxyEnabled);
                state.proxyHost = readText(m_edtProxyHost);
                state.proxyPort = readIntFromEdit(m_edtProxyPort, 0);
                state.proxyUser = readText(m_edtProxyUser);
                state.proxyPass = readText(m_edtProxyPass);

                state.scanScheduled = getToggleChecked(m_chkScanScheduled);
                state.scanFrequency = (int)SendMessageA(m_cmbScanFrequency, CB_GETCURSEL, 0, 0);
                state.scanHour = CW_ReadHourFromEdit(m_edtScanHour, 0);
                state.scanMinute = readIntFromEdit(m_edtScanMinute, 0);
                state.scanDay = (int)SendMessageA(m_cmbScanDay, CB_GETCURSEL, 0, 0);

                state.scanArchives = getToggleChecked(m_chkScanArchives);
                state.maxScanSizeMb = readIntFromEdit(m_edtMaxScanSize, 0);
                state.maxFileSizeMb = readIntFromEdit(m_edtMaxFileSize, 0);
                state.maxFiles = readIntFromEdit(m_edtMaxFiles, 0);
                state.maxDepth = readIntFromEdit(m_edtMaxDepth, 0);

                state.databasePath = readText(m_edtDatabasePath);
                state.scanLogFile = readText(m_edtScanLog);
                state.updateLogFile = readText(m_edtUpdateLog);

                state.scanOle2 = getToggleChecked(m_chkScanOle2);
                state.priority = (int)SendMessageA(m_cmbPriority, CB_GETCURSEL, 0, 0);

                return state;
            };

            auto applyState = [&](const UiState& state)
            {
                if (!state.valid)
                {
                    loadFromConfig();
                    return;
                }

                setToggleChecked(m_chkRecursive, state.scanRecursive);
                setToggleChecked(m_chkScanMail, state.scanMail);
                setToggleChecked(m_radActionReport, state.infectedAction == 0);
                setToggleChecked(m_radActionRemove, state.infectedAction == 1);
                setToggleChecked(m_radActionQuarantine, state.infectedAction == 2);
                SetWindowTextA(m_edtQuarantine, state.quarantinePath.c_str());

                setToggleChecked(m_chkUpdateScheduled, state.updateScheduled);
                SetWindowTextA(m_edtMirror, state.dbMirror.c_str());
                SendMessageA(m_cmbUpdateFrequency, CB_SETCURSEL, state.updateFrequency, 0);
                writeIntToEdit(m_edtUpdateHour, state.updateHour);
                writeIntToEdit(m_edtUpdateMinute, state.updateMinute);
                SendMessageA(m_cmbUpdateDay, CB_SETCURSEL, state.updateDay, 0);
                setToggleChecked(m_chkUpdateOnStartup, state.updateOnStartup);
                setToggleChecked(m_chkCheckVersion, state.checkVersion);

                setToggleChecked(m_chkProxyEnabled, state.proxyEnabled);
                SetWindowTextA(m_edtProxyHost, state.proxyHost.c_str());
                writeIntToEdit(m_edtProxyPort, state.proxyPort);
                SetWindowTextA(m_edtProxyUser, state.proxyUser.c_str());
                SetWindowTextA(m_edtProxyPass, state.proxyPass.c_str());

                setToggleChecked(m_chkScanScheduled, state.scanScheduled);
                SendMessageA(m_cmbScanFrequency, CB_SETCURSEL, state.scanFrequency, 0);
                writeIntToEdit(m_edtScanHour, state.scanHour);
                writeIntToEdit(m_edtScanMinute, state.scanMinute);
                SendMessageA(m_cmbScanDay, CB_SETCURSEL, state.scanDay, 0);

                setToggleChecked(m_chkScanArchives, state.scanArchives);
                writeIntToEdit(m_edtMaxScanSize, state.maxScanSizeMb);
                writeIntToEdit(m_edtMaxFileSize, state.maxFileSizeMb);
                writeIntToEdit(m_edtMaxFiles, state.maxFiles);
                writeIntToEdit(m_edtMaxDepth, state.maxDepth);

                SetWindowTextA(m_edtDatabasePath, state.databasePath.c_str());
                SetWindowTextA(m_edtScanLog, state.scanLogFile.c_str());
                SetWindowTextA(m_edtUpdateLog, state.updateLogFile.c_str());

                setToggleChecked(m_chkScanOle2, state.scanOle2);
                SendMessageA(m_cmbPriority, CB_SETCURSEL, state.priority, 0);

                updateEnableStates();
            };

            UiState state = captureState();

            int newDpi = (int)HIWORD(wp);
            if (newDpi <= 0)
                newDpi = (int)LOWORD(wp);
            if (newDpi > 0)
                CW_SetScaleDpi(newDpi);
            else
                CW_ResetScaleDpi();

            if (lp)
            {
                RECT* suggested = (RECT*)lp;
                SetWindowPos(m_hwnd, NULL,
                             suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }

            if (m_hwndSidebar)
            {
                RemovePropA(m_hwndSidebar, s_prefsSidebarOwnerProp);
                m_sidebarOldProc = NULL;
                DestroyWindow(m_hwndSidebar);
                m_hwndSidebar = NULL;
            }

            for (int i = 0; i < PAGE_COUNT; ++i)
            {
                if (m_hwndPages[i])
                {
                    RemovePropA(m_hwndPages[i], s_prefsPageOwnerProp);
                    DestroyWindow(m_hwndPages[i]);
                    m_hwndPages[i] = NULL;
                    m_pageOldProc[i] = NULL;
                }
            }

            if (m_hwndBtnOk)
            {
                DestroyWindow(m_hwndBtnOk);
                m_hwndBtnOk = NULL;
            }
            if (m_hwndBtnCancel)
            {
                DestroyWindow(m_hwndBtnCancel);
                m_hwndBtnCancel = NULL;
            }

            createFonts();
            createLayout();
            applyState(state);

            int pageToShow = state.valid ? state.activePage : m_activePage;
            if (pageToShow < 0 || pageToShow >= PAGE_COUNT)
                pageToShow = PAGE_GENERAL;
            showPage(pageToShow);
            if (m_hwndSidebar)
                SendMessageA(m_hwndSidebar, LB_SETCURSEL, pageToShow, 0);

            InvalidateRect(m_hwnd, NULL, TRUE);
            return TRUE;
        }
#endif

        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lp;
            if (mis && mis->CtlID == IDC_PREFS_SIDEBAR)
            {
                mis->itemHeight = CW_Scale(36);
                return TRUE;
            }
            if (mis && isThemedComboId((int)mis->CtlID))
            {
                mis->itemHeight = CW_Scale(26);
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
            if (drawSidebarItem(dis))
                return TRUE;
            if (drawMnemonicStaticItem(dis))
                return TRUE;
            if (drawToggleItem(dis))
                return TRUE;
            if (drawComboItem(dis))
                return TRUE;
            break;
        }

        case WM_COMMAND:
        {
            int ctlId = (int)LOWORD(wp);
            int notif  = (int)HIWORD(wp);
            HWND ctlHwnd = (HWND)lp;

            if (notif == EN_CHANGE && ctlHwnd)
            {
                clearValidationMark(ctlHwnd);
            }
            else if ((notif == CBN_SELCHANGE || notif == LBN_SELCHANGE) && ctlHwnd)
            {
                clearValidationMark(ctlHwnd);
            }

            /* Owner-drawn toggles send BN_SETFOCUS / BN_KILLFOCUS via BS_NOTIFY.
               Only act on BN_CLICKED to prevent spurious state flips. */
            if (isToggleControlId(ctlId) && notif != BN_CLICKED)
                return FALSE;
            if (ctlId == IDC_PREFS_SIDEBAR && notif == LBN_SELCHANGE)
            {
                int sel = (int)SendMessageA(m_hwndSidebar, LB_GETCURSEL, 0, 0);
                showPage(sel);
                return TRUE;
            }
            break;
        }

        case WM_TIMER:
            if ((UINT_PTR)wp == s_prefsMnemonicTimerId)
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
            KillTimer(m_hwnd, s_prefsMnemonicTimerId);
            break;

        case WM_SYSCHAR:
        {
            /* Sidebar mnemonic fallback only when sidebar is focused,
               so panel-specific mnemonics keep priority elsewhere. */
            if (m_hwndSidebar && GetFocus() == m_hwndSidebar)
            {
                int page = findSidebarMnemonicPageIndex((char)wp);
                if (page >= 0)
                {
                    SendMessageA(m_hwndSidebar, LB_SETCURSEL, page, 0);
                    showPage(page);
                    return TRUE;
                }
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

        case WM_CTLCOLOREDIT:
        {
            HWND ctrl = (HWND)lp;
            if (ctrl && GetPropA(ctrl, s_invalidFieldProp) != NULL)
            {
                CWTheme* theme = CW_GetTheme();
                HDC hdc = (HDC)wp;
                SetTextColor(hdc, theme ? theme->colorText() : RGB(0, 0, 0));
                SetBkColor(hdc, theme && theme->isDark() ? RGB(68, 38, 38) : RGB(255, 235, 238));
                return (INT_PTR)getValidationEditBrush(theme);
            }
            break;
        }

        case WM_CTLCOLORLISTBOX:
        {
            HWND ctrl = (HWND)lp;
            if (ctrl && GetPropA(ctrl, s_invalidFieldProp) != NULL)
            {
                CWTheme* theme = CW_GetTheme();
                HDC hdc = (HDC)wp;
                SetTextColor(hdc, theme ? theme->colorText() : RGB(0, 0, 0));
                SetBkColor(hdc, theme && theme->isDark() ? RGB(68, 38, 38) : RGB(255, 235, 238));
                return (INT_PTR)getValidationEditBrush(theme);
            }
            break;
        }

        case WM_NOTIFY:
        {
            NMHDR* hdr = (NMHDR*)lp;
            if (hdr && hdr->code == UDN_DELTAPOS)
            {
                NMUPDOWN* nmud = (NMUPDOWN*)lp;
                int ctlId = (int)hdr->idFrom;
                if (ctlId == IDC_PREFS_UPDATES_SPIN_HOUR || ctlId == IDC_PREFS_SCHED_SPIN_HOUR)
                {
                    HWND edit = (ctlId == IDC_PREFS_UPDATES_SPIN_HOUR) ? m_edtUpdateHour : m_edtScanHour;
                    int h = CW_ReadHourFromEdit(edit, 0);
                    h = (h + nmud->iDelta + 24) % 24;
                    CW_WriteHourToEdit(edit, h);
                    return 0;
                }
                if (ctlId == IDC_PREFS_UPDATES_SPIN_MIN || ctlId == IDC_PREFS_SCHED_SPIN_MIN)
                {
                    HWND edit = (ctlId == IDC_PREFS_UPDATES_SPIN_MIN) ? m_edtUpdateMinute : m_edtScanMinute;
                    char mbuf[8] = {0};
                    GetWindowTextA(edit, mbuf, sizeof(mbuf));
                    int m = atoi(mbuf);
                    m = (m + nmud->iDelta + 60) % 60;
                    CW_WriteMinuteToEdit(edit, m);
                    return 0;
                }
            }
            if (hdr && hdr->code == NM_CUSTOMDRAW && isCheckOrRadio(hdr->hwndFrom))
            {
                NMCUSTOMDRAW* cd = (NMCUSTOMDRAW*)lp;
                if (cd->dwDrawStage == CDDS_PREPAINT)
                {
                    CWTheme* theme = CW_GetTheme();
                    if (theme)
                    {
                        SetTextColor(cd->hdc, theme->colorText());
                        SetBkColor(cd->hdc, theme->colorBg());
                    }
                    return CDRF_DODEFAULT;
                }
            }
            break;
        }
    }

    return CWDialog::handleMessage(msg, wp, lp);
}

/* ─── C Wrapper ─────────────────────────────────────────────── */
int CW_PrefsDialogRun(HWND hwndParent, CWConfig *cfg)
{
    CWPrefsDialog dlg(*cfg);
    return (int)dlg.runModal(hwndParent, 680, 440);
}

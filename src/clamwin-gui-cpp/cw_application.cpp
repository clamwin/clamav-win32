/*
 * ClamWin Free Antivirus — CWApplication implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_application.h"
#include "cw_dashboard.h"
#include "cw_dashboard.h"
#include "cw_cli_args.h"
#include "cw_gui_shared.h"
#include "cw_dpi.h"
#include "cw_theme.h"

#include <string.h>

CWApplication* CWApplication::s_instance = NULL;

static const WCHAR* s_reportScanText = L"&Virus Scan Report";
static const WCHAR* s_reportUpdateText = L"Virus &Database Update Report";
static const WCHAR* s_menuSeparatorMarker = L"__CW_MENU_SEPARATOR__";
static const UINT_PTR CW_TRAY_RETRY_TIMER_ID = 43;
static const UINT_PTR CW_VERSION_CHECK_TIMER_ID = 44;
static const char* s_reportsPopupClass = "ClamWinDarkReportsMenu";

struct CWReportsPopupState
{
    HWND owner;
    HFONT font;
    bool hasScan;
    bool hasUpdate;
    int hover;
    int width;
    int itemHeight;
    int padX;
};

static bool CW_UseCustomDarkReportsMenu()
{
    CWTheme* theme = CW_GetTheme();
    return theme && theme->isDark() && !theme->useClassicPalette();
}

static void CW_ApplyRoundedPopupRegion(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0)
        return;

    int radius = CW_Scale(12);
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius, radius);
    if (rgn)
        SetWindowRgn(hwnd, rgn, TRUE);
}

static int CW_ReportsPopupItemFromY(const CWReportsPopupState* s, int y)
{
    if (!s)
        return -1;
    if (y < 0 || y >= (s->itemHeight * 2))
        return -1;
    return (y / s->itemHeight);
}

static bool CW_ReportsPopupItemEnabled(const CWReportsPopupState* s, int idx)
{
    if (!s)
        return false;
    if (idx == 0)
        return s->hasScan;
    if (idx == 1)
        return s->hasUpdate;
    return false;
}

static UINT CW_ReportsPopupItemCommand(int idx)
{
    return idx == 0 ? IDM_TRAY_SCANREPORT : IDM_TRAY_UPDATEREPORT;
}

static WCHAR CW_FindMnemonicChar(const WCHAR* text)
{
    if (!text)
        return 0;

    for (int i = 0; text[i] != 0; ++i)
    {
        if (text[i] == L'&')
        {
            if (text[i + 1] == L'&')
            {
                ++i;
                continue;
            }
            if (text[i + 1] != 0)
            {
                WCHAR ch = text[i + 1];
                if (ch >= L'A' && ch <= L'Z')
                    ch = (WCHAR)(ch - L'A' + L'a');
                return ch;
            }
        }
    }

    return 0;
}

static void CW_DrawReportsPopup(HWND hwnd, HDC hdc, CWReportsPopupState* s)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF bg = theme ? theme->colorSurface() : RGB(45, 45, 45);
    COLORREF fg = theme ? theme->colorText() : RGB(230, 230, 230);
    COLORREF fgMuted = theme ? theme->colorTextMuted() : RGB(150, 150, 150);
    COLORREF hover = theme ? RGB(68, 68, 68) : RGB(70, 70, 70);

    RECT rc;
    GetClientRect(hwnd, &rc);
    const bool showMnemonics = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    HBRUSH hBg = CreateSolidBrush(bg);
    FillRect(hdc, &rc, hBg);
    DeleteObject(hBg);

    for (int i = 0; i < 2; ++i)
    {
        RECT itemRc;
        itemRc.left = 0;
        itemRc.right = rc.right;
        itemRc.top = i * s->itemHeight;
        itemRc.bottom = itemRc.top + s->itemHeight;

        const bool enabled = CW_ReportsPopupItemEnabled(s, i);
        if (i == s->hover && enabled)
        {
            HBRUSH hHover = CreateSolidBrush(hover);
            FillRect(hdc, &itemRc, hHover);
            DeleteObject(hHover);
        }

        RECT textRc = itemRc;
        textRc.left += s->padX;
        textRc.right -= s->padX;
        HGDIOBJ oldFont = SelectObject(hdc, s->font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, enabled ? fg : fgMuted);
        DrawTextW(hdc, i == 0 ? s_reportScanText : s_reportUpdateText,
                  -1, &textRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | (showMnemonics ? 0 : DT_HIDEPREFIX));
        if (oldFont)
            SelectObject(hdc, oldFont);
    }
}

static LRESULT CALLBACK CW_ReportsPopupProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CWReportsPopupState* s = reinterpret_cast<CWReportsPopupState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTA* cs = reinterpret_cast<CREATESTRUCTA*>(lp);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }

        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;

        case WM_SIZE:
            CW_ApplyRoundedPopupRegion(hwnd);
            return 0;

        case WM_PAINT:
            if (s)
            {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                CW_DrawReportsPopup(hwnd, ps.hdc, s);
                EndPaint(hwnd, &ps);
                return 0;
            }
            break;

        case WM_MOUSEMOVE:
            if (s)
            {
                int idx = CW_ReportsPopupItemFromY(s, (int)(short)HIWORD(lp));
                if (idx >= 0 && !CW_ReportsPopupItemEnabled(s, idx))
                    idx = -1;
                if (idx != s->hover)
                {
                    s->hover = idx;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;

        case WM_LBUTTONUP:
            if (s)
            {
                int idx = CW_ReportsPopupItemFromY(s, (int)(short)HIWORD(lp));
                if (idx >= 0 && CW_ReportsPopupItemEnabled(s, idx))
                    PostMessageA(s->owner, WM_COMMAND, CW_ReportsPopupItemCommand(idx), 0);
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_MENU)
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (wp == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
                return 0;
            }
            break;

        case WM_SYSKEYDOWN:
            if (wp == VK_MENU)
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;

        case WM_KEYUP:
            if (wp == VK_MENU)
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;

        case WM_SYSKEYUP:
            if (wp == VK_MENU)
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;

        case WM_SYSCHAR:
            if (s)
            {
                WCHAR key = (WCHAR)wp;
                if (key >= L'A' && key <= L'Z')
                    key = (WCHAR)(key - L'A' + L'a');

                for (int i = 0; i < 2; ++i)
                {
                    const WCHAR* text = (i == 0) ? s_reportScanText : s_reportUpdateText;
                    if (!CW_ReportsPopupItemEnabled(s, i))
                        continue;
                    if (CW_FindMnemonicChar(text) == key)
                    {
                        PostMessageA(s->owner, WM_COMMAND, CW_ReportsPopupItemCommand(i), 0);
                        DestroyWindow(hwnd);
                        return 0;
                    }
                }
            }
            return 0;

        case WM_KILLFOCUS:
            if (wp == 0)
                return 0;
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            if (s)
            {
                if (s->font && s->font != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
                    DeleteObject(s->font);
                delete s;
                SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool CW_EnsureReportsPopupClass(HINSTANCE hInst)
{
    WNDCLASSA wc;
    if (GetClassInfoA(hInst, s_reportsPopupClass, &wc))
        return true;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = CW_ReportsPopupProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = s_reportsPopupClass;
    return RegisterClassA(&wc) != 0;
}

static bool CW_ShowCustomDarkReportsMenu(HWND owner, bool hasScanReport, bool hasUpdateReport)
{
    HINSTANCE hInst = (HINSTANCE)GetModuleHandleA(NULL);
    if (!CW_EnsureReportsPopupClass(hInst))
        return false;

    CWReportsPopupState* s = new CWReportsPopupState();
    memset(s, 0, sizeof(*s));
    s->owner = owner;
    s->hasScan = hasScanReport;
    s->hasUpdate = hasUpdateReport;
    s->hover = -1;
    s->width = CW_Scale(300);
    s->itemHeight = CW_Scale(40);
    s->padX = CW_Scale(20);

    NONCLIENTMETRICSW ncm;
    memset(&ncm, 0, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        ncm.lfMenuFont.lfHeight = -CW_Scale(13);
        ncm.lfMenuFont.lfWeight = FW_NORMAL;
        s->font = CreateFontIndirectW(&ncm.lfMenuFont);
    }
    if (!s->font)
        s->font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    int height = s->itemHeight * 2;
    POINT pt;
    GetCursorPos(&pt);
    int x = pt.x - s->width + CW_Scale(8);
    int y = pt.y - height + CW_Scale(8);

    RECT wa;
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0))
    {
        if (x < wa.left) x = wa.left;
        if (y < wa.top) y = wa.top;
        if (x + s->width > wa.right) x = wa.right - s->width;
        if (y + height > wa.bottom) y = wa.bottom - height;
    }

    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                s_reportsPopupClass,
                                "",
                                WS_POPUP,
                                x, y, s->width, height,
                                owner,
                                NULL,
                                hInst,
                                s);
    if (!hwnd)
    {
        if (s->font && s->font != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(s->font);
        delete s;
        return false;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    CW_ApplyRoundedPopupRegion(hwnd);
    SetForegroundWindow(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    return true;
}

static void CW_DrawThemedMenuItem(const DRAWITEMSTRUCT* dis)
{
    if (!dis)
        return;

    const WCHAR* text = reinterpret_cast<const WCHAR*>(dis->itemData);
    if (!text)
        text = L"";

    const bool isSeparator = (lstrcmpW(text, s_menuSeparatorMarker) == 0);
    CWTheme* theme = CW_GetTheme();
    const bool selected = ((dis->itemState & ODS_SELECTED) != 0) && !isSeparator;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;

    COLORREF bgColor = RGB(255, 255, 255);
    COLORREF textColor = RGB(32, 32, 32);

    if (theme)
    {
        /* Win11-like: flat surface with clearer selected contrast in dark mode. */
        bgColor = selected ? (theme->isDark() ? RGB(78, 78, 78) : RGB(56, 56, 56))
                           : theme->colorSurface();
        if (disabled)
            textColor = theme->isDark() ? RGB(180, 180, 180) : theme->colorTextMuted();
        else if (selected)
            textColor = RGB(245, 245, 245);
        else
            textColor = theme->colorText();
    }
    else if (disabled)
    {
        textColor = RGB(128, 128, 128);
    }
    else if (selected)
    {
        textColor = RGB(245, 245, 245);
    }

    HBRUSH hBg = CreateSolidBrush(bgColor);
    FillRect(dis->hDC, &dis->rcItem, hBg);
    DeleteObject(hBg);

    if (isSeparator)
    {
        if (theme && theme->isDark())
        {
            /* Dark mode: spacing-only separators to avoid bright dividing lines. */
            return;
        }

        RECT lineRc = dis->rcItem;
        const int inset = CW_Scale(10);
        const int midY = (lineRc.top + lineRc.bottom) / 2;
        COLORREF lineColor = theme ? theme->colorTextMuted() : RGB(96, 96, 96);
        HPEN hPen = CreatePen(PS_SOLID, 1, lineColor);
        if (hPen)
        {
            HGDIOBJ oldPen = SelectObject(dis->hDC, hPen);
            MoveToEx(dis->hDC, lineRc.left + inset, midY, NULL);
            LineTo(dis->hDC, lineRc.right - inset, midY);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(hPen);
        }
        return;
    }

    RECT textRc = dis->rcItem;
    const int padX = CW_Scale(16);
    textRc.left += padX;
    textRc.right -= CW_Scale(12);
    const bool showMnemonics = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, textColor);
    DrawTextW(dis->hDC,
              text,
              -1,
              &textRc,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT | (showMnemonics ? 0 : DT_HIDEPREFIX));
}

static void CW_MeasureThemedMenuItem(MEASUREITEMSTRUCT* mis)
{
    if (!mis)
        return;

    const WCHAR* text = reinterpret_cast<const WCHAR*>(mis->itemData);
    if (!text)
        text = L"";

    if (lstrcmpW(text, s_menuSeparatorMarker) == 0)
    {
        mis->itemWidth = (UINT)CW_Scale(180);
        {
            CWTheme* theme = CW_GetTheme();
            mis->itemHeight = (UINT)(theme && theme->isDark() ? CW_Scale(10) : CW_Scale(14));
        }
        return;
    }

    HDC hdc = GetDC(NULL);
    SIZE sz;
    sz.cx = 0;
    sz.cy = 18;

    HFONT hMenuFont = NULL;
    HFONT hOldFont = NULL;
    NONCLIENTMETRICSW ncm;
    memset(&ncm, 0, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);

    if (hdc)
    {
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            hMenuFont = CreateFontIndirectW(&ncm.lfMenuFont);
            if (hMenuFont)
                hOldFont = (HFONT)SelectObject(hdc, hMenuFont);
        }

        GetTextExtentPoint32W(hdc, text, lstrlenW(text), &sz);

        if (hOldFont)
            SelectObject(hdc, hOldFont);
        if (hMenuFont)
            DeleteObject(hMenuFont);
        ReleaseDC(NULL, hdc);
    }

    {
        const LONG textWidth = sz.cx;
        const LONG textHeight = sz.cy;
        /* Match modern context menu spacing without classic checkmark gutter. */
        mis->itemWidth = (UINT)(textWidth + CW_Scale(36));

        {
            const UINT desiredHeight = (UINT)(textHeight + CW_Scale(16));
            const UINT minHeight = (UINT)CW_Scale(34);
            mis->itemHeight = (desiredHeight > minHeight) ? desiredHeight : minHeight;
        }
    }
}

CWApplication::CWApplication()
    : m_hInst(NULL)
    , m_hwndTray(NULL)
    , m_dash(m_config)
    , m_hIcon(NULL)
    , m_taskbarCreatedMsg(0)
{
    s_instance = this;
}

CWApplication::~CWApplication()
{
    s_instance = NULL;
}

/* ─── run ───────────────────────────────────────────────────── */

int CWApplication::run(HINSTANCE hInst, LPSTR cmdLine)
{
    m_hInst = hInst;

    CWCliArgs cli;
    CW_ParseCommandLineArgs(cmdLine, cli);

    std::string startupConfigPath;
    if (cli.hasSwitches)
    {
        startupConfigPath = cli.configFile;
    }
    else if (cmdLine && cmdLine[0])
    {
        startupConfigPath = cmdLine;
    }

    if (cli.hasSwitches)
    {
        bool handledCliMode =
            _stricmp(cli.mode.c_str(), "scanner") == 0 ||
            _stricmp(cli.mode.c_str(), "updater") == 0 ||
            _stricmp(cli.mode.c_str(), "update") == 0 ||
            _stricmp(cli.mode.c_str(), "viewlog") == 0;

        if (!handledCliMode)
            goto normal_startup;

        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
        InitCommonControlsEx(&icc);

        LoadLibraryA("riched20.dll");
        CoInitialize(NULL);

        if (!cli.configFile.empty())
            m_config.load(cli.configFile);
        else
            m_config.load();

        CW_ThemeInit();

        int cliRc = 0;
        if (_stricmp(cli.mode.c_str(), "scanner") == 0)
        {
            const char* target = cli.paths.empty() ? "" : cli.paths[0].c_str();
            cliRc = CW_ScanDialogRun(NULL, &m_config, target);
        }
        else if (_stricmp(cli.mode.c_str(), "updater") == 0 ||
                 _stricmp(cli.mode.c_str(), "update") == 0)
        {
            cliRc = CW_UpdateDialogRun(NULL, &m_config);
        }
        else if (_stricmp(cli.mode.c_str(), "viewlog") == 0)
        {
            const char* logPath = cli.paths.empty() ? m_config.scanLogFile.c_str() : cli.paths[0].c_str();
            CW_LogViewerRun(NULL, logPath, "ClamWin Report");
            cliRc = 0;
        }

        CW_ThemeDeinit();
        CoUninitialize();
        return cliRc;
    }

normal_startup:

    /* Single-instance check */
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "ClamWinMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        HWND hExisting = FindWindowA("ClamWinTrayClass", NULL);
        if (hExisting) PostMessageA(hExisting, WM_COMMAND, IDM_TRAY_OPEN, 0);
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    /* Init common controls */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icc);

    LoadLibraryA("riched20.dll");
    CoInitialize(NULL);

    /* Load config */
    if (!startupConfigPath.empty())
        m_config.load(startupConfigPath);
    else
        m_config.load();

    /* Init Theme before any windows are created */
    CW_ThemeInit();

    /* Create hidden tray window */
    if (!registerHiddenClass() || !createHiddenWindow())
    {
        CoUninitialize();
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    m_taskbarCreatedMsg = RegisterWindowMessageA("TaskbarCreated");

    /* Create tray icon */
    createTray();

    /* Start scheduler */
    m_scheduler.start(m_hwndTray, &m_config);

    /* Run on startup update if configured */
    if (m_config.updateOnStartup)
        PostMessageA(m_hwndTray, WM_COMMAND, IDM_TRAY_UPDATE, 0);

    /* Schedule a delayed version check (randomised 2-10 min after startup)
     * to avoid hitting the API at launch and to spread server load. */
    if (m_config.checkVersion)
    {
        srand((unsigned)GetTickCount());
        UINT delayMs = 120000 + (rand() % 480000);  /* 2–10 minutes */

        /* Test override: set CLAMWIN_UPDATE_CHECK_DELAY_MS to force startup delay.
         * Useful for deterministic local testing without waiting. */
        char delayBuf[32] = {0};
        DWORD got = GetEnvironmentVariableA("CLAMWIN_UPDATE_CHECK_DELAY_MS", delayBuf, sizeof(delayBuf));
        if (got > 0 && got < sizeof(delayBuf))
        {
            long v = atol(delayBuf);
            if (v >= 0)
                delayMs = (UINT)v;
        }

        SetTimer(m_hwndTray, CW_VERSION_CHECK_TIMER_ID, delayMs, NULL);
    }

    /* Message loop */
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    m_scheduler.stop();
    m_updateChecker.waitForThread();
    m_tray.destroy();
    CW_ThemeDeinit();
    CoUninitialize();
    if (hMutex) CloseHandle(hMutex);
    return (int)msg.wParam;
}

/* ─── Hidden window registration ────────────────────────────── */

bool CWApplication::registerHiddenClass()
{
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = staticWndProc;
    wc.hInstance     = m_hInst;
    wc.lpszClassName = "ClamWinTrayClass";
    return RegisterClassA(&wc) != 0;
}

bool CWApplication::createHiddenWindow()
{
    m_hwndTray = CreateWindowExA(0, "ClamWinTrayClass", "ClamWin",
                                  WS_OVERLAPPEDWINDOW,
                                  0, 0, 0, 0,
                                  NULL, NULL, m_hInst,
                                  static_cast<LPVOID>(this));
    return m_hwndTray != NULL;
}

/* ─── Tray ──────────────────────────────────────────────────── */

void CWApplication::createTray()
{
    m_hIcon = LoadIconA(m_hInst, MAKEINTRESOURCEA(IDI_CLAMWIN));
    if (m_tray.create(m_hwndTray, m_hIcon, "ClamWin"))
    {
        KillTimer(m_hwndTray, CW_TRAY_RETRY_TIMER_ID);
        updateTrayTip();
    }
    else
    {
        /* Explorer shell can be late on legacy systems; retry until it is ready. */
        SetTimer(m_hwndTray, CW_TRAY_RETRY_TIMER_ID, 1500, NULL);
    }
}

void CWApplication::destroyTray()
{
    m_tray.destroy();
}

void CWApplication::updateTrayTip()
{
    CW_ProtectionStatus status = CW_GetProtectionStatus(&m_config);
    const char* tip;
    switch (status)
    {
        case CW_STATUS_OK:   tip = "ClamWin - Protected";             break;
        case CW_STATUS_WARN: tip = "ClamWin - Database may be outdated"; break;
        default:             tip = "ClamWin - Database not found";    break;
    }
    m_tray.setIcon(m_hIcon, tip);
}

/* ─── Balloon tip helper ────────────────────────────────────── */

void CWApplication::showBalloonNotify(const char* msg, DWORD flags)
{
    if (!m_config.trayNotify)
        return;
    m_tray.showBalloon("ClamWin Free Antivirus", msg, flags);
}

/* ─── Actions ───────────────────────────────────────────────── */

/* ─── dialogParent / hideDashForModal / restoreDash ──────────── */

/* ─── dialogParent / hideDashForModal / restoreDash ──────────── */

HWND CWApplication::dialogParent() const
{
    /* Use dashboard as modal owner if visible, so dialogs stack on top of it.
     * Fall back to tray hidden window if dashboard is closed. */
    HWND dash = activeDash();
    return dash ? dash : m_hwndTray;
}

void CWApplication::hideDashForModal()
{
    /* No-op — user prefers dialogs to stack over the visible dashboard */
}

void CWApplication::restoreDash()
{
    /* No-op */
}

void CWApplication::doOpenDashboard()
{
    /* m_dash is persistent; if it doesn't have an HWND yet, create it.
     * Otherwise just show it and bring it to the front. */
    if (!m_dash.hwnd())
        m_dash.open(m_hwndTray);

    ShowWindow(m_dash.hwnd(), SW_SHOW);
    SetForegroundWindow(m_dash.hwnd());
}

void CWApplication::doHelp()
{
    ShellExecuteA(dialogParent(), "open", CLAMWIN_WEBSITE, NULL, NULL, SW_SHOWNORMAL);
}

void CWApplication::onVersionCheckResult(WPARAM wp, LPARAM lp)
{
    CWVersionResult* result = reinterpret_cast<CWVersionResult*>(lp);
    if (!result)
        return;

    if (result->available)
    {
        /* Notify the dashboard so it can show an update banner */
        m_dash.setUpdateAvailable(result->versionStr);

        /* Tray balloon — clicking the balloon opens the dashboard */
        char balloon[256];
        _snprintf(balloon, sizeof(balloon),
                  "ClamWin %s is available.\n"
                  "Click here for details.",
                  result->versionStr);
        balloon[sizeof(balloon) - 1] = '\0';
        showBalloonNotify(balloon, NIIF_INFO);
    }

    delete result;
}

void CWApplication::doScan()
{
    BROWSEINFOA bi;
    char path[MAX_PATH] = "";
    memset(&bi, 0, sizeof(bi));
    bi.hwndOwner      = dialogParent();
    bi.pszDisplayName = path;
    bi.lpszTitle      = "Select a folder to scan:";
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl)
    {
        if (SHGetPathFromIDListA(pidl, path))
        {
            int rc = CW_ScanDialogRun(dialogParent(), &m_config, path);
            if (rc == 1)
                showBalloonNotify(
                    "Virus has been detected during scan! Please review the scan report.",
                    NIIF_ERROR);
            else if (rc != 0 && rc != -1)
                showBalloonNotify(
                    "An error occurred during scan. Please review the scan report.",
                    NIIF_WARNING);
            updateTrayTip();
        }
        CoTaskMemFree(pidl);
    }
}

void CWApplication::doScanMemory()
{
    int rc = CW_ScanMemoryDialogRun(dialogParent(), &m_config);
    if (rc == 1)
        showBalloonNotify(
            "Virus has been detected in memory! Please review the scan report.",
            NIIF_ERROR);
    else if (rc != 0 && rc != -1)
        showBalloonNotify(
            "An error occurred during memory scan. Please review the scan report.",
            NIIF_WARNING);
    updateTrayTip();
}

void CWApplication::doUpdate()
{
    int rc = CW_UpdateDialogRun(dialogParent(), &m_config);
    if (rc == CW_UPDATE_RC_SUCCESS || rc == CW_UPDATE_RC_NO_CHANGES)
    {
        /* Successful DB update: refresh dashboard banner immediately. */
        m_dash.refreshStatus();
        if (rc == CW_UPDATE_RC_SUCCESS)
            showBalloonNotify("Virus database has been updated.", NIIF_INFO);
    }
    else if (rc != CW_UPDATE_RC_CANCELLED)
    {
        showBalloonNotify(
            "An error occurred during Virus Database Update. "
            "Please review the update report.",
            NIIF_WARNING);
    }
    updateTrayTip();
}

void CWApplication::doPreferences()
{
    if (CW_PrefsDialogRun(dialogParent(), &m_config))
    {
        m_config.save();
        m_scheduler.stop();
        m_scheduler.start(m_hwndTray, &m_config);
        updateTrayTip();
    }
}

void CWApplication::doScheduledScan()
{
    const char* path = m_config.scanPath.empty()
                     ? m_config.databasePath.c_str()
                     : m_config.scanPath.c_str();

    /* Notify user that a scheduled task is starting */
    {
        char balloon[256];
        const char* desc = m_config.scanDescription.empty()
                         ? "Scheduled Scan"
                         : m_config.scanDescription.c_str();
        _snprintf(balloon, sizeof(balloon), "Running Scheduled Task:\n%s", desc);
        balloon[sizeof(balloon) - 1] = '\0';
        showBalloonNotify(balloon, NIIF_INFO);
    }

    int rc;
    if (m_config.scanMemory)
        rc = CW_ScanMemoryDialogRun(dialogParent(), &m_config);
    else
        rc = CW_ScanDialogRun(dialogParent(), &m_config, path);

    if (rc == 1)
        showBalloonNotify(
            "Virus has been detected during scheduled scan! "
            "Please review the scan report.",
            NIIF_ERROR);
    else if (rc != 0 && rc != -1)
        showBalloonNotify(
            "An error occurred during scheduled scan. "
            "Please review the scan report.",
            NIIF_WARNING);

    updateTrayTip();
}

void CWApplication::doScheduledUpdate()
{
    showBalloonNotify("Running Scheduled Task:\nVirus Database Update", NIIF_INFO);

    int rc = CW_UpdateDialogRun(dialogParent(), &m_config);
    if (rc == CW_UPDATE_RC_SUCCESS || rc == CW_UPDATE_RC_NO_CHANGES)
    {
        m_dash.refreshStatus();
        if (rc == CW_UPDATE_RC_SUCCESS)
            showBalloonNotify("Virus database has been updated.", NIIF_INFO);
    }
    else if (rc != CW_UPDATE_RC_CANCELLED)
    {
        showBalloonNotify(
            "An error occurred during Scheduled Virus Database Update. "
            "Please review the update report.",
            NIIF_WARNING);
    }
    updateTrayTip();
}

void CWApplication::doSchedule()
{
    if (CW_ScheduleDialogRun(dialogParent(), &m_config))
    {
        m_config.save();
        m_scheduler.stop();
        m_scheduler.start(m_hwndTray, &m_config);
    }
}

void CWApplication::doScanReport()
{
    if (!m_config.scanLogFile.empty())
        CW_LogViewerRun(dialogParent(), m_config.scanLogFile.c_str(), "Virus Scan Report");
}

void CWApplication::doUpdateReport()
{
    if (!m_config.updateLogFile.empty())
        CW_LogViewerRun(dialogParent(), m_config.updateLogFile.c_str(), "Virus Database Update Report");
}

void CWApplication::showContextMenu()
{
    const bool hasScanReport = !m_config.scanLogFile.empty();
    const bool hasUpdateReport = !m_config.updateLogFile.empty();
    m_tray.showContextMenu(hasScanReport, hasUpdateReport);
}

void CWApplication::showReportsMenu()
{
    const bool hasScanReport = !m_config.scanLogFile.empty();
    const bool hasUpdateReport = !m_config.updateLogFile.empty();

    if (CW_UseCustomDarkReportsMenu())
    {
        if (CW_ShowCustomDarkReportsMenu(m_hwndTray, hasScanReport, hasUpdateReport))
            return;
    }

    HMENU hReportsMenu = CreatePopupMenu();
    if (!hReportsMenu)
        return;

    MENUINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = MNS_NOCHECK;
    SetMenuInfo(hReportsMenu, &mi);

    AppendMenuA(hReportsMenu,
                MF_OWNERDRAW | (hasScanReport ? 0 : MF_GRAYED),
                IDM_TRAY_SCANREPORT,
                (LPCSTR)s_reportScanText);
    AppendMenuA(hReportsMenu,
                MF_OWNERDRAW | (hasUpdateReport ? 0 : MF_GRAYED),
                IDM_TRAY_UPDATEREPORT,
                (LPCSTR)s_reportUpdateText);

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(m_hwndTray);
    TrackPopupMenu(hReportsMenu,
                   TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0,
                   m_hwndTray, NULL);
    PostMessage(m_hwndTray, WM_NULL, 0, 0);
    DestroyMenu(hReportsMenu);
}

/* ─── Window proc ───────────────────────────────────────────── */

LRESULT CWApplication::handleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
            return 0;

        case WM_TRAYICON:
            switch (LOWORD(lp))
            {
                case WM_RBUTTONUP:  showContextMenu(); break;
                case WM_LBUTTONDBLCLK: doOpenDashboard();         break;
                case NIN_BALLOONUSERCLICK: doOpenDashboard();     break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wp))
            {
                case IDM_TRAY_OPEN:         doOpenDashboard();  break;
                case IDM_TRAY_SCAN:         doScan();           break;
                case IDM_TRAY_SCANMEM:      doScanMemory();     break;
                case IDM_TRAY_UPDATE:       doUpdate();         break;
                case IDM_TRAY_REPORTS:      showReportsMenu();  break;
                case IDM_TRAY_PREFS:        doPreferences();    break;
                case IDM_TRAY_SCHEDULE:     doSchedule();       break;
                case IDM_TRAY_SCANREPORT:   doScanReport();     break;
                case IDM_TRAY_UPDATEREPORT: doUpdateReport();   break;
                case IDM_TRAY_SCHEDULED_SCAN:   doScheduledScan();   break;
                case IDM_TRAY_SCHEDULED_UPDATE: doScheduledUpdate(); break;
                case IDM_TRAY_ABOUT:
                    CW_AboutDialogRun(dialogParent(), &m_config);
                    break;
                case IDM_TRAY_HELP:
                    doHelp();
                    break;
                case IDM_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            return 0;

        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
            if (mis && mis->CtlType == ODT_MENU)
            {
                CW_MeasureThemedMenuItem(mis);
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (dis && dis->CtlType == ODT_MENU)
            {
                CW_DrawThemedMenuItem(dis);
                return TRUE;
            }
            break;
        }

        case WM_TIMER:
            if (wp == 42) /* SCHEDULER_TIMER_ID */
            {
                m_scheduler.check();
            }
            else if (wp == CW_TRAY_RETRY_TIMER_ID)
            {
                if (!m_tray.isCreated())
                    createTray();
                else
                    KillTimer(hwnd, CW_TRAY_RETRY_TIMER_ID);
            }
            else if (wp == CW_VERSION_CHECK_TIMER_ID)
            {
                KillTimer(hwnd, CW_VERSION_CHECK_TIMER_ID);
                m_updateChecker.startCheck(m_hwndTray);
            }
            return 0;

        case WM_SETTINGCHANGE:
            /* "ImmersiveColorSet" is passed on Windows 10+ theme changes */
            CW_GetTheme()->updateSystemTheme();
            /* Repaint dashboard if it's open */
            if (m_dash.hwnd())
                InvalidateRect(m_dash.hwnd(), NULL, TRUE);
            return 0;

        case WM_CW_VERSION_RESULT:
            onVersionCheckResult(wp, lp);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    /* Handle TaskbarCreated for tray recreation after explorer restart */
    if (m_taskbarCreatedMsg != 0 && msg == m_taskbarCreatedMsg)
    {
        createTray();
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

LRESULT CALLBACK CWApplication::staticWndProc(HWND hwnd, UINT msg,
                                               WPARAM wp, LPARAM lp)
{
    CWApplication* self = NULL;

    if (msg == WM_CREATE)
    {
        CREATESTRUCTA* cs = reinterpret_cast<CREATESTRUCTA*>(lp);
        self = static_cast<CWApplication*>(cs->lpCreateParams);
        if (self)
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<CWApplication*>(
                   GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->handleMessage(hwnd, msg, wp, lp);

    return DefWindowProcA(hwnd, msg, wp, lp);
}

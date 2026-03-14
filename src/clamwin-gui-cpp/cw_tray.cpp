/*
 * ClamWin Free Antivirus — CWTray
 *
 * System Tray module encapsulated into a class.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_tray.h"
#include "cw_gui_shared.h"
#include "cw_dpi.h"
#include "cw_theme.h"

#include <string.h>

/* Win10+ requires NOTIFYICON_VERSION_4 and NIF_SHOWTIP for visible toasts.
 * These are normally available with _WIN32_IE >= 0x0600 but we target 0x0500.
 * Define them ourselves and use at runtime only on modern shells. */
#ifndef NOTIFYICON_VERSION_4
#  define NOTIFYICON_VERSION_4  4
#endif
#ifndef NIF_SHOWTIP
#  define NIF_SHOWTIP           0x00000080
#endif

static const WCHAR* s_trayOpen = L"&Open ClamWin";
static const WCHAR* s_traySep = L"__CW_MENU_SEPARATOR__";
static const WCHAR* s_trayScanFiles = L"Scan &Files...";
static const WCHAR* s_trayScanMemory = L"Scan &Memory";
static const WCHAR* s_trayUpdateDatabase = L"&Update Database";
static const WCHAR* s_trayDisplayReports = L"Display &Reports";
static const WCHAR* s_trayVirusScanReport = L"&Virus Scan Report";
static const WCHAR* s_trayVirusDbUpdateReport = L"Virus &Database Update Report";
static const WCHAR* s_trayPreferences = L"&Preferences";
static const WCHAR* s_trayScheduledScans = L"Scheduled S&cans";
static const WCHAR* s_trayAbout = L"&About";
static const WCHAR* s_trayExit = L"E&xit";
static const char* s_trayCustomMenuClass = "ClamWinDarkTrayMenu";

struct CWTrayPopupItemDef
{
    UINT id;
    const WCHAR* text;
    bool separator;
    bool disabled;
    bool arrow;
    bool bold;
};

struct CWTrayPopupState
{
    HWND owner;
    HFONT font;
    HFONT fontBold;
    CWTrayPopupItemDef items[16];
    int count;
    int hover;
    int width;
    int itemHeight;
    int separatorHeight;
    int padX;
    bool isReportsSubmenu;
    bool enableScanReport;
    bool enableUpdateReport;
    HWND parentMenu;
    HWND submenuWnd;
    int reportsItemIndex;
};

static HWND cwShowReportsSubmenu(HWND parentHwnd, CWTrayPopupState* parentState);

static RECT cwTrayPopupItemRect(const CWTrayPopupState* state, int index)
{
    RECT rc;
    rc.left = 0;
    rc.right = state ? state->width : 0;
    rc.top = 0;
    rc.bottom = 0;
    if (!state || index < 0 || index >= state->count)
        return rc;

    int y = 0;
    for (int i = 0; i < state->count; ++i)
    {
        int h = state->items[i].separator ? state->separatorHeight : state->itemHeight;
        if (i == index)
        {
            rc.top = y;
            rc.bottom = y + h;
            return rc;
        }
        y += h;
    }
    return rc;
}

static COLORREF cwAdjustColor(COLORREF color, int delta)
{
    int r = (int)GetRValue(color) + delta;
    int g = (int)GetGValue(color) + delta;
    int b = (int)GetBValue(color) + delta;
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return RGB(r, g, b);
}

static bool cwUseCustomDarkTrayMenu()
{
    CWTheme* theme = CW_GetTheme();
    return theme && theme->isDark() && !theme->useClassicPalette();
}

static WCHAR cwFindMnemonicChar(const WCHAR* text)
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

static int cwTrayPopupTotalHeight(const CWTrayPopupState* state)
{
    int total = 0;
    for (int i = 0; i < state->count; ++i)
        total += state->items[i].separator ? state->separatorHeight : state->itemHeight;
    return total;
}

static int cwTrayPopupHitTest(const CWTrayPopupState* state, int y)
{
    int top = 0;
    for (int i = 0; i < state->count; ++i)
    {
        int h = state->items[i].separator ? state->separatorHeight : state->itemHeight;
        if (y >= top && y < top + h)
            return i;
        top += h;
    }
    return -1;
}

static bool cwTrayPopupItemSelectable(const CWTrayPopupItemDef* item)
{
    return item && !item->separator && !item->disabled;
}

static void cwApplyRoundedRegion(HWND hwnd)
{
    if (!hwnd)
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0)
        return;

    int radius = CW_Scale(12);
    HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius, radius);
    if (rgn)
    {
        SetWindowRgn(hwnd, rgn, TRUE);
    }
}

static void cwTrayPopupDraw(HWND hwnd, HDC hdc, CWTrayPopupState* state)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF bg = theme ? theme->colorSurface() : RGB(45, 45, 45);
    COLORREF fg = theme ? theme->colorText() : RGB(230, 230, 230);
    COLORREF fgMuted = theme ? theme->colorTextMuted() : RGB(150, 150, 150);
    COLORREF hover = theme ? cwAdjustColor(theme->colorSurface(), 20) : RGB(70, 70, 70);

    RECT rc;
    GetClientRect(hwnd, &rc);
    const bool showMnemonics = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    {
        HBRUSH hBg = CreateSolidBrush(bg);
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);
    }

    int y = 0;
    for (int i = 0; i < state->count; ++i)
    {
        const CWTrayPopupItemDef& item = state->items[i];
        int h = item.separator ? state->separatorHeight : state->itemHeight;
        RECT itemRc;
        itemRc.left = 1;
        itemRc.top = y;
        itemRc.right = rc.right - 1;
        itemRc.bottom = y + h;

        if (!item.separator && i == state->hover && !item.disabled)
        {
            HBRUSH hHover = CreateSolidBrush(hover);
            FillRect(hdc, &itemRc, hHover);
            DeleteObject(hHover);
        }

        if (!item.separator)
        {
            RECT textRc = itemRc;
            textRc.left += state->padX;
            textRc.right -= state->padX;

            HFONT useFont = item.bold && state->fontBold ? state->fontBold : state->font;
            HGDIOBJ oldFont = SelectObject(hdc, useFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, item.disabled ? fgMuted : fg);
            DrawTextW(hdc,
                      item.text,
                      -1,
                      &textRc,
                      DT_SINGLELINE | DT_VCENTER | DT_LEFT | (showMnemonics ? 0 : DT_HIDEPREFIX));
            if (oldFont)
                SelectObject(hdc, oldFont);

            if (item.arrow)
            {
                RECT arrowRc = itemRc;
                arrowRc.right -= CW_Scale(10);
                SetTextColor(hdc, item.disabled ? fgMuted : fg);
                DrawTextW(hdc, L">", -1, &arrowRc, DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
            }
        }

        y += h;
    }
}

static LRESULT CALLBACK cwTrayPopupWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CWTrayPopupState* state = reinterpret_cast<CWTrayPopupState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_NCCREATE:
        {
            CREATESTRUCTA* cs = reinterpret_cast<CREATESTRUCTA*>(lp);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }

        case WM_PAINT:
        {
            if (!state)
                break;
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            cwTrayPopupDraw(hwnd, ps.hdc, state);
            EndPaint(hwnd, &ps);
            return 0;

        case WM_SIZE:
            cwApplyRoundedRegion(hwnd);
            return 0;
        }

        case WM_MOUSEMOVE:
            if (state)
            {
                int idx = cwTrayPopupHitTest(state, (int)(short)HIWORD(lp));
                if (idx >= 0 && !cwTrayPopupItemSelectable(&state->items[idx]))
                    idx = -1;
                if (idx != state->hover)
                {
                    state->hover = idx;
                    InvalidateRect(hwnd, NULL, FALSE);
                }

                if (!state->isReportsSubmenu)
                {
                    if (idx == state->reportsItemIndex)
                    {
                        cwShowReportsSubmenu(hwnd, state);
                    }
                    else if (state->submenuWnd)
                    {
                        DestroyWindow(state->submenuWnd);
                        state->submenuWnd = NULL;
                    }
                }
            }
            return 0;

        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;

        case WM_LBUTTONUP:
            if (state)
            {
                int idx = cwTrayPopupHitTest(state, (int)(short)HIWORD(lp));
                if (idx >= 0 && cwTrayPopupItemSelectable(&state->items[idx]))
                {
                    if (!state->isReportsSubmenu && idx == state->reportsItemIndex)
                    {
                        cwShowReportsSubmenu(hwnd, state);
                        return 0;
                    }

                    PostMessageA(state->owner, WM_COMMAND, state->items[idx].id, 0);

                    if (state->isReportsSubmenu && state->parentMenu)
                        DestroyWindow(state->parentMenu);

                    if (state->submenuWnd)
                    {
                        DestroyWindow(state->submenuWnd);
                        state->submenuWnd = NULL;
                    }
                }
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
                return 0;
            }
            if (wp == VK_MENU)
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (state && (wp == VK_RETURN || wp == VK_SPACE) && state->hover >= 0)
            {
                if (cwTrayPopupItemSelectable(&state->items[state->hover]))
                {
                    if (!state->isReportsSubmenu && state->hover == state->reportsItemIndex)
                    {
                        cwShowReportsSubmenu(hwnd, state);
                        return 0;
                    }

                    PostMessageA(state->owner, WM_COMMAND, state->items[state->hover].id, 0);
                    if (state->isReportsSubmenu && state->parentMenu)
                        DestroyWindow(state->parentMenu);

                    if (state->submenuWnd)
                    {
                        DestroyWindow(state->submenuWnd);
                        state->submenuWnd = NULL;
                    }
                    DestroyWindow(hwnd);
                    return 0;
                }
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
            if (state)
            {
                WCHAR key = (WCHAR)wp;
                if (key >= L'A' && key <= L'Z')
                    key = (WCHAR)(key - L'A' + L'a');
                for (int i = 0; i < state->count; ++i)
                {
                    const CWTrayPopupItemDef& item = state->items[i];
                    if (!cwTrayPopupItemSelectable(&item))
                        continue;

                    if (cwFindMnemonicChar(item.text) == key)
                    {
                        if (!state->isReportsSubmenu && i == state->reportsItemIndex)
                        {
                            cwShowReportsSubmenu(hwnd, state);
                            return 0;
                        }

                        PostMessageA(state->owner, WM_COMMAND, item.id, 0);
                        if (state->isReportsSubmenu && state->parentMenu)
                            DestroyWindow(state->parentMenu);
                        if (state->submenuWnd)
                        {
                            DestroyWindow(state->submenuWnd);
                            state->submenuWnd = NULL;
                        }
                        DestroyWindow(hwnd);
                        return 0;
                    }
                }
            }
            return 0;

        case WM_KILLFOCUS:
            if (wp == 0)
                return 0;
            if (state && !state->isReportsSubmenu && state->submenuWnd && IsWindow(state->submenuWnd))
                return 0;
            if (state && !state->isReportsSubmenu && state->submenuWnd && (HWND)wp == state->submenuWnd)
                return 0;
            if (state && state->isReportsSubmenu && state->parentMenu && (HWND)wp == state->parentMenu)
                return 0;
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
            if (state)
            {
                if (state->parentMenu)
                {
                    CWTrayPopupState* parentState = reinterpret_cast<CWTrayPopupState*>(GetWindowLongPtrA(state->parentMenu, GWLP_USERDATA));
                    if (parentState && parentState->submenuWnd == hwnd)
                        parentState->submenuWnd = NULL;
                }

                if (state->submenuWnd)
                {
                    DestroyWindow(state->submenuWnd);
                    state->submenuWnd = NULL;
                }

                if (state->font) DeleteObject(state->font);
                if (state->fontBold) DeleteObject(state->fontBold);
                delete state;
                SetWindowLongPtrA(hwnd, GWLP_USERDATA, 0);
            }
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool cwEnsureTrayPopupClass(HINSTANCE hInst)
{
    WNDCLASSA wc;
    if (GetClassInfoA(hInst, s_trayCustomMenuClass, &wc))
        return true;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = cwTrayPopupWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = s_trayCustomMenuClass;
    wc.style = CS_DBLCLKS;
    return RegisterClassA(&wc) != 0;
}

static HWND cwCreateTrayPopupWindow(HWND owner, CWTrayPopupState* state, int x, int y)
{
    HINSTANCE hInst = (HINSTANCE)GetModuleHandleA(NULL);
    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                s_trayCustomMenuClass,
                                "",
                                WS_POPUP,
                                x, y, state->width, cwTrayPopupTotalHeight(state),
                                owner,
                                NULL,
                                hInst,
                                state);
    if (!hwnd)
        return NULL;
    return hwnd;
}

static HWND cwShowReportsSubmenu(HWND parentHwnd, CWTrayPopupState* parentState)
{
    if (!parentHwnd || !parentState)
        return NULL;
    if (parentState->submenuWnd)
        return parentState->submenuWnd;

    CWTrayPopupState* sub = new CWTrayPopupState();
    ZeroMemory(sub, sizeof(*sub));
    sub->owner = parentState->owner;
    sub->hover = -1;
    sub->width = CW_Scale(300);
    sub->itemHeight = CW_Scale(40);
    sub->separatorHeight = CW_Scale(8);
    sub->padX = CW_Scale(20);
    sub->isReportsSubmenu = true;
    sub->enableScanReport = parentState->enableScanReport;
    sub->enableUpdateReport = parentState->enableUpdateReport;
    sub->parentMenu = parentHwnd;
    sub->submenuWnd = NULL;
    sub->reportsItemIndex = -1;

    NONCLIENTMETRICSW ncm;
    ZeroMemory(&ncm, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        ncm.lfMenuFont.lfHeight = -CW_Scale(13);
        ncm.lfMenuFont.lfWeight = FW_NORMAL;
        sub->font = CreateFontIndirectW(&ncm.lfMenuFont);
        ncm.lfMenuFont.lfWeight = FW_SEMIBOLD;
        sub->fontBold = CreateFontIndirectW(&ncm.lfMenuFont);
    }
    if (!sub->font)
        sub->font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!sub->fontBold)
        sub->fontBold = sub->font;

    int i = 0;
    sub->items[i++] = { IDM_TRAY_SCANREPORT, s_trayVirusScanReport, false, !sub->enableScanReport, false, false };
    sub->items[i++] = { IDM_TRAY_UPDATEREPORT, s_trayVirusDbUpdateReport, false, !sub->enableUpdateReport, false, false };
    sub->count = i;

    RECT parentWindowRc;
    GetWindowRect(parentHwnd, &parentWindowRc);
    RECT reportsRc = cwTrayPopupItemRect(parentState, parentState->reportsItemIndex);
    int x = parentWindowRc.right - 2;
    int y = parentWindowRc.top + reportsRc.top;

    HWND subHwnd = cwCreateTrayPopupWindow(parentHwnd, sub, x, y);
    if (!subHwnd)
    {
        if (sub->font && sub->font != (HFONT)GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(sub->font);
        if (sub->fontBold && sub->fontBold != sub->font) DeleteObject(sub->fontBold);
        delete sub;
        return NULL;
    }

    parentState->submenuWnd = subHwnd;

    ShowWindow(subHwnd, SW_SHOWNORMAL);
    cwApplyRoundedRegion(subHwnd);
    SetForegroundWindow(subHwnd);
    InvalidateRect(subHwnd, NULL, TRUE);
    return subHwnd;
}

static bool cwShowCustomDarkTrayPopup(HWND owner, bool enableScanReport, bool enableUpdateReport)
{
    HINSTANCE hInst = (HINSTANCE)GetModuleHandleA(NULL);
    if (!cwEnsureTrayPopupClass(hInst))
        return false;

    CWTrayPopupState* state = new CWTrayPopupState();
    ZeroMemory(state, sizeof(*state));
    state->owner = owner;
    state->hover = -1;
    state->width = CW_Scale(300);
    state->itemHeight = CW_Scale(40);
    state->separatorHeight = CW_Scale(8);
    state->padX = CW_Scale(20);
    state->isReportsSubmenu = false;
    state->enableScanReport = enableScanReport;
    state->enableUpdateReport = enableUpdateReport;
    state->parentMenu = NULL;
    state->submenuWnd = NULL;
    state->reportsItemIndex = -1;

    NONCLIENTMETRICSW ncm;
    ZeroMemory(&ncm, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        ncm.lfMenuFont.lfHeight = -CW_Scale(13);
        ncm.lfMenuFont.lfWeight = FW_NORMAL;
        state->font = CreateFontIndirectW(&ncm.lfMenuFont);
        ncm.lfMenuFont.lfWeight = FW_SEMIBOLD;
        state->fontBold = CreateFontIndirectW(&ncm.lfMenuFont);
    }
    if (!state->font)
        state->font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!state->fontBold)
        state->fontBold = state->font;

    int i = 0;
    state->items[i++] = { IDM_TRAY_OPEN, s_trayOpen, false, false, false, true };
    state->items[i++] = { 0, s_traySep, true, true, false, false };
    state->items[i++] = { IDM_TRAY_SCAN, s_trayScanFiles, false, false, false, false };
    state->items[i++] = { IDM_TRAY_SCANMEM, s_trayScanMemory, false, false, false, false };
    state->items[i++] = { IDM_TRAY_UPDATE, s_trayUpdateDatabase, false, false, false, false };
    state->items[i++] = { 0, s_traySep, true, true, false, false };
    state->items[i++] = { IDM_TRAY_REPORTS, s_trayDisplayReports, false, (!enableScanReport && !enableUpdateReport), true, false };
    state->reportsItemIndex = i - 1;
    state->items[i++] = { 0, s_traySep, true, true, false, false };
    state->items[i++] = { IDM_TRAY_PREFS, s_trayPreferences, false, false, false, false };
    state->items[i++] = { IDM_TRAY_SCHEDULE, s_trayScheduledScans, false, false, false, false };
    state->items[i++] = { IDM_TRAY_ABOUT, s_trayAbout, false, false, false, false };
    state->items[i++] = { 0, s_traySep, true, true, false, false };
    state->items[i++] = { IDM_TRAY_EXIT, s_trayExit, false, false, false, false };
    state->count = i;

    int height = cwTrayPopupTotalHeight(state);
    POINT pt;
    GetCursorPos(&pt);
    int x = pt.x - state->width + CW_Scale(8);
    int y = pt.y - height + CW_Scale(8);

    RECT wa;
    if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0))
    {
        if (x < wa.left) x = wa.left;
        if (y < wa.top) y = wa.top;
        if (x + state->width > wa.right) x = wa.right - state->width;
        if (y + height > wa.bottom) y = wa.bottom - height;
    }

    HWND hwnd = cwCreateTrayPopupWindow(owner, state, x, y);
    if (!hwnd)
    {
        if (state->font && state->font != (HFONT)GetStockObject(DEFAULT_GUI_FONT)) DeleteObject(state->font);
        if (state->fontBold && state->fontBold != state->font) DeleteObject(state->fontBold);
        delete state;
        return false;
    }

    SetForegroundWindow(owner);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    cwApplyRoundedRegion(hwnd);
    SetForegroundWindow(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);

    return true;

    SetForegroundWindow(owner);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

CWTray::CWTray()
    : m_created(false)
    , m_version4(false)
{
    memset(&m_nid, 0, sizeof(m_nid));
}

CWTray::~CWTray()
{
    destroy();
}

bool CWTray::create(HWND hwnd, HICON hIcon, const char* tooltip)
{
    if (m_created)
        return true;

    /* Use legacy size on old shells (e.g. Win98) for compatibility. */
    m_nid.cbSize = NOTIFYICONDATAA_V2_SIZE;
    m_nid.hWnd   = hwnd;
    m_nid.uID    = IDI_CLAMWIN;
    m_nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon  = hIcon;
    if (tooltip)
        lstrcpynA(m_nid.szTip, tooltip, 128);
    else
        m_nid.szTip[0] = '\0';

    m_created = Shell_NotifyIconA(NIM_ADD, &m_nid) != FALSE;

    if (!m_created)
    {
        m_nid.cbSize = NOTIFYICONDATAA_V1_SIZE;
        m_created = Shell_NotifyIconA(NIM_ADD, &m_nid) != FALSE;
    }

    if (m_created)
    {
        /* Try VERSION_4 first (Win7+) — required for visible toasts on Win10/11.
         * Fall back to VERSION (v3) for older shells like XP/Vista. */
        m_nid.uVersion = NOTIFYICON_VERSION_4;
        m_nid.uFlags = 0;
        if (Shell_NotifyIconA(NIM_SETVERSION, &m_nid))
        {
            m_version4 = true;
        }
        else
        {
            m_nid.uVersion = NOTIFYICON_VERSION;
            Shell_NotifyIconA(NIM_SETVERSION, &m_nid);
            m_version4 = false;
        }
    }

    return m_created;
}

void CWTray::destroy()
{
    if (m_created)
    {
        Shell_NotifyIconA(NIM_DELETE, &m_nid);
        m_created = false;
    }
}

void CWTray::setIcon(HICON hIcon, const char* tooltip)
{
    if (!m_created) return;

    m_nid.uFlags = NIF_ICON | NIF_TIP;
    m_nid.hIcon = hIcon;
    if (tooltip)
        lstrcpynA(m_nid.szTip, tooltip, 128);
    Shell_NotifyIconA(NIM_MODIFY, &m_nid);
}

void CWTray::showBalloon(const char* title, const char* msg, DWORD flags)
{
    if (!m_created) return;

    /* NIF_SHOWTIP is required on Win10/11 with VERSION_4 to actually
     * show the balloon as a visible toast instead of only in Action Center. */
    m_nid.uFlags = NIF_INFO | (m_version4 ? NIF_SHOWTIP : 0);
    lstrcpynA(m_nid.szInfoTitle, title ? title : "", 64);
    lstrcpynA(m_nid.szInfo, msg ? msg : "", 256);
    m_nid.dwInfoFlags = flags;
    m_nid.uTimeout = 10000;
    Shell_NotifyIconA(NIM_MODIFY, &m_nid);
}

void CWTray::showContextMenu(bool enableScanReport, bool enableUpdateReport)
{
    if (!m_created) return;

    if (cwUseCustomDarkTrayMenu())
    {
        if (cwShowCustomDarkTrayPopup(m_nid.hWnd, enableScanReport, enableUpdateReport))
            return;
    }

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    HMENU hReportsMenu = CreatePopupMenu();
    if (!hReportsMenu)
    {
        DestroyMenu(hMenu);
        return;
    }

    MENUINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = MNS_NOCHECK;
    SetMenuInfo(hMenu, &mi);
    SetMenuInfo(hReportsMenu, &mi);

    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_OPEN, (LPCSTR)s_trayOpen);
    AppendMenuA(hMenu, MF_OWNERDRAW | MF_DISABLED, 60001, (LPCSTR)s_traySep);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_SCAN,    (LPCSTR)s_trayScanFiles);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_SCANMEM, (LPCSTR)s_trayScanMemory);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_UPDATE,  (LPCSTR)s_trayUpdateDatabase);
    AppendMenuA(hMenu, MF_OWNERDRAW | MF_DISABLED, 60002, (LPCSTR)s_traySep);
    AppendMenuA(hReportsMenu,
                MF_OWNERDRAW | (enableScanReport ? 0 : MF_GRAYED),
                IDM_TRAY_SCANREPORT,
                (LPCSTR)s_trayVirusScanReport);
    AppendMenuA(hReportsMenu,
                MF_OWNERDRAW | (enableUpdateReport ? 0 : MF_GRAYED),
                IDM_TRAY_UPDATEREPORT,
                (LPCSTR)s_trayVirusDbUpdateReport);
    AppendMenuA(hMenu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)hReportsMenu, (LPCSTR)s_trayDisplayReports);
    AppendMenuA(hMenu, MF_OWNERDRAW | MF_DISABLED, 60003, (LPCSTR)s_traySep);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_PREFS,    (LPCSTR)s_trayPreferences);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_SCHEDULE, (LPCSTR)s_trayScheduledScans);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_ABOUT,    (LPCSTR)s_trayAbout);
    AppendMenuA(hMenu, MF_OWNERDRAW | MF_DISABLED, 60004, (LPCSTR)s_traySep);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_TRAY_EXIT, (LPCSTR)s_trayExit);

    SetMenuDefaultItem(hMenu, IDM_TRAY_OPEN, FALSE);

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(m_nid.hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, m_nid.hWnd, NULL);
    PostMessage(m_nid.hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

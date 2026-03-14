/*
 * ClamWin Free Antivirus — CWDialog implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_dialog.h"
#include "cw_dpi.h"
#include "cw_mnemonic.h"
#include "cw_theme.h"
#include <string.h>

typedef HRESULT (WINAPI *CWSetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *CWDwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);

static CWDwmSetWindowAttributeFn getDwmSetWindowAttributeFn()
{
    static HMODULE s_dwmapi = NULL;
    static CWDwmSetWindowAttributeFn s_fn = NULL;
    static bool s_checked = false;

    if (!s_checked)
    {
        s_checked = true;
        s_dwmapi = LoadLibraryA("dwmapi.dll");
        if (s_dwmapi)
            s_fn = reinterpret_cast<CWDwmSetWindowAttributeFn>(GetProcAddress(s_dwmapi, "DwmSetWindowAttribute"));
    }
    return s_fn;
}

static CWSetWindowThemeFn getSetWindowThemeFn()
{
    static HMODULE s_uxtheme = NULL;
    static CWSetWindowThemeFn s_fn = NULL;
    static bool s_checked = false;

    if (!s_checked)
    {
        s_checked = true;
        s_uxtheme = LoadLibraryA("uxtheme.dll");
        if (s_uxtheme)
            s_fn = reinterpret_cast<CWSetWindowThemeFn>(GetProcAddress(s_uxtheme, "SetWindowTheme"));
    }
    return s_fn;
}

static bool isModernThemeClass(const char* className)
{
    if (!className || !className[0])
        return false;

    if (lstrcmpiA(className, "Edit") == 0)
        return true;
    if (lstrcmpiA(className, "ListBox") == 0)
        return true;
    if (lstrcmpiA(className, "ComboBox") == 0)
        return true;
    if (lstrcmpiA(className, "Button") == 0)
        return true;
    if (strstr(className, "RICHEDIT") != NULL)
        return true;
    return false;
}

static void tryApplyModernTheme(HWND hwnd)
{
    CWTheme* theme = CW_GetTheme();
    if (theme && theme->useClassicPalette())
        return;

    CWSetWindowThemeFn setWindowThemeFn = getSetWindowThemeFn();
    if (!setWindowThemeFn)
        return;

    char className[64];
    className[0] = '\0';
    GetClassNameA(hwnd, className, sizeof(className));
    if (!isModernThemeClass(className))
        return;

    const WCHAR* visualStyle = L"Explorer";
    if (theme && theme->isDark())
        visualStyle = L"DarkMode_Explorer";

    setWindowThemeFn(hwnd, visualStyle, NULL);
}

static void tryApplyDarkTitleBar(HWND hwnd)
{
    CWTheme* theme = CW_GetTheme();
    if (!theme || theme->useClassicPalette() || !theme->isDark())
        return;

    CWDwmSetWindowAttributeFn dwmSetWindowAttributeFn = getDwmSetWindowAttributeFn();
    if (!dwmSetWindowAttributeFn)
        return;

    BOOL enabled = TRUE;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_20H1 = 20;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_1809 = 19;

    HRESULT hr = dwmSetWindowAttributeFn(hwnd,
                                         DWMWA_USE_IMMERSIVE_DARK_MODE_20H1,
                                         &enabled,
                                         sizeof(enabled));
    if (FAILED(hr))
    {
        dwmSetWindowAttributeFn(hwnd,
                                DWMWA_USE_IMMERSIVE_DARK_MODE_1809,
                                &enabled,
                                sizeof(enabled));
    }
}

static BOOL CALLBACK applyModernThemeEnumProc(HWND hwnd, LPARAM lp)
{
    (void)lp;
    tryApplyModernTheme(hwnd);
    return TRUE;
}

static BOOL CALLBACK clearAccelUiStateEnumProc(HWND hwnd, LPARAM lp)
{
    (void)lp;
    SendMessageA(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);
    return TRUE;
}

static const char* s_dialogMnemonicCueProp = "CW_DIALOG_SHOW_MNEMONICS";

static COLORREF adjustColor(COLORREF color, int delta)
{
    int r = (int)GetRValue(color) + delta;
    int g = (int)GetGValue(color) + delta;
    int b = (int)GetBValue(color) + delta;

    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;

    return RGB(r, g, b);
}

static void drawThemedButton(DRAWITEMSTRUCT* dis, CWTheme* theme)
{
    if (!dis || !theme) return;

    HWND rootDlg = GetAncestor(dis->hwndItem, GA_ROOT);
    bool showMnemonics = CWDialog::getDialogMnemonicCues(rootDlg);

    if (theme->useClassicPalette())
    {
        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;
        HWND hwndBtn = dis->hwndItem;
        const bool isPressed = (dis->itemState & ODS_SELECTED) != 0;
        const bool isDisabled = (dis->itemState & ODS_DISABLED) != 0;

        FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));
        DrawFrameControl(hdc, &rc, DFC_BUTTON,
                         DFCS_BUTTONPUSH | (isPressed ? DFCS_PUSHED : 0) | (isDisabled ? DFCS_INACTIVE : 0));

        char text[256];
        GetWindowTextA(hwndBtn, text, sizeof(text));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(isDisabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));

        HFONT hFont = (HFONT)SendMessageA(hwndBtn, WM_GETFONT, 0, 0);
        HGDIOBJ oldFont = NULL;
        if (hFont)
            oldFont = SelectObject(hdc, hFont);

        RECT tr = rc;
        if (isPressed)
            OffsetRect(&tr, 1, 1);
        CW_DrawMnemonicTextAlways(hdc,
                      hFont,
                      tr,
                      text,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                      showMnemonics);

        if (oldFont)
            SelectObject(hdc, oldFont);

        if (dis->itemState & ODS_FOCUS)
        {
            RECT focusRc = rc;
            InflateRect(&focusRc, -3, -3);
            DrawFocusRect(hdc, &focusRc);
        }
        return;
    }

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    HWND hwndBtn = dis->hwndItem;

    const bool isDisabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool isPressed  = (dis->itemState & ODS_SELECTED) != 0;
    const bool isHot      = (dis->itemState & ODS_HOTLIGHT) != 0;

    COLORREF fillColor;
    COLORREF borderColor;
    COLORREF textColor;

    if (isDisabled)
    {
        fillColor = theme->colorSurface();
        borderColor = theme->colorTextMuted();
        textColor = theme->colorTextMuted();
    }
    else if (isPressed)
    {
        fillColor = adjustColor(theme->colorAccent(), -24);
        borderColor = adjustColor(theme->colorAccent(), -36);
        textColor = RGB(255, 255, 255);
    }
    else if (isHot)
    {
        fillColor = adjustColor(theme->colorAccent(), 18);
        borderColor = adjustColor(theme->colorAccent(), -10);
        textColor = RGB(255, 255, 255);
    }
    else
    {
        fillColor = theme->colorAccent();
        borderColor = adjustColor(theme->colorAccent(), -18);
        textColor = RGB(255, 255, 255);
    }

    HBRUSH bgBrush = theme->brushBg();
    FillRect(hdc, &rc, bgBrush);

    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBrush = SelectObject(hdc, fillBrush);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, CW_Scale(8), CW_Scale(8));
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);

    char text[256];
    GetWindowTextA(hwndBtn, text, sizeof(text));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    HFONT hFont = (HFONT)SendMessageA(hwndBtn, WM_GETFONT, 0, 0);
    HGDIOBJ oldFont = NULL;
    if (hFont)
        oldFont = SelectObject(hdc, hFont);
    CW_DrawMnemonicTextAlways(hdc,
                              hFont,
                              rc,
                              text,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                              showMnemonics);
    if (oldFont)
        SelectObject(hdc, oldFont);

    if (dis->itemState & ODS_FOCUS)
    {
        RECT focusRc = rc;
        InflateRect(&focusRc, -CW_Scale(4), -CW_Scale(4));
        DrawFocusRect(hdc, &focusRc);
    }
}

CWDialog::CWDialog()
    : m_hwnd(NULL)
{
}

void CWDialog::setDialogMnemonicCues(HWND hwnd, bool show)
{
    if (!hwnd)
        return;

    if (show)
        SetPropA(hwnd, s_dialogMnemonicCueProp, (HANDLE)1);
    else
        RemovePropA(hwnd, s_dialogMnemonicCueProp);
}

bool CWDialog::getDialogMnemonicCues(HWND hwnd)
{
    if (!hwnd)
        return false;
    return GetPropA(hwnd, s_dialogMnemonicCueProp) != NULL;
}

CWDialog::~CWDialog()
{
}

void CWDialog::endDialog(INT_PTR result)
{
    if (m_hwnd) EndDialog(m_hwnd, result);
}

bool CWDialog::focusNextTabStop(HWND from, bool previous)
{
    if (!m_hwnd)
        return false;

    HWND start = from;
    if (!start)
        start = GetFocus();

    HWND next = GetNextDlgTabItem(m_hwnd, start, previous ? TRUE : FALSE);
    if (!next)
        return false;

    SetFocus(next);
    return true;
}

bool CWDialog::focusFirstTabStop()
{
    return focusNextTabStop(NULL, false);
}

/* ─── runModal ─────────────────────────────────────────────── */

INT_PTR CWDialog::runModal(HWND parent, int w, int h)
{
    /* Build minimal dialog template in memory.
     * Dialog units: ~4 dlu per pixel at 96dpi. */

    /* Align to DWORD for DLGTEMPLATE */
    struct {
        DLGTEMPLATE hdr;
        WORD menu;
        WORD windowClass;
        WORD title;
    } tmpl;

    memset(&tmpl, 0, sizeof(tmpl));
    tmpl.hdr.style          = WS_POPUP | WS_CAPTION | WS_SYSMENU
                            | DS_CENTER | DS_MODALFRAME | WS_VISIBLE;
    tmpl.hdr.dwExtendedStyle = 0;
    tmpl.hdr.cdit            = 0;
    tmpl.hdr.x               = 0;
    tmpl.hdr.y               = 0;

    /* Scale requested pixels to high-DPI, then translate to DLUs exactly */
    w = CW_Scale(w);
    h = CW_Scale(h);
    LONG baseUnits = GetDialogBaseUnits();
    int baseX = LOWORD(baseUnits);
    int baseY = HIWORD(baseUnits);

    tmpl.hdr.cx              = (short)(w * 4 / baseX);
    tmpl.hdr.cy              = (short)(h * 8 / baseY);
    /* menu, windowClass, title are all zero = none/default/empty */

    return DialogBoxIndirectParamA(GetModuleHandleA(NULL),
                                   &tmpl.hdr,
                                   parent,
                                   staticDlgProc,
                                   reinterpret_cast<LPARAM>(this));
}

/* ─── Default message handler ────────────────────────────────── */

INT_PTR CWDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            bool setFocusBySystem = onInit();
            /* Show keyboard cues (accelerator underlines) immediately. */
            SendMessageA(m_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);
            EnumChildWindows(m_hwnd, clearAccelUiStateEnumProc, 0);
            tryApplyDarkTitleBar(m_hwnd);
            EnumChildWindows(m_hwnd, applyModernThemeEnumProc, 0);
            return setFocusBySystem ? TRUE : FALSE;
        }

        case WM_UPDATEUISTATE:
        {
            /* Keep accelerator underlines visible even when Windows asks to hide cues. */
            if (LOWORD(wp) == UIS_SET && (HIWORD(wp) & UISF_HIDEACCEL) != 0)
            {
                SendMessageA(m_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL), 0);
                EnumChildWindows(m_hwnd, clearAccelUiStateEnumProc, 0);
                return TRUE;
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                FillRect((HDC)wp, &rc, theme->brushBg());
                return TRUE;
            }
            return FALSE;
        }

        case WM_CTLCOLORDLG:
        {
            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                HDC hdc = (HDC)wp;
                SetBkColor(hdc, theme->colorBg());
                SetTextColor(hdc, theme->colorText());
                return (INT_PTR)theme->brushBg();
            }
            return FALSE;
        }

        case WM_CTLCOLORSTATIC:
        {
            HWND ctrl = (HWND)lp;
            if (useDefaultControlColors(ctrl))
                return FALSE;

            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                HDC hdc = (HDC)wp;
                SetBkMode(hdc, TRANSPARENT);
                SetBkColor(hdc, theme->colorBg());
                SetTextColor(hdc, theme->colorText());
                return (INT_PTR)theme->brushBg();
            }
            return FALSE;
        }

        case WM_CTLCOLOREDIT:
        {
            HWND ctrl = (HWND)lp;
            if (useDefaultControlColors(ctrl))
                return FALSE;

            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                HDC hdc = (HDC)wp;
                SetBkColor(hdc, theme->colorSurface());
                SetTextColor(hdc, theme->colorText());
                return (INT_PTR)theme->brushSurface();
            }
            return FALSE;
        }

        case WM_CTLCOLORBTN:
        {
            HWND ctrl = (HWND)lp;
            if (useDefaultControlColors(ctrl))
                return FALSE;

            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                HDC hdc = (HDC)wp;
                SetBkColor(hdc, theme->colorSurface());
                SetTextColor(hdc, theme->colorText());
                return (INT_PTR)theme->brushSurface();
            }
            return FALSE;
        }

        case WM_CTLCOLORLISTBOX:
        {
            HWND ctrl = (HWND)lp;
            if (useDefaultControlColors(ctrl))
                return FALSE;

            CWTheme* theme = CW_GetTheme();
            if (theme)
            {
                HDC hdc = (HDC)wp;
                SetBkColor(hdc, theme->colorSurface());
                SetTextColor(hdc, theme->colorText());
                return (INT_PTR)theme->brushSurface();
            }
            return FALSE;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
            if (dis && dis->CtlType == ODT_BUTTON)
            {
                CWTheme* theme = CW_GetTheme();
                if (theme)
                {
                    drawThemedButton(dis, theme);
                    return TRUE;
                }
            }
            return FALSE;
        }

        case WM_COMMAND:
            if (onCommand(LOWORD(wp), (HWND)lp))
                return TRUE;
            /* Default: IDOK / IDCANCEL close the dialog */
            if (LOWORD(wp) == IDOK)     { endDialog(IDOK);     return TRUE; }
            if (LOWORD(wp) == IDCANCEL) { endDialog(IDCANCEL); return TRUE; }
            return FALSE;

        case WM_CLOSE:
            onClose();
            return TRUE;
    }
    return FALSE;   /* unhandled */
}

/* ─── Static DlgProc ────────────────────────────────────────── */

INT_PTR CALLBACK CWDialog::staticDlgProc(HWND hwnd, UINT msg,
                                          WPARAM wp, LPARAM lp)
{
    CWDialog* self = NULL;

    if (msg == WM_INITDIALOG)
    {
        self = reinterpret_cast<CWDialog*>(lp);
        if (self)
        {
            self->m_hwnd = hwnd;
            SetWindowLongPtrA(hwnd, DWLP_USER,
                              reinterpret_cast<LONG_PTR>(self));
        }
    }
    else
    {
        self = reinterpret_cast<CWDialog*>(
                   GetWindowLongPtrA(hwnd, DWLP_USER));
    }

    if (self)
        return self->handleMessage(msg, wp, lp);

    return FALSE;
}

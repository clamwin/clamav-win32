/*
 * ClamWin Free Antivirus — Owner-drawn toggle / checkbox helpers
 *
 * Shared between CWPrefsDialog and CWScheduleDialog (and any future dialog).
 * Include once per .cpp that needs owner-drawn checkboxes.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include "cw_mnemonic.h"
#include "cw_theme.h"
#include "cw_dpi.h"
#include <windows.h>

/* ── Property name stored on the HWND ─────────────────────────── */
static const char* const CW_TOGGLE_PROP = "CW_TOGGLE";

/* ── State accessors ───────────────────────────────────────────── */
inline bool CW_ToggleGetChecked(HWND hwnd)
{
    if (!hwnd) return false;
    return GetPropA(hwnd, CW_TOGGLE_PROP) != NULL;
}

inline void CW_ToggleSetChecked(HWND hwnd, bool checked)
{
    if (!hwnd) return;
    if (checked)
        SetPropA(hwnd, CW_TOGGLE_PROP, (HANDLE)1);
    else
        RemovePropA(hwnd, CW_TOGGLE_PROP);
}

/*
 * CW_DrawOwnerToggle
 *
 * Draws a single BS_OWNERDRAW checkbox or radio button using the current theme.
 * Call from WM_DRAWITEM after confirming dis->CtlType == ODT_BUTTON.
 * hFont   — the dialog's normal font (used for label text).
 * isRadio — true for radio buttons, false for checkboxes.
 */
inline void CW_DrawOwnerToggle(DRAWITEMSTRUCT* dis,
                               HFONT hFont,
                               bool isRadio = false,
                               bool showMnemonics = true)
{
    if (!dis) return;

    CWTheme* theme = CW_GetTheme();
    if (!theme) return;

    HDC  hdc = dis->hDC;
    RECT rc  = dis->rcItem;

    bool checked  = CW_ToggleGetChecked(dis->hwndItem);
    bool disabled = (dis->itemState & ODS_DISABLED) != 0;

    /* Background */
    FillRect(hdc, &rc, theme->brushBg());

    /* Box / circle */
    int markSize = CW_Scale(18);
    RECT markRc;
    markRc.left   = rc.left + CW_Scale(2);
    markRc.top    = rc.top + (rc.bottom - rc.top - markSize) / 2;
    markRc.right  = markRc.left + markSize;
    markRc.bottom = markRc.top  + markSize;

    COLORREF fill   = checked  ? theme->colorAccent()    : theme->colorSurface();
    COLORREF border = disabled ? theme->colorTextMuted() : theme->colorTextMuted();

    HBRUSH markBrush = CreateSolidBrush(fill);
    HPEN   markPen   = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBr = SelectObject(hdc, markBrush);
    HGDIOBJ oldPn = SelectObject(hdc, markPen);

    if (isRadio)
        Ellipse(hdc, markRc.left, markRc.top, markRc.right, markRc.bottom);
    else
        RoundRect(hdc, markRc.left, markRc.top, markRc.right, markRc.bottom,
                  CW_Scale(4), CW_Scale(4));

    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPn);
    DeleteObject(markBrush);
    DeleteObject(markPen);

    /* Inner mark */
    if (checked)
    {
        if (isRadio)
        {
            RECT dot = markRc;
            InflateRect(&dot, -CW_Scale(5), -CW_Scale(5));
            HBRUSH dotBrush = CreateSolidBrush(RGB(255, 255, 255));
            HPEN   dotPen   = CreatePen(PS_NULL, 0, 0);
            HGDIOBJ oldDot    = SelectObject(hdc, dotBrush);
            HGDIOBJ oldDotPen = SelectObject(hdc, dotPen);
            Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
            SelectObject(hdc, oldDot);
            SelectObject(hdc, oldDotPen);
            DeleteObject(dotBrush);
            DeleteObject(dotPen);
        }
        else
        {
            HPEN checkPen = CreatePen(PS_SOLID, CW_Scale(2), RGB(255, 255, 255));
            HGDIOBJ oldCk = SelectObject(hdc, checkPen);
            MoveToEx(hdc, markRc.left + CW_Scale(4),  markRc.top + CW_Scale(9),  NULL);
            LineTo  (hdc, markRc.left + CW_Scale(8),  markRc.top + CW_Scale(13));
            LineTo  (hdc, markRc.left + CW_Scale(14), markRc.top + CW_Scale(5));
            SelectObject(hdc, oldCk);
            DeleteObject(checkPen);
        }
    }

    /* Label */
    char text[256] = {0};
    GetWindowTextA(dis->hwndItem, text, sizeof(text));
    RECT tr  = rc;
    tr.left  = markRc.right + CW_Scale(10);

    SetBkMode  (hdc, TRANSPARENT);
    SetTextColor(hdc, disabled ? theme->colorTextMuted() : theme->colorText());

    CW_DrawMnemonicTextAlways(hdc,
                              hFont,
                              tr,
                              text,
                              DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                              showMnemonics);

    /* Focus rect */
    if (dis->itemState & ODS_FOCUS)
    {
        RECT fr      = tr;
        int maxRight = fr.left + CW_Scale(300);
        if (fr.right > maxRight) fr.right = maxRight;
        DrawFocusRect(hdc, &fr);
    }
}

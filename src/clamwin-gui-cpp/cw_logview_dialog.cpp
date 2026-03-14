/*
 * ClamWin Free Antivirus — CWLogViewDialog
 *
 * Implements a scalable dialog similar to wxDialogLogView.py
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_logview_dialog.h"
#include "cw_dpi.h"
#include "cw_theme.h"
#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>

static const int CW_LOGVIEW_ID_VSCROLL = 6101;
static const int CW_LOGVIEW_ID_HSCROLL = 6102;
static const UINT_PTR CW_LOGVIEW_SCROLL_TIMER_ID = 6103;

static COLORREF adjustColorLocal(COLORREF color, int delta)
{
    int r = (int)GetRValue(color) + delta;
    int g = (int)GetGValue(color) + delta;
    int b = (int)GetBValue(color) + delta;
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return RGB(r, g, b);
}

LRESULT CALLBACK CW_LogViewScrollbarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CWLogViewDialog* dlg = reinterpret_cast<CWLogViewDialog*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (!dlg)
        return DefWindowProcA(hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            dlg->paintCustomScrollbar(hwnd, ps.hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            dlg->onCustomScrollbarMouseDown(hwnd, (int)(short)LOWORD(lp), (int)(short)HIWORD(lp));
            return 0;

        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd)
            {
                dlg->onCustomScrollbarMouseMove(hwnd, (int)(short)LOWORD(lp), (int)(short)HIWORD(lp));
            }
            return 0;

        case WM_LBUTTONUP:
            if (GetCapture() == hwnd)
                ReleaseCapture();
            dlg->onCustomScrollbarMouseUp(hwnd);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void configureRichEditTheme(HWND hwndEdit, CWTheme* theme)
{
    if (!hwndEdit || !theme)
        return;

    SendMessageA(hwndEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)theme->colorSurface());

    CHARFORMATA cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = theme->colorText();
    SendMessageA(hwndEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
}

static void applyRichEditContentTheme(HWND hwndEdit, CWTheme* theme)
{
    if (!hwndEdit || !theme)
        return;

    CHARFORMATA cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = theme->colorText();

    /* Force color over existing content, not just future typed text. */
    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(hwndEdit, EM_SETSEL, -1, -1);
}

static bool tryApplyDarkFlatScrollbars(HWND hwndEdit, CWTheme* theme)
{
    if (!hwndEdit || !theme || !theme->isDark())
        return false;

    if (!InitializeFlatSB(hwndEdit))
        return false;

    FlatSB_SetScrollProp(hwndEdit, WSB_PROP_VSTYLE, FSB_FLAT_MODE, TRUE);
    FlatSB_SetScrollProp(hwndEdit, WSB_PROP_HSTYLE, FSB_FLAT_MODE, TRUE);
    FlatSB_SetScrollProp(hwndEdit, WSB_PROP_VBKGCOLOR, (INT_PTR)theme->colorSurface(), TRUE);
    FlatSB_SetScrollProp(hwndEdit, WSB_PROP_HBKGCOLOR, (INT_PTR)theme->colorSurface(), TRUE);
    return true;
}

CWLogViewDialog::CWLogViewDialog(const char* logfile, const char* title)
    : m_logfile(logfile)
    , m_title(title)
    , m_hwndText(NULL)
    , m_hwndBtnOk(NULL)
    , m_hwndVScroll(NULL)
    , m_hwndHScroll(NULL)
    , m_hFont(NULL)
    , m_flatScrollbarInit(false)
    , m_useCustomScrollbars(false)
    , m_dragV(false)
    , m_dragH(false)
    , m_dragOffset(0)
    , m_maxLineWidthPx(0)
{
}

CWLogViewDialog::~CWLogViewDialog()
{
    if (m_hFont) DeleteObject(m_hFont);
}

bool CWLogViewDialog::onInit()
{
    SetWindowTextA(m_hwnd, m_title);

    m_hFont = CreateFontA(CW_Scale(12), 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    CWTheme* theme = CW_GetTheme();
    const bool useModernControls = !theme || !theme->useClassicPalette();
    m_useCustomScrollbars = useModernControls && theme && theme->isDark();
    const char* textClass = useModernControls ? RICHEDIT_CLASSA : "EDIT";

    DWORD textStyle = WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL;
    if (!m_useCustomScrollbars)
        textStyle |= WS_VSCROLL | WS_HSCROLL;

    /* Use RichEdit on modern systems for better native behavior; keep classic EDIT on legacy OS. */
    DWORD textExStyle = m_useCustomScrollbars ? 0 : WS_EX_CLIENTEDGE;
    m_hwndText = CreateWindowExA(textExStyle, textClass, "",
                                 textStyle,
                                 0, 0, 0, 0, m_hwnd, NULL, GetModuleHandle(NULL), NULL);

    if (useModernControls && theme)
    {
        configureRichEditTheme(m_hwndText, theme);
        if (!m_useCustomScrollbars)
            m_flatScrollbarInit = tryApplyDarkFlatScrollbars(m_hwndText, theme);
    }

    SendMessage(m_hwndText, WM_SETFONT, (WPARAM)m_hFont, 0);

    /* Create the OK button */
    DWORD btnStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_OWNERDRAW;

    m_hwndBtnOk = CreateWindowExA(0, "BUTTON", "&OK",
                                  btnStyle,
                                  0, 0, 0, 0, m_hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
    SendMessage(m_hwndBtnOk, WM_SETFONT, (WPARAM)m_hFont, 0);

    if (m_useCustomScrollbars)
    {
        m_hwndVScroll = CreateWindowExA(0, "STATIC", "",
                                        WS_CHILD | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        m_hwnd, (HMENU)CW_LOGVIEW_ID_VSCROLL,
                                        GetModuleHandle(NULL), NULL);
        m_hwndHScroll = CreateWindowExA(0, "STATIC", "",
                                        WS_CHILD | WS_VISIBLE,
                                        0, 0, 0, 0,
                                        m_hwnd, (HMENU)CW_LOGVIEW_ID_HSCROLL,
                                        GetModuleHandle(NULL), NULL);
        if (m_hwndVScroll)
        {
            SetWindowLongPtrA(m_hwndVScroll, GWLP_USERDATA, (LONG_PTR)this);
            SetWindowLongPtrA(m_hwndVScroll, GWLP_WNDPROC, (LONG_PTR)CW_LogViewScrollbarProc);
        }
        if (m_hwndHScroll)
        {
            SetWindowLongPtrA(m_hwndHScroll, GWLP_USERDATA, (LONG_PTR)this);
            SetWindowLongPtrA(m_hwndHScroll, GWLP_WNDPROC, (LONG_PTR)CW_LogViewScrollbarProc);
        }
    }

    /* Read the log file into the edit control */
    HANDLE hFile = CreateFileA(m_logfile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD size = GetFileSize(hFile, NULL);
        if (size > 0)
        {
            char* buf = new char[size + 1];
            DWORD read = 0;
            if (ReadFile(hFile, buf, size, &read, NULL))
            {
                buf[read] = 0;
                /* Note: Standard edit control needs \r\n for line breaks */
                SetWindowTextA(m_hwndText, buf);

                m_maxLineWidthPx = computeMaxLineWidthPx(buf);

                if (useModernControls && theme)
                    applyRichEditContentTheme(m_hwndText, theme);
                
                // Scroll to the end
                int len = GetWindowTextLength(m_hwndText);
                SendMessage(m_hwndText, EM_SETSEL, len, len);
                SendMessage(m_hwndText, EM_SCROLLCARET, 0, 0);
            }
            delete[] buf;
        }
        CloseHandle(hFile);
    }
    else
    {
        SetWindowTextA(m_hwndText, "Failed to open or read the log file.");
        m_maxLineWidthPx = computeMaxLineWidthPx("Failed to open or read the log file.");
        if (useModernControls && theme)
            applyRichEditContentTheme(m_hwndText, theme);
    }

    /* WM_SIZE is not guaranteed during initial dialog creation, so size controls now. */
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    layoutControls(rc.right - rc.left, rc.bottom - rc.top);

    if (m_useCustomScrollbars)
    {
        SetTimer(m_hwnd, CW_LOGVIEW_SCROLL_TIMER_ID, 80, NULL);
        refreshCustomScrollbars();
    }

    SetFocus(m_hwndBtnOk);
    return false; // did set focus ourselves
}

bool CWLogViewDialog::onCommand(int id, HWND src)
{
    if (id == IDOK || id == IDCANCEL)
    {
        if (m_useCustomScrollbars)
            KillTimer(m_hwnd, CW_LOGVIEW_SCROLL_TIMER_ID);

        if (m_flatScrollbarInit && m_hwndText)
        {
            UninitializeFlatSB(m_hwndText);
            m_flatScrollbarInit = false;
        }
        endDialog(id);
        return true;
    }
    return false;
}

void CWLogViewDialog::layoutControls(int w, int h)
{
    if (!m_hwndText || !m_hwndBtnOk) return;

    int pad = CW_Scale(10);
    int btnH = CW_Scale(28);
    int btnW = CW_Scale(100);

    if (m_useCustomScrollbars && m_hwndVScroll && m_hwndHScroll)
    {
        int sb = CW_Scale(16);
        int textW = w - (pad * 2) - sb;
        int textH = h - (pad * 3) - btnH - sb;
        if (textW < CW_Scale(120)) textW = CW_Scale(120);
        if (textH < CW_Scale(80)) textH = CW_Scale(80);

        MoveWindow(m_hwndText, pad, pad, textW, textH, TRUE);
        MoveWindow(m_hwndVScroll, pad + textW, pad, sb, textH, TRUE);
        MoveWindow(m_hwndHScroll, pad, pad + textH, textW, sb, TRUE);
        MoveWindow(m_hwndBtnOk, (w - btnW) / 2, h - pad - btnH, btnW, btnH, TRUE);
        refreshCustomScrollbars();
        return;
    }

    MoveWindow(m_hwndText, pad, pad, w - (pad * 2), h - (pad * 3) - btnH, TRUE);
    MoveWindow(m_hwndBtnOk, (w - btnW) / 2, h - pad - btnH, btnW, btnH, TRUE);
}

int CWLogViewDialog::lineHeightPx() const
{
    if (!m_hwndText || !m_hFont)
        return CW_Scale(16);

    HDC hdc = GetDC(m_hwndText);
    if (!hdc)
        return CW_Scale(16);

    HGDIOBJ oldFont = SelectObject(hdc, m_hFont);
    TEXTMETRICA tm;
    ZeroMemory(&tm, sizeof(tm));
    GetTextMetricsA(hdc, &tm);
    if (oldFont)
        SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndText, hdc);

    return tm.tmHeight > 0 ? tm.tmHeight : CW_Scale(16);
}

int CWLogViewDialog::computeMaxLineWidthPx(const char* text) const
{
    if (!text || !m_hwndText || !m_hFont)
        return 0;

    HDC hdc = GetDC(m_hwndText);
    if (!hdc)
        return 0;

    HGDIOBJ oldFont = SelectObject(hdc, m_hFont);
    int maxWidth = 0;
    const char* start = text;
    const char* p = text;

    while (1)
    {
        if (*p == '\r' || *p == '\n' || *p == '\0')
        {
            int len = (int)(p - start);
            if (len > 0)
            {
                SIZE sz;
                sz.cx = 0;
                sz.cy = 0;
                GetTextExtentPoint32A(hdc, start, len, &sz);
                if (sz.cx > maxWidth)
                    maxWidth = sz.cx;
            }

            if (*p == '\0')
                break;

            if (*p == '\r' && *(p + 1) == '\n')
                ++p;
            ++p;
            start = p;
            continue;
        }
        ++p;
    }

    if (oldFont)
        SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndText, hdc);
    return maxWidth;
}

void CWLogViewDialog::setTopVisibleLine(int topLine)
{
    int first = (int)SendMessageA(m_hwndText, EM_GETFIRSTVISIBLELINE, 0, 0);
    int delta = topLine - first;
    if (delta != 0)
        SendMessageA(m_hwndText, EM_LINESCROLL, 0, delta);
}

void CWLogViewDialog::setHorizontalScrollPx(int x)
{
    POINT pt;
    pt.x = 0;
    pt.y = 0;
    SendMessageA(m_hwndText, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
    pt.x = x;
    SendMessageA(m_hwndText, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
}

void CWLogViewDialog::refreshCustomScrollbars()
{
    if (!m_useCustomScrollbars)
        return;

    if (m_hwndVScroll)
        InvalidateRect(m_hwndVScroll, NULL, TRUE);
    if (m_hwndHScroll)
        InvalidateRect(m_hwndHScroll, NULL, TRUE);
}

void CWLogViewDialog::paintCustomScrollbar(HWND hwnd, HDC hdc)
{
    if (!hdc)
        return;

    CWTheme* theme = CW_GetTheme();
    COLORREF track = theme ? adjustColorLocal(theme->colorSurface(), 12) : RGB(80, 80, 80);
    COLORREF thumb = theme ? adjustColorLocal(theme->colorTextMuted(), -10) : RGB(130, 130, 130);

    RECT rc;
    GetClientRect(hwnd, &rc);
    HBRUSH hTrack = CreateSolidBrush(track);
    FillRect(hdc, &rc, hTrack);
    DeleteObject(hTrack);

    const bool vertical = (hwnd == m_hwndVScroll);
    RECT txtRc;
    GetClientRect(m_hwndText, &txtRc);

    int trackLen = vertical ? (rc.bottom - rc.top) : (rc.right - rc.left);
    int minThumb = CW_Scale(24);
    int thumbPos = 0;
    int thumbLen = trackLen;

    if (vertical)
    {
        int total = (int)SendMessageA(m_hwndText, EM_GETLINECOUNT, 0, 0);
        int page = txtRc.bottom - txtRc.top;
        int lh = lineHeightPx();
        page = lh > 0 ? (page / lh) : 1;
        if (page < 1) page = 1;
        int maxPos = total - page;
        if (maxPos < 0) maxPos = 0;
        int first = (int)SendMessageA(m_hwndText, EM_GETFIRSTVISIBLELINE, 0, 0);
        if (first < 0) first = 0;
        if (first > maxPos) first = maxPos;

        if (total > 0)
            thumbLen = (trackLen * page) / total;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;

        thumbPos = (maxPos > 0 && trackLen > thumbLen)
            ? ((trackLen - thumbLen) * first) / maxPos
            : 0;
    }
    else
    {
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        SendMessageA(m_hwndText, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        int pos = pt.x;
        int page = txtRc.right - txtRc.left;
        int content = m_maxLineWidthPx;
        if (content < page) content = page;
        int maxPos = content - page;
        if (maxPos < 0) maxPos = 0;

        thumbLen = content > 0 ? (trackLen * page) / content : trackLen;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;

        if (pos < 0) pos = 0;
        if (pos > maxPos) pos = maxPos;

        thumbPos = (maxPos > 0 && trackLen > thumbLen)
            ? ((trackLen - thumbLen) * pos) / maxPos
            : 0;
    }

    RECT tr = rc;
    if (vertical)
    {
        tr.top += thumbPos;
        tr.bottom = tr.top + thumbLen;
    }
    else
    {
        tr.left += thumbPos;
        tr.right = tr.left + thumbLen;
    }

    HBRUSH hThumb = CreateSolidBrush(thumb);
    FillRect(hdc, &tr, hThumb);
    DeleteObject(hThumb);
}

void CWLogViewDialog::onCustomScrollbarMouseDown(HWND hwnd, int x, int y)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    const bool vertical = (hwnd == m_hwndVScroll);
    int coord = vertical ? y : x;
    int trackLen = vertical ? (rc.bottom - rc.top) : (rc.right - rc.left);
    int minThumb = CW_Scale(24);
    int thumbPos = 0;
    int thumbLen = trackLen;
    int page = 1;
    int maxPos = 0;

    RECT txtRc;
    GetClientRect(m_hwndText, &txtRc);

    if (vertical)
    {
        int total = (int)SendMessageA(m_hwndText, EM_GETLINECOUNT, 0, 0);
        page = txtRc.bottom - txtRc.top;
        int lh = lineHeightPx();
        page = lh > 0 ? (page / lh) : 1;
        if (page < 1) page = 1;
        maxPos = total - page;
        if (maxPos < 0) maxPos = 0;
        int first = (int)SendMessageA(m_hwndText, EM_GETFIRSTVISIBLELINE, 0, 0);
        if (first < 0) first = 0;
        if (first > maxPos) first = maxPos;

        thumbLen = total > 0 ? (trackLen * page) / total : trackLen;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;
        thumbPos = (maxPos > 0 && trackLen > thumbLen)
            ? ((trackLen - thumbLen) * first) / maxPos
            : 0;
    }
    else
    {
        POINT pt;
        pt.x = 0;
        pt.y = 0;
        SendMessageA(m_hwndText, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        int pos = pt.x;
        page = txtRc.right - txtRc.left;
        int content = m_maxLineWidthPx;
        if (content < page) content = page;
        maxPos = content - page;
        if (maxPos < 0) maxPos = 0;

        thumbLen = content > 0 ? (trackLen * page) / content : trackLen;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;

        if (pos < 0) pos = 0;
        if (pos > maxPos) pos = maxPos;
        thumbPos = (maxPos > 0 && trackLen > thumbLen)
            ? ((trackLen - thumbLen) * pos) / maxPos
            : 0;
    }

    if (coord >= thumbPos && coord <= (thumbPos + thumbLen))
    {
        m_dragOffset = coord - thumbPos;
        m_dragV = vertical;
        m_dragH = !vertical;
        return;
    }

    if (coord < thumbPos)
    {
        if (vertical)
            setTopVisibleLine((int)SendMessageA(m_hwndText, EM_GETFIRSTVISIBLELINE, 0, 0) - page);
        else
        {
            POINT pt;
            pt.x = 0; pt.y = 0;
            SendMessageA(m_hwndText, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
            setHorizontalScrollPx(pt.x - page);
        }
    }
    else
    {
        if (vertical)
            setTopVisibleLine((int)SendMessageA(m_hwndText, EM_GETFIRSTVISIBLELINE, 0, 0) + page);
        else
        {
            POINT pt;
            pt.x = 0; pt.y = 0;
            SendMessageA(m_hwndText, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
            setHorizontalScrollPx(pt.x + page);
        }
    }

    refreshCustomScrollbars();
}

void CWLogViewDialog::onCustomScrollbarMouseMove(HWND hwnd, int x, int y)
{
    const bool vertical = (hwnd == m_hwndVScroll);
    if ((vertical && !m_dragV) || (!vertical && !m_dragH))
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    int coord = vertical ? y : x;
    int trackLen = vertical ? (rc.bottom - rc.top) : (rc.right - rc.left);
    int minThumb = CW_Scale(24);

    RECT txtRc;
    GetClientRect(m_hwndText, &txtRc);

    if (vertical)
    {
        int total = (int)SendMessageA(m_hwndText, EM_GETLINECOUNT, 0, 0);
        int page = txtRc.bottom - txtRc.top;
        int lh = lineHeightPx();
        page = lh > 0 ? (page / lh) : 1;
        if (page < 1) page = 1;
        int maxPos = total - page;
        if (maxPos < 0) maxPos = 0;

        int thumbLen = total > 0 ? (trackLen * page) / total : trackLen;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;

        int dragPos = coord - m_dragOffset;
        if (dragPos < 0) dragPos = 0;
        if (dragPos > (trackLen - thumbLen)) dragPos = trackLen - thumbLen;

        int top = (maxPos > 0 && trackLen > thumbLen)
            ? (dragPos * maxPos) / (trackLen - thumbLen)
            : 0;
        setTopVisibleLine(top);
    }
    else
    {
        int page = txtRc.right - txtRc.left;
        int content = m_maxLineWidthPx;
        if (content < page) content = page;
        int maxPos = content - page;
        if (maxPos < 0) maxPos = 0;

        int thumbLen = content > 0 ? (trackLen * page) / content : trackLen;
        if (thumbLen < minThumb) thumbLen = minThumb;
        if (thumbLen > trackLen) thumbLen = trackLen;

        int dragPos = coord - m_dragOffset;
        if (dragPos < 0) dragPos = 0;
        if (dragPos > (trackLen - thumbLen)) dragPos = trackLen - thumbLen;

        int pos = (maxPos > 0 && trackLen > thumbLen)
            ? (dragPos * maxPos) / (trackLen - thumbLen)
            : 0;
        setHorizontalScrollPx(pos);
    }

    refreshCustomScrollbars();
}

void CWLogViewDialog::onCustomScrollbarMouseUp(HWND hwnd)
{
    (void)hwnd;
    m_dragV = false;
    m_dragH = false;
}

INT_PTR CWLogViewDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    if (m_useCustomScrollbars && msg == WM_TIMER && wp == CW_LOGVIEW_SCROLL_TIMER_ID)
    {
        refreshCustomScrollbars();
        return TRUE;
    }

    if (m_useCustomScrollbars && msg == WM_COMMAND && (HWND)lp == m_hwndText)
    {
        WORD code = HIWORD(wp);
        if (code == EN_VSCROLL || code == EN_HSCROLL)
            refreshCustomScrollbars();
    }

    if (msg == WM_SIZE)
    {
        int w = LOWORD(lp);
        int h = HIWORD(lp);
        layoutControls(w, h);
        /* Redraw cleanly */
        InvalidateRect(m_hwnd, NULL, TRUE);
        return TRUE;
    }
    return CWDialog::handleMessage(msg, wp, lp);
}

/* ─── C Wrapper ─────────────────────────────────────────────── */

void CW_LogViewerRun(HWND hwndParent, const char *logfile, const char *title)
{
    CWLogViewDialog dlg(logfile, title);
    /* Make the dialog resizable because it is a log viewer */
    // Since CWDialog::runModal does not pass WS_THICKFRAME to the dynamic template,
    // we would have to modify window style when it's created.
    // Fortunately, CWDialog::runModal gives us a fixed box. For now we use the standard box pattern.
    // We can just add WS_THICKFRAME to the style of the dialog window in WM_INITDIALOG later, or ignore for now.
    
    dlg.runModal(hwndParent, 560, 400);
}

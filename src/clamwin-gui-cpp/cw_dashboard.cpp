/*
 * ClamWin Free Antivirus — CWDashboard implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_dashboard.h"
#include "cw_application.h"
#include "cw_gui_shared.h"
#include "cw_dpi.h"
#include "cw_theme.h"
#include "cw_update_checker.h"
#include <commctrl.h>
#include <string.h>
#include <time.h>

#define MAINWND_CLASS_A  "ClamWinDashboard"
#define MAINWND_WIDTH    560
#define MAINWND_HEIGHT   518

/* ─── Card table ────────────────────────────────────────────── */

const CWDashboard::CardInfo CWDashboard::s_cards[] = {
    { IDC_CARD_SCAN,     "Scan &Files",       "Select files or folders to scan"  },
    { IDC_CARD_UPDATE,   "&Update Database",  "Download latest virus definitions" },
    { IDC_CARD_SCANMEM,  "Scan &Memory",      "Check running processes"           },
    { IDC_CARD_REPORTS,  "View &Reports",     "Scan and update log files"         },
    { IDC_CARD_PREFS,    "&Preferences",      "Configure scanner and updates"     },
    { IDC_CARD_SCHEDULE, "Scheduled S&cans",  "Manage scan schedule"              },
    { IDC_CARD_HELP,     "&Help",             "Online support and documentation" },
    { IDC_CARD_ABOUT,    "&About ClamWin",    "Version, licenses, and credits" },
};
const int CWDashboard::s_cardCount =
    (int)(sizeof(s_cards) / sizeof(s_cards[0]));

/* ─── Constructor / Destructor ──────────────────────────────── */

CWDashboard::CWDashboard(CWConfig& config)
    : m_config(config)
    , m_status(CW_STATUS_ERROR)
    , m_hoverCard(-1)
    , m_showMnemonics(false)
    , m_hwndTooltip(NULL)
    , m_updateAvailable(false)
    , m_updateLayoutAdjusted(false)
    , m_fontTitle(NULL), m_fontDesc(NULL), m_fontBanner(NULL)
    , m_fontBannerSub(NULL), m_fontStatus(NULL)
{
    memset(&m_dbInfo, 0, sizeof(m_dbInfo));
    m_newVersion[0] = '\0';
}

CWDashboard::~CWDashboard()
{
    destroyCardTooltips();
    destroyFonts();
}

/* ─── open ──────────────────────────────────────────────────── */

bool CWDashboard::open(HWND parent)
{
    refreshStatus();

    int initialHeight = CW_Scale(MAINWND_HEIGHT);
    if (m_updateAvailable)
        initialHeight += CW_Scale(46);

    bool ok = create(MAINWND_CLASS_A,
                     "ClamWin Free Antivirus",
                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                     WS_EX_APPWINDOW, parent,
                     CW_USEDEFAULT, CW_USEDEFAULT,
                     CW_Scale(MAINWND_WIDTH), initialHeight);
    if (ok) show();
    return ok;
}

/* ─── refreshStatus ─────────────────────────────────────────── */

void CWDashboard::refreshStatus()
{
    m_status = CW_GetProtectionStatus(&m_config);
    CW_GetDBInfo(m_config.databasePath.c_str(), &m_dbInfo);
    if (m_hwnd) invalidate();
}

void CWDashboard::setUpdateAvailable(const char* versionStr)
{
    m_updateAvailable = true;
    if (versionStr)
    {
        lstrcpynA(m_newVersion, versionStr, sizeof(m_newVersion));
    }
    else
    {
        m_newVersion[0] = '\0';
    }

    if (m_hwnd)
    {
        if (!m_updateLayoutAdjusted)
        {
            RECT wr;
            if (GetWindowRect(m_hwnd, &wr))
            {
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;
                SetWindowPos(m_hwnd, NULL, 0, 0, w, h + CW_Scale(46),
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            m_updateLayoutAdjusted = true;
        }
        invalidate();
    }
}

/* ─── fillWndClass override ─────────────────────────────────── */

void CWDashboard::fillWndClass(WNDCLASSA& wc)
{
    CWWindow::fillWndClass(wc);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon = LoadIconA(m_hInst, MAKEINTRESOURCEA(IDI_CLAMWIN));
}

/* ─── onCreate ──────────────────────────────────────────────── */

bool CWDashboard::onCreate()
{
    createFonts();
    initCardTooltips();
    return true;
}

void CWDashboard::initCardTooltips()
{
    if (!m_hwnd)
        return;

    m_hwndTooltip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, "",
                                    WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    m_hwnd, NULL, m_hInst, NULL);
    if (!m_hwndTooltip)
        return;

    SetWindowPos(m_hwndTooltip, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SendMessageA(m_hwndTooltip, TTM_SETMAXTIPWIDTH, 0, CW_Scale(320));

    for (int i = 0; i < s_cardCount; ++i)
    {
        TOOLINFOA ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_SUBCLASS;
        ti.hwnd = m_hwnd;
        ti.uId = (UINT_PTR)(100 + i);
        ti.lpszText = const_cast<LPSTR>(s_cards[i].desc);
        SendMessageA(m_hwndTooltip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    }

    updateCardTooltipRects();
}

void CWDashboard::updateCardTooltipRects()
{
    if (!m_hwnd || !m_hwndTooltip)
        return;

    RECT client;
    RECT cardRc;
    GetClientRect(m_hwnd, &client);
    for (int i = 0; i < s_cardCount; ++i)
    {
        getCardRect(i, client, cardRc);

        TOOLINFOA ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(ti);
        ti.hwnd = m_hwnd;
        ti.uId = (UINT_PTR)(100 + i);
        ti.rect = cardRc;
        SendMessageA(m_hwndTooltip, TTM_NEWTOOLRECTA, 0, (LPARAM)&ti);
    }
}

void CWDashboard::destroyCardTooltips()
{
    if (m_hwndTooltip)
    {
        DestroyWindow(m_hwndTooltip);
        m_hwndTooltip = NULL;
    }
}

/* ─── Font management ───────────────────────────────────────── */

static HFONT makeFont(int pt, int weight, const char* face)
{
    HFONT hf = CreateFontA(-CW_Scale(pt), 0, 0, 0, weight,
                            FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, 0, 0,
                            CLEARTYPE_QUALITY, 0, face);
    if (!hf)
        hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    return hf;
}

static void buildMnemonicTitle(const char* src, char* out, int outCap, int& mnemonicIndex)
{
    int inPos = 0;
    int outPos = 0;
    mnemonicIndex = -1;

    if (!src || !out || outCap <= 0)
    {
        return;
    }

    while (src[inPos] != '\0' && outPos < outCap - 1)
    {
        if (src[inPos] == '&')
        {
            /* Escaped ampersand: "&&" -> "&" */
            if (src[inPos + 1] == '&')
            {
                out[outPos++] = '&';
                inPos += 2;
                continue;
            }

            /* Mnemonic marker before a real character. */
            if (src[inPos + 1] != '\0')
            {
                if (mnemonicIndex < 0)
                    mnemonicIndex = outPos;
                ++inPos;
                continue;
            }
        }

        out[outPos++] = src[inPos++];
    }

    out[outPos] = '\0';
}

void CWDashboard::createFonts()
{
    m_fontTitle      = makeFont(16, FW_SEMIBOLD, "Tahoma");
    m_fontDesc       = makeFont(13, FW_NORMAL,   "Tahoma");
    m_fontBanner     = makeFont(18, FW_SEMIBOLD, "Tahoma");
    m_fontBannerSub  = makeFont(12, FW_NORMAL,   "Tahoma");
    m_fontStatus     = makeFont(12, FW_NORMAL,   "Tahoma");
}

void CWDashboard::destroyFonts()
{
    /* Only delete if not a stock object */
    auto del = [](HFONT& hf) {
        if (hf && hf != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(hf);
        hf = NULL;
    };
    del(m_fontTitle);
    del(m_fontDesc);
    del(m_fontBanner);
    del(m_fontBannerSub);
    del(m_fontStatus);
}

/* ─── Layout helper ─────────────────────────────────────────── */

void CWDashboard::getCardRect(int index, const RECT& client, RECT& out) const
{
    const int margin   = CW_Scale(16);
    const int cardW    = (client.right - margin * 3) / 2;
    const int cardH    = CW_Scale(70);
    int       topOff   = CW_Scale(110);
    if (m_updateAvailable)
        topOff += CW_Scale(46);  /* make room for the update banner */
    const int col      = index % 2;
    const int row      = index / 2;

    out.left   = margin + col * (cardW + margin);
    out.top    = topOff + row * (cardH + margin);
    out.right  = out.left + cardW;
    out.bottom = out.top + cardH;
}

int CWDashboard::cardAtPoint(POINT pt) const
{
    RECT client, cardRc;
    GetClientRect(m_hwnd, &client);
    for (int i = 0; i < s_cardCount; ++i)
    {
        getCardRect(i, client, cardRc);
        if (PtInRect(&cardRc, pt)) return i;
    }
    return -1;
}

/* ─── Paint ─────────────────────────────────────────────────── */

void CWDashboard::paintBanner(HDC hdc, const RECT& client)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF bannerColor;
    const COLORREF bannerTextColor = RGB(255, 255, 255);
    const char* statusText;
    switch (m_status)
    {
        case CW_STATUS_OK:
            bannerColor = theme ? theme->colorSuccess() : RGB(46, 125, 50);
            statusText  = "Virus definitions are up to date";
            break;
        case CW_STATUS_WARN:
            bannerColor = theme ? theme->colorAccent() : RGB(230, 162, 0);
            statusText  = "Virus definitions may be out of date";
            break;
        default:
            bannerColor = theme ? theme->colorWarning() : RGB(198, 40, 40);
            statusText  = "Virus definitions not found";
            break;
    }

    const int yOffset = m_updateAvailable ? CW_Scale(46) : 0;
    RECT bannerRc = { CW_Scale(12), CW_Scale(12) + yOffset,
                      client.right - CW_Scale(12), CW_Scale(95) + yOffset };
    HBRUSH hBr = CreateSolidBrush(bannerColor);
    HPEN   hPn = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ oldBr = SelectObject(hdc, hBr);
    HGDIOBJ oldPn = SelectObject(hdc, hPn);
    RoundRect(hdc, bannerRc.left, bannerRc.top,
                   bannerRc.right, bannerRc.bottom, CW_Scale(12), CW_Scale(12));
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPn);
    DeleteObject(hBr);
    DeleteObject(hPn);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, bannerTextColor);
    SelectObject(hdc, m_fontBanner);
    RECT tr = { bannerRc.left + CW_Scale(20), bannerRc.top + CW_Scale(12),
                bannerRc.right - CW_Scale(20), bannerRc.top + CW_Scale(38) };
    DrawTextA(hdc, statusText, -1, &tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    /* Sub-text: DB info */
    SelectObject(hdc, m_fontBannerSub);
    char dbLine[256];
    if (m_dbInfo.total_sigs > 0)
    {
        char tStr[64] = "";
        if (m_dbInfo.updated_time > 0)
        {
            time_t t = (time_t)m_dbInfo.updated_time;
            struct tm* tm = localtime(&t);
            if (tm) strftime(tStr, 64, "%d %b %Y %I:%M %p", tm);
        }
        wsprintfA(dbLine,
                  "Database: v%d (main) / v%d (daily) - %d signatures\n"
                  "Last updated: %s",
                  m_dbInfo.main_ver, m_dbInfo.daily_ver,
                  m_dbInfo.total_sigs, tStr);
    }
    else
    {
        lstrcpyA(dbLine, "No virus database found. Click 'Update Database' to download.");
    }
    RECT sr = { bannerRc.left + CW_Scale(20), bannerRc.top + CW_Scale(40),
                bannerRc.right - CW_Scale(20), bannerRc.bottom - CW_Scale(6) };
    DrawTextA(hdc, dbLine, -1, &sr, DT_LEFT | DT_WORDBREAK);
}

void CWDashboard::paintCards(HDC hdc, const RECT& client)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF borderColor = theme ? theme->colorTextMuted() : RGB(200, 200, 200);
    COLORREF titleColor = theme ? theme->colorText() : RGB(33, 33, 33);
    COLORREF descColor = theme ? theme->colorTextMuted() : RGB(117, 117, 117);

    for (int i = 0; i < s_cardCount; ++i)
    {
        RECT cardRc;
        getCardRect(i, client, cardRc);

        const bool   hover  = (i == m_hoverCard);
        const COLORREF bg   = hover
                            ? (theme ? theme->colorSurfaceHover() : RGB(227, 242, 253))
                            : (theme ? theme->colorSurface() : RGB(245, 245, 245));
        HBRUSH hBr    = CreateSolidBrush(bg);
        HPEN   hPn    = CreatePen(PS_SOLID, 1, borderColor);
        HGDIOBJ oldBr = SelectObject(hdc, hBr);
        HGDIOBJ oldPn = SelectObject(hdc, hPn);
        RoundRect(hdc, cardRc.left, cardRc.top,
                       cardRc.right, cardRc.bottom, CW_Scale(8), CW_Scale(8));
        SelectObject(hdc, oldBr);
        SelectObject(hdc, oldPn);
        DeleteObject(hBr);
        DeleteObject(hPn);

        SetTextColor(hdc, titleColor);
        SelectObject(hdc, m_fontTitle);
        RECT tr = { cardRc.left + CW_Scale(16), cardRc.top + CW_Scale(14),
                    cardRc.right - CW_Scale(12), cardRc.top + CW_Scale(36) };

        char titleText[64];
        int mnemonicIndex = -1;
        buildMnemonicTitle(s_cards[i].title, titleText, (int)sizeof(titleText), mnemonicIndex);

        DrawTextA(hdc, titleText, -1, &tr, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

        if (m_showMnemonics && mnemonicIndex >= 0)
        {
            SIZE fullSz = { 0, 0 };
            GetTextExtentPoint32A(hdc, titleText, lstrlenA(titleText), &fullSz);
            TEXTMETRICA tm;
            GetTextMetricsA(hdc, &tm);

            int textTop = tr.top + ((tr.bottom - tr.top - fullSz.cy) / 2);
            int prefixLen = mnemonicIndex;
            if (prefixLen > 0)
            {
                SIZE prefixSz = { 0, 0 };
                GetTextExtentPoint32A(hdc, titleText, prefixLen, &prefixSz);
                tr.left += prefixSz.cx;
            }

            SIZE mnemonicSz = { 0, 0 };
            GetTextExtentPoint32A(hdc, &titleText[mnemonicIndex], 1, &mnemonicSz);

            const int underlineGap = CW_Scale(2);
            const int underlineThickness = CW_Scale(1);
            const int baselineY = textTop + tm.tmAscent;
            const int y = baselineY + underlineGap;

            HPEN hPen = CreatePen(PS_SOLID,
                                  underlineThickness > 0 ? underlineThickness : 1,
                                  titleColor);
            HGDIOBJ oldPen = SelectObject(hdc, hPen);
            MoveToEx(hdc, tr.left, y, NULL);
            LineTo(hdc, tr.left + mnemonicSz.cx, y);
            SelectObject(hdc, oldPen);
            DeleteObject(hPen);
        }

        SetTextColor(hdc, descColor);
        SelectObject(hdc, m_fontDesc);
        RECT dr = { cardRc.left + CW_Scale(16), cardRc.top + CW_Scale(40),
                    cardRc.right - CW_Scale(12), cardRc.bottom - CW_Scale(8) };
        DrawTextA(hdc, s_cards[i].desc, -1, &dr,
                  DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }
}

void CWDashboard::paintStatusBar(HDC hdc, const RECT& client)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF statusBg = theme ? theme->colorSurface() : RGB(250, 250, 250);
    COLORREF borderColor = theme ? theme->colorTextMuted() : RGB(200, 200, 200);
    COLORREF textColor = theme ? theme->colorTextMuted() : RGB(117, 117, 117);

    RECT sbarRc = { 0, client.bottom - CW_Scale(28), client.right, client.bottom };
    HBRUSH hBr = CreateSolidBrush(statusBg);
    FillRect(hdc, &sbarRc, hBr);
    DeleteObject(hBr);

    HPEN hPn = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldPn = SelectObject(hdc, hPn);
    MoveToEx(hdc, 0, sbarRc.top, NULL);
    LineTo(hdc, client.right, sbarRc.top);
    SelectObject(hdc, oldPn);
    DeleteObject(hPn);

    SelectObject(hdc, m_fontStatus);
    SetTextColor(hdc, textColor);
    const char* txt = "\xA9 2026 ClamWin Project. All rights reserved.";
    RECT tr = { CW_Scale(8), sbarRc.top + CW_Scale(4), client.right - CW_Scale(8), client.bottom - CW_Scale(4) };
    DrawTextA(hdc, txt, -1, &tr, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}

/* ─── Update available banner ───────────────────────────────── */

void CWDashboard::paintUpdateBanner(HDC hdc, const RECT& client)
{
    CWTheme* theme = CW_GetTheme();
    COLORREF bannerBg   = RGB(216, 102, 0);
    COLORREF bannerText = RGB(255, 255, 255);

    /* Positioned just below the status banner */
    RECT rc;
    rc.left   = CW_Scale(12);
    rc.top    = CW_Scale(12);
    rc.right  = client.right - CW_Scale(12);
    rc.bottom = rc.top + CW_Scale(38);

    HBRUSH hBr = CreateSolidBrush(bannerBg);
    HPEN   hPn = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ oldBr = SelectObject(hdc, hBr);
    HGDIOBJ oldPn = SelectObject(hdc, hPn);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, CW_Scale(8), CW_Scale(8));
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPn);
    DeleteObject(hBr);
    DeleteObject(hPn);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, bannerText);

    /* Use a larger, bolder font for high-contrast update messaging. */
    HFONT hMsgFont = makeFont(15, FW_SEMIBOLD, "Tahoma");
    HGDIOBJ oldMsgFont = NULL;
    if (hMsgFont)
        oldMsgFont = SelectObject(hdc, hMsgFont);

    char prefixMsg[220];
    _snprintf(prefixMsg, sizeof(prefixMsg),
              "New release: ClamWin %s is available!  ",
              m_newVersion);
    prefixMsg[sizeof(prefixMsg) - 1] = '\0';

    const char* linkMsg = "Click here to download";
    const char* suffixMsg = ".";

    RECT textRc = { rc.left + CW_Scale(16), rc.top, rc.right - CW_Scale(16), rc.bottom };

    /* Draw prefix text. */
    DrawTextA(hdc, prefixMsg, -1, &textRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    SIZE prefixSz = {0, 0};
    GetTextExtentPoint32A(hdc, prefixMsg, lstrlenA(prefixMsg), &prefixSz);
    int linkX = textRc.left + prefixSz.cx;

    /* Draw link text underlined for hyperlink affordance. */
    HFONT hUnderline = NULL;
    HFONT oldFont = NULL;
    LOGFONTA lf;
    ZeroMemory(&lf, sizeof(lf));
    if (hMsgFont && GetObjectA(hMsgFont, sizeof(lf), &lf) == sizeof(lf))
    {
        lf.lfUnderline = TRUE;
        lf.lfWeight = FW_BOLD;
        hUnderline = CreateFontIndirectA(&lf);
    }
    if (hUnderline)
        oldFont = (HFONT)SelectObject(hdc, hUnderline);

    RECT linkRc = { linkX, rc.top, rc.right - CW_Scale(16), rc.bottom };
    DrawTextA(hdc, linkMsg, -1, &linkRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    if (oldFont)
        SelectObject(hdc, oldFont);
    if (hUnderline)
        DeleteObject(hUnderline);

    SIZE linkSz = {0, 0};
    GetTextExtentPoint32A(hdc, linkMsg, lstrlenA(linkMsg), &linkSz);

    RECT suffixRc = { linkX + linkSz.cx, rc.top, rc.right - CW_Scale(16), rc.bottom };
    DrawTextA(hdc, suffixMsg, -1, &suffixRc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

    if (oldMsgFont)
        SelectObject(hdc, oldMsgFont);
    if (hMsgFont && hMsgFont != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
        DeleteObject(hMsgFont);
}

void CWDashboard::onPaint(HDC hdc)
{
    CWTheme* theme = CW_GetTheme();
    RECT client;
    GetClientRect(m_hwnd, &client);

    /* Double-buffer */
    HDC      hdcMem = CreateCompatibleDC(hdc);
    HBITMAP  hbm    = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    HGDIOBJ  hbmOld = SelectObject(hdcMem, hbm);

    /* Background */
    HBRUSH hBg = CreateSolidBrush(theme ? theme->colorBg() : RGB(255, 255, 255));
    FillRect(hdcMem, &client, hBg);
    DeleteObject(hBg);

    if (m_updateAvailable)
        paintUpdateBanner(hdcMem, client);
    paintBanner(hdcMem, client);
    paintCards(hdcMem, client);
    paintStatusBar(hdcMem, client);

    BitBlt(hdc, 0, 0, client.right, client.bottom, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
}

/* ─── onMessage — handle mouse and size ─────────────────────── */

LRESULT CWDashboard::onMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CLOSE:
            hide();
            return 0;

        case WM_MOUSEMOVE:
        {
            POINT pt = { LOWORD(lp), HIWORD(lp) };
            int newHover = cardAtPoint(pt);

            /* Check if hovering the update banner */
            bool onBanner = false;
            if (m_updateAvailable)
            {
                RECT bannerRc = { CW_Scale(12), CW_Scale(12),
                                  0, CW_Scale(12) + CW_Scale(38) };
                RECT client;
                GetClientRect(m_hwnd, &client);
                bannerRc.right = client.right - CW_Scale(12);
                onBanner = PtInRect(&bannerRc, pt) != 0;
            }

            if (newHover != m_hoverCard)
            {
                m_hoverCard = newHover;
                invalidate();
                TRACKMOUSEEVENT tme;
                tme.cbSize    = sizeof(tme);
                tme.dwFlags   = TME_LEAVE;
                tme.hwndTrack = m_hwnd;
                TrackMouseEvent(&tme);
            }
            SetCursor(LoadCursorA(NULL,
                      (m_hoverCard >= 0 || onBanner) ? (LPCSTR)IDC_HAND : (LPCSTR)IDC_ARROW));
            return 0;
        }

        case WM_MOUSELEAVE:
            if (m_hoverCard >= 0) { m_hoverCard = -1; invalidate(); }
            return 0;

        case WM_LBUTTONUP:
        {
            POINT pt = { LOWORD(lp), HIWORD(lp) };

            /* Check click on update banner first */
            if (m_updateAvailable)
            {
                RECT bannerRc = { CW_Scale(12), CW_Scale(12),
                                  0, CW_Scale(12) + CW_Scale(38) };
                RECT client;
                GetClientRect(m_hwnd, &client);
                bannerRc.right = client.right - CW_Scale(12);
                if (PtInRect(&bannerRc, pt))
                {
                    ShellExecuteA(m_hwnd, "open",
                                  CWUpdateChecker::downloadUrl(),
                                  NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
            }

            int card = cardAtPoint(pt);
            if (card >= 0) postTrayCommand(s_cards[card].id);
            return 0;
        }

        case WM_SYSKEYDOWN:
        {
            const bool showNow = ((GetKeyState(VK_MENU) & 0x8000) != 0) || (wp == VK_MENU);
            if (showNow != m_showMnemonics)
            {
                m_showMnemonics = showNow;
                invalidate();
            }

            /* Consume bare Alt to avoid SC_KEYMENU focus/menu activation. */
            if (wp == VK_MENU)
                return 0;
            break;
        }

        case WM_SYSKEYUP:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                invalidate();
            }

            if (wp == VK_MENU)
                return 0;
            break;

        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_KEYMENU)
                return 0;
            break;

        case WM_KILLFOCUS:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                invalidate();
            }
            break;

        case WM_SYSCHAR:
        {
            switch ((char)wp)
            {
                case 'f': case 'F': postTrayCommand(IDC_CARD_SCAN);     return 0;
                case 'u': case 'U': postTrayCommand(IDC_CARD_UPDATE);   return 0;
                case 'm': case 'M': postTrayCommand(IDC_CARD_SCANMEM);  return 0;
                case 'r': case 'R': postTrayCommand(IDC_CARD_REPORTS);  return 0;
                case 'p': case 'P': postTrayCommand(IDC_CARD_PREFS);    return 0;
                case 'c': case 'C': postTrayCommand(IDC_CARD_SCHEDULE); return 0;
                case 'h': case 'H': postTrayCommand(IDC_CARD_HELP);     return 0;
                case 'a': case 'A': postTrayCommand(IDC_CARD_ABOUT);    return 0;
            }
            break;
        }

        case WM_ERASEBKGND:
            return 1;   /* handled in WM_PAINT */

        case WM_SIZE:
            updateCardTooltipRects();
            break;

        case WM_DESTROY:
            destroyCardTooltips();
            break;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            mmi->ptMinTrackSize.x = CW_Scale(460);
            mmi->ptMinTrackSize.y = CW_Scale(496) + (m_updateAvailable ? CW_Scale(46) : 0);
            return 0;
        }

        case WM_SETTINGCHANGE:
            if (CW_GetTheme())
                CW_GetTheme()->updateSystemTheme();
            invalidate();
            return 0;
    }
    return CWWindow::onMessage(msg, wp, lp);
}

/* ─── onCommand ─────────────────────────────────────────────── */

void CWDashboard::onCommand(int id, HWND src)
{
    (void)src;
    postTrayCommand(id);
}

/* ─── postTrayCommand ───────────────────────────────────────── */

void CWDashboard::postTrayCommand(int menuId)
{
    /* Map card ID to tray menu ID */
    int trayId = menuId;
    switch (menuId)
    {
        case IDC_CARD_SCAN:     trayId = IDM_TRAY_SCAN;       break;
        case IDC_CARD_UPDATE:   trayId = IDM_TRAY_UPDATE;     break;
        case IDC_CARD_SCANMEM:  trayId = IDM_TRAY_SCANMEM;    break;
        case IDC_CARD_REPORTS:  trayId = IDM_TRAY_REPORTS;    break;
        case IDC_CARD_PREFS:    trayId = IDM_TRAY_PREFS;      break;
        case IDC_CARD_SCHEDULE: trayId = IDM_TRAY_SCHEDULE;   break;
        case IDC_CARD_HELP:     trayId = IDM_TRAY_HELP;       break;
        case IDC_CARD_ABOUT:    trayId = IDM_TRAY_ABOUT;      break;
    }
    
    /* Post directly to the tray window to avoid infinite PostMessage loops */
    if (CWApplication::instance())
    {
        HWND target = CWApplication::instance()->getTrayHwnd();
        if (target) PostMessageA(target, WM_COMMAND, (WPARAM)trayId, 0);
    }
}

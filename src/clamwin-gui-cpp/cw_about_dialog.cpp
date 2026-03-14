/*
 * ClamWin Free Antivirus — CWAboutDialog
 *
 * Modern About dialog with themed layout, logo panels,
 * and clickable links. No DB info shown (banner has it).
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_about_dialog.h"
#include "cw_dpi.h"
#include "cw_theme.h"
#include "clamav-version.h"
#include <commctrl.h>
#include <stdio.h>
#include <gdiplus.h>
#include <vector>
#include <string>

#define IDC_LINK_WEB      4001
#define IDC_LINK_CLAMAV   4002
#define IDC_LINK_NETFARM  4003
#define IDC_LOGO_CLAMWIN  4101
#define IDC_LOGO_CLAMAV   4102
#define IDC_LOGO_NETFARM  4103

/* ─── PNG resource loader via GDI+ ──────────────────────────── */

void* CWAboutDialog::loadPngResource(int resourceId)
{
    HRSRC hRes = FindResourceA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(resourceId), (LPCSTR)RT_RCDATA);
    if (!hRes) return NULL;

    HGLOBAL hLoad = LoadResource(GetModuleHandleA(NULL), hRes);
    if (!hLoad) return NULL;

    const BYTE* data = (const BYTE*)LockResource(hLoad);
    DWORD size = SizeofResource(GetModuleHandleA(NULL), hRes);
    if (!data || size == 0) return NULL;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) return NULL;

    void* pMem = GlobalLock(hMem);
    memcpy(pMem, data, size);
    GlobalUnlock(hMem);

    IStream* pStream = NULL;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream)))
    {
        GlobalFree(hMem);
        return NULL;
    }

    Gdiplus::Image* img = Gdiplus::Image::FromStream(pStream);
    pStream->Release();

    if (img && img->GetLastStatus() != Gdiplus::Ok)
    {
        delete img;
        return NULL;
    }
    return img;
}

/* ─── Constructor / Destructor ──────────────────────────────── */

CWAboutDialog::CWAboutDialog(CWConfig& cfg)
    : m_cfg(cfg)
    , m_hFontTitle(NULL), m_hFontSection(NULL)
    , m_hFontNormal(NULL), m_hFontSmall(NULL), m_hFontLink(NULL)
    , m_hwndBtnOk(NULL)
    , m_hwndLinkWeb(NULL), m_hwndLinkClamAV(NULL), m_hwndLinkNetfarm(NULL)
    , m_pImgClamAV(NULL), m_pImgClamWin(NULL), m_pImgNetfarm(NULL)
{
}

CWAboutDialog::~CWAboutDialog()
{
    destroyFonts();
    destroyImages();
}

void CWAboutDialog::createFonts()
{
    m_hFontTitle = CreateFontA(-CW_Scale(20), 0, 0, 0, FW_BOLD, 0, 0, 0,
                               DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    if (!m_hFontTitle)
        m_hFontTitle = CreateFontA(-CW_Scale(20), 0, 0, 0, FW_BOLD, 0, 0, 0,
                                   DEFAULT_CHARSET, 0, 0, DEFAULT_QUALITY, 0, "Tahoma");

    m_hFontSection = CreateFontA(-CW_Scale(13), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                                 DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    if (!m_hFontSection)
        m_hFontSection = CreateFontA(-CW_Scale(13), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                                     DEFAULT_CHARSET, 0, 0, DEFAULT_QUALITY, 0, "Tahoma");

    m_hFontNormal = CreateFontA(-CW_Scale(12), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    if (!m_hFontNormal)
        m_hFontNormal = CreateFontA(-CW_Scale(12), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                    DEFAULT_CHARSET, 0, 0, DEFAULT_QUALITY, 0, "Tahoma");

    m_hFontSmall = CreateFontA(-CW_Scale(11), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                               DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    if (!m_hFontSmall)
        m_hFontSmall = CreateFontA(-CW_Scale(11), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                   DEFAULT_CHARSET, 0, 0, DEFAULT_QUALITY, 0, "Tahoma");

    m_hFontLink = CreateFontA(-CW_Scale(12), 0, 0, 0, FW_NORMAL, TRUE, 0, 0,
                              DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, "Segoe UI");
    if (!m_hFontLink)
        m_hFontLink = CreateFontA(-CW_Scale(12), 0, 0, 0, FW_NORMAL, TRUE, 0, 0,
                                  DEFAULT_CHARSET, 0, 0, DEFAULT_QUALITY, 0, "Tahoma");
}

void CWAboutDialog::destroyFonts()
{
    auto del = [](HFONT& hf) {
        if (hf) { DeleteObject(hf); hf = NULL; }
    };
    del(m_hFontTitle);
    del(m_hFontSection);
    del(m_hFontNormal);
    del(m_hFontSmall);
    del(m_hFontLink);
}

void CWAboutDialog::destroyImages()
{
    auto del = [](void*& ptr) {
        if (ptr) { delete reinterpret_cast<Gdiplus::Image*>(ptr); ptr = NULL; }
    };
    del(m_pImgClamAV);
    del(m_pImgClamWin);
    del(m_pImgNetfarm);
}

/* ─── Control helpers ───────────────────────────────────────── */

HWND CWAboutDialog::addLabel(const char* text, int x, int y, int w, int h, HFONT font, int id)
{
    HWND hw = CreateWindowExA(0, "STATIC", text,
                              WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                              x, y, w, h, m_hwnd,
                              id ? (HMENU)(INT_PTR)id : NULL,
                              GetModuleHandleA(NULL), NULL);
    if (hw && font) SendMessageA(hw, WM_SETFONT, (WPARAM)font, 0);
    return hw;
}

HWND CWAboutDialog::addLink(const char* text, int x, int y, int w, int h, HFONT font, int id)
{
    HWND hw = CreateWindowExA(0, "STATIC", text,
                              WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_NOTIFY,
                              x, y, w, h, m_hwnd,
                              (HMENU)(INT_PTR)id,
                              GetModuleHandleA(NULL), NULL);
    if (hw && font) SendMessageA(hw, WM_SETFONT, (WPARAM)font, 0);
    return hw;
}

HWND CWAboutDialog::addBitmap(int id, int x, int y, int w, int h)
{
    HWND hw = CreateWindowExA(0, "STATIC", NULL,
                              WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                              x, y, w, h, m_hwnd,
                              (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    return hw;
}

/* ─── onInit ────────────────────────────────────────────────── */

bool CWAboutDialog::onInit()
{
    createFonts();

    /* Load logo images from embedded resources */
    m_pImgClamWin  = loadPngResource(IDB_CLAMWIN_LOGO);
    m_pImgClamAV   = loadPngResource(IDB_CLAMAV_LOGO);
    m_pImgNetfarm  = loadPngResource(IDB_NETFARM_LOGO);

    /* Set window icon to force left margin spacing in the title bar */
    HICON hIcon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(100)); // 100 is clamwin.ico in clamwin.rc
    if (hIcon)
    {
        SendMessageA(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessageA(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    const int dlgW = CW_Scale(520);
    const int pad = CW_Scale(32);
    
    /* Layout grid coordinates */
    const int iconSize = CW_Scale(64);
    const int textX = pad + iconSize + CW_Scale(20);
    const int contentW = dlgW - textX - pad;
    const int contentRight = dlgW - pad;

    int y = CW_Scale(20);

    /* Header block */
    int yClamWin = y;
    addLabel("ClamWin Free Antivirus", textX, y, contentW, CW_Scale(28), m_hFontTitle);
    y += CW_Scale(30);

    /* Retrieve version from resources */
    std::string versionStr = CLAMWIN_VERSION_STR;
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH))
    {
        DWORD dummy;
        DWORD infoSize = GetFileVersionInfoSizeA(exePath, &dummy);
        if (infoSize > 0)
        {
            std::vector<BYTE> infoData(infoSize);
            if (GetFileVersionInfoA(exePath, dummy, infoSize, infoData.data()))
            {
                struct LANGANDCODEPAGE {
                    WORD wLanguage;
                    WORD wCodePage;
                } *lpTranslate;
                UINT cbTranslate;

                if (VerQueryValueA(infoData.data(), "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate) && cbTranslate >= sizeof(LANGANDCODEPAGE))
                {
                    char subBlock[256];
                    snprintf(subBlock, sizeof(subBlock), "\\StringFileInfo\\%04x%04x\\FileVersion", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
                    
                    char* lpBuffer = NULL;
                    UINT cbBufSize = 0;
                    if (VerQueryValueA(infoData.data(), subBlock, (LPVOID*)&lpBuffer, &cbBufSize) && cbBufSize > 0)
                    {
                        versionStr = lpBuffer;
                    }
                }
            }
        }
    }

    std::string verLabel = "Version " + versionStr;
    addLabel(verLabel.c_str(), textX, y, contentW, CW_Scale(18), m_hFontNormal);
    y += CW_Scale(18);
    m_hwndLinkWeb = addLink(CLAMWIN_WEBSITE, textX, y, contentW, CW_Scale(18),
                            m_hFontLink, IDC_LINK_WEB);
    y += CW_Scale(26); /* Increased from 20 for more space below text block */

    int hClamWin = y - yClamWin;
    addBitmap(IDC_LOGO_CLAMWIN, pad, yClamWin + (hClamWin - iconSize)/2, iconSize, iconSize);

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                    pad, y, contentRight - pad, CW_Scale(1), m_hwnd,
                    NULL, GetModuleHandleA(NULL), NULL);
    y += CW_Scale(18);

    /* Scanning Engine row */
    int yClamAV = y;
    addLabel("Scanning Engine", textX, y, contentW, CW_Scale(20), m_hFontSection);
    y += CW_Scale(20);
    addLabel("Powered by ClamAV version " CLAMAV_VERSION ", maintained by Cisco Talos", textX, y, contentW, CW_Scale(18), m_hFontNormal);
    y += CW_Scale(18);
    m_hwndLinkClamAV = addLink("https://www.clamav.net", textX, y, contentW, CW_Scale(18),
                               m_hFontLink, IDC_LINK_CLAMAV);
    y += CW_Scale(26); /* Increased */

    int hClamAV = y - yClamAV;
    addBitmap(IDC_LOGO_CLAMAV, pad, yClamAV + (hClamAV - iconSize)/2, iconSize, iconSize);

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                    pad, y, contentRight - pad, CW_Scale(1), m_hwnd,
                    NULL, GetModuleHandleA(NULL), NULL);
    y += CW_Scale(18); /* Increased */

    /* Legacy port row */
    int yNetfarm = y;
    addLabel("Legacy Windows Ports", textX, y, contentW, CW_Scale(20), m_hFontSection);
    y += CW_Scale(20);
    addLabel("Netfarm S.r.l.", textX, y, contentW, CW_Scale(18), m_hFontNormal);
    y += CW_Scale(18);
    m_hwndLinkNetfarm = addLink("https://oss.netfarm.it/clamav/", textX, y, contentW, CW_Scale(18),
                                m_hFontLink, IDC_LINK_NETFARM);
    y += CW_Scale(26); /* Increased */

    int hNetfarm = y - yNetfarm;
    
    /* Optically align Netfarm logo (which has no internal padding) to match ClamAV visual weight */
    const int netfarmSize = CW_Scale(46);
    const int netfarmX = pad + (iconSize - netfarmSize) / 2;
    addBitmap(IDC_LOGO_NETFARM, netfarmX, yNetfarm + (hNetfarm - netfarmSize)/2, netfarmSize, netfarmSize);

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                    pad, y, contentRight - pad, CW_Scale(1), m_hwnd,
                    NULL, GetModuleHandleA(NULL), NULL);
    y += CW_Scale(18); /* Increased */

    /* Authors row */
    addLabel("Authors", textX, y, contentW, CW_Scale(20), m_hFontSection);
    y += CW_Scale(20);
    addLabel("Alex Cherney <alex@clamwin.com>", textX, y, contentW, CW_Scale(18), m_hFontNormal);
    y += CW_Scale(18);
    addLabel("Gianluigi Tiesi <sherpya@netfarm.it>", textX, y, contentW, CW_Scale(18), m_hFontNormal);
    y += CW_Scale(26); /* Increased */

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                    pad, y, contentRight - pad, CW_Scale(1), m_hwnd,
                    NULL, GetModuleHandleA(NULL), NULL);
    y += CW_Scale(18); /* Increased */

    /* Centered legal text */
    {
        HWND h = addLabel("Copyright ClamWin Pty Ltd (c) 2004-2026\n"
                          "Portions Copyright Cisco Inc. (ClamAV)\n"
                          "ClamWin is not affiliated with ClamAV or Cisco Inc.", 
                          pad, y, contentRight - pad, CW_Scale(56), m_hFontSmall);
        if (h) {
            SetWindowLongPtrA(h, GWL_STYLE, GetWindowLongPtrA(h, GWL_STYLE) | SS_CENTER);
            SetWindowLongPtrA(h, GWLP_ID, 4201);
        }
    }
    y += CW_Scale(60); 
    {
        HWND h = addLabel("This program is free software; you can redistribute it and/or modify "
                          "it under the terms of the GNU General Public License (GPLv2).",
                          pad, y, contentRight - pad, CW_Scale(40), m_hFontSmall); /* Increased height to 40 so it doesn't clip */
        if (h) {
            SetWindowLongPtrA(h, GWL_STYLE, GetWindowLongPtrA(h, GWL_STYLE) | SS_CENTER);
            SetWindowLongPtrA(h, GWLP_ID, 4201);
        }
    }
    y += CW_Scale(46); /* Increased space explicitly above OK button to un-overlap it */

    /* ── OK button ──────────────────────────────────────────── */
    m_hwndBtnOk = CreateWindowExA(0, "BUTTON", "&OK",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
                                  (dlgW - CW_Scale(96)) / 2, y,
                                  CW_Scale(96), CW_Scale(30),
                                  m_hwnd, (HMENU)IDOK, GetModuleHandleA(NULL), NULL);
    SendMessageA(m_hwndBtnOk, WM_SETFONT, (WPARAM)m_hFontNormal, 0);

    SetWindowTextA(m_hwnd, "About ClamWin Free Antivirus");
    SetFocus(m_hwndBtnOk);
    return false; /* Did set focus ourselves */
}

/* ─── onCommand ─────────────────────────────────────────────── */

bool CWAboutDialog::onCommand(int id, HWND src)
{
    (void)src;
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
            endDialog(id);
            return true;

        case IDC_LINK_WEB:
            ShellExecuteA(m_hwnd, "open", CLAMWIN_WEBSITE, NULL, NULL, SW_SHOWNORMAL);
            return true;

        case IDC_LINK_CLAMAV:
            ShellExecuteA(m_hwnd, "open", "https://www.clamav.net", NULL, NULL, SW_SHOWNORMAL);
            return true;

        case IDC_LINK_NETFARM:
            ShellExecuteA(m_hwnd, "open", "https://oss.netfarm.it/clamav/", NULL, NULL, SW_SHOWNORMAL);
            return true;
    }
    return false;
}

/* ─── handleMessage — custom painting for links ─────────────── */

INT_PTR CWAboutDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (!dis || dis->CtlType != ODT_STATIC)
                break;

            void* ptrImg = NULL;
            if (dis->CtlID == IDC_LOGO_CLAMWIN) ptrImg = m_pImgClamWin;
            else if (dis->CtlID == IDC_LOGO_CLAMAV) ptrImg = m_pImgClamAV;
            else if (dis->CtlID == IDC_LOGO_NETFARM) ptrImg = m_pImgNetfarm;
            else break;

            CWTheme* theme = CW_GetTheme();
            COLORREF bg = theme ? theme->colorBg() : GetSysColor(COLOR_3DFACE);
            HBRUSH hBg = CreateSolidBrush(bg);
            FillRect(dis->hDC, &dis->rcItem, hBg);
            DeleteObject(hBg);

            if (ptrImg)
            {
                Gdiplus::Image* img = reinterpret_cast<Gdiplus::Image*>(ptrImg);
                Gdiplus::Graphics g(dis->hDC);
                g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

                const int rcW = dis->rcItem.right - dis->rcItem.left;
                const int rcH = dis->rcItem.bottom - dis->rcItem.top;
                int drawW = img->GetWidth();
                int drawH = img->GetHeight();

                /* Auto-scale down if image is larger than control * 1.5 approx for DPI,
                   but keeping it simple: just draw it scaled into the rect keeping aspect ratio */
                float scaleX = (float)rcW / (float)drawW;
                float scaleY = (float)rcH / (float)drawH;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                if (scale > 1.0f) scale = 1.0f; // Don't scale up

                drawW = (int)(drawW * scale);
                drawH = (int)(drawH * scale);

                int x = dis->rcItem.left + (rcW - drawW) / 2;
                int y = dis->rcItem.top + (rcH - drawH) / 2;

                g.DrawImage(img, x, y, drawW, drawH);
            }
            return TRUE;
        }

        case WM_CTLCOLORSTATIC:
        {
            HWND ctrl = (HWND)lp;
            HDC hdc = (HDC)wp;
            CWTheme* theme = CW_GetTheme();
            int ctlId = GetWindowLongA(ctrl, GWL_ID);

            /* Color link controls blue */
            if (ctrl == m_hwndLinkWeb || ctrl == m_hwndLinkClamAV || ctrl == m_hwndLinkNetfarm)
            {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, theme ? theme->colorAccent() : RGB(0, 102, 204));
                return (INT_PTR)(theme ? theme->brushBg() : GetStockObject(WHITE_BRUSH));
            }
            else if (ctlId == 4201) /* Muted text */
            {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, theme ? theme->colorTextMuted() : RGB(128, 128, 128));
                return (INT_PTR)(theme ? theme->brushBg() : GetStockObject(WHITE_BRUSH));
            }
            break;
        }

        case WM_SETCURSOR:
        {
            HWND ctrl = (HWND)wp;
            if (ctrl == m_hwndLinkWeb || ctrl == m_hwndLinkClamAV || ctrl == m_hwndLinkNetfarm)
            {
                SetCursor(LoadCursorA(NULL, (LPCSTR)32649)); /* IDC_HAND */
                return TRUE;
            }
            break;
        }
    }

    return CWDialog::handleMessage(msg, wp, lp);
}

/* ─── C Wrapper ─────────────────────────────────────────────── */
void CW_AboutDialogRun(HWND hwndParent, CWConfig *cfg)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    {
        CWAboutDialog dlg(*cfg);
        dlg.runModal(hwndParent, 520, 515);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
}

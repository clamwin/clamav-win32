/*
 * ClamWin Free Antivirus — CWTheme
 *
 * Dynamic system Light/Dark mode aesthetics with Google Drive-style palette.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once

#include <windows.h>

class CWTheme
{
public:
    CWTheme();
    ~CWTheme();

    /* Reloads registry keys and updates brushes. Call on WM_SETTINGCHANGE. */
    void updateSystemTheme();

    /* Accessors for current system state */
    bool isDark() const { return m_useDarkTheme; }
    bool useClassicPalette() const { return m_useClassicPalette; }

    /* --- Theme Palette --- */
    
    /* Backgrounds */
    COLORREF colorBg() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_3DFACE);
        return m_useDarkTheme ? RGB(31, 31, 31) : RGB(240, 244, 249);
    }
    COLORREF colorSurface() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_WINDOW);
        return m_useDarkTheme ? RGB(45, 45, 45) : RGB(255, 255, 255);
    }
    COLORREF colorSurfaceHover() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_3DFACE);
        return m_useDarkTheme ? RGB(60, 60, 60) : RGB(245, 247, 251);
    }
    
    /* Text */
    COLORREF colorText() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_WINDOWTEXT);
        return m_useDarkTheme ? RGB(227, 227, 227) : RGB(31, 31, 31);
    }
    COLORREF colorTextMuted() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_GRAYTEXT);
        return m_useDarkTheme ? RGB(160, 160, 160) : RGB(90, 90, 90);
    }
    
    /* Accents */
    COLORREF colorAccent() const
    {
        if (m_useClassicPalette) return GetSysColor(COLOR_HIGHLIGHT);
        return RGB(63, 141, 253);
    } /* Google Blue */
    COLORREF colorSuccess() const { return RGB(15, 157, 88); } /* Google Green */
    COLORREF colorWarning() const { return RGB(234, 67, 53); } /* Google Red */

    /* --- GDI Resources (Cached for performance) --- */
    HBRUSH brushBg() const { return m_brBg; }
    HBRUSH brushSurface() const { return m_brSurface; }
    HBRUSH brushSurfaceHover() const { return m_brSurfaceHover; }
    HBRUSH brushAccent() const { return m_brAccent; }

private:
    /* No copy */
    CWTheme(const CWTheme&);
    CWTheme& operator=(const CWTheme&);

    void freeBrushes();
    void createBrushes();

    bool   m_useDarkTheme;
    bool   m_useClassicPalette;
    
    HBRUSH m_brBg;
    HBRUSH m_brSurface;
    HBRUSH m_brSurfaceHover;
    HBRUSH m_brAccent;
};

/* Global accessor for the current theme */
void CW_ThemeInit();
void CW_ThemeDeinit();
CWTheme* CW_GetTheme();

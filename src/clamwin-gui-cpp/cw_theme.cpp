/*
 * ClamWin Free Antivirus — CWTheme
 *
 * Dynamic system Light/Dark mode aesthetics with Google Drive-style palette.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_theme.h"

static const char* s_themeRegPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static const char* s_themeRegKey  = "AppsUseLightTheme";

static bool isLegacyClassicOs()
{
    OSVERSIONINFOA vi;
    ZeroMemory(&vi, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);
    if (!GetVersionExA(&vi))
        return false;

    if (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        return true; /* Win95/98/ME */

    if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && vi.dwMajorVersion <= 5)
        return true; /* Win2000/XP/2003 */

    return false;
}

CWTheme::CWTheme()
    : m_useDarkTheme(false) /* Safer fallback: light mode */
    , m_useClassicPalette(false)
    , m_brBg(NULL)
    , m_brSurface(NULL)
    , m_brSurfaceHover(NULL)
    , m_brAccent(NULL)
{
    updateSystemTheme();
}

CWTheme::~CWTheme()
{
    freeBrushes();
}

void CWTheme::updateSystemTheme()
{
    if (isLegacyClassicOs())
    {
        m_useClassicPalette = true;
        m_useDarkTheme = false;
        freeBrushes();
        createBrushes();
        return;
    }

    m_useClassicPalette = false;

    HKEY hKey;
    DWORD val = 0;
    DWORD len = sizeof(val);
    DWORD type = 0;

    /* Read Windows 10/11 app light theme preference
     * If 1 = Light Mode, 0 = Dark Mode.
     * Fails gracefully on older OS versions. */
    if (RegOpenKeyExA(HKEY_CURRENT_USER, s_themeRegPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, s_themeRegKey, NULL, &type, (LPBYTE)&val, &len) == ERROR_SUCCESS)
        {
            if (type == REG_DWORD)
            {
                m_useDarkTheme = (val == 0);
            }
        }
        RegCloseKey(hKey);
    }
    else
    {
        /* Could not detect theme, keep modern UI in light mode by default. */
        m_useDarkTheme = false;
    }

    freeBrushes();
    createBrushes();
}

void CWTheme::freeBrushes()
{
    if (m_brBg) { DeleteObject(m_brBg); m_brBg = NULL; }
    if (m_brSurface) { DeleteObject(m_brSurface); m_brSurface = NULL; }
    if (m_brSurfaceHover) { DeleteObject(m_brSurfaceHover); m_brSurfaceHover = NULL; }
    if (m_brAccent) { DeleteObject(m_brAccent); m_brAccent = NULL; }
}

void CWTheme::createBrushes()
{
    m_brBg = CreateSolidBrush(colorBg());
    m_brSurface = CreateSolidBrush(colorSurface());
    m_brSurfaceHover = CreateSolidBrush(colorSurfaceHover());
    m_brAccent = CreateSolidBrush(colorAccent());
}

/* Global Wrapper for components that don't pass around CWTheme 
 * (Though ideal pattern is injection via CWApplication). 
 */
static CWTheme* s_themeInstance = NULL;

void CW_ThemeInit()
{
    if (!s_themeInstance)
    {
        s_themeInstance = new CWTheme();
    }
}

void CW_ThemeDeinit()
{
    if (s_themeInstance)
    {
        delete s_themeInstance;
        s_themeInstance = NULL;
    }
}

CWTheme* CW_GetTheme()
{
    return s_themeInstance;
}

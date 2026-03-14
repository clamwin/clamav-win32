#pragma once
#include <windows.h>

inline int& CW_DpiCache()
{
    static int dpi = 0;
    return dpi;
}

inline int CW_GetScaleDpi()
{
    int dpi = CW_DpiCache();
    return dpi > 0 ? dpi : 96;
}

inline void CW_SetScaleDpi(int dpi)
{
    if (dpi <= 0) dpi = 96;
    CW_DpiCache() = dpi;
}

inline void CW_ResetScaleDpi()
{
    CW_DpiCache() = 0;
}

inline int CW_Scale(int pixels)
{
    int& dpi = CW_DpiCache();
    if (dpi == 0)
    {
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(NULL, hdc);
        }
        if (dpi == 0) dpi = 96; /* fallback to 100% */
    }
    return MulDiv(pixels, dpi, 96);
}

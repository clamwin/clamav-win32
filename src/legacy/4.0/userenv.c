/*
 * Legacy Windows Compatibility Layer
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _WIN64
#include "legacy.h"
#include "dynload.h"
#include "loadlibrary.h"
#include "initializer.h"

BOOL WINAPI GetUserProfileDirectoryA_compat(HANDLE hToken, LPSTR lpProfileDir, LPDWORD lpcchSize)
{
    char szPath[MAX_PATH];
    DWORD dwSize;

    if (lpcchSize == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(dwSize = GetWindowsDirectoryA(szPath, MAX_PATH - 1)))
        return FALSE;

    dwSize++;

    if (*lpcchSize < dwSize)
    {
        *lpcchSize = dwSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *lpcchSize = dwSize;
    strncpy(lpProfileDir, szPath, dwSize - 1);
    lpProfileDir[dwSize - 1] = 0;
    return TRUE;
}

BOOL WINAPI GetUserProfileDirectoryW_compat(HANDLE hToken, LPWSTR lpProfileDir, LPDWORD lpcchSize)
{
    char szPath[MAX_PATH + 1];

    if (lpcchSize == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetUserProfileDirectoryA_compat(NULL, szPath, lpcchSize))
        return FALSE;

    if (!MultiByteToWideChar(CP_ACP, 0, szPath, *lpcchSize, lpProfileDir, *lpcchSize))
        return FALSE;

    return TRUE;
}

imp_GetUserProfileDirectoryW pGetUserProfileDirectoryW = GetUserProfileDirectoryW_compat;

INITIALIZER(init_userenv_4_0)
{
    OSVERSIONINFO osvi = {0};
    TRACE("Init @ " __FILE__ "\n");

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

#ifdef _UNICODE
    if (osvi.dwMajorVersion < 5) // win2k or later
        return;

    HMODULE userenv = LoadLibraryFromWin32(TEXT("userenv.dll"));
    if (userenv)
        IMPORT_FUNCTION(userenv, GetUserProfileDirectoryW);
#endif
}

#endif // _WIN64

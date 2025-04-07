/*
 * Clamav Native Windows Port: UAC helper
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef _UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <shellapi.h>

#include "dynload.h"

imp_AttachConsole pAttachConsole = NULL;
imp_GetConsoleProcessList pGetConsoleProcessList = NULL;

static bool IsProcessElevated()
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;

    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    bool isElevated = false;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        isElevated = (elevation.TokenIsElevated != 0);

    CloseHandle(hToken);
    return isElevated;
}

static void AttachParentConsole(void)
{
    if (!pAttachConsole || !pGetConsoleProcessList)
        return;

    DWORD dwProcessList[1];
    if (pGetConsoleProcessList(dwProcessList, 1) != 1)
        return;

    FreeConsole();

    if (!pAttachConsole(ATTACH_PARENT_PROCESS))
    {
        wchar_t text[1024];
        wsprintf(text, L"Failed with error %ld", GetLastError());
        MessageBox(NULL, text, L"AttachConsole", MB_OK);
        AllocConsole();
    }

    fflush(stdout);
    fflush(stderr);

    (void)freopen("CONOUT$", "w", stdout);
    (void)freopen("CONOUT$", "w", stderr);
    (void)freopen("CONIN$", "r", stdin);
}

BOOL EnsureElevated()
{
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionEx(&osvi) || (osvi.dwMajorVersion < 6))
        return TRUE;

    if (IsProcessElevated())
    {
        AttachParentConsole();
        return TRUE;
    }

    wchar_t szExePath[MAX_PATH];
    if (!GetModuleFileName(NULL, szExePath, MAX_PATH - 1))
    {
        fprintf(stderr, "GetModuleFileName() failed with %ld\n", GetLastError());
        return FALSE;
    }

    int argc;
    LPWSTR cmdLine = GetCommandLineW();
    LPWSTR *argv = CommandLineToArgvW(cmdLine, &argc);

    if (!argv)
    {
        fprintf(stderr, "CommandLineToArgvW() failed with %ld\n", GetLastError());
        return FALSE;
    }

    size_t totalLength = 0;
    for (int i = 1; i < argc; i++)
        totalLength += wcslen(argv[i]) + 1;

    wchar_t *params = malloc((totalLength + 1) * sizeof(wchar_t));

    if (!params)
    {
        LocalFree(argv);
        fprintf(stderr, "Out of memory\n");
        return false;
    }

    params[0] = L'\0';
    for (int i = 1; i < argc; i++)
    {
        wcscat(params, argv[i]);
        if (i < argc - 1)
            wcscat(params, L" ");
    }

    LocalFree(argv);

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.hwnd = NULL;
    sei.lpVerb = L"runas";
    sei.lpFile = szExePath;
    sei.lpParameters = params;
    sei.nShow = SW_HIDE;

    if (!ShellExecuteEx(&sei))
    {
        DWORD le = GetLastError();
        if (le == ERROR_CANCELLED)
            printf("No action\n");
        else
            fprintf(stderr, "ShellExecuteEx() failed with %ld\n", le);
    }

    if (sei.hProcess)
    {
        DWORD code;
        WaitForSingleObject(sei.hProcess, INFINITE);
        GetExitCodeProcess(sei.hProcess, &code);
        CloseHandle(sei.hProcess);
    }

    free(params);
    return FALSE;
}
#else
BOOL EnsureElevated()
{
    return TRUE
}
#endif // _UNICODE

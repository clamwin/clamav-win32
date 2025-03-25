/*
 * Clamav Native Windows Port: dllmain
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#include "platform.h"

#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

#ifndef _WIN64
#include "dynload.h"

BOOL bIsWow64 = FALSE;

static imp_IsWow64Process pIsWow64Process;
static imp_Wow64DisableWow64FsRedirection pWow64DisableWow64FsRedirection;

LIBCLAMAV_EXPORT BOOL disablefsredir(void)
{
    PVOID OldValue = NULL;
    if (bIsWow64)
        return pWow64DisableWow64FsRedirection(&OldValue);
    return TRUE;
}
#endif

/* avoid bombing in stupid msvcrt checks - msvcrt8 only */
#ifdef _MSC_VER
void clamavInvalidParameterHandler(const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t pReserved)
{
    fprintf(stderr, "\nW00ps!! you have something strange with this file\n(maybe crt versions mismatch)\n");

#ifdef _DEBUG
    if (expression && function && file && line)
        fwprintf(stderr, L"Expression: %s (%s at %s:%d)\n\n", expression, function, file, line);
#endif
}
#else
#define _set_invalid_parameter_handler(x)
#endif

static void processattach(void)
{
    ULONG HeapFragValue = 2;
    WSADATA wsaData;

#ifndef _WIN64
    HMODULE kernel32 = GetModuleHandleW(L"kernel32");
    if (IMPORT_KERNEL32_FUNC(IsWow64Process))
    {
        if (!pIsWow64Process(GetCurrentProcess(), &bIsWow64))
            fprintf(stderr, "[dllmain] IsWow64Process() failed %d\n", GetLastError());
        else if (bIsWow64)
            IMPORT_KERNEL32_FUNC(Wow64DisableWow64FsRedirection);
    }
#endif

#if _WIN32_WINNT >= _WIN32_WINNT_WINXP
    if (!IsDebuggerPresent())
    {
        if (!HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue)))
        {
            DWORD le = GetLastError();
            /* ERROR_GEN_FAILURE on wine */
            if ((le != ERROR_NOT_SUPPORTED) && (le != ERROR_CALL_NOT_IMPLEMENTED) && (le != ERROR_GEN_FAILURE))
                fprintf(stderr, "[DllMain] Error setting up low-fragmentation heap: le=%ld\n", le);
        }
    }
#endif
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
        fprintf(stderr, "[DllMain] Error at WSAStartup(): %d\n", WSAGetLastError());

#ifndef _WIN64
    /* Some of Windows API tries to load dll from system32 and if fs redirection
       is disabled it will fail because the image loaded is 64bit, so we will preload
       needed ones (I hope :D) */
    if (bIsWow64)
    {
        LoadLibraryW(L"mswsock.dll");
        LoadLibraryW(L"winrnr.dll");
        LoadLibraryW(L"wshtcpip.dll");
        LoadLibraryW(L"iphlpapi.dll");
        LoadLibraryW(L"rsaenh.dll");
    }
#endif
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        processattach();
        _set_invalid_parameter_handler(clamavInvalidParameterHandler);
        break;
    case DLL_PROCESS_DETACH:
        WSACleanup();
        break;
    }
    return TRUE;
}

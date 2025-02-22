/*
 * Clamav Native Windows Port: dllmain
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include "osdeps.h"

#include <stdbool.h>
#include <pthread.h>

extern void tls_index_alloc(void);
extern void tls_index_free(void);
extern void tls_storage_alloc(void);
extern void tls_storage_free(void);

extern void jit_init(void);
extern void jit_uninit(void);

#ifndef _WIN64
BOOL bIsWow64 = FALSE;
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

static void cwi_processattach(void)
{
    ULONG HeapFragValue = 2;
    WSADATA wsaData;

#ifndef _WIN64
    if (!IsWow64Process(GetCurrentProcess(), &bIsWow64))
        fprintf(stderr, "[dllmain] IsWow64Process() failed %d\n", GetLastError());
#endif
    jit_init();

    if (!IsDebuggerPresent())
    {
        if (!HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue)))
        {
            DWORD le = GetLastError();
            /* ERROR_GEN_FAILURE on wine */
            if ((le != ERROR_NOT_SUPPORTED) && (le != ERROR_CALL_NOT_IMPLEMENTED) && (le != ERROR_GEN_FAILURE))
                fprintf(stderr, "[DllMain] Error setting up low-fragmentation heap: le=%d\n", le);
        }
    }

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
        fprintf(stderr, "[DllMain] Error at WSAStartup(): %d\n", WSAGetLastError());

#ifndef _WIN64
    /* Some of Windows API tries to load dll from system32 and if fs redirection
       is disabled it will fail because the image loaded is 64bit, so we will preload
       needed ones (I hope :D) */
    if (bIsWow64)
    {
        /* winsock */
        LoadLibrary("mswsock.dll");
        LoadLibrary("winrnr.dll");
        LoadLibrary("wshtcpip.dll");

        /* wintrust for sigcheck */
        LoadLibrary("rsaenh.dll");
    }
#endif
}

extern int cw_sig_init(void);
int cw_init(void)
{
    return cw_sig_init();
}

static int cw_getregvalue(const char *key, char *path)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD flags = KEY_QUERY_VALUE;
    unsigned char data[MAX_PATH];
    DWORD datalen = sizeof(data);

#ifndef _WIN64
    if (bIsWow64)
        flags |= 0x0100; /* KEY_WOW64_64KEY */
#endif

    /* First look in HKCU then in HKLM */
    if ((RegOpenKeyExA(HKEY_CURRENT_USER, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS) &&
        (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DATADIRBASEKEY, 0, flags, &hKey) != ERROR_SUCCESS))
        return 0;

    if ((RegQueryValueExA(hKey, key, NULL, &dwType, data, &datalen) == ERROR_SUCCESS) &&
            datalen && ((dwType == REG_SZ) || dwType == REG_EXPAND_SZ))
    {
        path[0] = 0;
        ExpandEnvironmentStrings((LPCSTR) data, path, MAX_PATH - 1);
        path[MAX_PATH - 1] = 0;
        RegCloseKey(hKey);
        return 1;
    }

    RegCloseKey(hKey);
    return 0;
}

/* look at win32/compat/libclamav_main.c for more info */
char _DATADIR[MAX_PATH] = "db";
char _CONFDIR[MAX_PATH] = ".";
char _CONFDIR_CLAMD[MAX_PATH] = "clamd.conf";
char _CONFDIR_FRESHCLAM[MAX_PATH] = "freshclam.conf";
char _CONFDIR_MILTER[MAX_PATH] = "clamav-milter.conf";

#undef DATADIR
#undef CONFDIR
const char *DATADIR = _DATADIR;
const char *CONFDIR = _CONFDIR;
const char *CONFDIR_CLAMD = _CONFDIR_CLAMD;
const char *CONFDIR_FRESHCLAM = _CONFDIR_FRESHCLAM;
const char *CONFDIR_MILTER = _CONFDIR_MILTER;

#define DATADIR _DATADIR
#define CONFDIR _CONFDIR
#define CONFDIR_CLAMD _CONFDIR_CLAMD
#define CONFDIR_FRESHCLAM _CONFDIR_FRESHCLAM
#define CONFDIR_MILTER _CONFDIR_MILTER

#include "common/optparser.c"

void fix_paths()
{
    if (!cw_getregvalue("ConfigDir", _CONFDIR))
    {
        char dirname[MAX_PATH] = "";
        char *lSlash;
        if (!GetModuleFileNameA(NULL, dirname, MAX_PATH - 1))
        {
            fprintf(stderr, "Please don't launch the executable from a so long path\n");
            abort();
        }

        if ((lSlash = strrchr(dirname, '\\')))
            *lSlash = 0;

        strncpy(_CONFDIR, dirname, MAX_PATH);
        _CONFDIR[MAX_PATH - 1] = 0;
    }

    if (!cw_getregvalue("DataDir", _DATADIR))
    {
        strncpy(_DATADIR, _CONFDIR, MAX_PATH);
        _DATADIR[MAX_PATH - 1] = 0;
        strncat(_DATADIR, "\\db", MAX_PATH - strlen(_DATADIR) - 1);
    }

    snprintf(_CONFDIR_CLAMD, sizeof(_CONFDIR_CLAMD), "%s\\%s", _CONFDIR, "clamd.conf");
    snprintf(_CONFDIR_FRESHCLAM, sizeof(_CONFDIR_FRESHCLAM), "%s\\%s", _CONFDIR, "freshclam.conf");
    snprintf(_CONFDIR_MILTER, sizeof(_CONFDIR_MILTER), "%s\\%s", _CONFDIR, "clamav-milter.conf");
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef PTW32_STATIC_LIB
        pthread_win32_process_attach_np();
#endif
        cwi_processattach();
        _set_invalid_parameter_handler(clamavInvalidParameterHandler);
        fix_paths();
        tls_index_alloc();
        tls_storage_alloc();
        break;
    case DLL_THREAD_ATTACH:
        tls_storage_alloc();
#ifdef PTW32_STATIC_LIB
        return pthread_win32_thread_attach_np();
#else
        return TRUE;
#endif
    case DLL_THREAD_DETACH:
        tls_storage_free();
#ifdef PTW32_STATIC_LIB
        return pthread_win32_thread_detach_np();
#else
        return TRUE;
#endif
    case DLL_PROCESS_DETACH:
        tls_storage_free();
        tls_index_free();
#ifdef PTW32_STATIC_LIB
        pthread_win32_thread_detach_np();
        pthread_win32_process_detach_np();
#endif
        WSACleanup();
        jit_uninit();
    }
    return TRUE;
}

/*
 * Clamav Native Windows Port: dllmain
 *
 * Copyright (c) 2005-2010 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <osdeps.h>
#include <pthread.h>

uint32_t cw_platform = 0;
helpers_t cw_helpers;

extern void tls_index_alloc(void);
extern void tls_index_free(void);
extern void tls_storage_alloc(void);
extern void tls_storage_free(void);

extern void jit_init(void);
extern void jit_uninit(void);

typedef struct _cbdata_t
{
    HANDLE hObject;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG dwMilliseconds;
    ULONG dwFlags;
    HANDLE hStop;
} cbdata_t;

typedef struct _tdata_t
{
    HANDLE hThread;
    DWORD  dwTid;
    cbdata_t *cbdata;
} tdata_t;

DWORD WINAPI WaitThread(LPVOID lpParam)
{
    cbdata_t *cbdata = (cbdata_t *) lpParam;
    DWORD result;
    HANDLE wEvents[2] = { cbdata->hObject, cbdata->hStop };

    result = WaitForMultipleObjects(2, wEvents, FALSE, cbdata->dwMilliseconds);

    if (result == WAIT_OBJECT_0)
        cbdata->Callback(cbdata->Context, FALSE);
    else if (result == WAIT_TIMEOUT)
        cbdata->Callback(cbdata->Context, TRUE);

    return 0;
}

BOOL WINAPI cRegisterWaitForSingleObject(PHANDLE phNewWaitObject,
                                         HANDLE hObject,
                                         WAITORTIMERCALLBACK Callback,
                                         PVOID Context,
                                         ULONG dwMilliseconds,
                                         ULONG dwFlags)
{
    tdata_t *tdata = calloc(1, sizeof(tdata_t));
    tdata->cbdata = calloc(1, sizeof(cbdata_t));
    tdata->cbdata->hObject = hObject;
    tdata->cbdata->Callback = Callback;
    tdata->cbdata->Context = Context;
    tdata->cbdata->dwMilliseconds = dwMilliseconds;
    tdata->cbdata->dwFlags = dwFlags;
    tdata->cbdata->hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    tdata->hThread = CreateThread(NULL, 0, WaitThread, (LPVOID) tdata->cbdata, 0, &tdata->dwTid);
    *phNewWaitObject = (HANDLE) tdata;
    return TRUE;
}

BOOL WINAPI cUnregisterWaitEx(HANDLE WaitHandle, HANDLE CompletionEvent)
{
    tdata_t *tdata = (tdata_t *) WaitHandle;
    SetEvent(tdata->cbdata->hStop);
    if (WaitForSingleObject(tdata->hThread, 15000) != WAIT_OBJECT_0)
    {
        fprintf(stderr, "[tpool] Warning Thread %d still alive, killing it\n", tdata->dwTid);
        TerminateThread(tdata->hThread, 0);
    }
    CloseHandle(tdata->cbdata->hStop);
    CloseHandle(tdata->hThread);

    if (CompletionEvent) SetEvent(CompletionEvent);

    free(tdata->cbdata);
    free(tdata);

    return TRUE;
}

BOOL cw_iswow64(void)
{
    BOOL bIsWow64 = FALSE;

    if (!cw_helpers.k32.wow64)
        return FALSE;

    if (cw_helpers.k32.IsWow64Process(GetCurrentProcess(), &bIsWow64))
        return bIsWow64;

    fprintf(stderr, "[dllmain] IsWow64Process() failed %d\n", GetLastError());
    return FALSE;
}

static BOOL WINAPI Wow64NullRedirFunc(PVOID *state)
{
    return TRUE;
}

#define Q(string) # string

#define IMPORT_FUNC(m, x) \
    cw_helpers.m.x = ( imp_##x ) GetProcAddress(cw_helpers.m.hLib, Q(x));

#define IMPORT_FUNC_OR_FAIL(m, x) \
    IMPORT_FUNC(m, x); \
    if (!cw_helpers.m.x) cw_helpers.m.ok = FALSE;

#define IMPORT_FUNC_OR_DISABLE(m, x, o) \
    IMPORT_FUNC(m, x); \
    if (!cw_helpers.m.x) cw_helpers.m.o = FALSE;

static void dynLoad(void)
{
    /* avoid popping messages while trying to load libraries */
    UINT oldErr = SetErrorMode(SEM_FAILCRITICALERRORS);
    SetErrorMode(oldErr | SEM_FAILCRITICALERRORS);

    memset(&cw_helpers, 0, sizeof(cw_helpers));
    memset(&cw_helpers.av32, 0, sizeof(advapi32_t));
    memset(&cw_helpers.k32, 0, sizeof(kernel32_t));
    memset(&cw_helpers.psapi, 0, sizeof(psapi_t));
    memset(&cw_helpers.ws2, 0, sizeof(ws2_32_t));
    memset(&cw_helpers.wt, 0, sizeof(wintrust_t));
    memset(&cw_helpers.dnsapi, 0, sizeof(dnsapi_t));

    /* kernel 32*/
    cw_helpers.k32.hLib = LoadLibraryA("kernel32.dll");
    if (cw_helpers.k32.hLib) /* Unlikely */
    {
        /* Win2k + */
        IMPORT_FUNC(k32, HeapSetInformation);

        /* Win64 WoW from 32 applications */
        cw_helpers.k32.wow64 = TRUE;
        IMPORT_FUNC_OR_DISABLE(k32, IsWow64Process, wow64);
        IMPORT_FUNC_OR_DISABLE(k32, Wow64DisableWow64FsRedirection, wow64);
        IMPORT_FUNC_OR_DISABLE(k32, Wow64RevertWow64FsRedirection, wow64);

        if (!cw_iswow64())
        {
            cw_helpers.k32.Wow64DisableWow64FsRedirection = Wow64NullRedirFunc;
            cw_helpers.k32.Wow64RevertWow64FsRedirection = Wow64NullRedirFunc;
        }

        cw_helpers.k32.tpool = TRUE;
        IMPORT_FUNC_OR_DISABLE(k32, RegisterWaitForSingleObject, tpool);
        IMPORT_FUNC_OR_DISABLE(k32, UnregisterWaitEx, tpool);

        if (!cw_helpers.k32.tpool)
        {
            cw_helpers.k32.RegisterWaitForSingleObject = cRegisterWaitForSingleObject;
            cw_helpers.k32.UnregisterWaitEx = cUnregisterWaitEx;
            cw_helpers.k32.tpool = TRUE;
        }

        /* kernel32 */
        cw_helpers.k32.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(k32, CreateToolhelp32Snapshot);
        IMPORT_FUNC_OR_FAIL(k32, Process32First);
        IMPORT_FUNC_OR_FAIL(k32, Process32Next);
        IMPORT_FUNC_OR_FAIL(k32, Module32First);
        IMPORT_FUNC_OR_FAIL(k32, Module32Next);
        IMPORT_FUNC_OR_FAIL(k32, CreateRemoteThread);
    }

    /* advapi32 */
    cw_helpers.av32.hLib = LoadLibraryA("advapi32.dll");
    if (cw_helpers.av32.hLib) /* Unlikely */
    {
        /* Win2k + */
        IMPORT_FUNC(av32, ChangeServiceConfig2A);

        cw_helpers.av32.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(av32, OpenProcessToken);
        IMPORT_FUNC_OR_FAIL(av32, LookupPrivilegeValueA);
        IMPORT_FUNC_OR_FAIL(av32, AdjustTokenPrivileges);
    }

    /* ws2_32 ipv6 */
    if ((cw_helpers.ws2.hLib = LoadLibraryA("wship6.dll")))
    {
        cw_helpers.ws2.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(ws2, getaddrinfo);
        IMPORT_FUNC_OR_FAIL(ws2, freeaddrinfo);
    }

    if (!cw_helpers.ws2.ok)
    {
        if (cw_helpers.ws2.hLib)
            FreeLibrary(cw_helpers.ws2.hLib);
        if ((cw_helpers.ws2.hLib = LoadLibraryA("ws2_32.dll"))) /* Unlikely */
        {
            cw_helpers.ws2.ok = TRUE;
            IMPORT_FUNC_OR_FAIL(ws2, getaddrinfo);
            IMPORT_FUNC_OR_FAIL(ws2, freeaddrinfo);
        }
    }

    /* psapi */
    cw_helpers.psapi.hLib = LoadLibraryA("psapi.dll");
    if (cw_helpers.psapi.hLib)
    {
        cw_helpers.psapi.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(psapi, EnumProcessModules);
        IMPORT_FUNC_OR_FAIL(psapi, EnumProcesses);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleBaseNameA);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleFileNameExA);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleFileNameExW);
        IMPORT_FUNC_OR_FAIL(psapi, GetModuleInformation);
        IMPORT_FUNC_OR_FAIL(psapi, GetMappedFileNameW);
    }

    /* wintrust / mscat32 /  */
    cw_helpers.wt.hLib = cw_helpers.wt.hLib_wt = LoadLibraryA("wintrust.dll");
    while (cw_helpers.wt.hLib)
    {
        cw_helpers.wt.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(wt, WinVerifyTrust);

        /* WinVerifyTrust is mandatory */
        if (!cw_helpers.wt.ok)
            break;

        /* try import cat stuff from wintrust dll */
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminAddCatalog);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminEnumCatalogFromHash);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminAcquireContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminReleaseContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminReleaseCatalogContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminCalcHashFromFileHandle);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATCatalogInfoFromContext);

        cw_helpers.wt.hLib = cw_helpers.wt.hLib_crypt32 = LoadLibraryA("crypt32.dll");
        IMPORT_FUNC_OR_FAIL(wt, CryptQueryObject);
        IMPORT_FUNC_OR_FAIL(wt, CryptMsgGetParam);
        IMPORT_FUNC_OR_FAIL(wt, CertGetNameStringA);
        IMPORT_FUNC_OR_FAIL(wt, CertFindCertificateInStore);

        /* Digital signature verification is disabled for now on old platforms,
           typically win9x. For an unknown reason the process locks the checked
           file in a way only a reboot can unlock it again */
        break;

        /* if ok I can avoid loading mscat32 */
        if (cw_helpers.wt.ok)
            break;

        cw_helpers.wt.ok = TRUE;
        cw_helpers.wt.hLib = cw_helpers.wt.hLib_mscat32 = LoadLibraryA("mscat32.dll");

        /* try importing from mscat32 */
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminAddCatalog);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminEnumCatalogFromHash);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminAcquireContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminReleaseContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminReleaseCatalogContext);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATAdminCalcHashFromFileHandle);
        IMPORT_FUNC_OR_FAIL(wt, CryptCATCatalogInfoFromContext);
        break;
    }

    cw_helpers.dnsapi.hLib = LoadLibraryA("dnsapi.dll");
    if (cw_helpers.dnsapi.hLib)
    {
        cw_helpers.dnsapi.ok = TRUE;
        IMPORT_FUNC_OR_FAIL(dnsapi, DnsQuery_A);
        IMPORT_FUNC_OR_FAIL(dnsapi, DnsRecordListFree);
    }

    /* DynLoad jit */
    jit_init();

    /* Restore original error mode */
    SetErrorMode(oldErr);
}
static void dynUnLoad(void)
{
    if (cw_helpers.wt.hCatAdmin)
        cw_helpers.wt.CryptCATAdminReleaseContext(cw_helpers.wt.hCatAdmin, 0);
    if (cw_helpers.wt.hLib_wt)
        FreeLibrary(cw_helpers.wt.hLib_wt);
    if (cw_helpers.wt.hLib_mscat32)
        FreeLibrary(cw_helpers.wt.hLib_mscat32);
    if (cw_helpers.wt.hLib_crypt32)
        FreeLibrary(cw_helpers.wt.hLib_crypt32);

    if (cw_helpers.k32.hLib)
        FreeLibrary(cw_helpers.k32.hLib);
    if (cw_helpers.av32.hLib)
        FreeLibrary(cw_helpers.av32.hLib);
    if (cw_helpers.ws2.hLib)
        FreeLibrary(cw_helpers.ws2.hLib);
    if (cw_helpers.psapi.hLib)
        FreeLibrary(cw_helpers.psapi.hLib);
    if (cw_helpers.dnsapi.hLib)
        FreeLibrary(cw_helpers.dnsapi.hLib);

    jit_uninit();
}

static uint32_t GetWindowsVersion(void)
{
    OSVERSIONINFOA osv;
    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

    if (!GetVersionEx((LPOSVERSIONINFOA) &osv))
        osv.dwPlatformId = 0x0010400; /* Worst case report as Win95 */

    return ((osv.dwPlatformId << 16) | (osv.dwMajorVersion << 8) | (osv.dwMinorVersion));
}

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

    cw_platform = GetWindowsVersion();
    dynLoad();

    if (cw_helpers.k32.HeapSetInformation && !IsDebuggerPresent())
    {
        if (!cw_helpers.k32.HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue))
            && (GetLastError() != ERROR_NOT_SUPPORTED))
            fprintf(stderr, "[DllMain] Error setting up low-fragmentation heap: %d\n", GetLastError());
    }

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
        fprintf(stderr, "[DllMain] Error at WSAStartup(): %d\n", WSAGetLastError());

    /* Some of Windows API tries to load dll from system32 and if fs redirection
       is disabled it will fail because the image loaded is 64bit, so we will preload
       needed ones (I hope :D) */
    if (cw_iswow64())
    {
        /* winsock */
        LoadLibrary("mswsock.dll");
        LoadLibrary("winrnr.dll");
        LoadLibrary("wshtcpip.dll");

        /* wintrust for sigcheck */
        LoadLibrary("rsaenh.dll");
    }
}

/* Winsock internals */
#define SO_SYNCHRONOUS_ALERT    0x10
#define SO_SYNCHRONOUS_NONALERT 0x20
#define SO_OPENTYPE             0x7008

/* currently unused */
void cw_async_noalert(void)
{
    int sockopt = SO_SYNCHRONOUS_NONALERT;

    if (setsockopt(INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (char *) &sockopt, sizeof(sockopt)) < 0)
        fprintf(stderr, "[DllMain] Error setting sockets in synchronous non-alert mode (%d)\n", WSAGetLastError());
}

extern int cw_sig_init(void);
int cw_init(void)
{
    return cw_sig_init();
}

void fix_paths();

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            pthread_win32_process_attach_np();
            cwi_processattach();
            _set_invalid_parameter_handler(clamavInvalidParameterHandler);
            fix_paths();
            tls_index_alloc();
            tls_storage_alloc();
            break;
        case DLL_THREAD_ATTACH:
            tls_storage_alloc();
            return pthread_win32_thread_attach_np();
        case DLL_THREAD_DETACH:
            tls_storage_free();
            return pthread_win32_thread_detach_np();
        case DLL_PROCESS_DETACH:
            tls_storage_free();
            tls_index_free();
            pthread_win32_thread_detach_np();
            pthread_win32_process_detach_np();
            WSACleanup();
            dynUnLoad();
    }
    return TRUE;
}

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

static int cw_getregvalue(const char *key, char *path)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD flags = KEY_QUERY_VALUE;
    unsigned char data[MAX_PATH];
    DWORD datalen = sizeof(data);

    if (cw_iswow64())
        flags |= KEY_WOW64_64KEY;

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

#include <shared/getopt.c>
#include <shared/optparser.c>

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
    }

    if (!cw_getregvalue("DataDir", _DATADIR))
    {
        strncpy(_DATADIR, _CONFDIR, MAX_PATH);
        strncat(_DATADIR, "\\db", MAX_PATH);
    }

    snprintf(_CONFDIR_CLAMD, sizeof(_CONFDIR_CLAMD), "%s\\%s", _CONFDIR, "clamd.conf");
    snprintf(_CONFDIR_FRESHCLAM, sizeof(_CONFDIR_FRESHCLAM), "%s\\%s", _CONFDIR, "freshclam.conf");
    snprintf(_CONFDIR_MILTER, sizeof(_CONFDIR_MILTER), "%s\\%s", _CONFDIR, "clamav-milter.conf");
}

/*
 *  Copyright (C) 2021-2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *  Copyright (C) 2008-2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 *  Authors: Gianluigi Tiesi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifdef _UNICODE
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <winsvc.h>

#include "dynload.h"
#include "output.h"

void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);

static SERVICE_STATUS svc;
static SERVICE_STATUS_HANDLE svc_handle;
static SERVICE_TABLE_ENTRY DT[] = {{L"Service", ServiceMain}, {NULL, NULL}};

static HANDLE evStart;
static HANDLE DispatcherThread;
static int checkpoint_every = 5000;

#if _WIN32_WINNT > _WIN32_WINNT_WINXP
#include <shellapi.h>

bool IsProcessElevated()
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
    DWORD dwProcessList[1];
    if (GetConsoleProcessList(dwProcessList, 1) != 1)
        return;

    FreeConsole();

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
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

static bool EnsureElevated()
{
    if (IsProcessElevated())
    {
        AttachParentConsole();
        return true;
    }

    wchar_t szExePath[MAX_PATH];
    if (!GetModuleFileName(NULL, szExePath, MAX_PATH - 1))
    {
        fprintf(stderr, "GetModuleFileName() failed with %ld\n", GetLastError());
        return false;
    }

    int argc;
    LPWSTR cmdLine = GetCommandLineW();
    LPWSTR *argv = CommandLineToArgvW(cmdLine, &argc);

    if (!argv)
    {
        fprintf(stderr, "CommandLineToArgvW() failed with %ld\n", GetLastError());
        return false;
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
    return false;
}
#else
#define EnsureElevated() (true)
#endif // _WIN32_WINNT > _WIN32_WINNT_WINXP

bool svc_uninstall(const wchar_t *name, bool verbose)
{
    SC_HANDLE sm, svc;
    bool ret = false;

    if (!EnsureElevated())
        return true;

    if (!(sm = OpenSCManager(NULL, NULL, DELETE)))
    {
        fwprintf(stderr, L"Unable to Open SCManager (%ld)\n", GetLastError());
        return false;
    }

    if ((svc = OpenService(sm, name, DELETE)))
    {
        if (DeleteService(svc))
        {
            if (verbose)
                wprintf(L"Service %ls successfully removed\n", name);
        }
        else
        {
            fwprintf(stderr, L"Unable to Open Service %ls (%ld)\n", name, GetLastError());
            ret = false;
        }
    }
    else
    {
        if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            if (verbose)
                wprintf(L"Service %ls does not exist\n", name);
        }
        else
        {
            fwprintf(stderr, L"Unable to Open Service %ls (%ld)\n", name, GetLastError());
            ret = false;
        }
    }

    if (svc)
        CloseServiceHandle(svc);
    CloseServiceHandle(sm);
    return ret;
}

bool svc_install(const wchar_t *name, const wchar_t *dname, wchar_t *desc)
{
    SC_HANDLE sm, svc;
    wchar_t modulepath[MAX_PATH];
    wchar_t binpath[MAX_PATH];
    SERVICE_DESCRIPTION sdesc = {desc};

    if (!EnsureElevated())
        return true;

    if (!GetModuleFileName(NULL, modulepath, MAX_PATH - 1))
    {
        fprintf(stderr, "Unable to get the executable name (%ld)\n", GetLastError());
        return false;
    }

    svc_uninstall(name, false);

    if (!(sm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE | DELETE)))
    {
        fprintf(stderr, "Unable to Open SCManager (%ld)\n", GetLastError());
        return false;
    }

    if (wcschr(modulepath, L' '))
        _snwprintf(binpath, MAX_PATH - 1, L"\"%ls\" --daemon --service-mode", modulepath);
    else
        _snwprintf(binpath, MAX_PATH - 1, L"%ls --daemon --service-mode", modulepath);
    binpath[MAX_PATH - 1] = L'\0';

    svc = CreateService(sm, name, dname, SERVICE_CHANGE_CONFIG,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        binpath,
                        NULL, /* Load group order */
                        NULL, /* Tag Id */
                        NULL, /* Dependencies */
                        NULL, /* User -> Local System */
                        L"");

    if (!svc)
    {
        fwprintf(stderr, L"Unable to Create Service %ls (%ld)\n", name, GetLastError());
        CloseServiceHandle(sm);
        return false;
    }

    imp_ChangeServiceConfig2W pChangeServiceConfig2W = NULL;
    HMODULE advapi32 = GetModuleHandle(L"advapi32");
    if (advapi32)
    {
        IMPORT_FUNCTION(advapi32, ChangeServiceConfig2W);
        if (pChangeServiceConfig2W)
        {
            if (!pChangeServiceConfig2W(svc, SERVICE_CONFIG_DESCRIPTION, &sdesc))
                fwprintf(stderr, L"Unable to set description for Service %ls (%ld)\n", name, GetLastError());
        }
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(sm);

    wprintf(L"Service %ls successfully created.\n", name);
    wprintf(L"Use 'net start %ls' and 'net stop %ls' to start/stop the service.\n", name, name);
    return true;
}

static void svc_getcpvalue(const wchar_t *name)
{
    HKEY hKey;
    DWORD dwType;
    DWORD value = checkpoint_every, vlen = sizeof(DWORD);
    wchar_t subkey[MAX_PATH];

    _snwprintf(subkey, MAX_PATH - 1, L"SYSTEM\\CurrentControlSet\\Services\\%ls", name);
    subkey[MAX_PATH - 1] = L'\0';

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    if ((RegQueryValueEx(hKey, L"Checkpoint", NULL, &dwType, (LPBYTE)&value, &vlen) == ERROR_SUCCESS) &&
        (vlen == sizeof(DWORD) && (dwType == REG_DWORD)))
        checkpoint_every = value;

    RegCloseKey(hKey);
}

void svc_register(wchar_t *name)
{
    DWORD tid;
    DT->lpServiceName = name;
    svc_getcpvalue(name);

    evStart = CreateEvent(NULL, TRUE, FALSE, NULL);
    DispatcherThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StartServiceCtrlDispatcher, (LPVOID)DT, 0, &tid);
}

void svc_ready(void)
{
    WaitForSingleObject(evStart, INFINITE);

    svc.dwCurrentState = SERVICE_RUNNING;
    svc.dwControlsAccepted |= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    svc.dwCheckPoint = 0;

    if (!SetServiceStatus(svc_handle, &svc))
    {
        logg(LOGG_ERROR, "[service] SetServiceStatus() failed with %ld\n", GetLastError());
        exit(1);
    }
}

int svc_checkpoint(const char *type, const char *name, unsigned int custom, void *context)
{
    if (svc.dwCurrentState == SERVICE_START_PENDING)
    {
        svc.dwCheckPoint++;
        if ((svc.dwCheckPoint % checkpoint_every) == 0)
            SetServiceStatus(svc_handle, &svc);
    }
    return 0;
}

void WINAPI ServiceCtrlHandler(DWORD code)
{
    switch (code)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        svc.dwCurrentState = SERVICE_STOPPED;
        svc.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        SetServiceStatus(svc_handle, &svc);
        return;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    }

    SetServiceStatus(svc_handle, &svc);
}

BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType)
{
    if (CtrlType == CTRL_C_EVENT)
    {
        SetConsoleCtrlHandler(cw_stop_ctrl_handler, FALSE);
        fprintf(stderr, "Control+C pressed, aborting...\n");
        exit(0);
    }
    return TRUE;
}

void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    svc.dwServiceType = SERVICE_WIN32;
    svc.dwCurrentState = SERVICE_START_PENDING;
    svc.dwControlsAccepted = 0;
    svc.dwWin32ExitCode = NO_ERROR;
    svc.dwServiceSpecificExitCode = 0;
    svc.dwCheckPoint = 0;
    svc.dwWaitHint = 0;

    if (!(svc_handle = RegisterServiceCtrlHandler(DT->lpServiceName, ServiceCtrlHandler)))
    {
        logg(LOGG_ERROR, "[service] RegisterServiceCtrlHandler() failed with %d\n", GetLastError());
        exit(1);
    }

    if (!SetServiceStatus(svc_handle, &svc))
    {
        logg(LOGG_ERROR, "[service] SetServiceStatus() failed with %d\n", GetLastError());
        exit(1);
    }

    SetEvent(evStart);
    WaitForSingleObject(DispatcherThread, INFINITE);
    cw_stop_ctrl_handler(CTRL_C_EVENT);
}
#endif // _UNICODE

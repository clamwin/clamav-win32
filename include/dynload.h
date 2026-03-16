/*
 * Clamav Native Windows Port: Dynamic loading helpers
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#ifndef _DYNLOAD_H_
#define _DYNLOAD_H_
#include <windows.h>
#include <tlhelp32.h>
#include <windns.h>

#define Q(string) #string
#define IMPORT_FUNCTION(hLib, func)                                  \
    do                                                               \
    {                                                                \
        imp_##func symbol = (imp_##func)GetProcAddress(hLib, #func); \
        if (symbol)                                                  \
            p##func = symbol;                                        \
    } while (0)

typedef BOOL(WINAPI *imp_IsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
typedef BOOL(WINAPI *imp_Wow64DisableWow64FsRedirection)(PVOID OldValue);
typedef VOID(WINAPI *imp_GetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);

typedef BOOL(WINAPI *imp_CreateHardLinkW)(LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef DWORD(WINAPI *imp_GetLongPathNameW)(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer);
typedef BOOL(WINAPI *imp_TzSpecificLocalTimeToSystemTime)(const TIME_ZONE_INFORMATION *lpTimeZoneInformation, const SYSTEMTIME *lpLocalTime, LPSYSTEMTIME lpUniversalTime);

typedef HANDLE(WINAPI *imp_CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI *imp_Process32FirstW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
typedef BOOL(WINAPI *imp_Process32NextW)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL(WINAPI *imp_Module32FirstW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);
typedef BOOL(WINAPI *imp_Module32NextW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);

typedef BOOL(WINAPI *imp_ChangeServiceConfig2W)(SC_HANDLE hService, DWORD dwInfoLevel, LPVOID lpInfo);
typedef BOOL(WINAPI *imp_AttachConsole)(DWORD dwProcessId);
typedef DWORD(WINAPI *imp_GetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);

typedef PVOID(WINAPI *imp_AddVectoredExceptionHandler)(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler);
typedef BOOL(WINAPI *imp_RegisterWaitForSingleObject)(PHANDLE phNewWaitObject, HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags);
typedef BOOL(WINAPI *imp_UnregisterWait)(HANDLE WaitHandle);
typedef BOOL(WINAPI *imp_UnregisterWaitEx)(HANDLE WaitHandle, HANDLE CompletionEvent);
typedef BOOL(WINAPI *imp_HeapSetInformation)(HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength);
typedef int(WINAPI *imp_MultiByteToWideChar)(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
typedef HANDLE(WINAPI *imp_CreateThread)(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);

typedef DNS_STATUS(WINAPI *imp_DnsQuery_A)(PCSTR pszName, WORD wType, DWORD Options, PIP4_ARRAY aipServers, PDNS_RECORD *ppQueryResults, PVOID *pReserved);
typedef VOID(WINAPI *imp_DnsRecordListFree)(PDNS_RECORD pRecordList, DNS_FREE_TYPE FreeType);

typedef BOOLEAN(WINAPI *imp_SystemFunction036)(PVOID RandomBuffer, ULONG RandomBufferLength);

typedef BOOL(WINAPI *imp_GetUserProfileDirectoryW)(HANDLE hToken, LPWSTR lpProfileDir, LPDWORD lpcchSize);

typedef int(WINAPI *imp_GetHostNameW)(PWSTR name, int namelen);
#endif /* _DYNLOAD_H_ */

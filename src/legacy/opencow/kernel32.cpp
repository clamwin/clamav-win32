/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is for Open Layer for Unicode (opencow).
 *
 * The Initial Developer of the Original Code is Brodie Thiesfield.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// define these symbols so that we don't get dllimport linkage
// from the system headers
#define _KERNEL32_

#include <windows.h>
#include <mbstring.h>
#include <processenv.h>

#include "MbcsBuffer.h"

// ----------------------------------------------------------------------------
// Definitions

#ifndef INVALID_FILE_ATTRIBUTES
# define INVALID_FILE_ATTRIBUTES    ((DWORD)-1)
#endif
#ifndef IS_INTRESOURCE
# define IS_INTRESOURCE(p)          (((unsigned long)(p) >> 16) == 0UL)
#endif

// ----------------------------------------------------------------------------
// Globals

extern int       g_nDebug;

// ----------------------------------------------------------------------------
// API

ATOM WINAPI
AddAtomW(
    IN LPCWSTR lpString
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpString))
        return 0;

    return ::AddAtomA(mbcsString);
}

// BeginUpdateResourceA
// BeginUpdateResourceW
// BuildCommDCBAndTimeoutsW
// BuildCommDCBW
// CallNamedPipeW
// CommConfigDialogW
// CompareStringW
// CopyFileExW


int WINAPI
CompareStringW(
    LCID Locale,
    DWORD dwCmpFlags,
    PCNZWCH lpString1,
    int cchCount1,
    PCNZWCH lpString2,
    int cchCount2)
{
    CMbcsBuffer mbcsString1, mbcsString2;

    if (!mbcsString1.FromUnicode(lpString1))
        return 0;

    if (!mbcsString2.FromUnicode(lpString2))
        return 0;

    return ::CompareStringA(Locale, dwCmpFlags, mbcsString1, cchCount1, mbcsString2, cchCount2);
}


BOOL WINAPI
CopyFileW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN BOOL bFailIfExists
    )
{
    CMbcsBuffer mbcsExistingFileName;
    if (!mbcsExistingFileName.FromUnicode(lpExistingFileName))
        return FALSE;

    CMbcsBuffer mbcsNewFileName;
    if (!mbcsNewFileName.FromUnicode(lpNewFileName))
        return FALSE;

    return ::CopyFileA(mbcsExistingFileName, mbcsNewFileName, bFailIfExists);
}

BOOL WINAPI
CreateDirectoryExW(
    IN LPCWSTR lpTemplateDirectory,
    IN LPCWSTR lpNewDirectory,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    CMbcsBuffer mbcsTemplateDirectory;
    if (!mbcsTemplateDirectory.FromUnicode(lpTemplateDirectory))
        return FALSE;

    CMbcsBuffer mbcsNewDirectory;
    if (!mbcsNewDirectory.FromUnicode(lpNewDirectory))
        return FALSE;

    return ::CreateDirectoryExA(mbcsTemplateDirectory, mbcsNewDirectory, lpSecurityAttributes);
}

BOOL WINAPI
CreateDirectoryW(
    IN LPCWSTR lpPathName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    CMbcsBuffer mbcsPathName;
    if (!mbcsPathName.FromUnicode(lpPathName))
        return FALSE;

    return ::CreateDirectoryA(mbcsPathName, lpSecurityAttributes);
}

HANDLE WINAPI
CreateEventW(
    IN LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::CreateEventA(lpEventAttributes, bManualReset, bInitialState, mbcsName);
}

HANDLE WINAPI
CreateFileMappingW(
    IN HANDLE hFile,
    IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect,
        dwMaximumSizeHigh, dwMaximumSizeLow, mbcsName);
}

HANDLE WINAPI
CreateFileW(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return INVALID_HANDLE_VALUE;

    dwShareMode &= ~FILE_SHARE_DELETE;
    dwFlagsAndAttributes &= ~FILE_FLAG_BACKUP_SEMANTICS;

    return ::CreateFileA(mbcsFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);
}

HANDLE WINAPI
CreateMailslotW(
    IN LPCWSTR lpName,
    IN DWORD nMaxMessageSize,
    IN DWORD lReadTimeout,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return INVALID_HANDLE_VALUE;

    return ::CreateMailslotA(mbcsName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes);
}

HANDLE WINAPI
CreateMutexW(
    IN LPSECURITY_ATTRIBUTES lpMutexAttributes,
    IN BOOL bInitialOwner,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::CreateMutexA(lpMutexAttributes, bInitialOwner, mbcsName);
}

// CreateNamedPipeW

BOOL WINAPI
CreateProcessW(
    IN LPCWSTR lpApplicationName,
    IN LPWSTR lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCWSTR lpCurrentDirectory,
    IN LPSTARTUPINFOW lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation
    )
{
    CMbcsBuffer mbcsApplicationName;
    if (!mbcsApplicationName.FromUnicode(lpApplicationName))
        return FALSE;

    CMbcsBuffer mbcsCommandLine;
    if (!mbcsCommandLine.FromUnicode(lpCommandLine))
        return FALSE;

    CMbcsBuffer mbcsCurrentDirectory;
    if (!mbcsCurrentDirectory.FromUnicode(lpCurrentDirectory))
        return FALSE;

    STARTUPINFOA startupInfoA;
    ::ZeroMemory(&startupInfoA, sizeof(startupInfoA));
    startupInfoA.cb              = sizeof(startupInfoA);
    startupInfoA.dwX             = lpStartupInfo->dwX;
    startupInfoA.dwY             = lpStartupInfo->dwY;
    startupInfoA.dwXSize         = lpStartupInfo->dwXSize;
    startupInfoA.dwYSize         = lpStartupInfo->dwYSize;
    startupInfoA.dwXCountChars   = lpStartupInfo->dwXCountChars;
    startupInfoA.dwYCountChars   = lpStartupInfo->dwYCountChars;
    startupInfoA.dwFillAttribute = lpStartupInfo->dwFillAttribute;
    startupInfoA.dwFlags         = lpStartupInfo->dwFlags;
    startupInfoA.wShowWindow     = lpStartupInfo->wShowWindow;
    startupInfoA.hStdInput       = lpStartupInfo->hStdInput;
    startupInfoA.hStdOutput      = lpStartupInfo->hStdOutput;
    startupInfoA.hStdError       = lpStartupInfo->hStdError;

    CMbcsBuffer mbcsDesktop;
    if (!mbcsDesktop.FromUnicode(lpStartupInfo->lpDesktop))
        return FALSE;
    startupInfoA.lpDesktop = mbcsDesktop;

    CMbcsBuffer mbcsTitle;
    if (!mbcsTitle.FromUnicode(lpStartupInfo->lpTitle))
        return FALSE;
    startupInfoA.lpTitle = mbcsTitle;

    return ::CreateProcessA(mbcsApplicationName, mbcsCommandLine,
        lpProcessAttributes, lpThreadAttributes, bInheritHandles,
        dwCreationFlags, lpEnvironment, mbcsCurrentDirectory,
        &startupInfoA, lpProcessInformation);
}

HANDLE WINAPI
CreateSemaphoreW(
    IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, mbcsName);
}

typedef HANDLE (WINAPI *fpCreateWaitableTimerA)(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCSTR lpTimerName
    );

HANDLE WINAPI
CreateWaitableTimerW(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCWSTR lpTimerName
    )
{
    static fpCreateWaitableTimerA pCreateWaitableTimerA =
        (fpCreateWaitableTimerA) ::GetProcAddress(
            ::GetModuleHandleA("kernel32.dll"), "CreateWaitableTimerA");
    if (!pCreateWaitableTimerA) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return NULL;
    }

    CMbcsBuffer mbcsTimerName;
    if (!mbcsTimerName.FromUnicode(lpTimerName))
        return NULL;

    return pCreateWaitableTimerA(lpTimerAttributes, bManualReset, mbcsTimerName);
}

BOOL WINAPI
DeleteFileW(
    IN LPCWSTR lpFileName
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return FALSE;

    return ::DeleteFileA(mbcsFileName);
}

// EndUpdateResourceA
// EndUpdateResourceW
// EnumCalendarInfoExW
// EnumCalendarInfoW
// EnumDateFormatsExW
// EnumDateFormatsW
// EnumSystemCodePagesW
// EnumSystemLocalesW
// EnumTimeFormatsW

DWORD WINAPI
ExpandEnvironmentStringsW(
    IN LPCWSTR lpSrc,
    OUT LPWSTR lpDst,
    IN DWORD nSize
    )
{
    CMbcsBuffer mbcsSrc;
    if (!mbcsSrc.FromUnicode(lpSrc))
        return FALSE;

    DWORD dwResult = 0;
    CMbcsBuffer mbcsDst;
    do {
        if (dwResult > (DWORD) mbcsDst.BufferSize()) {
            if (!mbcsDst.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::ExpandEnvironmentStringsA(mbcsSrc, mbcsDst, (DWORD) mbcsDst.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsDst.BufferSize());
    int nDstLen = ::lstrlenA(mbcsDst);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsDst, nDstLen + 1, 0, 0);
    if (nSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsDst, nDstLen + 1, lpDst, nSize);
    return nRequiredSize; // include the NULL
}

VOID WINAPI
FatalAppExitW(
    IN UINT uAction,
    IN LPCWSTR lpMessageText
    )
{
    CMbcsBuffer mbcsMessageText;
    mbcsMessageText.FromUnicode(lpMessageText);
    ::FatalAppExitA(uAction, mbcsMessageText);
}

// FillConsoleOutputCharacterW

ATOM WINAPI
FindAtomW(
    IN LPCWSTR lpString
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpString))
        return 0;

    return ::FindAtomA(mbcsString);
}

HANDLE WINAPI
FindFirstChangeNotificationW(
    IN LPCWSTR lpPathName,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter
    )
{
    CMbcsBuffer mbcsPathName;
    if (!mbcsPathName.FromUnicode(lpPathName))
        return INVALID_HANDLE_VALUE;

    return ::FindFirstChangeNotificationA(mbcsPathName, bWatchSubtree, dwNotifyFilter);
}

static void
CopyFindDataAtoW(
    LPWIN32_FIND_DATAA lpFindDataA,
    LPWIN32_FIND_DATAW lpFindDataW
    )
{
    lpFindDataW->dwFileAttributes = lpFindDataA->dwFileAttributes;
    lpFindDataW->ftCreationTime   = lpFindDataA->ftCreationTime;
    lpFindDataW->ftLastAccessTime = lpFindDataA->ftLastAccessTime;
    lpFindDataW->ftLastWriteTime  = lpFindDataA->ftLastWriteTime;
    lpFindDataW->nFileSizeHigh    = lpFindDataA->nFileSizeHigh;
    lpFindDataW->nFileSizeLow     = lpFindDataA->nFileSizeLow;
    lpFindDataW->dwReserved0      = lpFindDataA->dwReserved0;
    lpFindDataW->dwReserved1      = lpFindDataA->dwReserved1;

    ::MultiByteToWideChar(CP_ACP, 0, lpFindDataA->cFileName, -1,
        lpFindDataW->cFileName, MAX_PATH);

    ::MultiByteToWideChar(CP_ACP, 0, lpFindDataA->cAlternateFileName, -1,
        lpFindDataW->cAlternateFileName,
        ARRAY_SIZE(lpFindDataW->cAlternateFileName));
}

HANDLE WINAPI
FindFirstFileW(
    IN LPCWSTR lpFileName,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return INVALID_HANDLE_VALUE;

    WIN32_FIND_DATAA findDataA;
    HANDLE hFile = ::FindFirstFileA(mbcsFileName, &findDataA);
    if (hFile == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;

    CopyFindDataAtoW(&findDataA, lpFindFileData);
    return hFile;
}

BOOL WINAPI
FindNextFileW(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATAW lpFindFileData
    )
{
    WIN32_FIND_DATAA findDataA;
    BOOL bSuccess = ::FindNextFileA(hFindFile, &findDataA);
    if (bSuccess)
        CopyFindDataAtoW(&findDataA, lpFindFileData);
    return bSuccess;
}

HRSRC WINAPI
FindResourceExW(
    IN HMODULE hModule,
    IN LPCWSTR lpType,
    IN LPCWSTR lpName,
    IN WORD    wLanguage
    )
{
    LPCSTR lpTypeA = (LPCSTR) lpType;
    CMbcsBuffer mbcsType;
    if (!IS_INTRESOURCE(lpType)) {
        if (!mbcsType.FromUnicode(lpType))
            return NULL;
        lpTypeA = mbcsType;
    }

    LPCSTR lpNameA = (LPCSTR) lpName;
    CMbcsBuffer mbcsName;
    if (!IS_INTRESOURCE(lpName)) {
        if (!mbcsName.FromUnicode(lpName))
            return NULL;
        lpNameA = mbcsName;
    }

    return ::FindResourceExA(hModule, lpTypeA, lpNameA, wLanguage);
}

HRSRC WINAPI
FindResourceW(
    IN HMODULE hModule,
    IN LPCWSTR lpName,
    IN LPCWSTR lpType
    )
{
    LPCSTR lpTypeA = (LPCSTR) lpType;
    CMbcsBuffer mbcsType;
    if (!IS_INTRESOURCE(lpType)) {
        if (!mbcsType.FromUnicode(lpType))
            return NULL;
        lpTypeA = mbcsType;
    }

    LPCSTR lpNameA = (LPCSTR) lpName;
    CMbcsBuffer mbcsName;
    if (!IS_INTRESOURCE(lpName)) {
        if (!mbcsName.FromUnicode(lpName))
            return NULL;
        lpNameA = mbcsName;
    }

    return ::FindResourceA(hModule, lpTypeA, lpNameA);
}

DWORD WINAPI FormatMessageW(
    IN DWORD   dwFlags,
    IN LPCVOID lpSource,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN LPWSTR  lpBuffer,
    IN DWORD   nSize,
    IN va_list *Arguments
    )
{
    if (!lpBuffer || nSize == 0)
        return 0;

    CMbcsBuffer mbcsBuffer;
    DWORD dwSize = ::FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId, mbcsBuffer, mbcsBuffer.BufferSize(), Arguments);

    if (dwSize == 0)
        return 0;

    int uSize = MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, -1, lpBuffer, nSize);

    if (uSize == 0)
        return 0;

    return uSize - 1;
}

BOOL WINAPI
FreeEnvironmentStringsW(
    IN LPWSTR lpStrings
    )
{
    if (lpStrings)
        ::free(lpStrings);
    return TRUE;
}

UINT WINAPI
GetAtomNameW(
    IN ATOM nAtom,
    OUT LPWSTR lpBuffer,
    IN int nSize
    )
{
    CMbcsBuffer mbcsBuffer;
    UINT uiLen = ::GetAtomNameA(nAtom, mbcsBuffer, mbcsBuffer.BufferSize());
    if (!uiLen)
        return 0;

    int nLen = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, uiLen + 1, lpBuffer, nSize);
    if (!nLen) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    return (UINT) (nLen - 1);
}

//TODO: MSLU adds support for CP_UTF7 and CP_UTF8
BOOL WINAPI
GetCPInfoExW(
    IN UINT          CodePage,
    IN DWORD         dwFlags,
    OUT LPCPINFOEXW  lpCPInfoEx
    )
{
    CPINFOEXA cpInfoA;
    BOOL bSuccess = ::GetCPInfoExA(CodePage, dwFlags, &cpInfoA);
    if (!bSuccess)
        return FALSE;

    lpCPInfoEx->CodePage = cpInfoA.CodePage;
    ::CopyMemory(lpCPInfoEx->DefaultChar, cpInfoA.DefaultChar, MAX_DEFAULTCHAR);
    ::CopyMemory(lpCPInfoEx->LeadByte, cpInfoA.LeadByte, MAX_LEADBYTES);
    lpCPInfoEx->MaxCharSize = cpInfoA.MaxCharSize;
    lpCPInfoEx->UnicodeDefaultChar = cpInfoA.UnicodeDefaultChar;
    ::MultiByteToWideChar(CP_ACP, 0, cpInfoA.CodePageName, -1, lpCPInfoEx->CodePageName, MAX_PATH);

    return TRUE;
}

// GetCalendarInfoW

BOOL WINAPI
GetComputerNameW(
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD lpnSize
    )
{
    CMbcsBuffer mbcsBuffer;
    DWORD dwLen = mbcsBuffer.BufferSize();
    if (!::GetComputerNameA(mbcsBuffer, &dwLen))
        return FALSE;

    int nRequiredLen = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, (int) dwLen + 1, 0, 0);
    if (*lpnSize < (DWORD) nRequiredLen) {
        *lpnSize = (DWORD) (nRequiredLen - 1);
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, (int) dwLen + 1, lpBuffer, *lpnSize);
    *lpnSize = (DWORD) (nRequiredLen - 1);
    return TRUE;
}

// GetConsoleTitleW
// GetCurrencyFormatW

DWORD WINAPI
GetCurrentDirectoryW(
    IN DWORD uSize,
    OUT LPWSTR lpBuffer
    )
{
    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetCurrentDirectoryA((DWORD) mbcsBuffer.BufferSize(), mbcsBuffer);
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);
    return nRequiredSize - 1; // do not include the NULL
}

// GetDateFormatW
// GetDefaultCommConfigW

typedef BOOL (WINAPI *fpGetDiskFreeSpaceExA)(
    IN LPCSTR lpDirectoryName,
    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

BOOL WINAPI
GetDiskFreeSpaceExW(
    IN LPCWSTR lpDirectoryName,
    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    // dynamically loaded
    static fpGetDiskFreeSpaceExA pGetDiskFreeSpaceExA = 0;
    if (!pGetDiskFreeSpaceExA) {
        pGetDiskFreeSpaceExA = (fpGetDiskFreeSpaceExA)
            ::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
        if (!pGetDiskFreeSpaceExA) {
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return FALSE;
        }
    }

    CMbcsBuffer mbcsDirectoryName;
    if (!mbcsDirectoryName.FromUnicode(lpDirectoryName))
        return FALSE;

    return pGetDiskFreeSpaceExA(mbcsDirectoryName, lpFreeBytesAvailableToCaller,
        lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
}

BOOL WINAPI
GetDiskFreeSpaceW(
    IN LPCWSTR lpRootPathName,
    OUT LPDWORD lpSectorsPerCluster,
    OUT LPDWORD lpBytesPerSector,
    OUT LPDWORD lpNumberOfFreeClusters,
    OUT LPDWORD lpTotalNumberOfClusters
    )
{
    CMbcsBuffer mbcsRootPathName;
    if (!mbcsRootPathName.FromUnicode(lpRootPathName))
        return FALSE;

    return ::GetDiskFreeSpaceA(mbcsRootPathName, lpSectorsPerCluster,
        lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
}

UINT WINAPI
GetDriveTypeW(
    IN LPCWSTR lpRootPathName
    )
{
    CMbcsBuffer mbcsRootPathName;
    if (!mbcsRootPathName.FromUnicode(lpRootPathName))
        return DRIVE_UNKNOWN;

    return ::GetDriveTypeA(mbcsRootPathName);
}

#undef GetEnvironmentStrings
#undef GetEnvironmentStringsW
LPWSTR WINAPI
GetEnvironmentStringsW(
    VOID
    )
{
    LPSTR pStringsA = ::GetEnvironmentStrings();
    if (!pStringsA)
        return 0;

    int nTotalLen = 0;
    int nLen = ::lstrlenA(pStringsA);
    while (nLen > 0) {
        nTotalLen += nLen + 1;
        nLen = ::lstrlenA(pStringsA + nTotalLen);
    }
    ++nTotalLen;

    int nRequiredLen = ::MultiByteToWideChar(CP_ACP, 0, pStringsA, nTotalLen, 0, 0);
    if (nRequiredLen < 1) {
        ::FreeEnvironmentStringsA(pStringsA);
        return 0;
    }

    LPWSTR pStringsW = (LPWSTR) ::malloc(nRequiredLen * sizeof(wchar_t));
    if (!pStringsW) {
        ::FreeEnvironmentStringsA(pStringsA);
        return 0;
    }

    ::MultiByteToWideChar(CP_ACP, 0, pStringsA, nTotalLen, pStringsW, nRequiredLen);
    ::FreeEnvironmentStringsA(pStringsA);

    return pStringsW;
}

DWORD WINAPI
GetEnvironmentVariableW(
    IN LPCWSTR lpName,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return 0;

    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetEnvironmentVariableA(mbcsName, mbcsBuffer, (DWORD) mbcsBuffer.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (nSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, nSize);
    return nRequiredSize - 1; // don't include NULL
}

// GetFileAttributesExW

DWORD WINAPI
GetFileAttributesW(
    IN LPCWSTR lpFileName
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return INVALID_FILE_ATTRIBUTES;

    return ::GetFileAttributesA(mbcsFileName);
}

DWORD WINAPI
GetFullPathNameW(
    IN LPCWSTR lpFileName,
    IN DWORD uSize,
    OUT LPWSTR lpBuffer,
    OUT LPWSTR *lpFilePart
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return 0;

    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        char * pszFilePart;
        dwResult = ::GetFullPathNameA(mbcsFileName, (DWORD) mbcsBuffer.BufferSize(), mbcsBuffer, &pszFilePart);
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);

    // set the file part pointer to the beginning of the file, we know that
    // there must be a slash in the path somewhere, so this loop won't run
    // off the beginning of the string
    if (lpFilePart) {
        wchar_t * pFilePart = lpBuffer + nRequiredSize - 1;
        while (*pFilePart != L'\\')
            --pFilePart;
        *lpFilePart = pFilePart;
    }

    return nRequiredSize - 1; // do not include the NULL
}

// GetLocaleInfoW

DWORD WINAPI
GetLogicalDriveStringsW(
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer
    )
{
    CMbcsBuffer mbcsDriveStrings;
    if (!mbcsDriveStrings.SetCapacity(nBufferLength))
        return 0;

    int len = ::GetLogicalDriveStringsA(mbcsDriveStrings.BufferSize(), mbcsDriveStrings);
    if (len > 0 && len < mbcsDriveStrings.BufferSize()) {
        // include terminating null character by using len + 1
        int nResult = ::MultiByteToWideChar(CP_ACP, 0, mbcsDriveStrings,
            len + 1, lpBuffer, nBufferLength);
        if (nResult == 0) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                len = ::MultiByteToWideChar(CP_ACP, 0, mbcsDriveStrings, len + 1, 0, 0);
            else
                len = 0;
        }
        else {
            len = nResult - 1; // don't include the NULL
        }
    }

    return len;
}

DWORD WINAPI
GetLongPathNameW(
    IN LPCWSTR  lpszShortPath,
    OUT LPWSTR  lpszLongPath,
    IN DWORD    cchBuffer
    )
{
    CMbcsBuffer mbcsShortPath;
    if (!mbcsShortPath.FromUnicode(lpszShortPath))
        return 0;

    DWORD dwResult = 0;
    CMbcsBuffer mbcsLongPath;
    do {
        if (dwResult > (DWORD) mbcsLongPath.BufferSize()) {
            if (!mbcsLongPath.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetLongPathNameA(mbcsShortPath, mbcsLongPath, (DWORD) mbcsLongPath.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsLongPath.BufferSize());
    int nLongPathLen = ::lstrlenA(mbcsLongPath);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsLongPath, nLongPathLen + 1, 0, 0);
    if (cchBuffer < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsLongPath, nLongPathLen + 1, lpszLongPath, cchBuffer);
    return nRequiredSize - 1; // do not include the NULL
}

DWORD WINAPI
GetModuleFileNameW(
    IN HMODULE hModule,
    OUT LPWSTR lpFilename,
    IN DWORD nSize
    )
{
    CMbcsBuffer mbcsFilename;
    if (!mbcsFilename.SetCapacity(nSize))
        return 0;

    //TODO: does the return value include the NULL char or not??
    if (!::GetModuleFileNameA(hModule, mbcsFilename, mbcsFilename.BufferSize()))
        return 0;

    return (DWORD) ::MultiByteToWideChar(CP_ACP, 0, mbcsFilename, -1, lpFilename, nSize);
}

extern "C" HMODULE WINAPI
GetModuleHandleW(
    IN LPCWSTR lpModuleName
    )
{
    CMbcsBuffer mbcsModuleName;
    if (!mbcsModuleName.FromUnicode(lpModuleName))
        return NULL;

    return ::GetModuleHandleA(mbcsModuleName);
}

// GetNamedPipeHandleStateW
// GetNumberFormatW
// GetPrivateProfileIntW
// GetPrivateProfileSectionNamesW
// GetPrivateProfileSectionW
// GetPrivateProfileStringW
// GetPrivateProfileStructW
// GetProfileIntW
// GetProfileSectionW
// GetProfileStringW

DWORD WINAPI
GetShortPathNameW(
    IN LPCWSTR  lpszLongPath,
    OUT LPWSTR  lpszShortPath,
    IN DWORD    cchBuffer
    )
{
    CMbcsBuffer mbcsLongPath;
    if (!mbcsLongPath.FromUnicode(lpszLongPath))
        return 0;

    DWORD dwResult = 0;
    CMbcsBuffer mbcsShortPath;
    do {
        if (dwResult > (DWORD) mbcsShortPath.BufferSize()) {
            if (!mbcsShortPath.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetShortPathNameA(mbcsLongPath, mbcsShortPath, (DWORD) mbcsShortPath.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsShortPath.BufferSize());
    int nShortPathLen = ::lstrlenA(mbcsShortPath);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsShortPath, nShortPathLen + 1, 0, 0);
    if (cchBuffer < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsShortPath, nShortPathLen + 1, lpszShortPath, cchBuffer);
    return nRequiredSize - 1; // do not include the NULL
}

VOID WINAPI
GetStartupInfoW(
    OUT LPSTARTUPINFOW lpStartupInfo
    )
{
    // we use static buffers here because they must be valid for the caller
    // to use after we have returned. Also, we know that they will not change
    // from call to call because this is information which was generated at
    // application startup time and is never changed from then.
    static bool bIsInitialized = false;
    static wchar_t wszDesktop[MAX_PATH] = { 0 };
    static wchar_t wszTitle[MAX_PATH] = { 0 };
    static STARTUPINFOA startupInfoA = { 0 };
    if (!bIsInitialized) {
        ::GetStartupInfoA(&startupInfoA);
        ::MultiByteToWideChar(CP_ACP, 0, startupInfoA.lpDesktop, -1, wszDesktop, MAX_PATH);
        ::MultiByteToWideChar(CP_ACP, 0, startupInfoA.lpTitle, -1, wszTitle, MAX_PATH);
        bIsInitialized = true;
    }

    ::ZeroMemory(lpStartupInfo, sizeof(STARTUPINFOW));
    lpStartupInfo->cb               = sizeof(STARTUPINFOW);
    lpStartupInfo->dwX              = startupInfoA.dwX;
    lpStartupInfo->dwY              = startupInfoA.dwY;
    lpStartupInfo->dwXSize          = startupInfoA.dwXSize;
    lpStartupInfo->dwYSize          = startupInfoA.dwYSize;
    lpStartupInfo->dwXCountChars    = startupInfoA.dwXCountChars;
    lpStartupInfo->dwYCountChars    = startupInfoA.dwYCountChars;
    lpStartupInfo->dwFillAttribute  = startupInfoA.dwFillAttribute;
    lpStartupInfo->dwFlags          = startupInfoA.dwFlags;
    lpStartupInfo->wShowWindow      = startupInfoA.wShowWindow;
    lpStartupInfo->hStdInput        = startupInfoA.hStdInput;
    lpStartupInfo->hStdOutput       = startupInfoA.hStdOutput;
    lpStartupInfo->hStdError        = startupInfoA.hStdError;

    if (startupInfoA.lpDesktop)
        lpStartupInfo->lpDesktop = wszDesktop;
    if (startupInfoA.lpTitle)
        lpStartupInfo->lpTitle = wszTitle;
}

// GetStringTypeExW

BOOL WINAPI
GetStringTypeW(
    IN DWORD    dwInfoType,
    IN LPCWSTR  lpSrcStr,
    IN int      cchSrc,
    OUT LPWORD  lpCharType
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpSrcStr, cchSrc))
        return FALSE;

    return ::GetStringTypeA(LOCALE_SYSTEM_DEFAULT, dwInfoType,
        mbcsString, mbcsString.Length(), lpCharType);
}

UINT WINAPI
GetSystemDirectoryW(
    OUT LPWSTR lpBuffer,
    IN UINT uSize
    )
{
    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetSystemDirectoryA(mbcsBuffer, (DWORD) mbcsBuffer.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);
    return nRequiredSize - 1; // do not include the NULL
}

UINT WINAPI
GetSystemWindowsDirectoryW(
    OUT LPWSTR lpBuffer,
    IN UINT uSize
    )
{
    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        // The GetSystemWindowsDirectoryA function doesn't exist on Windows 95/98/ME.
        // As the result is the shared windows directory, this is the same as the
        // standard GetWindowsDirectoryA call on these platforms.
        dwResult = ::GetWindowsDirectoryA(mbcsBuffer, (DWORD) mbcsBuffer.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);
    return nRequiredSize - 1; // do not include the NULL
}

UINT WINAPI
GetTempFileNameW(
    IN LPCWSTR lpPathName,
    IN LPCWSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPWSTR lpTempFileName
    )
{
    CMbcsBuffer mbcsPathName;
    if (!mbcsPathName.FromUnicode(lpPathName))
        return 0;

    CMbcsBuffer mbcsPrefixString;
    if (!mbcsPrefixString.FromUnicode(lpPrefixString))
        return 0;

    char szTempFileName[MAX_PATH];
    UINT uiResult = ::GetTempFileNameA(mbcsPathName, mbcsPrefixString, uUnique, szTempFileName);
    if (uiResult == 0)
        return 0;

    ::MultiByteToWideChar(CP_ACP, 0, szTempFileName, -1, lpTempFileName, MAX_PATH);
    return uiResult;
}

DWORD WINAPI
GetTempPathW(
    IN DWORD uSize,
    OUT LPWSTR lpBuffer
    )
{
    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetTempPathA((DWORD) mbcsBuffer.BufferSize(), mbcsBuffer);
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);
    return nRequiredSize - 1; // do not include the NULL
}

// GetTimeFormatW
// GetVersionExW
// GetVolumeInformationW

UINT WINAPI
GetWindowsDirectoryW(
    OUT LPWSTR lpBuffer,
    IN UINT uSize
    )
{
    DWORD dwResult = 0;
    CMbcsBuffer mbcsBuffer;
    do {
        if (dwResult > (DWORD) mbcsBuffer.BufferSize()) {
            if (!mbcsBuffer.SetCapacity((int) dwResult))
                return 0;
        }

        dwResult = ::GetWindowsDirectoryA(mbcsBuffer, (DWORD) mbcsBuffer.BufferSize());
        if (!dwResult)
            return 0;
    }
    while (dwResult > (DWORD) mbcsBuffer.BufferSize());
    int nBufferLen = ::lstrlenA(mbcsBuffer);

    // ensure the supplied buffer is big enough
    int nRequiredSize = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, 0, 0);
    if (uSize < (DWORD) nRequiredSize)
        return (DWORD) nRequiredSize;

    ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, nBufferLen + 1, lpBuffer, uSize);
    return nRequiredSize - 1; // do not include the NULL
}

ATOM WINAPI
GlobalAddAtomW(
    IN LPCWSTR lpString
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpString))
        return 0;

    return ::GlobalAddAtomA(mbcsString);
}

ATOM WINAPI
GlobalFindAtomW(
    IN LPCWSTR lpString
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpString))
        return 0;

    return ::GlobalFindAtomA(mbcsString);
}

UINT
WINAPI
GlobalGetAtomNameW(
    IN ATOM nAtom,
    OUT LPWSTR lpBuffer,
    IN int nSize
    )
{
    CMbcsBuffer mbcsBuffer;
    UINT uiLen = ::GlobalGetAtomNameA(nAtom, mbcsBuffer, mbcsBuffer.BufferSize());
    if (!uiLen)
        return 0;

    int nLen = ::MultiByteToWideChar(CP_ACP, 0, mbcsBuffer, uiLen + 1, lpBuffer, nSize);
    if (!nLen) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    return (UINT) (nLen - 1);
}

BOOL WINAPI
IsBadStringPtrW(
    IN LPCWSTR lpsz,
    IN UINT_PTR ucchMax
    )
{
    if (ucchMax == 0)
        return 0;

    //Note: slow because we call the IsBadReadPtr function
    // once for every character before we examine the character,
    // but I can't see another way to do this.
    do
    {
        if (::IsBadReadPtr(lpsz, sizeof(wchar_t)))
            return 1;
    }
    while (*lpsz++ && --ucchMax > 0);

    return 0;
}

//TODO: MSLU adds support for CP_UTF7 and CP_UTF8
int WINAPI
LCMapStringW(
    IN LCID     Locale,
    IN DWORD    dwMapFlags,
    IN LPCWSTR  lpSrcStr,
    IN int      cchSrc,
    OUT LPWSTR  lpDestStr,
    IN int      cchDest
    )
{
    CMbcsBuffer mbcsString;
    if (!mbcsString.FromUnicode(lpSrcStr, cchSrc))
        return 0;

    // get the buffer size required
    int nRequired = ::LCMapStringA(Locale, dwMapFlags,
        mbcsString, mbcsString.Length(), 0, 0);
    if (nRequired == 0)
        return 0;
    if ((dwMapFlags & LCMAP_SORTKEY) && (cchDest == 0))
        return nRequired;

    CMbcsBuffer mbcsDest;
    if (!mbcsDest.SetCapacity(nRequired))
        return 0;

    nRequired = ::LCMapStringA(Locale, dwMapFlags,
        mbcsString, mbcsString.Length(), mbcsDest, mbcsDest.BufferSize());
    if (nRequired == 0)
        return 0;

    if (dwMapFlags & LCMAP_SORTKEY) {
        if (cchDest < nRequired) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        ::CopyMemory(lpDestStr, mbcsDest.get(), nRequired);
        return nRequired;
    }

    return ::MultiByteToWideChar(CP_ACP, 0, mbcsDest, nRequired, lpDestStr, cchDest);
}

HMODULE WINAPI
LoadLibraryExW(
    IN LPCWSTR lpLibFileName,
    IN HANDLE hFile,
    IN DWORD dwFlags
    )
{
    CMbcsBuffer mbcsLibFileName;
    if (!mbcsLibFileName.FromUnicode(lpLibFileName))
        return NULL;

    return ::LoadLibraryExA(mbcsLibFileName, hFile, dwFlags);
}

HMODULE WINAPI
LoadLibraryW(
    IN LPCWSTR lpLibFileName
    )
{
    CMbcsBuffer mbcsLibFileName;
    if (!mbcsLibFileName.FromUnicode(lpLibFileName))
        return NULL;

    return ::LoadLibraryA(mbcsLibFileName);
}

BOOL WINAPI
MoveFileW(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName
    )
{
    CMbcsBuffer mbcsExistingFileName;
    if (!mbcsExistingFileName.FromUnicode(lpExistingFileName))
        return FALSE;

    CMbcsBuffer mbcsNewFileName;
    if (!mbcsNewFileName.FromUnicode(lpNewFileName))
        return FALSE;

    return ::MoveFileA(mbcsExistingFileName, mbcsNewFileName);
}

//TODO: MSLU adds support for CP_UTF7 and CP_UTF8 to Windows 95
extern "C" int WINAPI
OCOW_MultiByteToWideChar(
    IN UINT     CodePage,
    IN DWORD    dwFlags,
    IN LPCSTR   lpMultiByteStr,
    IN int      cbMultiByte,
    OUT LPWSTR  lpWideCharStr,
    IN int      cchWideChar
    )
{
    return ::MultiByteToWideChar(CodePage, dwFlags,
        lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

HANDLE WINAPI
OpenEventW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::OpenEventA(dwDesiredAccess, bInheritHandle, mbcsName);
}

HANDLE WINAPI
OpenFileMappingW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::OpenFileMappingA(dwDesiredAccess, bInheritHandle, mbcsName);
}

HANDLE WINAPI
OpenMutexW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::OpenMutexA(dwDesiredAccess, bInheritHandle, mbcsName);
}

HANDLE WINAPI
OpenSemaphoreW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpName
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return NULL;

    return ::OpenSemaphoreA(dwDesiredAccess, bInheritHandle, mbcsName);
}

typedef HANDLE (WINAPI *fpOpenWaitableTimerA)(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpTimerName
    );

HANDLE WINAPI
OpenWaitableTimerW(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCWSTR lpTimerName
    )
{
    static fpOpenWaitableTimerA pOpenWaitableTimerA =
        (fpOpenWaitableTimerA) ::GetProcAddress(
            ::GetModuleHandleA("kernel32.dll"), "OpenWaitableTimerA");
    if (!pOpenWaitableTimerA) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return NULL;
    }

    CMbcsBuffer mbcsTimerName;
    if (!mbcsTimerName.FromUnicode(lpTimerName))
        return NULL;

    return pOpenWaitableTimerA(dwDesiredAccess, bInheritHandle, mbcsTimerName);
}

VOID WINAPI
OutputDebugStringW(
    IN LPCWSTR lpOutputString
    )
{
    CMbcsBuffer mbcsOutputString;
    if (!mbcsOutputString.FromUnicode(lpOutputString))
        return;

    ::OutputDebugStringA(mbcsOutputString);
}

// PeekConsoleInputW
// QueryDosDeviceW
// ReadConsoleInputW
// ReadConsoleOutputCharacterW
// ReadConsoleOutputW
// ReadConsoleW

BOOL WINAPI
RemoveDirectoryW(
    IN LPCWSTR lpPathName
    )
{
    CMbcsBuffer mbcsPathName;
    if (!mbcsPathName.FromUnicode(lpPathName))
        return FALSE;

    return ::RemoveDirectoryA(mbcsPathName);
}

// ScrollConsoleScreenBufferW
// SearchPathW
// SetCalendarInfoW

BOOL WINAPI
SetComputerNameW(
    IN LPCWSTR lpComputerName
    )
{
    // ensure that the computer name is valid as per NT guidelines,
    // we don't coerce the characters here
    const wchar_t * pTemp = lpComputerName;
    for (wchar_t c = *pTemp; c; c = *(++pTemp)) {
        if ((c >= L'A' && c <= L'Z') ||
            (c >= L'a' && c <= L'z') ||
            (c >= L'0' && c <= L'9')) {
            continue;
        }
        switch (c) {
            case L'!': case L'@': case L'#': case L'$': case L'%': case L'^':
            case L'&': case L')': case L'(': case L'.': case L'-': case L'_':
            case L'{': case L'}': case L'~': case L'\'':
                break;
            default:
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
        }
    }

    CMbcsBuffer mbcsComputerName;
    if (!mbcsComputerName.FromUnicode(lpComputerName))
        return FALSE;

    return ::SetComputerNameA(mbcsComputerName);
}

BOOL WINAPI
SetConsoleTitleW(
    IN LPCWSTR lpConsoleTitle
    )
{
    CMbcsBuffer mbcsConsoleTitle;
    if (!mbcsConsoleTitle.FromUnicode(lpConsoleTitle))
        return FALSE;

    return ::SetConsoleTitleA(mbcsConsoleTitle);
}

BOOL WINAPI
SetCurrentDirectoryW(
    IN LPCWSTR lpPathName
    )
{
    CMbcsBuffer mbcsPathName;
    if (!mbcsPathName.FromUnicode(lpPathName))
        return FALSE;

    return ::SetCurrentDirectoryA(mbcsPathName);
}

// SetDefaultCommConfigW

BOOL WINAPI
SetEnvironmentVariableW(
    IN LPCWSTR lpName,
    IN LPCWSTR lpValue
    )
{
    CMbcsBuffer mbcsName;
    if (!mbcsName.FromUnicode(lpName))
        return FALSE;

    CMbcsBuffer mbcsValue;
    if (!mbcsValue.FromUnicode(lpValue))
        return FALSE;

    return ::SetEnvironmentVariableA(mbcsName, mbcsValue);
}

BOOL WINAPI
SetFileAttributesW(
    IN LPCWSTR lpFileName,
    IN DWORD dwFileAttributes
    )
{
    CMbcsBuffer mbcsFileName;
    if (!mbcsFileName.FromUnicode(lpFileName))
        return FALSE;

    return ::SetFileAttributesA(mbcsFileName, dwFileAttributes);
}

// SetLocaleInfoW

BOOL WINAPI
SetVolumeLabelW(
    IN LPCWSTR lpRootPathName,
    IN LPCWSTR lpVolumeName
    )
{
    CMbcsBuffer mbcsRootPathName;
    if (!mbcsRootPathName.FromUnicode(lpRootPathName))
        return FALSE;

    CMbcsBuffer mbcsVolumeName;
    if (!mbcsVolumeName.FromUnicode(lpVolumeName))
        return FALSE;

    return ::SetVolumeLabelA(mbcsRootPathName, mbcsVolumeName);
}

// UpdateResourceA
// UpdateResourceW
// WaitNamedPipeW

//TODO: MSLU adds support for CP_UTF7 and CP_UTF8 to Windows 95
extern "C" int WINAPI
OCOW_WideCharToMultiByte(
    IN UINT     CodePage,
    IN DWORD    dwFlags,
    IN LPCWSTR  lpWideCharStr,
    IN int      cchWideChar,
    OUT LPSTR   lpMultiByteStr,
    IN int      cbMultiByte,
    IN LPCSTR   lpDefaultChar,
    OUT LPBOOL  lpUsedDefaultChar)
{
    dwFlags &= ~MB_ERR_INVALID_CHARS;
    return ::WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar,
        lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
}

// WriteConsoleInputW
// WriteConsoleOutputCharacterW
// WriteConsoleOutputW

BOOL WINAPI WriteConsoleW(
    IN HANDLE   hConsoleOutput,
    const VOID  *lpBuffer,
    IN DWORD    nNumberOfCharsToWrite,
    OUT         LPDWORD lpNumberOfCharsWritten,
    IN          LPVOID  lpReserved
  )
{
    CMbcsBuffer mbcsBuffer;
    if (!mbcsBuffer.FromUnicode((LPCWSTR)lpBuffer))
        return FALSE;

    return ::WriteConsoleA(hConsoleOutput,mbcsBuffer, nNumberOfCharsToWrite,
        lpNumberOfCharsWritten, lpReserved);
}


// WritePrivateProfileSectionW
// WritePrivateProfileStringW
// WritePrivateProfileStructW
// WriteProfileSectionW
// WriteProfileStringW

LPWSTR WINAPI
lstrcatW(
    IN OUT LPWSTR lpString1,
    IN LPCWSTR lpString2
    )
{
    if (!lpString1 || !lpString2)
        return NULL;
    if (!*lpString2)
        return lpString1;

    LPWSTR lpTemp = lpString1;
    while (*lpTemp)
        ++lpTemp;
    while (*lpString2)
        *lpTemp++ = *lpString2++;
    *lpTemp = 0;

    return lpString1;
}

// lstrcmpW
// lstrcmpiW

LPWSTR
WINAPI
lstrcpyW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2
    )
{
    if (!lpString1 || !lpString2)
        return NULL;

    LPWSTR lpTemp = lpString1;
    while (*lpString2)
        *lpTemp++ = *lpString2++;
    *lpTemp = 0;

    return lpString1;
}

LPWSTR WINAPI
lstrcpynW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2,
    IN int iMaxLength
    )
{
    if (!lpString1 || !lpString2)
        return NULL;
    if (iMaxLength < 1)
        return lpString1;

    LPWSTR lpTemp = lpString1;
    while (*lpString2 && iMaxLength-- > 1)
        *lpTemp++ = *lpString2++;
    *lpTemp = 0;

    return lpString1;
}

extern "C" int WINAPI
lstrlenW(
    IN LPCWSTR lpString
    )
{
    int len = 0;
    while (*lpString++)
        ++len;
    return len;
}

extern "C" BOOL WINAPI GetVersionExW(LPOSVERSIONINFOW lpVersionInformation)
{
    OSVERSIONINFOA osvi;

    if (lpVersionInformation->dwOSVersionInfoSize >= sizeof(OSVERSIONINFOW))
    {
        memset(&osvi, 0, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (!GetVersionExA(&osvi))
            return FALSE;

        memcpy(lpVersionInformation, &osvi, sizeof(osvi));
        ::MultiByteToWideChar(CP_ACP, 0, osvi.szCSDVersion, -1, lpVersionInformation->szCSDVersion,
                              sizeof(lpVersionInformation->szCSDVersion));
        return TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
}

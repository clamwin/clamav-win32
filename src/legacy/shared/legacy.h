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

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdio.h>
#include <stdlib.h>

/* bogus msvc defines */
#define _WIN32_WINNT_WIN10_TH2 NTDDI_WIN10_TH2
#define _WIN32_WINNT_WIN10_RS1 NTDDI_WIN10_RS1
#define _WIN32_WINNT_WIN10_RS2 NTDDI_WIN10_RS2
#define _WIN32_WINNT_WIN10_RS3 NTDDI_WIN10_RS3
#define _WIN32_WINNT_WIN10_RS4 NTDDI_WIN10_RS4
#define _WIN32_WINNT_WIN10_RS5 NTDDI_WIN10_RS5

#define _WIN32_WINNT_OLD _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A000002
#include <sdkddkver.h>
#include <winsock2.h>
#include <windows.h>
#include <winternl.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_OLD
#undef _WIN32_WINNT_OLD

#include <ntstatus.h>
#include <tchar.h>
#include <strsafe.h>

#define NtCurrentProcess() ((HANDLE)(LONG_PTR) - 1)
#define NtCurrentThread() ((HANDLE)(LONG_PTR) - 2)

#ifdef _MSC_VER
typedef BOOL WINBOOL;
typedef void *LPVOID;

#define ObjectNameInformation 1
#define FileBasicInformation 4
#define FileNameInformation 9
#define FileDispositionInformation 13
#define FileNormalizedNameInformation 48

typedef struct _FILE_BASIC_INFORMATION
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
    BOOLEAN DoDeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_NAME_INFORMATION
{
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

BOOLEAN NTAPI RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID HeapBase);
PVOID NTAPI RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size);

NTSTATUS NTAPI NtSetInformationFile(HANDLE hFile, PIO_STATUS_BLOCK io, PVOID ptr, ULONG len, FILE_INFORMATION_CLASS FileInformationClass);
NTSTATUS NTAPI NtQueryInformationFile(HANDLE hFile, PIO_STATUS_BLOCK io, PVOID ptr, ULONG len, FILE_INFORMATION_CLASS FileInformationClass);
#endif

#ifndef NO_FUNCTION_REDEFINITION
DWORD WINAPI GetLongPathNameW(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer);
DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);
WINBOOL WINAPI GetFileInformationByHandleEx(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
WINBOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
HANDLE WINAPI ReOpenFile(HANDLE hOriginalFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes);
#endif

typedef struct _REPARSE_DATA_BUFFER
{
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    } _;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

// https://github.com/mic101/windows/blob/master/WRK-v1.2/public/sdk/inc/mountmgr.h

typedef struct _MOUNTMGR_TARGET_NAME
{
    USHORT DeviceNameLength;
    WCHAR DeviceName[1];
} MOUNTMGR_TARGET_NAME, *PMOUNTMGR_TARGET_NAME;

typedef struct _MOUNTMGR_VOLUME_PATHS
{
    ULONG MultiSzLength;
    WCHAR MultiSz[1];
} MOUNTMGR_VOLUME_PATHS, *PMOUNTMGR_VOLUME_PATHS;

typedef struct _MOUNTMGR_MOUNT_POINT
{
    ULONG SymbolicLinkNameOffset;
    USHORT SymbolicLinkNameLength;
    ULONG UniqueIdOffset;
    USHORT UniqueIdLength;
    ULONG DeviceNameOffset;
    USHORT DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS
{
    ULONG Size;
    ULONG NumberOfMountPoints;
    MOUNTMGR_MOUNT_POINT MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

typedef enum _RTL_PATH_TYPE
{
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,    // "\\\\server\\share\\folder\\file.txt
    RtlPathTypeDriveAbsolute,  // "C:\\folder\\file.txt"
    RtlPathTypeDriveRelative,  // "C:folder\\file.txt"
    RtlPathTypeRooted,         // "\\folder\\file.txt"
    RtlPathTypeRelative,       // "folder\\file.txt"
    RtlPathTypeLocalDevice,    // "\\\\.\\PhysicalDrive0"
    RtlPathTypeRootLocalDevice // "\\\\?\\C:\\folder\\file.txt"
} RTL_PATH_TYPE;

#define MOUNTMGR_IS_VOLUME_NAME2(Buffer, Length) (            \
    (Length == 96 || (Length == 98 && Buffer[48] == '\\')) && \
    Buffer[0] == '\\' &&                                      \
    (Buffer[1] == '?' || Buffer[1] == '\\') &&                \
    Buffer[2] == '?' &&                                       \
    Buffer[3] == '\\' &&                                      \
    Buffer[4] == 'V' &&                                       \
    Buffer[5] == 'o' &&                                       \
    Buffer[6] == 'l' &&                                       \
    Buffer[7] == 'u' &&                                       \
    Buffer[8] == 'm' &&                                       \
    Buffer[9] == 'e' &&                                       \
    Buffer[10] == '{' &&                                      \
    Buffer[19] == '-' &&                                      \
    Buffer[24] == '-' &&                                      \
    Buffer[29] == '-' &&                                      \
    Buffer[34] == '-' &&                                      \
    Buffer[47] == '}')

#define MOUNTMGR_DOS_DEVICE_NAME L"\\\\.\\MountPointManager"

#define MOUNTMGRCONTROLTYPE ((ULONG)'m')

#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MOUNTMGR_QUERY_POINTS \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef __GNUC__
#define HOTFUNC __attribute__((hot))
#else
#define HOTFUNC
#endif

#ifndef WRAP_SUFFIX
#define WRAP_SUFFIX
#endif

#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define WRAP(func) CONCAT(func, WRAP_SUFFIX)

#ifdef LEGACY_TRACE
#include <inttypes.h>
#define TRACE(format, ...) _ftprintf(stderr, TEXT("[legacy] ") TEXT(format), ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#endif

#endif

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

#include <tchar.h>
#include <strsafe.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
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

#define MOUNTMGR_DOS_DEVICE_NAME L"\\\\.\\MountPointManager"

#define MOUNTMGRCONTROLTYPE ((ULONG)'m')

#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)


#ifdef _MSC_VER
typedef BOOL WINBOOL;
#endif

#ifdef __GNUC__
#define HOTFUNC __attribute__((hot))
#else
#define HOTFUNC
#endif

#ifdef LEGACY_TRACE
#include <stdio.h>
#include <inttypes.h>
#define TRACE(format, ...) _ftprintf(stderr, TEXT("[legacy] ") TEXT(format), ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#endif

#endif

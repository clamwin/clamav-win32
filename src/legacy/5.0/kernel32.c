/*
 * Windows XP Compatibility Layer
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

#include "legacy.h"

#include <fileapi.h>
#include <psapi.h>

DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

WINBOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    TRACE("SetFileInformationByHandle(0x%p, %d, 0x%p, %d)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);

    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    BOOL success = FALSE;

    // Validate parameters.
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL)
    {
        TRACE("SetFileInformationByHandle: hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Map Win32 file information class to NT file information class
    // and prepare the appropriate structure
    PVOID ntBuffer = NULL;
    ULONG ntBufferSize = 0;
    FILE_INFORMATION_CLASS ntInfoClass;

    switch (FileInformationClass)
    {
    case FileBasicInfo:
    {
        if (dwBufferSize < sizeof(FILE_BASIC_INFO))
        {
            TRACE("SetFileInformationByHandle[FileBasicInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        PFILE_BASIC_INFO win32BasicInfo = (PFILE_BASIC_INFO)lpFileInformation;
        PFILE_BASIC_INFORMATION ntBasicInfo;

        ntBufferSize = sizeof(FILE_BASIC_INFORMATION);
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntBasicInfo = (PFILE_BASIC_INFORMATION)ntBuffer;
        ntBasicInfo->CreationTime = win32BasicInfo->CreationTime;
        ntBasicInfo->LastAccessTime = win32BasicInfo->LastAccessTime;
        ntBasicInfo->LastWriteTime = win32BasicInfo->LastWriteTime;
        ntBasicInfo->ChangeTime = win32BasicInfo->ChangeTime;
        ntBasicInfo->FileAttributes = win32BasicInfo->FileAttributes;

        ntInfoClass = FileBasicInformation;
        break;
    }
    case FileRenameInfo:
    case FileRenameInfoEx:
    {
        if (dwBufferSize < sizeof(FILE_RENAME_INFO))
        {
            TRACE("SetFileInformationByHandle[FileRenameInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        PFILE_RENAME_INFO win32RenameInfo = (PFILE_RENAME_INFO)lpFileInformation;
#if 0
        UNICODE_STRING ntPath;
        // Convert the DOS path to an NT native path.
        if (!RtlDosPathNameToNtPathName_U(win32RenameInfo->FileName, &ntPath, NULL, NULL))
        {
            TRACE("RtlDosPathNameToNtPathName_U failed\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        // Print the converted NT path.
        TRACE("NT Path: [%ls]\n", ntPath.Buffer);

        // Map FileRenameInfo to FileRenameInformation
        PFILE_RENAME_INFORMATION ntRenameInfo;

        // Calculate the NT buffer size
        ntBufferSize = sizeof(FILE_RENAME_INFORMATION) - sizeof(wchar_t) + ntPath.Length;
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntRenameInfo = (PFILE_RENAME_INFORMATION)ntBuffer;
        ntRenameInfo->ReplaceIfExists = win32RenameInfo->ReplaceIfExists;
        ntRenameInfo->RootDirectory = win32RenameInfo->RootDirectory;
        ntRenameInfo->FileNameLength = ntPath.Length;
        memcpy(ntRenameInfo->FileName, ntPath.Buffer, ntPath.Length);

        status = NtSetInformationFile(
            hFile,
            &ioStatusBlock,
            ntBuffer,
            ntBufferSize,
            FileRenameInformation);

        // Clean up
        free(ntBuffer);

        // Convert NT status to Win32 error and set return value
        if (NT_SUCCESS(status))
        {
            TRACE("SetFileInformationByHandle -> NtSetInformationFile Rename OK\n");
            return TRUE;
        }
        else
        {
            TRACE("SetFileInformationByHandle -> NtSetInformationFile Rename failed: 0x%08lx (%ld)\n",
                  status, RtlNtStatusToDosError(status));
#else
        {
#endif
            // try with RenameFileEx
            wchar_t sourcePath[MAX_PATH + 1];

            if (!GetFinalPathNameByHandleW(hFile, sourcePath, MAX_PATH, VOLUME_NAME_DOS))
                return FALSE;

            DWORD moveFlags = 0;
            if (win32RenameInfo->ReplaceIfExists)
                moveFlags |= MOVEFILE_REPLACE_EXISTING;

            return MoveFileExW(sourcePath, win32RenameInfo->FileName, moveFlags);
        }
    }
    case FileDispositionInfo:
    {
        if (dwBufferSize < sizeof(FILE_DISPOSITION_INFORMATION))
        {
            TRACE("SetFileInformationByHandle[FileRenameInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        // Map FileDispositionInfo to FileDispositionInformation
        PFILE_DISPOSITION_INFO win32DispInfo = (PFILE_DISPOSITION_INFO)lpFileInformation;
        PFILE_DISPOSITION_INFORMATION ntDispInfo;

        ntBufferSize = sizeof(FILE_DISPOSITION_INFORMATION);
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntDispInfo = (PFILE_DISPOSITION_INFORMATION)ntBuffer;
        ntDispInfo->DoDeleteFile = win32DispInfo->DeleteFile;

        ntInfoClass = FileDispositionInformation;
        break;
    }
    default:
        TRACE("SetFileInformationByHandle: Unsupported FileInformationClass %d\n", FileInformationClass);
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    // Call NtSetInformationFile with the prepared NT structures
    status = NtSetInformationFile(
        hFile,
        &ioStatusBlock,
        ntBuffer,
        ntBufferSize,
        ntInfoClass);

    // Convert NT status to Win32 error and set return value
    if (NT_SUCCESS(status))
    {
        TRACE("SetFileInformationByHandle -> NtSetInformationFile OK\n");
        success = TRUE;
    }
    else
    {
        // Convert NTSTATUS to Win32 error code
        TRACE("SetFileInformationByHandle -> NtSetInformationFile failed: 0x%08lx (%ld)\n", status, RtlNtStatusToDosError(status));
        SetLastError(RtlNtStatusToDosError(status));
        success = FALSE;
    }

    // Clean up
    if (ntBuffer)
        free(ntBuffer);

    return success;
}

// https://github.com/zeroclear/xpext/blob/master/xpext_ver4/k32_file.cpp#L445
// https://stackoverflow.com/questions/65170/how-to-get-name-associated-with-open-handle/5286888#5286888
union ANY_BUFFER
{
    MOUNTMGR_TARGET_NAME TargetName;
    MOUNTMGR_VOLUME_PATHS TargetPaths;
    FILE_NAME_INFORMATION NameInfo;
    UNICODE_STRING UnicodeString;
    WCHAR Buffer[USHRT_MAX];
};

/*
 * GetFinalPathNameByHandleW - Retrieves the final path for the specified file
 *
 * @param hFile       - Handle to a file or directory
 * @param lpszFilePath - Buffer to receive the path
 * @param cchFilePath - Size of the buffer in characters
 * @param dwFlags     - Format of the returned path (currently only VOLUME_NAME_DOS supported)
 *
 * @return Number of characters in the final path (excluding terminator), or 0 on failure
 */
HOTFUNC DWORD WINAPI
GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    size_t requiredLength = 0;

    TRACE("GetFinalPathNameByHandleW(0x%p, 0x%p, %d, %d)\n", hFile, lpszFilePath, cchFilePath, dwFlags);

    // Validate input parameters.
    if (hFile == INVALID_HANDLE_VALUE)
    {
        TRACE("GetFinalPathNameByHandleW: -> ERROR_INVALID_PARAMETER (invalid handle)\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwFlags != VOLUME_NAME_DOS)
    {
        TRACE("GetFinalPathNameByHandleW: Unsupported dwFlags 0x%08x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Allocate buffers for path information
    // FIXME: too big?
    union ANY_BUFFER nameFull, nameRel, nameMnt;

    // pointer refs for readability
    wchar_t *deviceName = nameMnt.TargetName.DeviceName;
    wchar_t *fileName = nameRel.NameInfo.FileName;
    wchar_t targetPath[MAX_PATH + 1] = L"\\\\?\\";

    // Get object name information (full NT path)
    status = NtQueryObject(hFile, ObjectNameInformation, nameFull.Buffer, sizeof(nameFull.Buffer), NULL);
    if (!NT_SUCCESS(status))
    {
        TRACE("GetFinalPathNameByHandleW->NtQueryObject failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    // Get file name information (relative path)
    status = NtQueryInformationFile(hFile, &iosb, nameRel.Buffer, sizeof(nameRel.Buffer), FileNameInformation);
    if (!NT_SUCCESS(status))
    {
        TRACE("GetFinalPathNameByHandleW->NtQueryInformationFile failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    if (nameFull.UnicodeString.Length < nameRel.NameInfo.FileNameLength)
    {
        TRACE("Path length validation failed: full (%d) < relative (%d)\n",
              nameFull.UnicodeString.Length, nameRel.NameInfo.FileNameLength);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t nameLength = nameRel.NameInfo.FileNameLength / sizeof(wchar_t);

    // Extract the device path portion
    nameMnt.TargetName.DeviceNameLength = (USHORT)(nameFull.UnicodeString.Length - nameRel.NameInfo.FileNameLength);
    wcsncpy(nameMnt.TargetName.DeviceName,
            nameFull.UnicodeString.Buffer,
            nameMnt.TargetName.DeviceNameLength / sizeof(wchar_t));

    TRACE("deviceName: [%ls]\n", deviceName);
    TRACE("fileName: [%ls]\n", fileName);

    // skip Mup Device
    if (wcsncmp(deviceName, L"\\Device\\Mup", 11))
    {
        HANDLE hMountMgr = CreateFile(
            MOUNTMGR_DOS_DEVICE_NAME,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hMountMgr != INVALID_HANDLE_VALUE)
        {
            DWORD bytesReturned = 0;
            BOOL success = DeviceIoControl(hMountMgr,
                                           IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                           &nameMnt,
                                           sizeof(nameMnt),
                                           &nameMnt,
                                           sizeof(nameMnt),
                                           &bytesReturned,
                                           NULL);

            if (success && nameMnt.TargetPaths.MultiSzLength > 0)
            {
                TRACE("Resolved via MountMgr: %ls\n", targetPath);
                wcsncat(targetPath, nameMnt.TargetPaths.MultiSz, nameMnt.TargetPaths.MultiSzLength);
                wcsncat(targetPath, fileName, nameLength);
                requiredLength = wcslen(targetPath);
            }
            CloseHandle(hMountMgr);
        }
    }

    if (requiredLength == 0)
    {
        wcsncat(targetPath, L"UNC", 3);
        wcsncat(targetPath, fileName, nameLength);
        requiredLength = wcslen(targetPath);
    }

    if (lpszFilePath && (cchFilePath >= requiredLength))
    {
        wcsncpy(lpszFilePath, targetPath, requiredLength);
        lpszFilePath[requiredLength] = L'\0';
    }

    TRACE("GetFinalPathNameByHandleW -> %ls (%lld chars)\n", targetPath, requiredLength);
    // Return the length of the final path (excluding the terminating null).
    return (DWORD)requiredLength;
}

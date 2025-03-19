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

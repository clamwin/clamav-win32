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
#include "legacy.h"

WINBOOL WINAPI GetFileInformationByHandleEx(HANDLE hFile,
                                            FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                            LPVOID lpFileInformation,
                                            DWORD dwBufferSize)
{
    TRACE("GetFileInformationByHandleEx(0x%p, %d, 0x%p, %ld)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);

    // Validate input parameters
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL)
    {
        TRACE("GetFileInformationByHandleEx -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (FileInformationClass == FileBasicInfo)
    {
        // Check if the provided buffer is large enough for FILE_BASIC_INFO
        if (dwBufferSize < sizeof(FILE_BASIC_INFO))
        {
            TRACE("GetFileInformationByHandleEx -> ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        FILE_BASIC_INFO *pInfo = (FILE_BASIC_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE("GetFileInformationByHandleEx[FileBasicInfo] -> GetFileInformationByHandle Failed (%ld)\n", GetLastError());
            return FALSE;
        }

        // Copy the time and attribute data from the handle information
        memcpy(&pInfo->CreationTime, &fileInfo.ftCreationTime, sizeof(pInfo->CreationTime));
        memcpy(&pInfo->LastAccessTime, &fileInfo.ftLastAccessTime, sizeof(pInfo->LastAccessTime));
        memcpy(&pInfo->LastWriteTime, &fileInfo.ftLastWriteTime, sizeof(pInfo->LastWriteTime));
        // Windows XP does not provide a ChangeTime; using LastWriteTime as a fallback
        memcpy(&pInfo->ChangeTime, &fileInfo.ftLastWriteTime, sizeof(pInfo->ChangeTime));
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        return TRUE;
    }
    else if (FileInformationClass == FileStandardInfo)
    {
        // Check if the provided buffer is large enough for FILE_STANDARD_INFO
        if (dwBufferSize < sizeof(FILE_STANDARD_INFO))
        {
            TRACE("GetFileInformationByHandleEx[FileStandardInfo] -> ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_STANDARD_INFO *pInfo = (FILE_STANDARD_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE("GetFileInformationByHandleEx[FileStandardInfo] -> GetFileInformationByHandle Failed (%ld)\n", GetLastError());
            return FALSE;
        }

        // Compute the file size from its low and high parts
        LARGE_INTEGER fileSize;
        fileSize.LowPart = fileInfo.nFileSizeLow;
        fileSize.HighPart = fileInfo.nFileSizeHigh;

        pInfo->AllocationSize = fileSize;
        pInfo->EndOfFile = fileSize;
        pInfo->NumberOfLinks = fileInfo.nNumberOfLinks;
        pInfo->DeletePending = FALSE; // Not determinable via this API
        pInfo->Directory = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
        return TRUE;
    }
    else if (FileInformationClass == FileAttributeTagInfo)
    {
        // Check if the provided buffer is large enough for FILE_ATTRIBUTE_TAG_INFO
        if (dwBufferSize < sizeof(FILE_ATTRIBUTE_TAG_INFO))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_ATTRIBUTE_TAG_INFO *pInfo = (FILE_ATTRIBUTE_TAG_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE("GetFileInformationByHandleEx[FileAttributeTagInfo] -> GetFileInformationByHandle Failed (%ld)\n", GetLastError());
            return FALSE;
        }

        // Copy the file attributes from the handle information
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        pInfo->ReparseTag = 0; // Default value if not a reparse point

        // If the file is a reparse point, try to retrieve the reparse tag
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            // Allocate a buffer for the reparse point data.
            // Although the maximum size is defined by MAXIMUM_REPARSE_DATA_BUFFER,
            // we use a static buffer of 16 KB for simplicity.
            BYTE buffer[16 * 1024];
            DWORD dwBytesReturned = 0;
            if (DeviceIoControl(hFile,
                                FSCTL_GET_REPARSE_POINT,
                                NULL,
                                0,
                                buffer,
                                sizeof(buffer),
                                &dwBytesReturned,
                                NULL))
            {
                // Cast the buffer to a REPARSE_DATA_BUFFER pointer to retrieve the ReparseTag.
                // Note: REPARSE_DATA_BUFFER has a variable layout; here we only extract the ReparseTag.
                pInfo->ReparseTag = ((PREPARSE_DATA_BUFFER)buffer)->ReparseTag;
            }
            // If DeviceIoControl fails, ReparseTag remains 0.
        }
        return TRUE;
    }
    else
    {
        // Unsupported FileInformationClass
        TRACE("GetFileInformationByHandleEx: Unsupported FileInformationClass %d\n", FileInformationClass);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

WINBOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    TRACE("SetFileInformationByHandle(0x%p, %d, 0x%p, %ld)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);
    fprintf(stderr, "SetFileInformationByHandle: ERROR_CALL_NOT_IMPLEMENTED\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

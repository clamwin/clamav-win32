/*
 * Windows XP Compatibility Layer: test for SetFileInformationByHandle (rename)
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

#include <stdio.h>

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 3)
    {
        wprintf(L"Invalid arguments\n");
        return 1;
    }

    // Path to the file we want to rename
    const wchar_t *oldFileName = argv[1];
    // New file name (not path, just the name)
    const wchar_t *newFileName = argv[2];

    // Open the file with required permissions
    HANDLE hFile = CreateFile(
        oldFileName,
        DELETE,                             // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // Share mode
        NULL,                               // Security attributes
        OPEN_EXISTING,                      // Open existing file
        FILE_ATTRIBUTE_NORMAL,              // File attributes
        NULL                                // Template file handle
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Failed to open file. Error code: %lu\n", GetLastError());
        return 1;
    }

    // Prepare the rename info structure
    DWORD bufferSize = sizeof(FILE_RENAME_INFO) + (wcslen(newFileName) * sizeof(WCHAR));
    FILE_RENAME_INFO *renameInfo = malloc(bufferSize);

    if (!renameInfo)
    {
        wprintf(L"Memory allocation failed\n");
        CloseHandle(hFile);
        return 1;
    }

    // Fill the structure
    renameInfo->ReplaceIfExists = FALSE; // Don't replace if file with new name exists
    renameInfo->RootDirectory = NULL;    // Not using a root directory handle
    renameInfo->FileNameLength = wcslen(newFileName) * sizeof(WCHAR);
    memcpy(renameInfo->FileName, newFileName, renameInfo->FileNameLength);

    if (SetFileInformationByHandle(hFile, FileRenameInfo, renameInfo, bufferSize))
        wprintf(L"File renamed successfully to %ls\n", newFileName);
    else
        wprintf(L"Failed to rename file. Error code: %lu\n", GetLastError());

    // Clean up
    free(renameInfo);
    CloseHandle(hFile);

    return 0;
}

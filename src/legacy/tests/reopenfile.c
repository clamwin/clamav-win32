/*
 * Legacy Windows Compatibility Layer: testcase for ReOpenFile
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

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        wprintf(L"Invalid arguments\n");
        return 1;
    }

    // Step 1: Create original file handle with more permissive sharing
    HANDLE hFileOriginal = CreateFile(
        argv[1],                            // File name
        GENERIC_READ,                       // Access mode (read only)
        FILE_SHARE_READ | FILE_SHARE_WRITE, // More permissive share mode
        NULL,                               // Security attributes
        OPEN_ALWAYS,                        // Creation disposition
        FILE_ATTRIBUTE_NORMAL,              // File attributes
        NULL                                // Template file
    );

    if (hFileOriginal == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed with error %ld\n", GetLastError());
        return 1;
    }

    printf("Original file opened with read access\n");

    // Step 2: Reopen the file with write access
    HANDLE hFileReopened = ReOpenFile(
        hFileOriginal,                      // Original handle
        GENERIC_WRITE,                      // New desired access (write)
        FILE_SHARE_READ | FILE_SHARE_WRITE, // New share mode
        FILE_FLAG_WRITE_THROUGH             // New flags
    );

    if (hFileReopened == INVALID_HANDLE_VALUE)
    {
        printf("ReOpenFile failed with error %ld\n", GetLastError());
        CloseHandle(hFileOriginal);
        return 1;
    }

    printf("File reopened with write access\n");

    // Clean up
    CloseHandle(hFileOriginal);
    CloseHandle(hFileReopened);

    return 0;
}

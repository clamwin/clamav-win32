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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        wprintf(L"Invalid arguments\n");
        return 1;
    }

    HANDLE hFile = CreateFile(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fwprintf(stderr, L"CreateFile(%ld)\n", GetLastError());
        return 1;
    }

    wchar_t lpszFilePath[MAX_PATH];

    if (GetFinalPathNameByHandleW(hFile, lpszFilePath, MAX_PATH, VOLUME_NAME_DOS))
        wprintf(L"Result ->[%ls]\n", lpszFilePath);
    else
        wprintf(L"GetFinalPathNameByHandleW() failed with %d\n", GetLastError());

    CloseHandle(hFile);

    return 0;
}

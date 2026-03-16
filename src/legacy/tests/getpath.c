/*
 * Copyright (c) 2017 Stephen Griffin
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

static int _GetPath(wchar_t *dirname, int accessFlags)
{
    printf("\tGetPath: Testing access flags 0x%08X with %ls\r\n", accessFlags, dirname);

    HANDLE hnd = CreateFileW(dirname, accessFlags,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                           OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hnd == INVALID_HANDLE_VALUE)
    {
        errno = GetLastError();
        printf("\tGetPath: CreateFileW got error %X\r\n", errno);
        return -1;
    }

    wchar_t wdirname[MAX_PATH] = {0};
    if (!GetFinalPathNameByHandleW(hnd, wdirname, sizeof wdirname / sizeof(wchar_t), 0))
    {
        errno = GetLastError();
        printf("\tGetPath: GetFinalPathNameByHandleW got error %X\r\n", errno);
        CloseHandle(hnd);
        return -1;
    }

    printf("\tGetPath: GetFinalPathNameByHandleW returned: %ls\r\n", wdirname);

    CloseHandle(hnd);
    return 0;
}

static int GetPathExclusive(wchar_t *dirname, int sharingFlags)
{
    printf("GetPathExclusive: Testing sharing flags 0x%08X\r\n", sharingFlags);

    HANDLE hnd = CreateFileW(dirname, GENERIC_READ | GENERIC_WRITE,
                           sharingFlags, NULL,
                           OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hnd == INVALID_HANDLE_VALUE)
    {
        errno = GetLastError();
        printf("CreateFileW got error %X\r\n", errno);
        return -1;
    }

    _GetPath(dirname, 0);
    _GetPath(dirname, GENERIC_READ);

    CloseHandle(hnd);
    return 0;
}

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        printf("Please pass path to file to check\r\n");
        return 1;
    }

    printf("GetPath testing GetFinalPathNameByHandleW calls against %ls\r\n", argv[1]);
    printf("\r\nFirst test: don't open file first, just try to get the path\r\n");
    _GetPath(argv[1], 0);
    _GetPath(argv[1], GENERIC_READ);

    printf("\r\nSecond test: open the file first without sharing, then try to get the path\r\n");
    GetPathExclusive(argv[1], 0);

    printf("\r\nThird test: open the file first with FILE_SHARE_WRITE, then try to get the path\r\n");
    GetPathExclusive(argv[1], FILE_SHARE_WRITE);

    printf("\r\nFourth test: open the file first with FILE_SHARE_READ, then try to get the path\r\n");
    GetPathExclusive(argv[1], FILE_SHARE_READ);
    return 0;
}

/*
 * Legacy Windows Compatibility Layer: testcase for GetLongPathNameW
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
    wchar_t lpszFilePath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, lpszFilePath))
    {
        wprintf(L"GetTempPathW() failed with %d\n", GetLastError());
        return 0;
    }

    wprintf(L"->[%ls]\n", lpszFilePath);

    DWORD res;
    if ((res = GetLongPathNameW(lpszFilePath, lpszFilePath, MAX_PATH)))
        wprintf(L"->[%ls] (%ld)\n", lpszFilePath, res);
    else
        wprintf(L"GetLongPathNameW() failed with %d\n", GetLastError());

    return 0;
}

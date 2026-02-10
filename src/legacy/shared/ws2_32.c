/*
 * Legacy Windows Compatibility Layer
 *
 * Copyright (c) 2025-2026 Gianluigi Tiesi <sherpya@gmail.com>
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
#include "dynload.h"
#include "loadlibrary.h"
#include "initializer.h"

// added in rust 1.93
static int WINAPI GetHostNameW_compat(PWSTR name, int namelen)
{
    if (name == NULL || namelen <= 0)
    {
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    char ansiBuffer[256];
    if (gethostname(ansiBuffer, sizeof(ansiBuffer)))
        return SOCKET_ERROR;

    if (!MultiByteToWideChar(CP_ACP, 0, ansiBuffer, -1, name, namelen))
    {
        WSASetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    return 0;
}

imp_GetHostNameW pGetHostNameW = GetHostNameW_compat;

INITIALIZER(init_ws2_32)
{
    TRACE("Init @ " __FILE__ "\n");

    HMODULE ws2_32 = LoadLibraryFromWin32(TEXT("ws2_32.dll"));
    if (ws2_32)
        IMPORT_FUNCTION(ws2_32, GetHostNameW);
}

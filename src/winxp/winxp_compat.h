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

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdlib.h>

#define _WIN32_WINNT_WIN10_TH2 NTDDI_WIN10_TH2
#define _WIN32_WINNT_WIN10_RS1 NTDDI_WIN10_RS1
#define _WIN32_WINNT_WIN10_RS2 NTDDI_WIN10_RS2
#define _WIN32_WINNT_WIN10_RS3 NTDDI_WIN10_RS3
#define _WIN32_WINNT_WIN10_RS4 NTDDI_WIN10_RS4
#define _WIN32_WINNT_WIN10_RS5 NTDDI_WIN10_RS5

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A000002
#include <sdkddkver.h>
#include <windows.h>
#include <winternl.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP

#include <wchar.h>
#include <strsafe.h>

#ifdef _MSC_VER
typedef BOOL WINBOOL;
#endif

#ifdef __GNUC__
#define HOTFUNC __attribute__((hot))
#else
#define HOTFUNC
#endif

#ifdef TRACE_COMPAT
#include <stdio.h>
#define TRACE(format, ...) fwprintf(stderr, L"[winxp] " format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#endif

#endif

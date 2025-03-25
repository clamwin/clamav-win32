/*
 * Clamav Native Windows Port: Dynamic loading helpers
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#ifndef _DYNLOAD_H_
#define _DYNLOAD_H_
#include <windows.h>
#include <tlhelp32.h>

#define Q(string) #string
#define IMPORT_FUNCTION(hLib, func)                                  \
    ({                                                               \
        imp_##func symbol = (imp_##func)GetProcAddress(hLib, #func); \
        if (symbol)                                                  \
            p##func = symbol;                                        \
        symbol;                                                      \
    })

typedef BOOL(WINAPI *imp_IsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
typedef BOOL(WINAPI *imp_Wow64DisableWow64FsRedirection)(PVOID OldValue);

typedef BOOL(WINAPI *imp_CreateHardLinkW)(LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef DWORD(WINAPI *imp_GetLongPathNameW)(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer);

typedef HANDLE(WINAPI *imp_CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL(WINAPI *imp_Process32FirstW)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
typedef BOOL(WINAPI *imp_Process32NextW)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL(WINAPI *imp_Module32FirstW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);
typedef BOOL(WINAPI *imp_Module32NextW)(HANDLE hSnapshot, LPMODULEENTRY32W lpme);

#endif /* _DYNLOAD_H_ */

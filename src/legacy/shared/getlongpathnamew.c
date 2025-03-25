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

static RTL_PATH_TYPE WINAPI RtlDetermineDosPathNameType_U(PCWSTR Path)
{
    if ((Path[0] != '\\') && (Path[0] != '/'))
    {
        if (!Path[0] || (Path[1] != ':'))
            return RtlPathTypeRelative;
        if ((Path[2] == '\\') || (Path[2] == '/'))
            return RtlPathTypeDriveAbsolute;
        else
            return RtlPathTypeDriveRelative;
    }

    if ((Path[1] != '\\') && (Path[1] != '/'))
        return RtlPathTypeRooted;

    if ((Path[2] != '.') && (Path[2] != '?'))
        return RtlPathTypeUncAbsolute;

    if ((Path[3] == '\\') || (Path[3] == '/'))
        return RtlPathTypeLocalDevice;

    return Path[3] ? RtlPathTypeUncAbsolute : RtlPathTypeRootLocalDevice;
}

static PWCHAR WINAPI SkipPathTypeIndicator_U(LPWSTR Path)
{
    switch (RtlDetermineDosPathNameType_U(Path))
    {
    case RtlPathTypeUncAbsolute:
    {
        PWCHAR ReturnPath = Path + 2;

        if (!wcsnicmp(Path, L"?\\UNC", 5))
            ReturnPath += 6;

        for (int i = 2; *ReturnPath && (i > 0); ReturnPath++)
        {
            if ((*ReturnPath == L'\\') || (*ReturnPath == L'/'))
                i--;
        }
        return ReturnPath;
    }
    case RtlPathTypeDriveAbsolute:
        return Path + 3;
    case RtlPathTypeDriveRelative:
        return Path + 2;
    case RtlPathTypeRooted:
        return Path + 1;
    case RtlPathTypeRelative:
        return Path;
    default:
        return NULL;
    }
}

static const DWORD IllegalMask[4] = {0xFFFFFFFF, 0xFC009C05, 0x38000000, 0x10000000};

static BOOL WINAPI IsShortName_U(PWCHAR Name, ULONG Length)
{
    /* 8+3 */
    if (Length > 12)
        return FALSE;

    if (Length == 0)
        return FALSE;

    if (*Name == L'.') /* . or .. */
        return (Length == 1) || ((Length == 2) && Name[1] == '.');

    UNICODE_STRING UnicodeString = {
        .Length = (USHORT)(Length * sizeof(wchar_t)),
        .MaximumLength = (USHORT)(Length * sizeof(wchar_t)),
        .Buffer = Name};

    CHAR AnsiBuffer[MAX_PATH];
    ANSI_STRING AnsiString = {
        .Length = 0,
        .MaximumLength = sizeof(AnsiBuffer),
        .Buffer = AnsiBuffer};

    // pfnUnicodeStringToEightBitString
    if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE)))
        return FALSE;

    if (!AnsiString.Length)
        return TRUE;

    BOOL bExt = FALSE;
    for (int i = 0, Dots = Length - 1; i < AnsiString.Length; i++, Dots--)
    {
        char c = AnsiString.Buffer[i];

        if (IsDBCSLeadByte(c))
        {
            if (((!bExt) && (i >= 7)) || (i == AnsiString.Length - 1))
                return FALSE;
        }
        else
        {
            if (c > 0x7f || ((1 << (c & 0x1f)) & IllegalMask[c >> 5]) != 0)
                return FALSE;

            if (c == '.')
            {
                if (bExt || Dots > 3)
                    return FALSE;
                bExt = TRUE;
            }

            if (i >= 8 && !bExt)
                return FALSE;
        }
    }

    return TRUE;
}

static BOOL WINAPI IsLongName_U(PWCHAR FileName, ULONG Length)
{
    if (!Length || (Length > 12) || (*FileName == L'.'))
        return TRUE;

    BOOL bExt = FALSE;
    for (ULONG i = 0, Dots = Length - 1; i < Length; i++, Dots--)
    {
        if (FileName[i] == L'.')
        {
            if (bExt || (Dots > 3))
                return TRUE;
            bExt = TRUE;
        }

        if ((i >= 8) && !bExt)
            return TRUE;
    }

    return FALSE;
}

static BOOL WINAPI FindLFNorSFN_U(PWCHAR Path, PWCHAR *First, PWCHAR *Last, BOOL UseShort)
{
    while (TRUE)
    {
        while ((*Path == L'\\') || (*Path == L'/'))
            Path++;

        if (!*Path)
            break;

        PWCHAR p = Path + 1;
        while ((*p) && ((*p != L'\\') && (*p != L'/')))
            p++;

        ULONG Length = (ULONG)(p - Path);

        BOOL bFound = UseShort ? !IsShortName_U(Path, Length) : !IsLongName_U(Path, Length);
        if (bFound)
        {
            if (First && Last)
            {
                *First = Path;
                *Last = p;
            }
            return TRUE;
        }

        if (!*p)
            return FALSE;

        Path = p + 1;
    }

    return FALSE;
}

DWORD WINAPI GetLongPathNameW(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer)
{
    SIZE_T ReturnLength = 0;
    PWCHAR Buffer = NULL;
    PWCHAR Src, Dst;
    WCHAR LastChar;

    if (!lpszShortPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    ULONG uMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    if (GetFileAttributesW(lpszShortPath) == INVALID_FILE_ATTRIBUTES)
    {
        SetErrorMode(uMode);
        return 0;
    }

    LPWSTR lpFileName = NULL;
    PWCHAR Path = SkipPathTypeIndicator_U((LPWSTR)lpszShortPath);

    PWCHAR First, Last;
    if (!Path || (!*Path) || !(FindLFNorSFN_U(Path, &First, &Last, FALSE)))
    {
        ReturnLength = wcslen(lpszShortPath);
        if ((cchBuffer > ReturnLength) && lpszLongPath)
        {
            if (lpszLongPath != lpszShortPath)
                memmove(lpszLongPath, lpszShortPath, (ReturnLength + 1) * sizeof(wchar_t));
        }
        else
            ReturnLength++;
        goto cleanup;
    }

    SIZE_T Length = (wcslen(lpszShortPath) + 1) * sizeof(wchar_t);
    lpFileName = RtlAllocateHeap(GetProcessHeap(), 0, Length);
    if (!lpFileName)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    memmove(lpFileName, lpszShortPath, Length);

    First = &lpFileName[First - lpszShortPath];
    Last = &lpFileName[Last - lpszShortPath];

    if (cchBuffer && lpszLongPath &&
        (((lpszLongPath >= lpszShortPath) && (lpszLongPath < &lpszShortPath[Length / sizeof(wchar_t)])) ||
         ((lpszLongPath < lpszShortPath) && (&lpszLongPath[cchBuffer] >= lpszShortPath))))
    {
        if (!(Buffer = RtlAllocateHeap(GetProcessHeap(), 0, cchBuffer * sizeof(wchar_t))))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        Dst = Buffer;
    }
    else
        Dst = lpszLongPath;

    Src = lpFileName;
    ReturnLength = 0;
    PWCHAR Current = Last;

    while (TRUE)
    {
        Length = First - Src;
        ReturnLength += Length;

        if (Length && (cchBuffer > ReturnLength) && lpszLongPath)
        {
            memmove(Dst, Src, Length * sizeof(wchar_t));
            Dst += Length;
        }

        LastChar = *Current;
        *Current = L'\0';

        WIN32_FIND_DATAW FindFileData;
        HANDLE FindHandle = FindFirstFileW(lpFileName, &FindFileData);
        *Current = LastChar;

        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            ReturnLength = 0;
            break;
        }

        FindClose(FindHandle);
        Length = wcslen(FindFileData.cFileName);
        if (Length)
            First = FindFileData.cFileName;
        else
            Length = Last - First;

        ReturnLength += Length;

        if ((cchBuffer > ReturnLength) && lpszLongPath)
        {
            memmove(Dst, First, Length * sizeof(wchar_t));
            Dst += Length;
        }

        Src = Current;

        if (!*Current || !FindLFNorSFN_U(Current, &First, &Last, FALSE))
            break;

        Current = Last;
    }

    if (ReturnLength)
    {
        Length = wcslen(Src);
        ReturnLength += Length;

        if ((cchBuffer > ReturnLength) && lpszLongPath)
        {
            memmove(Dst, Src, (Length + 1) * sizeof(wchar_t));

            if (Buffer)
                memmove(lpszLongPath, Buffer, (ReturnLength + 1) * sizeof(wchar_t));
        }
        else
            ReturnLength++;
    }

cleanup:
    if (lpFileName)
        RtlFreeHeap(GetProcessHeap(), 0, lpFileName);
    if (Buffer)
        RtlFreeHeap(GetProcessHeap(), 0, Buffer);

    SetErrorMode(uMode);
    return (DWORD)ReturnLength;
}

/*
 * Clamav Native Windows Port: dummy functions
 *
 * Copyright (c) 2005-2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef _UNICODE
#undef UNICODE
#undef _UNICODE
#include <w32_stat.c>
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <io.h>

#include "posix-errno.h"

int w32_stat(const char* path, struct stat* buf)
{
#if 1
    return stat(path, buf);
#else
    char path2[MAX_PATH];
    size_t len = strlen(path);
    strncpy(path2, path, MAX_PATH - len);
    path2[MAX_PATH - len] = 0;

    for (char* p = path2 + len - 1; (*p == '/') || (*p == '\\'); p--)
        *p = 0;

    WIN32_FIND_DATAA fdata;
    HANDLE hFile = FindFirstFileA(path2, &fdata);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        cw_leerrno();
        return -1;
    }

    FindClose(hFile);

    memset(buf, 0, sizeof(struct stat));
    buf->st_mode = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _S_IFDIR : _S_IFREG;
    buf->st_size = fdata.nFileSizeLow;
    return 0;
#endif
}

int safe_open(const char* path, int flags, ...)
{
    int mode;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    return _open(path, flags, mode);
}

wchar_t *uncpath(const char *path)
{
    fprintf(stderr, "uncpath should never be called\n");
    abort();
}
#endif // _UNICODE

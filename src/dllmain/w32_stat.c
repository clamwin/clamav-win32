/*
 * Clamav Native Windows Port: w32_stat replacement
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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <io.h>

#include "posix-errno.h"

int w32_stat(const char* path, struct stat* buf)
{
    if ((strlen(path) == 2) && (path[1] == ':'))
    {
        char szDrive[] = "C:\\";
        szDrive[1] = path[1];
        return stat(szDrive, buf);
    }

    return stat(path, buf);
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

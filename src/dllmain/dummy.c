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

#include <windows.h>

#if _WIN32_WINNT < _WIN32_WINNT_WINXP
#include <stdio.h>
#include <stdlib.h>

int w32_stat(const char *path, void *buf)
{
    fprintf(stderr, "w32_stat should never be called\n");
    abort();
}

int safe_open(const char *path, int flags, ...)
{
    fprintf(stderr, "safe_open should never be called\n");
    abort();
}

wchar_t *uncpath(const char *path)
{
    fprintf(stderr, "uncpath should never be called\n");
    abort();
}
#endif

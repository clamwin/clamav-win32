/*
 * Clamav Native Windows Port: platform specific helpers
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

#include "platform.h"
#include "osdeps.h"

#include "clamav.h"
#include "others.h"

int cw_unlink(const char *pathname)
{
    LPCWSTR fname = mb2wc(pathname);

    if (!fname)
        return 1;

    DWORD dwAttrs = GetFileAttributesW(fname);
    SetFileAttributesW(fname, dwAttrs & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN));

    if (DeleteFileW(fname))
        return 0;

    cli_errmsg("%s cannot be deleted, scheduling for deletetion at next reboot\n", pathname);

    if (!MoveFileExW(fname, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
        cli_errmsg("%s cannot be scheduling for deletetion at next reboot\n", pathname);
        return 1;
    }

    return 0;
}

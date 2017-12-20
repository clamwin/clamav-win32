/*
 * Clamav Native Windows Port: minimal termios emulation
 *
 * Copyright (c) 2008 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <platform.h>
#include <termios.h>
#include <assert.h>
#include <posix-errno.h>

static BOOL isvalidtty(HANDLE h)
{
    assert(h != INVALID_HANDLE_VALUE);
    assert(GetFileType(h) == FILE_TYPE_CHAR);
    if ((h != INVALID_HANDLE_VALUE) && (GetFileType(h) == FILE_TYPE_CHAR))
        return TRUE;
    errno = EBADF;
    return FALSE;
}

int tcgetattr(int fd, struct termios *termios_p)
{
    HANDLE h = GetStdHandle((DWORD)(fd - 10));
    if (!isvalidtty(h)) return -1;
    if (GetConsoleMode(h, &termios_p->c_lflag)) return 0;
    errno = EBADF;
    return -1;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
    HANDLE h = GetStdHandle((DWORD)(fd - 10));
    if (!isvalidtty(h)) return -1;
    if (SetConsoleMode(h, termios_p->c_lflag)) return 0;
    errno = EBADF;
    return 0;
}

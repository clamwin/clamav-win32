/*
 * Clamav Native Windows Port: minimal termios emulation
 *
 * Copyright (c) 2008-2025 Gianluigi Tiesi <sherpya@gmail.com>
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
#include "termios.h"

#include <errno.h>

int tcgetattr(int fd, struct termios *termios_p)
{
    HANDLE hStdin;
    if ((fd != 0) || ((hStdin = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE))
    {
        _set_errno(EBADF);
        return -1;
    }

    if (GetConsoleMode(hStdin, &termios_p->c_lflag))
        return 0;

    _set_errno(EBADF);
    return -1;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p)
{
    HANDLE hStdin;
    if ((fd != 0) || ((hStdin = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE))
    {
        _set_errno(EBADF);
        return -1;
    }

    if (SetConsoleMode(hStdin, termios_p->c_lflag))
        return 0;

    _set_errno(EBADF);
    return -1;
}

/*
 * Clamav Native Windows Port: fcntl implementation
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

#include "fcntl.h"
#include "posix-errno.h"

int fcntl(int fd, int cmd, ...)
{
    va_list ap;
    va_start(ap, cmd);

    if (cmd == F_GETFL)
        return 0;
    if (cmd == F_SETFL)
    {
        u_long arg = va_arg(ap, long) == O_NONBLOCK;
        if (ioctlsocket((SOCKET)fd, FIONBIO, &arg))
        {
            cw_wseterrno();
            return -1;
        }
        return 0;
    }
    return -1;
}

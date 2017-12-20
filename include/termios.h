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

#ifndef _TERMIOS_H_
#define _TERMIOS_H_

#define ECHO (~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT))
#define TCSAFLUSH 2

typedef DWORD tcflag_t;

struct termios {
    tcflag_t c_lflag;
};

extern int tcgetattr(int fd, struct termios *termios_p);
extern int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);

#endif /* _TERMIOS_H_ */

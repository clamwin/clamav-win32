/*
 * Clamav Native Windows Port: unistd.h for msvc
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

#ifndef _UNISTD_H
#define _UNISTD_H

#ifndef __GNUC__
#define R_OK    4   /* Test for read permission */
#define W_OK    2   /* Test for write permission */
#define X_OK    0   /* Test for execute permission - 1 bombs msvcrt */
#define F_OK    0   /* Test for existence */
#endif

#ifndef __MINGW32__
#define S_IWUSR     S_IWRITE
#define S_IRUSR     S_IREAD
#define S_IRWXU     S_IREAD | S_IWRITE

#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

#define S_ISLNK(x)  (0)
#define sleep(x)    Sleep(x * 1000)

#include <sys/types.h>
extern int ftruncate(int fd, off_t length);

#endif /* _UNISTD_H */

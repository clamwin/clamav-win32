/*
 * Clamav Native Windows Port: mallinfo for win32
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

#ifndef _MALLINFO_H
#define _MALLINFO_H

#include <stdint.h>

struct mallinfo
{
    size_t arena;    /* Total size of memory allocated with sbrk/mmap */
    int ordblks;     /* Number of free chunks */
    int smblks;      /* Number of fastbin blocks */
    int hblks;       /* Number of mmap blocks */
    size_t hblkhd;   /* Space in mmap blocks */
    size_t usmblks;  /* Maximum total allocated space */
    size_t fsmblks;  /* Space in fastbin blocks */
    size_t uordblks; /* Total allocated space */
    size_t fordblks; /* Total free space */
    size_t keepcost; /* Top-most, releasable space */
};

struct mallinfo mallinfo(void);

#endif /* _MALLINFO_H */

#ifdef __GNUC__
#include_next <malloc.h>
#endif

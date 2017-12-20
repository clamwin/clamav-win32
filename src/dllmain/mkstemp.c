/* Copyright (C) 1991, 1992, 1996, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this software; if not, write to the
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */


#include <platform.h>
#include <process.h>

/* Generate a unique temporary file name from TEMPLATE.
   The last six characters of TEMPLATE must be "XXXXXX";
   they are replaced with a string that makes the filename unique.
   Returns a file descriptor open on the file for reading and writing.  */
int mkstemp(char *tmpl)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t len;
    char *xs;
    static uint64_t value;
    unsigned int count;
    int fd = -1;

    /* A lower bound on the number of temporary files to attempt to
       generate.  The maximum total number of temporary file names that
       can exist for a given template is 62**6.  It should never be
       necessary to try all these combinations.  Instead if a reasonable
       number of names is tried (we define reasonable as 62**3) fail to
       give the system administrator the chance to remove the problems.  */
    unsigned int attempts_min = 62 * 62 * 62;

    /* The number of times to attempt to generate a temporary file.  To
       conform to POSIX, this must be no smaller than TMP_MAX.  */
    unsigned int attempts = attempts_min < TMP_MAX ? TMP_MAX : attempts_min;

    len = strlen(tmpl);
    if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX"))
    {
        errno = EINVAL;
        return -1;
    }

    xs = &tmpl[len - 6];
    value += time(NULL) ^ _getpid();

    for (count = 0; count < attempts; value += 7777, ++count)
    {
        uint64_t v = value;

        /* Fill in the random bits.  */
        xs[0] = letters[v % 62];
        v /= 62;
        xs[1] = letters[v % 62];
        v /= 62;
        xs[2] = letters[v % 62];
        v /= 62;
        xs[3] = letters[v % 62];
        v /= 62;
        xs[4] = letters[v % 62];
        v /= 62;
        xs[5] = letters[v % 62];

        fd = _open(tmpl, _O_RDWR|_O_BINARY|_O_CREAT|_O_EXCL|_O_SHORT_LIVED, 0600);
        if (fd >= 0) return fd;
    }

    /* We return the null string if we can't find a unique file name.  */
    tmpl[0] = '\0';
    return -1;
}

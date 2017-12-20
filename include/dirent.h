/*
 * Clamav Native Windows Port: dirent win32 compatibility layer
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 * Wrapped Unicode version by Alex Cherney <alch@sourceforge.net>
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

#ifndef _DIRENT_H
#define _DIRENT_H

struct dirent {
    char *d_name;
    off_t d_ino;
};

typedef struct dir_struct {
    HANDLE hdir;
    DWORD nfiles;
    int init;
    char *pattern;
    struct dirent dent;
} DIR;

DIR *opendir(const char *dirname);
int closedir(DIR *d);
struct dirent *readdir_w(DIR *d);
struct dirent *readdir_a(DIR *d);
struct dirent *readdir(DIR *d);
void rewinddir(DIR *d);
int telldir(DIR *d);
void seekdir(DIR *d, size_t pos);

#endif /* _DIRENT_H */

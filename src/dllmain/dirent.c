/*
 *  Copyright (C) 2013-2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *  Copyright (C) 2009-2013 Sourcefire, Inc.
 *
 *  Authors: aCaB <acab@clamav.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#include <unistd.h>

#include "platform.h"
#include "dirent.h"

DIR *opendir(const char *name)
{
    DIR *d;
    DWORD attrs;
    int len;
    struct stat sb;

    if (w32_stat(name, &sb) < 0)
        return NULL;

    if (!S_ISDIR(sb.st_mode)) {
        errno = ENOTDIR;
        return NULL;
    }
    if (!(d = malloc(sizeof(*d)))) {
        errno = ENOMEM;
        return NULL;
    }
#ifdef UNICODE
    wchar_t *wpath = uncpath(name);
    if (!wpath)
        return NULL;
        wcsncpy(d->entry, wpath, MAX_PATH - 1);
        free(wpath);
#else
    strncpy(d->entry, name, MAX_PATH - 1);
#endif
    d->entry[MAX_PATH - 1] = 0;
    len = _tcsclen(d->entry);

    if (len >= MAX_PATH - 5) {
        free(d);
        errno = ENAMETOOLONG;
        return NULL;
    }
    while (len--) {
        if (d->entry[len] == TEXT('\\'))
            d->entry[len] = 0;
        else
            break;
    }

    _tcsnccat(d->entry, TEXT("\\*.*"), 4);
    d->dh = INVALID_HANDLE_VALUE;
    return d;
}

struct dirent *readdir(DIR *dirp)
{
    while (1) {
        if (dirp->dh == INVALID_HANDLE_VALUE) {
            if ((dirp->dh = FindFirstFile(dirp->entry, &dirp->wfd)) == INVALID_HANDLE_VALUE) {
                errno = ENOENT;
                return NULL;
            }
        } else {
            if (!(FindNextFile(dirp->dh, &dirp->wfd))) {
                errno = (GetLastError() == ERROR_NO_MORE_FILES) ? 0 : ENOENT;
                return NULL;
            }
        }
#ifdef UNICODE
        if (!WideCharToMultiByte(CP_UTF8, 0, dirp->wfd.cFileName, -1, dirp->ent.d_name, MAX_PATH - 1, NULL, NULL))
            continue; /* FIXME: WARN HERE ! */
#else
        dirp->ent.d_name[0] = 0;
        strncpy(dirp->ent.d_name, dirp->wfd.cFileName, MAX_PATH -1);
#endif
        dirp->ent.d_ino = dirp->wfd.ftCreationTime.dwLowDateTime ^ dirp->wfd.nFileSizeLow;
        if (!dirp->ent.d_ino) dirp->ent.d_ino = 0x1337;
        dirp->ent.d_type = (dirp->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
        break;
    }
    return &dirp->ent;
}

void rewinddir(DIR *dirp)
{
    if (dirp->dh != INVALID_HANDLE_VALUE)
        FindClose(dirp->dh);
    dirp->dh = INVALID_HANDLE_VALUE;
}

int closedir(DIR *dirp)
{
    rewinddir(dirp);
    free(dirp);
    return 0;
}

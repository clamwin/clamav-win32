/*
 * Clamav Native Windows Port: others.c hook
 * Copyright (c) 2009 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <osdeps.h>
#include <dirent.h>
#include <libclamav/others.h>
#include <shared/output.h>

int cw_unlink(const char *pathname)
{
    FIXATTRS(pathname);
    if (!DeleteFileA(pathname))
    {
        if (!ISLOCKED(GetLastError())) return 1;
        cli_warnmsg("%s cannot be deleted, scheduling for deletetion at next reboot\n", pathname);
        cw_movefile(pathname, NULL, 1);
    }
    return 0;
}

/* Recursively remove a directory, portable version */
int cw_rmdirs(const char *dirname)
{
    DIR *dd = NULL;
    struct dirent *dent = NULL;
    struct stat maind, statbuf;
    char *path = NULL;

    _chmod(dirname, 0700);
    if (stat(dirname, &maind) == -1) return 0; /* Doesn't exists */

    if (!_rmdir(dirname)) return 0; /* ok - deleted */

    if ((errno != ENOTEMPTY) && !isWin9x())
    {
        cli_errmsg("Can't remove temporary directory %s: %s\n", dirname, strerror(errno));
        return -1;
    }

    if ((dd = opendir(dirname)) == NULL) return -1;

    while ((dent = readdir(dd)))
    {
        size_t len = 0;
        /* Skip . and .. */
        if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;

        len = strlen(dirname) + strlen(dent->d_name) + 3;
        CW_CHECKALLOC(path, malloc(len), break);
        snprintf(path, len - 1, "%s/%s", dirname, dent->d_name);
        path[len - 1] = 0;

        /* stat the entry */
        if (lstat(path, &statbuf) == -1)
        {
            /* Possible causes:
               - Directory has only read permission but not execute,
               - The file is share-locked (win32_stat uses CreateFile())
               - Sherpya b0rked the dirent or other code
             */
            free(path);
            continue;
        }

        if (S_ISDIR(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode))
        {
            /* I've found a directory */
            _chmod(dirname, 0700); /* Fix permissions */
            if (_rmdir(path))
            {
                if ((errno == ENOTEMPTY) || isWin9x())
                    cw_rmdirs(path); /* If it's not empty then recurse */
                else
                {
                    /* Cannot be deleted */
                    cli_errmsg("[inner] Can't remove temporary directory %s: %s\n", path, strerror(errno));
                    closedir(dd);
                    free(path);
                    return 0;
                }
            }
        }
        else /* It's a regular file */
            if (cw_unlink(path)) cli_errmsg("Can't unlink temporary file %s: %s\n", path, strerror(errno));
        free(path);
    }

    closedir(dd);

    /* Finally try to remove the directory itself */
    if (_rmdir(dirname))
        cli_errmsg("[outer] Can't remove temporary directory %s: %s\n", dirname, strerror(errno));

    return 0;
}

#undef cli_rmdirs
/* note cli_dumpscan uses cli_unlink internally,
   but the file is created by libclamav */
#undef cli_unlink

cl_error_t cw_postscan_check(int fd, int result, const char *virname, void *context);
#include <libclamav/others.c>

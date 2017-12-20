/*
 * Clamav Native Windows Port: dirent for win32
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
 * Wrapped Unicode/Ansi by Alex Cherney <alch@sourceforge.net>
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
#include <assert.h>
#include <posix-errno.h>
#include <others.h>

#define dirent_perror(x) do { if (cw_leerrno()) cw_perror(x); return NULL; } while (0)

#define DIRENT_VALIDATE(e, err, ret) { if (!(e)) { errno = err; return ret; } }
#define DIRENT_ASSERT(e, err, ret) { assert(e); DIRENT_VALIDATE(e, err, ret); }
#define DIRENT_INIT_DIR(d) \
{\
    d->hdir = INVALID_HANDLE_VALUE; \
    d->nfiles = 0; \
    d->init = 0; \
    d->pattern = NULL; \
    d->dent.d_name = NULL; \
    d->dent.d_ino = -1; \
}

DIR *opendir(const char *dirname)
{
    struct stat sb;
    size_t len;
    DIR *d = NULL;

    DIRENT_ASSERT(dirname, EFAULT, NULL);
    DIRENT_VALIDATE((stat(dirname, &sb) != -1), ENOENT, NULL);
    DIRENT_VALIDATE((sb.st_mode & S_IFMT) == S_IFDIR, ENOTDIR, NULL);
    DIRENT_ASSERT((d = malloc(sizeof(DIR))), ENOMEM, NULL);

    DIRENT_INIT_DIR(d);

    len = strlen(dirname);
    DIRENT_ASSERT((d->pattern = malloc(len + 3)), ENOMEM, NULL);

    strcpy(d->pattern, dirname);
    cw_rmtrailslashes(d->pattern);
    strcat(d->pattern, "\\*");

    return d;
}

int closedir(DIR *d)
{
    DIRENT_VALIDATE(d, EFAULT, -11);
    if (d->pattern) free(d->pattern);
    if (d->init)
    {
        DIRENT_ASSERT((d->hdir != INVALID_HANDLE_VALUE), EBADF, -1);
        FindClose(d->hdir);
    }
    free(d);
    return 0;
}

/*
 * Unicode version of  readdir to be used on NT platforms
 * deals with unicode filenames by using 8.3 name if translation
 * from unicode to ansi fails
 */

struct dirent *readdir_w(DIR *d)
{
    WIN32_FIND_DATAW wfdw;
    wchar_t *name_w = NULL;
    char *name_a = NULL;

    DIRENT_VALIDATE(d, EFAULT, NULL);

    if (d->nfiles == 0)
    {
        if (!(name_w = cw_mb2wc(d->pattern)))
        {
            cli_warnmsg("[dirent] mb2wc(\"%s\") failed %d\n", d->pattern, GetLastError());
            errno = EINVAL;
            return NULL;
        }

        if ((d->hdir = FindFirstFileW(name_w, &wfdw)) == INVALID_HANDLE_VALUE)
        {
            free(name_w);
            dirent_perror(d->pattern);
        }

        d->init = 1;
    }
    else if (!FindNextFileW(d->hdir, &wfdw))
        dirent_perror("FindNextFileW()");

    if (name_w) free(name_w);

    /* avoid messing with multi byte and wide chars if . or .. */
    if (!wcscmp(wfdw.cFileName, L"."))
        name_a = _strdup(".");
    else if (!wcscmp(wfdw.cFileName, L".."))
        name_a = _strdup("..");
    else if (!(name_a = cw_wc2mb(wfdw.cFileName, WC_NO_BEST_FIT_CHARS)))
    {
        if (!wfdw.cAlternateFileName[0])
        {
            fwprintf(stderr, L"[dirent] alternative name path not found (ntfs 8dot3 disabled?) (err=%d)\n", GetLastError());
            errno = EINVAL; return NULL;
        }

        if (!(name_a = cw_wc2mb(wfdw.cAlternateFileName, 0)))
        {
            fwprintf(stderr, L"[dirent] alternative name path conversion failed (err=%d)\n", GetLastError());
            errno = EINVAL; return NULL;
        }
    }

    DIRENT_VALIDATE(name_a, EFAULT, NULL);
    d->nfiles++;

    if (d->dent.d_name) free(d->dent.d_name);
    d->dent.d_name = name_a;
    return (&d->dent);
}

/*
 * Ansi version of readdir to be used on 9x platforms
 * Windows9x doesn't allow creation of a filename in codepage different to
 * the default, therefore we don't have to mess up with unicows.dll
 * and can use the ansi functions
 */

struct dirent *readdir_a(DIR *d)
{
    WIN32_FIND_DATAA wfda;
    DIRENT_VALIDATE(d, EFAULT, NULL);

    if (d->nfiles == 0)
    {
        if ((d->hdir = FindFirstFileA(d->pattern, &wfda)) == INVALID_HANDLE_VALUE)
            dirent_perror(d->pattern);
        d->init = 1;
    }
    else if (!FindNextFileA(d->hdir, &wfda))
        dirent_perror("FindNextFileA()");

    d->nfiles++;

    if (d->dent.d_name) free(d->dent.d_name);

    /* is '?' valid in the path? */
    d->dent.d_name = _strdup(strchr(wfda.cFileName, '?') ? wfda.cAlternateFileName : wfda.cFileName);
    return (&d->dent);
}

/*
 * load an appropriate version (unicode or ansi) of readdir
 * we need that to handle scanning of unicode filenames in libclamav
 * Windows9x doesn't allow creation of a filename in codepage different to
 * the default, therefore we don't have to mess up with unicows.dll
 */
struct dirent *readdir(DIR *d)
{
    DIRENT_ASSERT(d, EFAULT, NULL);
    return (isWin9x()) ? readdir_a(d) : readdir_w(d);
}

void rewinddir(DIR *d)
{
    DIRENT_ASSERT(d, EFAULT,;);

    if (d->dent.d_name) free(d->dent.d_name);
    d->dent.d_name = NULL;
    d->nfiles = 0;

    DIRENT_ASSERT((!d->init || (d->hdir != INVALID_HANDLE_VALUE)), EBADF,;);
    if (!FindClose(d->hdir)) errno = EBADF;
    d->hdir = INVALID_HANDLE_VALUE;
}

int telldir(DIR *d)
{
    DIRENT_ASSERT(d, EFAULT, -1);
    DIRENT_ASSERT((!d->init || (d->hdir != INVALID_HANDLE_VALUE)), EBADF, -1);
    return d->nfiles;
}

void seekdir(DIR *d, size_t pos)
{
    DIRENT_ASSERT(d, EFAULT,;);

    /* go back to beginning of directory */
    rewinddir(d);

    /* loop until we have found position we care about */
    for (--pos; pos && readdir(d); pos--);

    /* flag invalid nPosition value */
    DIRENT_VALIDATE(!pos, EINVAL,;);
    return;
}

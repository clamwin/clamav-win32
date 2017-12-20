/*
 * Clamav Native Windows Port
 *
 * Copyright (c) 2005-2011 Gianluigi Tiesi <sherpya@netfarm.it>
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

#ifndef _OSDEPS_H_
#define _OSDEPS_H_

#include <platform.h>

/* undefined in platform.h to avoid redefinition, but needed again for win32 specific stuff */
#ifndef OUT
#define OUT
#endif

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <cwhelpers.h>

#define DATADIRBASEKEY  "Software\\ClamAV"

LIBCLAMAV_API extern uint32_t cw_platform;
LIBCLAMAV_API extern helpers_t cw_helpers;

extern int cw_movefile(const char *source, const char *dest, int reboot);
extern int cw_movefileex(const char *source, const char *dest, DWORD flags);

#define PlatformId          ((cw_platform >> 16) & 0x000000ff)
#define PlatformMajor       ((cw_platform >> 8 ) & 0x000000ff)
#define PlatformMinor       (cw_platform & 0x000000ff)
#define PlatformVersion     (cw_platform & 0x0000ffff)
#define isWin9x()           (PlatformId == VER_PLATFORM_WIN32_WINDOWS)
#define isOldOS()           (PlatformVersion <= 0x400)

static inline const char *cw_uncprefix(const char *filename)
{
    if (PATH_ISUNC(filename) || PATH_ISNET(filename) || isWin9x())
        return "";
    else
        return UNC_PREFIX;
}

#define ISLOCKED(error) \
    ((error == ERROR_ACCESS_DENIED) || (error == ERROR_SHARING_VIOLATION) || (error == ERROR_LOCK_VIOLATION))

#define FIXATTRS(filename) \
{ \
    DWORD dwAttrs = GetFileAttributes(filename); \
    SetFileAttributes(filename, dwAttrs & ~ (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN)); \
}

extern BOOL cw_iswow64(void);

static inline wchar_t *cw_mb2wc(const char *mb)
{
    wchar_t *wc = NULL;
    DWORD len = 0;

    if (!(len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mb, -1, NULL, 0)))
        return NULL;

    CW_CHECKALLOC(wc, malloc(len * sizeof(wchar_t)), return NULL);

    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mb, -1, wc, len))
        return wc;

    free(wc);
    return NULL;
}

#ifndef WC_NO_BEST_FIT_CHARS
#define WC_NO_BEST_FIT_CHARS 1024
#endif

static inline char *cw_wc2mb(const wchar_t *wc, DWORD flags)
{
    BOOL invalid = FALSE;
    DWORD len = 0, res = 0;
    char *mb = NULL;

    /* NT4 does not like WC_NO_BEST_FIT_CHARS */
    if (isOldOS()) flags &= ~WC_NO_BEST_FIT_CHARS;

    len = WideCharToMultiByte(CP_ACP, flags, wc, -1, NULL, 0, NULL, &invalid);
    if (!len && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        fprintf(stderr, "WideCharToMultiByte() failed with %d\n", GetLastError());
        return NULL;
    }

    CW_CHECKALLOC(mb, malloc(len), return NULL);

    res = WideCharToMultiByte(CP_ACP, flags, wc, -1, mb, len, NULL, &invalid);
    if (res && ((!invalid || (flags != WC_NO_BEST_FIT_CHARS)))) return mb;
    free(mb);
    return NULL;
}

static inline char *cw_getfullpathname(const char *path)
{
    char *fp = NULL;
    DWORD len = GetFullPathNameA(path, 0, NULL, NULL);
    if (!len) return NULL;

    CW_CHECKALLOC(fp, malloc(len + 1), return NULL);

    if (GetFullPathNameA(path, len, fp, NULL))
        return fp;
    free(fp);
    return NULL;
}

static inline char *cw_getcurrentdir(void)
{
    DWORD len = GetCurrentDirectoryA(0, NULL);
    char *cwd = NULL;
    if (!len) return NULL;
    len++;

    CW_CHECKALLOC(cwd, malloc(len), return NULL);

    len = GetCurrentDirectoryA(len - 1, cwd);
    if (len) return cwd;
    free(cwd);
    return NULL;
}

static inline void cw_pathtowin32(char *name)
{
    /* UNC Paths need to have only backslashes */
    char *p = name;
    while (*p)
    {
        if (*p == '/') *p = '\\';
        p++;
    }
}

static inline void cw_rmtrailslashes(char *path)
{
    size_t i = strlen(path) - 1;
    while ((i > 0) && ((path[i] == '/') || (path[i] == '\\')))
        path[i--] = 0;
}

static volatile const char portrev_rodata[] =
{
    0x43, 0x6c, 0x61, 0x6d, 0x57, 0x69, 0x6e, 0x20,
    0x41, 0x6e, 0x74, 0x69, 0x76, 0x69, 0x72, 0x75,
    0x73, 0x7c, 0x47, 0x50, 0x4c, 0x76, 0x32, 0x7c,
    0x28, 0x63, 0x29, 0x20, 0x32, 0x30, 0x31, 0x31,
    0x20, 0x43, 0x6c, 0x61, 0x6d, 0x57, 0x69, 0x6e,
    0x20, 0x50, 0x74, 0x79, 0x20, 0x4c, 0x74, 0x64,
    0x2f, 0x53, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x66,
    0x69, 0x72, 0x65, 0x20, 0x49, 0x6e, 0x63, 0x2e,
    0x7c, 0x41, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x3a,
    0x20, 0x47, 0x69, 0x61, 0x6e, 0x6c, 0x75, 0x69,
    0x67, 0x69, 0x20, 0x54, 0x69, 0x65, 0x73, 0x69,
    0x7c, 0x3c, 0x73, 0x68, 0x65, 0x72, 0x70, 0x79,
    0x61, 0x40, 0x6e, 0x65, 0x74, 0x66, 0x61, 0x72,
    0x6d, 0x2e, 0x69, 0x74, 0x3e
};

#endif /* _OSDEPS_H_ */

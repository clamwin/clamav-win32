/*
 * Clamav Native Windows Port: platform specific helpers
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

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef _WIN32_WINNT
#error "Please make sure _WIN32_WINNT is defined"
#endif

#include "cwdefs.h"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> /* ipv6 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <process.h> /* _getpid() */
#include <malloc.h>  /* _alloca() */
#include <stdint.h>

#ifdef _MSC_VER
#include <direct.h> /* _mkdir()  */
#endif

#include <io.h>

#ifdef PATH_MAX
#undef PATH_MAX
#endif

#ifdef MAX_PATH
#undef MAX_PATH
#endif

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

typedef struct pollfd
{
  SOCKET fd;
  short events;
  short revents;
} WSAPOLLFD, *PWSAPOLLFD, *LPWSAPOLLFD;
#endif

#if _WIN32_WINNT <= _WIN32_WINNT_WINXP
#ifndef _WIN64
#ifdef _MSC_VER
#include <ws2tcpip.h>
#else
#include <wspiapi.h>
#endif

#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif
#endif

#define MAX_PATH 260
#else
#define MAX_PATH 4096
#define stat(path, buf) w32_stat(path, buf)
#endif

extern int w32_stat(const char *path, struct stat *buf);
extern int safe_open(const char* path, int flags, ...);
extern wchar_t *uncpath(const char *path);

#define lstat stat
#define PATH_MAX MAX_PATH

#include "posix-errno.h"
#include "cw_inline.h"
#include "socket_inline.h"

/* resolv.h */
int res_init(void);
int res_query(const char *dname, int klass, int type, unsigned char *answer, int anslen);
int dn_expand(unsigned char *msg, unsigned char *eomorig, unsigned char *comp_dn, char *exp_dn, int length);

#define WORDS_BIGENDIAN 0
#define EAI_SYSTEM 0

#define difftime(time_end, time_beg) ((double)(time_end - time_beg))

/* <strings.h> */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#define ftruncate _chsize
extern char *strptime(const char *buf, const char *format, struct tm *tm);

/* errno remap */
#define strerror cw_strerror
#define perror cw_perror

extern int w32_rand(void);
extern void w32_srand(unsigned int seed);

/* avoid redefining std::rand */
#ifndef __cplusplus
#define srand w32_srand
#define rand w32_rand
#endif

#define mkdir(a, b) mkdir(a)

#if defined(_MSC_VER)
#define fseeko _fseeki64
#elif defined(__GNUC__)
#define fseeko fseeko64
extern int __cdecl fseeko64(FILE *stream, off64_t offset, int whence);
#else
#undef HAVE_FSEEKO
#endif

/* tmpfile() on win32 uses root dir, not suitable if non-admin */
#define tmpfile do_not_use_tmpfile_on_win32

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef unsigned short in_port_t;
typedef unsigned int in_addr_t;

/* <arpa/inet.h> */
extern const char *w32_inet_ntop(int af, const void *src, char *dst, socklen_t size);
#define inet_ntop w32_inet_ntop

#undef IMAGE_DOS_SIGNATURE

#define PATHSEP "\\"

#if defined(THIS_IS_LIBCLAMAV)
#define LIBCLAMAV_EXPORT __declspec(dllexport)
#else
#define LIBCLAMAV_EXPORT __declspec(dllimport)
#endif

LIBCLAMAV_EXPORT extern BOOL disablefsredir(void);

extern const char *cli_to_utf8_maybe_alloc(const char *s);
extern char *cli_strdup_to_utf8(const char *s);

/* mallinfo */
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

#endif /* _PLATFORM_H */

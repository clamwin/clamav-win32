/*
 * Clamav Native Windows Port: platform specific helpers
 *
 * Copyright (c) 2005-2010 Gianluigi Tiesi <sherpya@netfarm.it>
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

/* IsDebuggerPresent 0x0500 */
/* RegisterWaitForSingleObject 0x0500 */
/* UnregisterWaitEx 0x0500 */
/* HeapCompatibilityInformation 0x0501 */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "cwdefs.h"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> /* ipv6 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
//#include <direct.h>  /* _mkdir()  */
#include <process.h> /* _getpid() */
#include <malloc.h>  /* _alloca() */

#include "posix-errno.h"
#include "safe_ctype.h"
#include "cw_inline.h"
#include "socket_inline.h"

/* re-route main to cw_main to handle some startup code */
#ifndef CLAMWIN_MAIN_HANDLED
#define main cw_main
#endif

#ifndef PATH_MAX
#define PATH_MAX 260 // 32767
#endif
#define WORDS_BIGENDIAN 0
#define EAI_SYSTEM 0

WINBASEAPI DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

#undef strtok_r /* thanks to pthread.h */

/* <strings.h> */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

/* cw */
extern int cw_init(void);

/* ctrl + c handler */
extern BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType);

/* gnulib entries */
#ifndef __cplusplus
extern char *strtok_r(char *s, const char *delim, char **save_ptr);
extern char *strptime (const char *buf, const char *format, struct tm *tm);
#if defined(_MSC_VER) && (_MSC_VER <= 1400)
extern long long int strtoll(const char *nptr, char **endptr, int base);
#endif
#endif

#define lstat stat
#define stat(path, buf) w32_stat(path, buf)
extern int w32_stat(const char* path, struct stat* buf);

/* errno remap */
#define strerror cw_strerror
#define perror cw_perror

/* random */
extern int cw_rand(void);
extern void cw_srand(unsigned int seed);

/* avoid redefining std::rand */
#ifndef __cplusplus
#define rand cw_rand
#define srand cw_srand
#endif

#define mkdir(a, b) mkdir(a)

/* <stdio.h> / <stdarg.h> */
/* Use snprintf and vsnprintf from gnulib, win32 crt has broken a snprintf */
#undef snprintf
#undef vsnprintf
#define snprintf gnulib_snprintf
#define vsnprintf gnulib_vsnprintf
extern int gnulib_snprintf(char *str, size_t size, const char *format, ...);
extern int gnulib_vsnprintf(char *str, size_t size, const char *format, va_list args);

#if defined(_MSC_VER)
#define fseeko _fseeki64
#elif defined(__GNUC__)
#define fseeko fseeko64
extern int __cdecl fseeko64 (FILE* stream, off64_t offset, int whence);
#else
#undef HAVE_FSEEKO
#endif

/* tmpfile() on win32 uses root dir, not suitable if non-admin */
#define tmpfile do_not_use_tmpfile_on_win32

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
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

/* win32 headers have DATADIR enum */
#ifndef __cplusplus
LIBCLAMAV_EXPORT extern const char* DATADIR;
LIBCLAMAV_EXPORT extern const char* CONFDIR;
LIBCLAMAV_EXPORT extern const char* CONFDIR_CLAMD;
LIBCLAMAV_EXPORT extern const char* CONFDIR_FRESHCLAM;
LIBCLAMAV_EXPORT extern const char* CONFDIR_MILTER;
#endif

LIBCLAMAV_EXPORT extern BOOL disablefsredir(void);
LIBCLAMAV_EXPORT extern BOOL revertfsredir(void);

extern const char* cli_to_utf8_maybe_alloc(const char* s);
extern char* cli_strdup_to_utf8(const char* s);

#endif /* _PLATFORM_H */

/*
 * Clamav Native Windows Port: posix errno emulation
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

#ifndef _POSIX_ERRNO_H
#define _POSIX_ERRNO_H

#define MAX_CW_ERRNO        128

#if _MSC_VER >= 1600
#pragma warning(push)
#pragma warning(disable: 4005)  /* Macro redefinition */
#endif

/* winsock specific */
#ifndef ELOOP
#define ELOOP               44
#endif

#define EPROCLIM            45
#define EUSERS              46
#define EREMOTE             47
#define SYSNOTREADY         48
#define VERNOTSUPPORTED     49
#define NOTINITIALISED      50
#define EDISCON             51
#define ENOMORE             52
#define ECANCELLED          53
#define EINVALIDPROCTABLE   54
#define EINVALIDPROVIDER    55
#define EPROVIDERFAILEDINIT 56
#define SYSCALLFAILURE      57
#define SERVICE_NOT_FOUND   58
#define TYPE_NOT_FOUND      59
#define E_NO_MORE           60
#define E_CANCELLED         61
#define EREFUSED            62

/* h_errno, no remapping */
/*
#define HOST_NOT_FOUND      63
#define TRY_AGAIN           64
#define NO_RECOVERY         65
#define NO_DATA             66
*/

/* my own */
#define ENOTREADY           70

/* libdl */
#define ENOMOD              71 /* The specified module could not be found */
#define ENOPROC             72 /* The specified procedure could not be found */

/* #define STRUNCATE        80 */

#define OPERATION_ABORTED   81
#define IO_INCOMPLETE       82
#define IO_PENDING          83
#define INVALID_PARAMETER   87

/* linux */
#ifndef ENOTSOCK
#define ENOTSOCK            88  /* Socket operation on non-socket */
#endif

#ifndef EDESTADDRREQ
#define EDESTADDRREQ        89  /* Destination address required */
#endif

#ifndef EMSGSIZE
#define EMSGSIZE            90  /* Message too long */
#endif

#ifndef EPROTOTYPE
#define EPROTOTYPE          91  /* Protocol wrong type for socket */
#endif

#ifndef ENOPROTOOPT
#define ENOPROTOOPT         92  /* Protocol not available */
#endif

#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT     93  /* Protocol not supported */
#endif

#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT     94  /* Socket type not supported */
#endif

#ifndef EOPNOTSUPP
#define EOPNOTSUPP          95  /* Operation not supported on transport endpoint */
#endif

#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT        96  /* Protocol family not supported */
#endif

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT        97  /* Address family not supported by protocol */
#endif

#ifndef EADDRINUSE
#define EADDRINUSE          98  /* Address already in use */
#endif

#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL       99  /* Cannot assign requested address */
#endif

#ifndef ENETDOWN
#define ENETDOWN            100 /* Network is down */
#endif

#ifndef ENETUNREACH
#define ENETUNREACH         101 /* Network is unreachable */
#endif

#ifndef ENETRESET
#define ENETRESET           102 /* Network dropped connection because of reset */
#endif

#ifndef ECONNABORTED
#define ECONNABORTED        103 /* Software caused connection abort */
#endif

#ifndef ECONNRESET
#define ECONNRESET          104 /* Connection reset by peer */
#endif

#ifndef ENOBUFS
#define ENOBUFS             105 /* No buffer space available */
#endif

#ifndef EISCONN
#define EISCONN             106 /* Transport endpoint is already connected */
#endif

#ifndef ENOTCONN
#define ENOTCONN            107 /* Transport endpoint is not connected */
#endif

#ifndef ESHUTDOWN
#define ESHUTDOWN           108 /* Cannot send after transport endpoint shutdown */
#endif

#ifndef ETOOMANYREFS
#define ETOOMANYREFS        109 /* Too many references: cannot splice */
#endif

#define _ETIMEDOUT          110 /* Connection timed out */

#ifndef ECONNREFUSED
#define ECONNREFUSED        111 /* Connection refused */
#endif

#ifndef EHOSTDOWN
#define EHOSTDOWN           112 /* Host is down */
#endif

#ifndef EHOSTUNREACH
#define EHOSTUNREACH        113 /* No route to host */
#endif

#ifndef EALREADY
#define EALREADY            114 /* Operation already in progress */
#endif

#ifndef EINPROGRESS
#define EINPROGRESS         115 /* Operation now in progress */
#endif

#ifndef ESTALE
#define ESTALE              116 /* Stale NFS file handle */
#endif

#ifndef EUCLEAN
#define EUCLEAN             117 /* Structure needs cleaning */
#endif

#ifndef ENOTNAM
#define ENOTNAM             118 /* Not a XENIX named type file */
#endif

#ifndef ENAVAIL
#define ENAVAIL             119 /* No XENIX semaphores available */
#endif

#ifndef EISNAM
#define EISNAM              120 /* Is a named type file */
#endif

#ifndef EREMOTEIO
#define EREMOTEIO           121 /* Remote I/O error */
#endif

#ifndef EDQUOT
#define EDQUOT              122 /* Quota exceeded */
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK         EALREADY  /* Operation would block - should be EAGAIN */
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT           WSAETIMEDOUT /* needed by pthreads win32, it will use winsock value */
#endif

#if _MSC_VER >= 1600
#pragma warning(pop)
#endif

extern int cw_wseterrno(void);
extern const char *cw_strerror(int errnum);
extern void cw_perror(const char *msg);
extern int cw_leerrno(void);

#endif /* _POSIX_ERRNO_H */

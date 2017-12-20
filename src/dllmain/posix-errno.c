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

#include <platform.h>
#include <posix-errno.h>
#include <assert.h>
#include <others.h>

static const char *const cw_errlist[MAX_CW_ERRNO] =
{
    /* windows crt error messages */
    /*   0 NOERROR              */  "No error",
    /*   1 EPERM                */  "Operation not permitted",
    /*   2 ENOENT               */  "No such file or directory",
    /*   3 ESRCH                */  "No such process",
    /*   4 EINTR                */  "Interrupted function call",
    /*   5 EIO                  */  "Input/output error",
    /*   6 ENXIO                */  "No such device or address",
    /*   7 E2BIG                */  "Arg list too long",
    /*   8 ENOEXEC              */  "Exec format error",
    /*   9 EBADF                */  "Bad file descriptor",
    /*  10 ECHILD               */  "No child processes",
    /*  11 EAGAIN               */  "Resource temporarily unavailable",
    /*  12 ENOMEM               */  "Not enough space",
    /*  13 EACCES               */  "Permission denied",
    /*  14 EFAULT               */  "Bad address",
    /*  15 ENOTBLK              */  "Unknown error",
    /*  16 EBUSY                */  "Resource device",
    /*  17 EEXIST               */  "File exists",
    /*  18 EXDEV                */  "Improper link",
    /*  19 ENODEV               */  "No such device",
    /*  20 ENOTDIR              */  "Not a directory",
    /*  21 EISDIR               */  "Is a directory",
    /*  22 EINVAL               */  "Invalid argument",
    /*  23 ENFILE               */  "Too many open files in system",
    /*  24 EMFILE               */  "Too many open files",
    /*  25 ENOTTY               */  "Inappropriate I/O control operation",
    /*  26 ETXTBSY              */  "Unknown error",
    /*  27 EFBIG                */  "File too large",
    /*  28 ENOSPC               */  "No space left on device",
    /*  29 ESPIPE               */  "Invalid seek",
    /*  30 EROFS                */  "Read-only file system",
    /*  31 EMLINK               */  "Too many links",
    /*  32 EPIPE                */  "Broken pipe",
    /*  33 EDOM                 */  "Domain error",
    /*  34 ERANGE               */  "Result too large",
    /*  35 EUCLEAN              */  "Unknown error",
    /*  36 EDEADLK              */  "Resource deadlock avoided",
    /*  37 UNKNOWN              */  "Unknown error",
    /*  38 ENAMETOOLONG         */  "Filename too long",
    /*  39 ENOLCK               */  "No locks available",
    /*  40 ENOSYS               */  "Function not implemented",
    /*  41 ENOTEMPTY            */  "Directory not empty",
    /*  42 EILSEQ               */  "Illegal byte sequence",
    /*  43                      */  "Unknown error",

    /* winsock specific error messages */
    /*  44 ELOOP                */  "Cannot translate name",
    /*  45 EPROCLIM             */  "Too many processes",
    /*  46 EUSERS               */  "User quota exceeded",
    /*  47 EREMOTE              */  "Item is remote",
    /*  48 SYSNOTREADY          */  "Network subsystem is unavailable",
    /*  49 VERNOTSUPPORTED      */  "Winsock.dll version out of range",
    /*  50 NOTINITIALISED       */  "Successful WSAStartup not yet performed",
    /*  51 EDISCON              */  "Graceful shutdown in progress",
    /*  52 ENOMORE              */  "No more results",
    /*  53 ECANCELLED           */  "Call has been canceled",
    /*  54 EINVALIDPROCTABLE    */  "Procedure call table is invalid",
    /*  55 EINVALIDPROVIDER     */  "Service provider is invalid",
    /*  56 EPROVIDERFAILEDINIT  */  "Service provider failed to initialize",
    /*  57 SYSCALLFAILURE       */  "System call failure",
    /*  58 SERVICE_NOT_FOUND    */  "Service not found",
    /*  59 TYPE_NOT_FOUND       */  "Class type not found",
    /*  60 E_NO_MORE            */  "No more results",
    /*  61 E_CANCELLED          */  "Call was canceled",
    /*  62 EREFUSED             */  "Database query was refused",
    /*  63 HOST_NOT_FOUND       */  "Host not found",
    /*  64 TRY_AGAIN            */  "Nonauthoritative host not found",
    /*  65 NO_RECOVERY          */  "This is a nonrecoverable error",
    /*  66 NO_DATA              */  "Valid name, no data record of requested type",
    /*  67                      */  "Unknown error",
    /*  68                      */  "Unknown error",
    /*  69                      */  "Unknown error",

    /* my own */
    /*  70 ENOTREADY            */  "Drive not ready",

    /* libdl */
    /*  71 ENOMOD               */  "The specified module could not be found",
    /*  72 ENOPROC              */  "The specified procedure could not be found",
    /*  73                      */  "Unknown error",
    /*  74                      */  "Unknown error",
    /*  75                      */  "Unknown error",
    /*  76                      */  "Unknown error",
    /*  77                      */  "Unknown error",
    /*  78                      */  "Unknown error",
    /*  79                      */  "Unknown error",

    /*  80 STRUNCATE            */  "String truncation occurred",

    /*  81 OPERATION_ABORTED    */  "Unknown error",
    /*  82 IO_INCOMPLETE        */  "I/O incomplete",
    /*  83 IO_PENDING           */  "I/O pending",
    /*  84                      */  "Unknown error",
    /*  85                      */  "Unknown error",
    /*  86                      */  "Unknown error",
    /*  87 INVALID_PARAMETER    */  "Invalid parameter",

    /* linux */
    /*  88 ENOTSOCK             */  "Socket operation on non-socket",
    /*  89 EDESTADDRREQ         */  "Destination address required",
    /*  90 EMSGSIZE             */  "Message too long",
    /*  91 EPROTOTYPE           */  "Protocol wrong type for socket",
    /*  92 ENOPROTOOPT          */  "Protocol not available",
    /*  93 EPROTONOSUPPORT      */  "Protocol not supported",
    /*  94 ESOCKTNOSUPPORT      */  "Socket type not supported",
    /*  95 EOPNOTSUPP           */  "Operation not supported on transport endpoint",
    /*  96 EPFNOSUPPORT         */  "Protocol family not supported",
    /*  97 EAFNOSUPPORT         */  "Address family not supported by protocol",
    /*  98 EADDRINUSE           */  "Address already in use",
    /*  99 EADDRNOTAVAIL        */  "Cannot assign requested address",
    /* 100 ENETDOWN             */  "Network is down",
    /* 101 ENETUNREACH          */  "Network is unreachable",
    /* 102 ENETRESET            */  "Network dropped connection because of reset",
    /* 103 ECONNABORTED         */  "Software caused connection abort",
    /* 104 ECONNRESET           */  "Connection reset by peer",
    /* 105 ENOBUFS              */  "No buffer space available",
    /* 106 EISCONN              */  "Transport endpoint is already connected",
    /* 107 ENOTCONN             */  "Transport endpoint is not connected",
    /* 108 ESHUTDOWN            */  "Cannot send after transport endpoint shutdown",
    /* 109 ETOOMANYREFS         */  "Too many references: cannot splice",
    /* 110 ETIMEDOUT            */  "Connection timed out",
    /* 111 ECONNREFUSED         */  "Connection refused",
    /* 112 EHOSTDOWN            */  "Host is down",
    /* 113 EHOSTUNREACH         */  "No route to host",
    /* 114 EALREADY             */  "Operation already in progress",
    /* 115 EINPROGRESS          */  "Operation now in progress",
    /* 116 ESTALE               */  "Stale NFS file handle",
    /* 117 EUCLEAN              */  "Structure needs cleaning",
    /* 118 ENOTNAM              */  "Not a XENIX named type file",
    /* 119 ENAVAIL              */  "No XENIX semaphores available",
    /* 120 EISNAM               */  "Is a named type file",
    /* 121 EREMOTEIO            */  "Remote I/O error",
    /* 122 EDQUOT               */  "Quota exceeded",
    /* 123                      */  "Unknown error",
    /* 124                      */  "Unknown error",
    /* 125                      */  "Unknown error",
    /* 126                      */  "Unknown error",
    /* 127                      */  "Unknown error"
};

const char *cw_strerror(int errnum)
{
    /* need to use the one provided by winsock */
    if (errnum == ETIMEDOUT)
        errnum = _ETIMEDOUT;

    assert((errnum >= 0) && (errnum < MAX_CW_ERRNO));
    if ((errnum >= 0) && (errnum < MAX_CW_ERRNO))
        return cw_errlist[errnum];
    else
        return "Invalid error number";
}

void cw_perror(const char *msg)
{
    assert(msg);
    fprintf(stderr, "%s: %s\n", PATH_PLAIN(msg), cw_strerror(errno));
}

#define WSA(x)      case WSA##x  : return (errno = x)
#define WSA2(x, y)  case WSA##x  : return (errno = y)
#define WSAM(x)     case WSA_##x : return (errno = x)
#define WSAM2(x, y) case WSA_##x : return (errno = y)

int cw_wseterrno(void)
{
    int wsaerr = WSAGetLastError();
    switch (wsaerr)
    {
        /* winsock specific */
        /*  6       */  WSAM2(INVALID_HANDLE, EBADF);
        /*  8       */  WSAM2(NOT_ENOUGH_MEMORY, ENOMEM);
        /*  87      */  WSAM(INVALID_PARAMETER);
        /* 995      */  WSAM(OPERATION_ABORTED);
        /* 996      */  WSAM(IO_INCOMPLETE);
        /* 997      */  WSAM(IO_PENDING);
        /* 10004    */  WSA(EINTR);
        /* 10009    */  WSA(EBADF);
        /* 10013    */  WSA(EACCES);
        /* 10014    */  WSA(EFAULT);
        /* 10022    */  WSA(EINVAL);
        /* 10024    */  WSA(EMFILE);

        /* missing */
        /* 10035    */  WSA2(EWOULDBLOCK, EALREADY); /* should be EAGAIN */
        /* 10036    */  WSA(EINPROGRESS);
        /* 10037    */  WSA(EALREADY);

        /* map 1:1 */
        /* 10038    */  WSA(ENOTSOCK);
        /* 10039    */  WSA(EDESTADDRREQ);
        /* 10040    */  WSA(EMSGSIZE);
        /* 10041    */  WSA(EPROTOTYPE);
        /* 10042    */  WSA(ENOPROTOOPT);
        /* 10043    */  WSA(EPROTONOSUPPORT);
        /* 10044    */  WSA(ESOCKTNOSUPPORT);
        /* 10045    */  WSA(EOPNOTSUPP);
        /* 10046    */  WSA(EPFNOSUPPORT);
        /* 10047    */  WSA(EAFNOSUPPORT);
        /* 10048    */  WSA(EADDRINUSE);
        /* 10049    */  WSA(EADDRNOTAVAIL);
        /* 10050    */  WSA(ENETDOWN);
        /* 10051    */  WSA(ENETUNREACH);
        /* 10052    */  WSA(ENETRESET);
        /* 10053    */  WSA(ECONNABORTED);
        /* 10054    */  WSA(ECONNRESET);
        /* 10055    */  WSA(ENOBUFS);
        /* 10056    */  WSA(EISCONN);
        /* 10057    */  WSA(ENOTCONN);
        /* 10058    */  WSA(ESHUTDOWN);
        /* 10059    */  WSA(ETOOMANYREFS);
        /* 10060    */  WSA(ETIMEDOUT);
        /* 10061    */  WSA(ECONNREFUSED);
        /* 10062    */  WSA(ELOOP);
        /* 10063    */  WSA(ENAMETOOLONG);
        /* 10064    */  WSA(EHOSTDOWN);
        /* 10065    */  WSA(EHOSTUNREACH);
        /* 10066    */  WSA(ENOTEMPTY);
        /* 10067    */  WSA(EPROCLIM);
        /* 10068    */  WSA(EUSERS);
        /* 10069    */  WSA(EDQUOT);
        /* 10070    */  WSA(ESTALE);

        /* winsock specific */

        /* 10071    */  WSA(EREMOTE);
        /* 10091    */  WSA(SYSNOTREADY);
        /* 10092    */  WSA(VERNOTSUPPORTED);
        /* 10093    */  WSA(NOTINITIALISED);
        /* 10101    */  WSA(EDISCON);
        /* 10102    */  WSA(ENOMORE);
        /* 10103    */  WSA(ECANCELLED);
        /* 10104    */  WSA(EINVALIDPROCTABLE);
        /* 10105    */  WSA(EINVALIDPROVIDER);
        /* 10106    */  WSA(EPROVIDERFAILEDINIT);
        /* 10107    */  WSA(SYSCALLFAILURE);
        /* 10108    */  WSA(SERVICE_NOT_FOUND);
        /* 10109    */  WSA(TYPE_NOT_FOUND);
        /* 10110    */  WSAM(E_NO_MORE);
        /* 10111    */  WSAM(E_CANCELLED);
        /* 10112    */  WSA(EREFUSED);
        /* 11001    */  WSA(HOST_NOT_FOUND);
        /* 11002    */  WSA(TRY_AGAIN);
        /* 11003    */  WSA(NO_RECOVERY);
        /* 11004    */  WSA(NO_DATA);
    }
    cli_warnmsg("Winsocket Error #%d not mapped, please report\n", wsaerr);
    return (errno = NOERROR);
}

int cw_leerrno(void)
{
    DWORD err = GetLastError();
    switch (err)
    {
        case ERROR_NO_MORE_FILES:
            return (errno = NOERROR);
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
        case ERROR_BAD_PATHNAME:
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_NET_NAME:
            return (errno = ENOENT);
        case ERROR_MOD_NOT_FOUND:
        case ERROR_DLL_NOT_FOUND:
            return (errno = ENOMOD);
        case ERROR_PROC_NOT_FOUND:
            return (errno = ENOPROC);
        case ERROR_BAD_DEVICE:
            return (errno = ENODEV);
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
            return (errno = EACCES);
        case ERROR_NOT_READY:
            return (errno = ENOTREADY);
        case ERROR_FILENAME_EXCED_RANGE:
            cli_warnmsg("Filename too long, please report\n");
            return (errno = ENOENT);
    }
    cli_warnmsg("System Error #%d not mapped, please report\n", err);
    return (errno = NOERROR);
}

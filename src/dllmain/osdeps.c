/*
 * Clamav Native Windows Port: platform specific helpers
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
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
#include <osdeps.h>
#include <dirent.h>
#include <others.h>

#include <shared/output.h>

/* Get the alternative name for a file, so esotic names/paths can be easily accessed */
static char *cw_getaltname(const char *filename)
{
    size_t len = strlen(filename) + 6;
    size_t pos = 0;
    char *lslash = strrchr(filename, '\\');
    char *name_a = NULL, *fqname_a = NULL;
    wchar_t *name_w = NULL;
    HANDLE hf = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW wfdw;

    if (!lslash)
    {
        cli_errmsg("[platform] Unexpected path syntax\n");
        return NULL;
    }

    CW_CHECKALLOC(name_a, malloc(len), return NULL);

    snprintf(name_a, len - 1, "%s%s", cw_uncprefix(filename), filename);

    if (!(name_w = cw_mb2wc(name_a)))
    {
        cli_errmsg("[platform] Error in conversion from ansi to widechar (%d)\n", GetLastError());
        free(name_a);
        return NULL;
    }

    hf = FindFirstFileW(name_w, &wfdw);
    free(name_w);

    if (hf == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            cli_errmsg("[platform] No such file or directory\n");
        else
            cli_errmsg("[platform] FindFirstFileW() failed (%d)\n"
            "[%s] is not accessible by the OS\n", GetLastError(), name_a);
        free(name_a);
        return NULL;
    }

    FindClose(hf);
    free(name_a);
    if (wcslen(wfdw.cAlternateFileName) && (!(name_a = cw_wc2mb(wfdw.cAlternateFileName, 0))))
    {
        cli_errmsg("[platform] Error while getting alternate name (%d)\n", GetLastError());
        free(name_a);
        return NULL;
    }

    pos = lslash - filename + 1;
    len = pos + strlen(name_a) + 2;
    CW_CHECKALLOC(fqname_a, malloc(len), { free(name_a); return NULL; });
    strncpy(fqname_a, filename, pos);
    fqname_a[pos] = 0;
    strncat(fqname_a, name_a, len - 1 - strlen(fqname_a));
    free(name_a);
    return fqname_a;
}

/* A path is a path... not on Windows... */
char *cw_normalizepath(const char *path)
{
    size_t len = 0;
    char *plain = NULL, *seek = NULL;
    char *name_u = NULL;
    char *filename = NULL, *norm = NULL;

    assert(path);
    len = strlen(path);
    /* NULL + trailing \ */
    CW_CHECKALLOC(filename, malloc(len + 2), return NULL);

    strcpy(filename, path);
    cw_pathtowin32(filename);
    cw_rmtrailslashes(filename);
    len = strlen(filename);
    plain = PATH_PLAIN(filename);

    if ((strlen(plain) == 2) && (plain[1] == ':')) /* Allow c: d: notation */
    {
        strcat(filename, "\\");
        return filename;
    }
    else if (!PATH_ISNET(filename))
    {
        /* relative path, then add current directory */
        if (plain[1] != ':')
        {
            char *fq = NULL;
            size_t clen = 0;

            if (!(fq = cw_getcurrentdir()))
            {
                cli_errmsg("[platform] cw_getcurrentdir() failed %d\n", GetLastError());
                return NULL;
            }

            clen = len + strlen(fq) + 2;

            /* \path notation */
            if ((plain[0] == '\\') && ((seek = strchr(fq, ':')))) (*(seek + 1) = 0);

            /* reallocate the string to make room for the filename */
            CW_CHECKALLOC(fq, realloc(fq, clen + strlen(filename)), return NULL);
            strncat(fq, "\\", clen - 1 - strlen(fq));
            strncat(fq, filename, clen - 1 - strlen(fq));
            free(filename);
            filename = fq;
        }

        /* Win9x does not like paths like c:\\something */
        if (isWin9x())
        {
            char *p = NULL, *s = NULL;
            p = s = filename;
            while (*s && *p)
            {
                *p = *s;
                if (*s == '\\')
                {
                    p++;
                    while (*s && (*s == '\\')) s++;
                    continue;
                }
                p++; s++;
            }
            *p = 0;
        }
        else if (!PATH_ISUNC(filename))
        {
            len = strlen(filename) + sizeof(UNC_PREFIX) + 1;
            CW_CHECKALLOC(name_u, malloc(len), return NULL);
            snprintf(name_u, len - 1, "%s%s", cw_uncprefix(filename), filename);
            free(filename);
            filename = name_u;
        }
    }

    /* total path len > MAX_PATH, it's valid but some of windows api do not think so */
    if (len > MAX_PATH)
        norm = cw_getaltname(filename);
    else
        norm = cw_getfullpathname(filename);

    free(filename);

    cli_dbgmsg("\nPath converted from [%s] to [%s]\n", path, norm);
    return norm;
}

int cw_movefileex(const char *source, const char *dest, DWORD flags)
{
    assert(source);
    if (!source)
    {
        SetLastError(ERROR_BAD_ARGUMENTS);
        return 0;
    }
    if (!isWin9x()) return (MoveFileExA(source, dest, flags));

    /* Yuppi, MoveFileEx on Win9x */

    if (flags & MOVEFILE_REPLACE_EXISTING)
        DeleteFileA(dest);

    if (flags & MOVEFILE_DELAY_UNTIL_REBOOT)
    {
        char WinInitIni[MAX_PATH] = "";
        char ssource[MAX_PATH] = "";
        GetWindowsDirectoryA(WinInitIni, MAX_PATH - 1);
        WinInitIni[MAX_PATH - 1] = 0;
        strncat(WinInitIni, "\\wininit.ini", MAX_PATH - 1 - strlen(WinInitIni));
        WinInitIni[MAX_PATH - 1] = 0;

        /* Currently first copy the file to the destination, then schedule the remove.
           I cannot known the 8.3 path before having the file, so wininit stuff
           will fail, and windows 98 needs to be rebooted two times, since the first
           time freezes for me */

        if (dest) CopyFileA(source, dest, 0);

        if (!GetShortPathNameA(source, ssource, MAX_PATH - 1))
        {
            cli_warnmsg("GetShortPathNameA() for %s failed %d\n", source, GetLastError());
            return 0;
        }
        ssource[MAX_PATH - 1] = 0;
        return (WritePrivateProfileStringA("rename", "NUL", ssource, WinInitIni));
    }

    return (MoveFileA(source, dest));
}

int cw_movefile(const char *source, const char *dest, int reboot)
{
    if (!reboot)
    {
        FIXATTRS(source);
        FIXATTRS(dest);
        if (cw_movefileex(source, dest, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return 1;
        if (!ISLOCKED(GetLastError()))
        {
            cli_warnmsg("%s cannot be moved (%lu)\n", source, GetLastError());
            return 0;
        }
        cli_warnmsg("%s cannot be moved (%lu), scheduling the move operation for next reboot\n", source, GetLastError());
    }
    if (cw_movefileex(source, dest, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING))
        return 1;
    cli_warnmsg("error scheduling the move operation for reboot (%lu)\n", GetLastError());
    return 0;
}

static inline struct addrinfo *new_ai(int socktype, int protocol, int port, int address)
{
    struct addrinfo *ai;
    struct sockaddr_in *ai_addr;

    if (!(ai = calloc(1, sizeof(struct addrinfo))))
        return NULL;

    if (!(ai_addr = calloc(1, sizeof(struct sockaddr_in))))
        return NULL;

    ai_addr->sin_family = AF_INET;
    ai_addr->sin_port = port;
    ai_addr->sin_addr.s_addr = address;

    ai->ai_family = PF_INET;
    ai->ai_socktype = socktype;
    ai->ai_protocol = protocol;
    ai->ai_addrlen = sizeof(struct sockaddr_in);
    ai->ai_addr = (struct sockaddr *) ai_addr;

//    fprintf(stderr, "-> [%d.%d.%d.%d]\n", ((unsigned char *) &address)[0], ((unsigned char *) &address)[1], ((unsigned char *) &address)[2], ((unsigned char *) &address)[3]);

    return ai;
}

int cw_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    struct hostent *he;
    struct addrinfo **p_ai_next = res;
    struct addrinfo default_hints;
    char **p_addr;
    u_short port = 0;
    int i;

    if (cw_helpers.ws2.ok)
        return cw_helpers.ws2.getaddrinfo(node, service, hints, res);

    if (!(node || service))
        return EAI_NONAME;

    if (hints == NULL)
    {
        memset(&default_hints, 0, sizeof(default_hints));
        default_hints.ai_family = AF_UNSPEC;
        hints = &default_hints;
    }

    // only AI_PASSIVE flag is supported
    if (hints->ai_flags & ~AI_PASSIVE)
    {
        fprintf(stderr, "[getaddrinfo] unsupported ai_flags: 0x%x, please report\n", hints->ai_flags);
        return EAI_BADFLAGS;
    }

    if ((hints->ai_family != AF_UNSPEC) && (hints->ai_family != AF_INET))
        return EAI_FAMILY;

    // only numeric services are supported
    if (service)
    {
        char *p;
        port = htons(strtoul(service, &p, 10));
        if (*p)
            return EAI_NONAME;
    }

    if (!node) // TODO: or numeric
    {
        int address;

        if (hints->ai_flags & ~AI_PASSIVE)
            return EAI_BADFLAGS;

        address = htonl((hints->ai_flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
        if (!(*res = new_ai(hints->ai_socktype, hints->ai_protocol, port, address)))
            return EAI_MEMORY;

        return 0;
    }

    if (!(he = gethostbyname(node)))
    {
        switch (h_errno)
        {
            case HOST_NOT_FOUND:
            case NO_DATA:
                errno = 63;
                break;
            case TRY_AGAIN:
                errno = 64;
                break;
            case NO_RECOVERY:
            default:
                errno = 65;
        }
        return EAI_SYSTEM;
    }

    // WARNING: no loop over cnames
    if ((he->h_addrtype != AF_INET) || (he->h_length != sizeof(struct in_addr)))
    {
        fprintf(stderr, "[getaddrinfo] looping over cnames is not implemented\n");
        return EAI_SYSTEM;
    }

    *p_ai_next = NULL;
    for (p_addr = he->h_addr_list; *p_addr; p_addr++)
    {
        *p_ai_next = new_ai(hints->ai_socktype, hints->ai_protocol, port, ((struct in_addr *) *p_addr)->s_addr);
        if (!*p_ai_next)
            return EAI_MEMORY;
        p_ai_next = &((*p_ai_next)->ai_next);
    }

    return 0;
}

void cw_freeaddrinfo(struct addrinfo *res)
{
    struct addrinfo *prev;

    if (cw_helpers.ws2.ok)
    {
        cw_helpers.ws2.freeaddrinfo(res);
        return;
    }

    do
    {
        prev = res;
        res = res->ai_next;
        if (prev->ai_addr) free(prev->ai_addr);
        free(prev);
    } while (res);
}

const char *cw_gai_strerror(int errcode)
{
    switch (errcode)
    {
/*
        case EAI_ADDRFAMILY:
            return "Address family for hostname not supported";
*/
        case EAI_AGAIN:
            return "Temporary failure in name resolution";
        case EAI_BADFLAGS:
            return "Bad value for ai_flags";
        case EAI_FAIL:
            return "Non-recoverable failure in name resolution";
        case EAI_FAMILY:
            return "ai_family not supported";
        case EAI_MEMORY:
            return "Memory allocation failure";
        case EAI_NODATA:
            return "No address associated with hostname";
/*
        case EAI_NONAME:
            return "Name or service not known";
*/
        case EAI_SERVICE:
            return "Servname not supported for ai_socktype";
        case EAI_SOCKTYPE:
            return "ai_socktype not supported";
        case EAI_SYSTEM:
        default:
            return "System error";
    }
}

/* on Win32 rename() fails if newname exists, and yes this function is not atomic*/
#undef rename
int cw_rename(const char *oldname, const char *newname)
{
    FIXATTRS(newname);
    if (!DeleteFileA(newname) && (GetLastError() != ERROR_FILE_NOT_FOUND))
        return -1;
    return rename(oldname, newname);
}

/* A non TLS based and non thread safe canonical rand() implementation */
/* aCaB <acab@clamav.net> */
static unsigned long next = 1;
int cw_rand(void)
{
    next = next * 1103515245 + 12345;
    return((unsigned)(next/65536) % (RAND_MAX+1));
}

void cw_srand(unsigned int seed)
{
    next = seed;
}

/* default stop control handler */
BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType)
{
    if (CtrlType == CTRL_C_EVENT)
    {
        SetConsoleCtrlHandler(cw_stop_ctrl_handler, FALSE);
        fprintf(stderr, "Control+C pressed, aborting...\n");
        exit(0);
    }
    return TRUE;
}

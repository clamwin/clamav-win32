/*
 * Clamav Native Windows Port: <sys/socket.h> mappings
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

/* FIXME: SOCKET -> int, socket on win32 is UINT_PTR */

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

extern int cw_wseterrno(void);
extern int gnulib_snprintf(char *str, size_t size, const char *format, ...);

typedef int socklen_t;

/* <sys/socket.h> */
#define SHUT_RD     SD_RECEIVE
#define SHUT_WR     SD_SEND
#define SHUT_RDWR   SD_BOTH

#define endprotoent()
#define endservent()

/* made as macro to avoid explicit link of winsock lib when only ntohl is needed */
#define ntohl(x) (((uint32_t)x >> 24) | \
                 (((uint32_t)x >> 8) & 0xff00) | \
                 (((uint32_t)x << 8) & 0xff0000) | \
                 ((uint32_t)x << 24))

/* Winsock internals */
#define SO_SYNCHRONOUS_ALERT    0x10
#define SO_SYNCHRONOUS_NONALERT 0x20
#define SO_OPENTYPE             0x7008

/* only setting O_NONBLOCK is supported - F_GETFL returns always 0 */
#define F_GETFL     3   /* Get file status flags */
#define F_SETFL     4   /* Set file status flags */

#define O_NONBLOCK 04000

/* only setting O_NONBLOCK is supported - F_GETFL returns always 0 */
static inline int fcntl(int fd, int cmd, long arg)
{
    u_long mode = (arg & O_NONBLOCK);

    switch (cmd)
    {
        case F_GETFL:
            return 0;
        case F_SETFL:
            if (ioctlsocket(fd, FIONBIO, &mode))
            {
                cw_wseterrno();
                return -1;
            }
            return 0;
    }

    errno = EBADF;
    return -1;
}

#define sock_set_nonblock(sockfd) fcntl(sockfd, F_SETFL, O_NONBLOCK)

static inline SOCKET inl_accept(SOCKET sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    SOCKET res = accept(sockfd, addr, addrlen);
    if (res == INVALID_SOCKET)
        cw_wseterrno();
    return res;
}
#define accept(s, a, al) (int) inl_accept((SOCKET) s, a, al)

static inline int inl_bind(SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (bind(sockfd, addr, addrlen) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define bind inl_bind

static inline int inl_closesocket(SOCKET s)
{
    if (closesocket(s) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define closesocket(s) inl_closesocket((SOCKET) s)

static inline int inl_connect(SOCKET sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
    if (connect(sockfd, serv_addr, addrlen) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define connect inl_connect

/* freeaddrinfo */
/* getaddrinfo */
/* gethostbyaddr - herrno */

/* on win32 gethostbyname fails with dotted quad strings */
static inline struct hostent *inl_gethostbyname(const char *name)
{
    uint32_t a, b, c, d;
    char dummy;

    if ((sscanf(name, "%lu.%lu.%lu.%lu%c", &a, &b, &c, &d, &dummy) != 4) ||
        (a >= 0xff) || (b >= 0xff) || (c >= 0xff) || (d >= 0xff))
        return gethostbyname(name); /* herrno */
    else
    {
        static struct hostent he;
        static unsigned char ip_addr[4];
        static char *ip_aliases[1] = {0};
        static char *ip_addr_list[2] = {0,0};

        memset(&he, 0, sizeof(struct hostent));

        /* fill it */
        ip_addr[0] = a;
        ip_addr[1] = b;
        ip_addr[2] = c;
        ip_addr[3] = d;

        he.h_name = (char *) name;
        he.h_addr_list = ip_addr_list;
        he.h_addr_list[0] = (char *) ip_addr;
        he.h_aliases = ip_aliases;
        he.h_addrtype = AF_INET;
        he.h_length = 4;
        return &he;
    }
}
#define gethostbyname inl_gethostbyname

static inline int inl_gethostname(char *name, size_t len)
{
    if (gethostname(name, (int) len) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define gethostname inl_gethostname

/* getnameinfo */

static inline int inl_getpeername(SOCKET s, struct sockaddr *name, socklen_t *namelen)
{
    if (getpeername(s, name, namelen) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define getpeername inl_getpeername

/* getprotobyname */
/* getprotobynumber */
/* getservbyname */
/* getservbyport */

static inline int inl_getsockname(SOCKET s, struct sockaddr *name, socklen_t *namelen)
{
    if (getsockname(s, name, namelen) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define getsockname inl_getsockname

/* win32 getsockopt has a different proto */
static inline int inl_getsockopt(SOCKET s, int level, int optname, void *optval, socklen_t *optlen)
{
    if (getsockopt(s, level, optname, (char *) optval, optlen))
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define getsockopt inl_getsockopt

/* #define htonl() */
/* #define htons() */

/* inet_addr */
/* inet_ntoa */
/* ioctlsocket - win32 only */

static inline int inl_listen(SOCKET sockfd, int backlog)
{
    if (listen(sockfd, backlog) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define listen inl_listen

/* #define ntohl() */
/* #define ntohs() */

static inline ssize_t inl_recv(SOCKET s, void *buf, size_t len, int flags)
{
    ssize_t result = recv(s, (char *) buf, (int) len, flags);
    if (result == SOCKET_ERROR)
        cw_wseterrno();
    return result;
}
#define recv(s, b, l, f) inl_recv((SOCKET) s, b, l, f)

static inline ssize_t inl_recvfrom(SOCKET s, void *buf, size_t len, int flags,
                                  struct sockaddr *from, socklen_t *fromlen)
{
    ssize_t result = (ssize_t) recvfrom(s, (char *) buf, (int) len, flags, from, fromlen);
    if (result == SOCKET_ERROR)
        cw_wseterrno();
    return result;
}
#define recvfrom inl_recvfrom

static inline int inl_select(int nfds, fd_set *readfds, fd_set *writefds,
                             fd_set *exceptfds, struct timeval *timeout)
{
    int result = select(0, readfds, writefds, exceptfds, timeout);
    if (result == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return result;
}
#define select inl_select

static inline ssize_t inl_send(SOCKET s, const void *buf, size_t len, int flags)
{
    ssize_t result = send(s, (const char *) buf, (int) len, flags & (MSG_DONTROUTE | MSG_OOB));
    if (result == SOCKET_ERROR)
        cw_wseterrno();
    return result;
}
#define send inl_send

static inline ssize_t inl_sendto(SOCKET s, const void *buf, size_t len, int flags,
                                const struct sockaddr *to, socklen_t tolen)
{
    ssize_t result = sendto(s, (const char *) buf, (int) len, flags & (MSG_DONTROUTE | MSG_OOB), to, tolen);
    if (result == SOCKET_ERROR)
        cw_wseterrno();
    return result;
}
#define sendto inl_sendto

static inline int inl_setsockopt(SOCKET s, int level, int optname,
                                 const void *optval, socklen_t optlen)
{
    if (setsockopt(s, level, optname, (char *) optval, optlen) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define setsockopt inl_setsockopt

static inline int inl_shutdown(SOCKET s, int how)
{
    if (shutdown(s, how) == SOCKET_ERROR)
    {
        cw_wseterrno();
        return -1;
    }
    return 0;
}
#define shutdown(s, how) inl_shutdown((SOCKET) s, how)

static inline SOCKET inl_socket(int domain, int type, int protocol)
{
    SOCKET result = socket(domain, type, protocol);
    if (result == INVALID_SOCKET)
        cw_wseterrno();
    return result;
}
#define socket (int) inl_socket

/* <arpa/inet.h> */
static inline const char * WSAAPI inet_ntop(int af, const void *src, char *dst, size_t size)
{
    unsigned char *ip = (unsigned char *) src;
    if ((af != AF_INET) || (size < 16))
    {
        errno = EINVAL;
        return NULL;
    }
    gnulib_snprintf(dst, size, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return dst;
}

/* pollfd struct is _WIN32_WINNT >= 0x0600 */

typedef struct mypollfd
{
    SOCKET  fd;
    SHORT   events;
    SHORT   revents;
} mypollfd;

#define pollfd mypollfd

extern int poll_with_event(struct pollfd *fds, int nfds, int timeout, HANDLE event);

#ifndef POLLRDNORM
#define POLLRDNORM  0x0100
#define POLLRDBAND  0x0200
#define POLLIN      (POLLRDNORM | POLLRDBAND)
#define POLLPRI     0x0400

#define POLLWRNORM  0x0010
#define POLLOUT     (POLLWRNORM)
#define POLLWRBAND  0x0020

#define POLLERR     0x0001
#define POLLHUP     0x0002
#define POLLNVAL    0x0004
#endif

#endif /* _SYS_SOCKET_H */

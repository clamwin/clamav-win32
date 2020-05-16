/*
 * Clamav Native Windows Port: dns queries for freshclam
 *
 * Copyright (c) 2011 Gianluigi Tiesi <sherpya@netfarm.it>
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

#ifndef __RESOLV_H
#define __RESOLV_H

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#define NS_INT8SZ 1
#define NS_INT16SZ 2
#define NS_INT32SZ 4
#define NS_PACKETSZ 512
#define N_RETRY 5

 /* Bound checks to avoid buffer overflows */
#define NEED(len)                                                   \
    if (pt > (answend - len))                                       \
    {                                                               \
        logg("!DNS Resolver: Bound Check failed - Bad packet\n");   \
        return NULL;                                                \
    }

#define SETUINT16(var, value) *(uint16_t *) var = htons(value)

#define NS_GET16(var, ptr)              \
    do                                  \
    {                                   \
        NEED(NS_INT16SZ);               \
        var = ntohs(*(uint16_t*) ptr);  \
        ptr += NS_INT16SZ;              \
    } while (0)

#define NS_GET32(var, ptr)              \
    do                                  \
    {                                   \
        NEED(NS_INT32SZ);               \
        var = ntohl(*(uint32_t*) ptr);  \
        ptr += NS_INT32SZ;              \
    } while (0)

typedef struct
{
    unsigned    id : 16;     /*%< query identification number */
            /* fields in third byte */
    unsigned    rd : 1;      /*%< recursion desired */
    unsigned    tc : 1;      /*%< truncated message */
    unsigned    aa : 1;      /*%< authoritive answer */
    unsigned    opcode : 4;  /*%< purpose of message */
    unsigned    qr : 1;      /*%< response flag */
            /* fields in fourth byte */
    unsigned    rcode : 4;   /*%< response code */
    unsigned    cd : 1;      /*%< checking disabled by resolver */
    unsigned    ad : 1;      /*%< authentic data from named */
    unsigned    unused : 1;  /*%< unused bits (MBZ as of 4.9.3a3) */
    unsigned    ra : 1;      /*%< recursion available */
            /* remaining bytes */
    unsigned    qdcount : 16;    /*%< number of question entries */
    unsigned    ancount : 16;    /*%< number of answer entries */
    unsigned    nscount : 16;    /*%< number of authority entries */
    unsigned    arcount : 16;    /*%< number of resource entries */
} HEADER;


#define T_TXT 16
#define T_ANY 255
#define T_CNAME 5

#endif

/*
 * Clamav Native Windows Port: dns queries for freshclam
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

#include <platform.h>
#include <osdeps.h>
#include <iphlpapi.h>
#include <inttypes.h>

#include "others.h"
#include "shared/output.h"

#define TCPIP_PARAMS "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

/* I can't believe the vs2010 preprocessor pastes a macro after a dot */
#ifdef DnsRecordListFree
#undef DnsRecordListFree
#endif

char *txtquery_dnsapi(const char *domain, unsigned int *ttl);
char *txtquery_compat(const char *domain, unsigned int *ttl);

#define NS_INT8SZ 1
#define NS_PACKETSZ 512
#define N_RETRY 5

#define SETUINT16(x, v) *(uint16_t *) x = htons(v)

#define GETUINT16(x) ntohs(*(uint16_t *) x)
#define GETUINT32(x) ntohl(*(uint32_t *) x)

/* Bound checks to avoid buffer overflows */
#define NEED(len) \
    /* printf("DNS Resolver: Need %d bytes - Have %d bytes\n", len, numbytes - (seek - reply)); */ \
    if (((seek + len) - reply) > numbytes) \
    { \
        logg("!DNS Resolver: Bound Check failed - Bad packet\n"); \
        return NULL; \
    }

#ifndef HAVE_ATTRIB_PACKED
#define __attribute__(x)
#endif

#ifdef HAVE_PRAGMA_PACK
#pragma pack(1)
#endif

typedef struct _simple_dns_query
{
    uint16_t transaction_id __attribute__ ((packed));
    uint16_t flags __attribute__ ((packed));
    uint16_t questions __attribute__ ((packed));
    uint16_t ans_rrs __attribute__ ((packed));
    uint16_t auth_rss __attribute__ ((packed));
    uint16_t add_rss __attribute__ ((packed));
    /* queries here */
} simple_dns_query;

#ifdef HAVE_PRAGMA_PACK
#pragma pack()
#endif

char *dnsquery_dnsapi(const char *domain, int qtype, unsigned int *ttl);
char *dnsquery_compat(const char *domain, int qtype, unsigned int *ttl);

/* Switcher */
char *dnsquery(const char *domain, int qtype, unsigned int *ttl)
{
    // FIXME: UH?? freshclam/dns.c:81
    if ((qtype != DNS_TYPE_TEXT) && (qtype != DNS_TYPE_ANY))
    {
        if (ttl)
            *ttl = 2;
        return NULL;
    }
    return ((isWin9x() || isOldOS()) ? dnsquery_compat(domain, qtype, ttl) : dnsquery_dnsapi(domain, qtype, ttl));
}

static char *get_dns_fromreg(void)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    unsigned char data[MAX_PATH];
    DWORD datalen = MAX_PATH - 1;
    int i;
    char *keys[] = { "ClamWinNameServer", "NameServer", "DhcpNameServer", NULL };

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TCPIP_PARAMS, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return NULL;

    for (i = 0; keys[i]; i++)
    {
        datalen = MAX_PATH - 1;
        if ((RegQueryValueExA(hKey, keys[i], NULL, &dwType, data, &datalen) == ERROR_SUCCESS) &&
            (datalen > 1) && (dwType == REG_SZ))
        {
            char *space;
            RegCloseKey(hKey);
            if ((space = strchr(data, ' '))) *space = 0;
            if (inet_addr(data) == INADDR_NONE)
            {
                /* printf("DNS Resolver: Found %s key: %s - invalid address\n", keys[i], data); */
                return NULL;
            }
            else
            {
                /* printf("DNS Resolver: Found %s key: %s\n", keys[i], data); */
                return _strdup(data);
            }
        }
    }

    /* printf("DNS Resolver: No nameservers found in registry\n"); */
    RegCloseKey(hKey);
    return NULL;
}

static char *get_dns(void)
{
    FIXED_INFO *FixedInfo;
    ULONG ulOutBufLen;
    DWORD res;
    char *dns_server = NULL;

    FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, sizeof(FIXED_INFO));
    ulOutBufLen = sizeof(FIXED_INFO);

    switch (res = GetNetworkParams(FixedInfo, &ulOutBufLen))
    {
        case ERROR_BUFFER_OVERFLOW:
            GlobalFree(FixedInfo);
            FixedInfo = (FIXED_INFO *) GlobalAlloc(GPTR, ulOutBufLen);
        case NO_ERROR:
            break;
        case ERROR_NOT_SUPPORTED:
            /* printf("DNS Resolver: GetNetworkParams() not supported on this OS\n"); */
            GlobalFree(FixedInfo);
            return get_dns_fromreg();
        default:
            GlobalFree(FixedInfo);
            logg("!DNS Resolver: [1] Call to GetNetworkParams() failed %d\n", res);
            return NULL;
    }

    if ((res = GetNetworkParams(FixedInfo, &ulOutBufLen)) != ERROR_SUCCESS)
    {
        GlobalFree(FixedInfo);
        logg("!DNS Resolver: [2] Call to GetNetworkParams() failed %d\n", res);
        return NULL;
    }

/*
    More than one dns server - we just use the primary
    if (FixedInfo->DnsServerList.Next)
    {
        IP_ADDR_STRING *pIPAddr;
        pIPAddr = FixedInfo->DnsServerList.Next;
        while (pIPAddr)
        {
            //printf("DNS Resolver: Found additional DNS Server: %s\n", pIPAddr->IpAddress.String);
            pIPAddr = pIPAddr->Next;
        }
    }
*/

    dns_server = _strdup(FixedInfo->DnsServerList.IpAddress.String);
    GlobalFree(FixedInfo);
    return dns_server;
}

static char *do_query(struct hostent *he, const char *domain, unsigned int *ttl)
{
    struct sockaddr_in dns;
    char *packet, *seek, *txtreply;
    char reply[NS_PACKETSZ];
    simple_dns_query query, *res;
    int numbytes, addr_len, i;
    int start, rev, len, off;
    uint16_t tid;
    int sockfd = -1;
    struct timeval tv;

/* win32 random functions are enough here */
#undef rand
#undef srand
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec + clock() + rand());
    tid = rand();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        logg("!DNS Resolver: socket() failed, %s\n", strerror(errno));
        return NULL;
    }

    memset(&dns, 0, sizeof(dns));
    dns.sin_family = AF_INET;
    dns.sin_port = htons(53);
    dns.sin_addr = *((struct in_addr *) he->h_addr);

    /* Generate the packet */
    memset(&query, 0, sizeof(query));
    query.transaction_id = htons(tid);
    query.flags = htons(0x0100); /* Request + Recursion */
    query.questions = htons(1); /* 1 query */

    len = sizeof(query) + NS_INT8SZ + (int) strlen(domain) + NS_INT8SZ + (2 * sizeof(uint16_t)); /* \0 + Type + Class */
    packet = (char *) cli_malloc(len);

    memcpy(packet, &query, sizeof(query));
    off = sizeof(query) + NS_INT8SZ;

    start = 0;
    off += (int) strlen(domain);
    rev = start = off;

    /* String-ize */
    for (i = (int) strlen(domain); i >= 0; i--)
    {
        if (domain[i] != '.')
            packet[rev] = domain[i];
        else
        {
            packet[rev] = (uint8_t) (start - rev - 1);
            start = rev;
        }
        rev--;
    }

    /* First string length */
    packet[rev] = (uint8_t) (start - rev - 1);
    packet[off++] = 0;

    /* Type TXT */
    SETUINT16(&packet[off], DNS_TYPE_TEXT);
    off += sizeof(uint16_t);

    /* Class */
    SETUINT16(&packet[off], NS_INT8SZ);
    off += sizeof(uint16_t);

    if ((numbytes = sendto(sockfd, packet, (int) len, 0, (struct sockaddr *) &dns, sizeof(struct sockaddr))) == -1)
    {
        logg("!DNS Resolver: sendto() failed, %s\n", strerror(errno));
        free(packet);
        return NULL;
    }
    free(packet);

    addr_len = sizeof(struct sockaddr);
    if ((numbytes = recvfrom(sockfd, reply, NS_PACKETSZ, 0, (struct sockaddr *) &dns, &addr_len)) == -1)
    {
        logg("!DNS Resolver: recvfrom() failed, %s\n", strerror(errno));
        return NULL;
    }

    /* printf("DNS Resolver: Received %d bytes from the DNS\n", numbytes); */

    if (numbytes <= sizeof(simple_dns_query))
    {
        logg("!DNS Resolver: Short reply\n");
        return NULL;
    }

    seek = reply;
    res = (simple_dns_query *) seek;

    /* All your replies are belong to us? */
    if (ntohs(res->transaction_id) != tid)
    {
        logg("!DNS Resolver: Bad TID, expected 0x%04x got 0x%04x\n", tid, ntohs(res->transaction_id));
        return NULL;
    }

    /* Is a reply and result is ok */
    if (!(ntohs(res->flags) >> 15) || (ntohs(res->flags) & 0x000f))
    {
        logg("!DNS Resolver: Bad Reply\n");
        return NULL;
    }

    if (ntohs(res->ans_rrs) < 1)
    {
        logg("!DNS Resolver: No replies :(\n");
        return NULL;
    }

    seek += sizeof(simple_dns_query) + NS_INT8SZ;
    while (((seek - reply) < numbytes) && *seek) seek++; /* my request, don't care*/
    seek++;

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != DNS_TYPE_TEXT)
    {
        logg("!DNS Resolver: Query in DNS reply is not TXT\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != NS_INT8SZ)
    {
        logg("!DNS Resolver: Query in DNS reply has a different Class\n");
        return NULL;
    }
    seek += sizeof(uint16_t);
    seek += sizeof(uint16_t); /* Answer c0 0c ?? - Wireshark says Name */

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != DNS_TYPE_TEXT)
    {
        logg("!DNS Resolver: DNS reply Type is not TXT\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint16_t));
    if (GETUINT16(seek) != NS_INT8SZ)
    {
        logg("!DNS Resolver: DNS reply has a different Class\n");
        return NULL;
    }
    seek += sizeof(uint16_t);

    NEED(sizeof(uint32_t));
    if (ttl)
        *ttl = (unsigned int) GETUINT32(seek);
    seek += sizeof(uint32_t);

    NEED(sizeof(uint16_t));
    len = GETUINT16(seek);

    if (len > NS_PACKETSZ)
    {
        logg("!DNS Resolver: Oversized reply\n");
        return NULL;
    }
    seek += sizeof(uint16_t); /* Len */

    NEED(len);

    seek++;
    if (!*seek)
    {
        logg("!DNS Resolver: Empty TXT reply\n");
        return NULL;
    }

    txtreply = (char *) cli_malloc(len);
    memcpy(txtreply, seek, len);
    txtreply[len - 1] = 0;
    return txtreply;
}

/* Bare Version */
char *dnsquery_compat(const char *domain, int qtype, unsigned int *ttl)
{
    struct hostent *he;
    char *result = NULL, *nameserver = NULL;
    int i;

    // FIXME: implement TYPE A?
    if (qtype != DNS_TYPE_TEXT)
        return NULL;

    if ((nameserver = get_dns()) == NULL)
    {
        logg("!DNS Resolver: Cannot find a suitable DNS server\n");
        return NULL;
    }

    if ((he = gethostbyname(nameserver)) == NULL)
    {
        logg("!DNS Resolver: gethostbyname(nameserver) failed\n");
        return NULL;
    }

    for (i = 0; i < N_RETRY; i++)
    {
        if ((result = do_query(he, domain, ttl)) != NULL)
            break;
    }

    if (nameserver) free(nameserver);
    /* printf("DNS Resolver: Query done using compatibility Method\n"); */
    /* printf("DNS Resolver: Result is [%s]\n", result); */
    return result;
}


char *dnsquery_dnsapi(const char *domain, int qtype, unsigned int *ttl)
{
    PDNS_RECORD pRec, pRecOrig;
    char *result = NULL;

    if (!cw_helpers.dnsapi.ok)
        return dnsquery_compat(domain, qtype, ttl);

    if (cw_helpers.dnsapi.DnsQuery_A(domain, (WORD) qtype, DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE, NULL, &pRec, NULL) != ERROR_SUCCESS)
    {
        logg("%cDNS Resolver: Can't query %s\n", (qtype == DNS_TYPE_TEXT || qtype == DNS_TYPE_ANY) ? '^' : '*', domain);
        return NULL;
    }

    pRecOrig = pRec;

    while (pRec)
    {
        if (pRec->wType == (WORD) qtype)
        {
            switch (qtype)
            {
                case DNS_TYPE_TEXT:
                {
                    size_t len;
                    if ((pRec->wDataLength == 0) || (pRec->Data.TXT.dwStringCount == 0) || !pRec->Data.TXT.pStringArray[0])
                        break;
                    len = strlen(pRec->Data.TXT.pStringArray[0]);
                    result = (char *) cli_malloc(len + 1);
                    strncpy(result, (char *) pRec->Data.TXT.pStringArray[0], len);
                    result[len] = 0;
                    if (ttl)
                        *ttl = pRec->dwTtl;
                    goto done;
                }
                case DNS_TYPE_A:
                {
                    goto done;
                }
            }
        }
        pRec = pRec->pNext;
    }
done:
    cw_helpers.dnsapi.DnsRecordListFree(pRecOrig, DnsFreeRecordList);
    /* printf("DNS Resolver: Query done using DnsApi Method\n"); */
    /* printf("DNS Resolver: Result is [%s]\n", result); */
    return result;
}

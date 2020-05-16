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
#include <iptypes.h>
#include <stdint.h>

#include "others.h"
#include "shared/output.h"

/* I can't believe the vs2010 preprocessor pastes a macro after a dot */
#undef DnsRecordListFree

#if defined(__MINGW32__) && !defined(__MINGW64__)
#include <resolv.h>
/* including <iphlpapi.h> makes DATADIR define clash */
DWORD WINAPI GetNetworkParams (PFIXED_INFO pFixedInfo, PULONG pOutBufLen);

#define TCPIP_PARAMS "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

static char *dnsquery_dnsapi(const char *domain, int qtype, unsigned int *ttl);
static char *dnsquery_compat(const char *domain, int qtype, unsigned int *ttl);

/* Switcher */
char *dnsquery(const char *domain, int qtype, unsigned int *ttl)
{
    // FIXME: UH?? freshclam/dns.c:81
    if ((qtype != T_TXT) && (qtype != T_ANY))
    {
        if (ttl)
            *ttl = 2;
        return NULL;
    }

    return (cw_helpers.dnsapi.ok ? dnsquery_dnsapi : dnsquery_compat)(domain, qtype, ttl);
}

static char *get_dns_fromreg(void)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    unsigned char data[MAX_PATH];
    DWORD datalen = MAX_PATH - 1;
    char *keys[] = { "ClamWinNameServer", "NameServer", "DhcpNameServer", NULL };

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TCPIP_PARAMS, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return NULL;

    for (int i = 0; keys[i]; i++)
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
                logg("!DNS Resolver: Found %s key: %s - invalid address\n", keys[i], data);
                return NULL;
            }
            else
            {
                logg("*DNS Resolver: using %s as DNS server from %s key\n", data, keys[i]);
                return _strdup(data);
            }
        }
    }

    logg("!DNS Resolver: No nameservers found in registry\n");
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

static char *rcode_to_string(uint8_t rcode)
{
    static char *rcodes[] = {
        /*  0 */ "No error",
        /*  1 */ "Format error",
        /*  2 */ "Server failure",
        /*  3 */ "Non-Existent Domain",
        /*  4 */ "Not Implemented",
        /*  5 */ "Query Refused",
        /*  6 */ "Name Exists when it should not",
        /*  7 */ "RR Set Exists when it should not",
        /*  8 */ "RR Set that should exist does not",
        /*  9 */ "Server Not Authoritative for zone",
        /* 10 */ "Name not contained in zone",
        /* 11 */ "DSO-TYPE Not Implemented"
    };

    return (rcode < 12) ? rcodes[rcode] : "Unassigned";
}

/* courtesy of musl library, obfuscated c contest? ;)*/
static int dn_expand(const unsigned char* base, const unsigned char* end, const unsigned char* src, char* dest, int space)
{
    const unsigned char* p = src;
    char* dend, * dbegin = dest;
    int len = -1, i, j;
    if (p == end || space <= 0) return -1;
    dend = dest + (space > 254 ? 254 : space);
    /* detect reference loop using an iteration counter */
    for (i = 0; i < end - base; i += 2) {
        /* loop invariants: p<end, dest<dend */
        if (*p & 0xc0) {
            if (p + 1 == end) return -1;
            j = ((p[0] & 0x3f) << 8) | p[1];
            if (len < 0) len = p + 2 - src;
            if (j >= end - base) return -1;
            p = base + j;
        }
        else if (*p) {
            if (dest != dbegin) *dest++ = '.';
            j = *p++;
            if (j >= end - p || j >= dend - dest) return -1;
            while (j--) *dest++ = *p++;
        }
        else {
            *dest = 0;
            if (len < 0) len = p + 1 - src;
            return len;
        }
    }
    return -1;
}

static char *do_query(struct hostent *he, const char *domain, unsigned int *ttl)
{
    struct sockaddr_in dns;
    char *packet, *txtreply;
    unsigned char answer[NS_PACKETSZ], * answend, * pt;
    unsigned int cttl, size, txtlen = 0;
    HEADER query, *res;
    ssize_t len;
    int start, rev, off, type;
    uint16_t id;
    int sockfd;
    struct timeval tv;
    char host[128];
    int addr_len = sizeof(struct sockaddr);;

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

    /* win32 random functions are enough here */
#undef rand
#undef srand
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec + clock() + rand());
    id = rand();
    query.id = htons(id);

    /* ask for recursion */
    query.rd = 1;

    /* 1 query */
    query.qdcount = htons(1);

    len = sizeof(query) + NS_INT8SZ + (int) strlen(domain) + NS_INT8SZ + (2 * NS_INT16SZ); /* \0 + Type + Class */
    packet = (char *) cli_malloc(len);

    memcpy(packet, &query, sizeof(query));
    off = sizeof(query) + NS_INT8SZ;

    start = 0;
    off += (int) strlen(domain);
    rev = start = off;

    /* String-ize */
    for (int i = (int) strlen(domain); i >= 0; i--)
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
    SETUINT16(&packet[off], T_TXT);
    off += NS_INT16SZ;

    /* Class */
    SETUINT16(&packet[off], NS_INT8SZ);
    off += NS_INT16SZ;

    logg("*DNS Resolver (compat): Querying %s\n", domain);

    len = sendto(sockfd, packet, (int)len, 0, (struct sockaddr*) & dns, sizeof(struct sockaddr));
    free(packet);

    if (len == -1)
    {
        logg("!DNS Resolver: sendto() failed, %s\n", strerror(errno));
        return NULL;
    }

    if ((len = recvfrom(sockfd, answer, NS_PACKETSZ, 0, (struct sockaddr *) &dns, &addr_len)) == -1)
    {
        logg("!DNS Resolver: recvfrom() failed, %s\n", strerror(errno));
        return NULL;
    }

    /* printf("DNS Resolver: Received %d bytes from the DNS\n", numbytes); */

    if (len < sizeof(HEADER))
    {
        logg("!DNS Resolver: Short reply\n");
        return NULL;
    }

    res = (HEADER *) answer;

    /* All your replies are belong to us? */
    if (ntohs(res->id) != id)
    {
        logg("!DNS Resolver: Bad ID, expected 0x%04x got 0x%04x\n", id, ntohs(res->id));
        return NULL;
    }

    if (!res->qr)
    {
        logg("!DNS Resolver: Bad Reply - reply flag is not set\n");
        return NULL;
    }

    /* Is a reply and result is ok */
    if (res->rcode)
    {
        logg("!DNS Resolver: Bad Reply [%s] - %s (%d)\n", domain, rcode_to_string(res->rcode), res->rcode);
        return NULL;
    }

    if (ntohs(res->ancount) < 1)
    {
        logg("!DNS Resolver: No replies :(\n");
        return NULL;
    }

    answend = answer + len;
    pt = answer + sizeof(HEADER);

    if ((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0)
    {
        logg("!DNS Resolver: dn_expand failed\n");
        return NULL;
    }

    pt += len;

    NS_GET16(type, pt);
    if (type != T_TXT)
    {
        logg("!DNS Resolver: Query in DNS reply is not TXT\n");
        return NULL;
    }

    pt += NS_INT16SZ; /* class */
    size = 0;

    do
    {
        pt += size;
        if ((len = dn_expand(answer, answend, pt, host, sizeof(host))) < 0)
        {
            logg("!DNS Resolver: second dn_expand failed\n");
            return NULL;
        }

        pt += len;
        NS_GET16(type, pt);
        pt += NS_INT16SZ; /* class */
        NS_GET32(cttl, pt);
        NS_GET16(size, pt);

        if (((pt + size) < answer) || ((pt + size) > answend))
        {
            logg("!DNS Resolver: DNS rr overflow\n");
            return NULL;
        }
    } while (type == T_CNAME);

    if (type != T_TXT)
    {
        logg("!Not a TXT record\n");
        return NULL;
    }

    if (!size || ((txtlen = *pt) >= size) || !txtlen)
    {
        logg("!DNS Resolver: Broken TXT record (txtlen = %d, size = %d)\n", txtlen, size);
        return NULL;
    }

    if (!(txtreply = cli_malloc((size_t) txtlen + 1)))
        return NULL;

    memcpy(txtreply, pt + 1, txtlen);
    txtreply[txtlen] = 0;

    if (ttl)
        *ttl = cttl;

    return txtreply;
}

/* Bare Version */
static char *dnsquery_compat(const char *domain, int qtype, unsigned int *ttl)
{
    struct hostent *he;
    char *result, *nameserver;
    int i;

    // FIXME: implement TYPE A?
    if (qtype != T_TXT)
        return NULL;

    if (!(nameserver = get_dns()))
    {
        logg("!DNS Resolver: Cannot find a suitable DNS server\n");
        return NULL;
    }

    if (!(he = gethostbyname(nameserver)))
    {
        logg("!DNS Resolver: gethostbyname(nameserver) failed\n");
        free(nameserver);
        return NULL;
    }

    for (i = 0; i < N_RETRY; i++)
    {
        if ((result = do_query(he, domain, ttl)) != NULL)
            break;
    }

    free(nameserver);
    return result;
}

static
#else
#define dnsquery_dnsapi dnsquery
#endif /* defined(__MINGW32__) && !defined(__MINGW64__) */
char *dnsquery_dnsapi(const char *domain, int qtype, unsigned int *ttl)
{
    PDNS_RECORD pRec, pRecOrig;
    char *result = NULL;

    logg("*DNS Resolver (dnsapi): Querying %s\n", domain);

    if (cw_helpers.dnsapi.DnsQuery_A(domain, (WORD) qtype, DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_DONT_RESET_TTL_VALUES, NULL, &pRec, NULL) != ERROR_SUCCESS)
    {
        logg("!DNS Resolver: Can't query %s\n", domain);
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
            }
        }
        pRec = pRec->pNext;
    }
done:
    cw_helpers.dnsapi.DnsRecordListFree(pRecOrig, DnsFreeRecordList);
    if (!result)
        logg("!Not a TXT record\n");
    return result;
}

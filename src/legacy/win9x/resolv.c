/*
 * Legacy Windows Compatibility Layer: Windows 9x DNS Query
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "platform.h"

#include <iptypes.h>
#include <iphlpapi.h>

#include "resolv.h"
#include "output.h"

#define TCPIP_PARAMS "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"

int res_init(void)
{
    return 0;
}

static char *get_dns_fromreg(void)
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    char data[MAX_PATH];
    DWORD datalen = MAX_PATH - 1;
    char *keys[] = {"ClamWinNameServer", "NameServer", "DhcpNameServer", NULL};

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, TCPIP_PARAMS, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return NULL;

    for (int i = 0; keys[i]; i++)
    {
        datalen = MAX_PATH - 1;
        if ((RegQueryValueExA(hKey, keys[i], NULL, &dwType, (LPBYTE)data, &datalen) == ERROR_SUCCESS) &&
            (datalen > 1) && (dwType == REG_SZ))
        {
            char *space;
            RegCloseKey(hKey);
            if ((space = strchr(data, ' ')))
                *space = 0;
            if (inet_addr(data) == INADDR_NONE)
            {
                logg(LOGG_ERROR, "DNS Resolver: Found %s key: %s - invalid address\n", keys[i], data);
                return NULL;
            }
            else
            {
                logg(LOGG_DEBUG, "DNS Resolver: using %s as DNS server from %s key\n", data, keys[i]);
                return _strdup(data);
            }
        }
    }

    logg(LOGG_ERROR, "DNS Resolver: No nameservers found in registry\n");
    RegCloseKey(hKey);
    return NULL;
}

static char *get_dns(void)
{
    FIXED_INFO *FixedInfo;
    ULONG ulOutBufLen;
    DWORD res;
    char *dns_server = NULL;

    FixedInfo = calloc(1, sizeof(FIXED_INFO));
    ulOutBufLen = sizeof(FIXED_INFO);

    switch (res = GetNetworkParams(FixedInfo, &ulOutBufLen))
    {
    case ERROR_BUFFER_OVERFLOW:
        FixedInfo = realloc(FixedInfo, ulOutBufLen);
    case NO_ERROR:
        break;
    case ERROR_NOT_SUPPORTED:
        logg(LOGG_ERROR, "DNS Resolver: GetNetworkParams() not supported on this OS\n");
        free(FixedInfo);
        return get_dns_fromreg();
    default:
        free(FixedInfo);
        logg(LOGG_ERROR, "DNS Resolver: [1] Call to GetNetworkParams() failed %ld\n", res);
        return NULL;
    }

    if ((res = GetNetworkParams(FixedInfo, &ulOutBufLen)) != ERROR_SUCCESS)
    {
        free(FixedInfo);
        logg(LOGG_ERROR, "DNS Resolver: [2] Call to GetNetworkParams() failed %ld\n", res);
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
    free(FixedInfo);
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
        /* 11 */ "DSO-TYPE Not Implemented"};

    return (rcode < 12) ? rcodes[rcode] : "Unassigned";
}

/* courtesy of musl library, obfuscated c contest? ;)*/
int dn_expand(unsigned char *base, unsigned char *end, unsigned char *src, char *dest, int space)
{
    const unsigned char *p = src;
    char *dend, *dbegin = dest;
    int len = -1, i, j;
    if (p == end || space <= 0)
        return -1;
    dend = dest + (space > 254 ? 254 : space);
    /* detect reference loop using an iteration counter */
    for (i = 0; i < end - base; i += 2)
    {
        /* loop invariants: p<end, dest<dend */
        if (*p & 0xc0)
        {
            if (p + 1 == end)
                return -1;
            j = ((p[0] & 0x3f) << 8) | p[1];
            if (len < 0)
                len = p + 2 - src;
            if (j >= end - base)
                return -1;
            p = base + j;
        }
        else if (*p)
        {
            if (dest != dbegin)
                *dest++ = '.';
            j = *p++;
            if (j >= end - p || j >= dend - dest)
                return -1;
            while (j--)
                *dest++ = *p++;
        }
        else
        {
            *dest = 0;
            if (len < 0)
                len = p + 1 - src;
            return len;
        }
    }
    return -1;
}

int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen)
{
    struct hostent *he;
    char *nameserver;
    struct sockaddr_in dns;
    char *packet;
    HEADER query, *res;
    ssize_t len;
    int start, rev, off;
    uint16_t id;
    int sockfd;
    struct timeval tv;
    int addr_len = sizeof(struct sockaddr);
    int dlen = (int)strlen(dname);

    if (!(nameserver = get_dns()))
    {
        logg(LOGG_ERROR, "DNS Resolver: Cannot find a suitable DNS server\n");
        return -1;
    }

    he = gethostbyname(nameserver);
    free(nameserver);

    if (!he)
    {
        logg(LOGG_ERROR, "DNS Resolver: gethostbyname(nameserver) failed\n");
        return -1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        logg(LOGG_ERROR, "DNS Resolver: socket() failed, %s\n", strerror(errno));
        return -1;
    }

    memset(&dns, 0, sizeof(dns));
    dns.sin_family = AF_INET;
    dns.sin_port = htons(53);
    dns.sin_addr = *((struct in_addr *)he->h_addr);

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

    len = sizeof(query) + NS_INT8SZ + dlen + NS_INT8SZ + (2 * NS_INT16SZ); /* \0 + Type + Class */
    packet = malloc(len);

    memcpy(packet, &query, sizeof(query));
    off = sizeof(query) + NS_INT8SZ;

    start = 0;
    off += dlen;
    rev = start = off;

    /* String-ize */
    for (int i = dlen; i >= 0; i--)
    {
        if (dname[i] != '.')
            packet[rev] = dname[i];
        else
        {
            packet[rev] = (uint8_t)(start - rev - 1);
            start = rev;
        }
        rev--;
    }

    /* First string length */
    packet[rev] = (uint8_t)(start - rev - 1);
    packet[off++] = 0;

    /* Type TXT */
    NS_PUT16(&packet[off], T_TXT);
    off += NS_INT16SZ;

    /* Class */
    NS_PUT16(&packet[off], NS_INT8SZ);
    off += NS_INT16SZ;

    logg(LOGG_DEBUG, "DNS Resolver (compat): Querying %s\n", dname);

    len = sendto(sockfd, packet, (int)len, 0, (struct sockaddr *)&dns, sizeof(struct sockaddr));
    free(packet);

    if (len == -1)
    {
        logg(LOGG_ERROR, "DNS Resolver: sendto() failed, %s\n", strerror(errno));
        return -1;
    }

    if ((len = recvfrom(sockfd, answer, NS_PACKETSZ, 0, (struct sockaddr *)&dns, &addr_len)) == -1)
    {
        logg(LOGG_ERROR, "DNS Resolver: recvfrom() failed, %s\n", strerror(errno));
        return -1;
    }

    // printf("DNS Resolver: Received %d bytes from the DNS\n", len);

    if (len < sizeof(HEADER))
    {
        logg(LOGG_ERROR, "DNS Resolver: Short reply\n");
        return -1;
    }

    res = (HEADER *)answer;

    /* All your replies are belong to us? */
    if (ntohs(res->id) != id)
    {
        logg(LOGG_ERROR, "DNS Resolver: Bad ID, expected 0x%04x got 0x%04x\n", id, ntohs(res->id));
        return -1;
    }

    if (!res->qr)
    {
        logg(LOGG_ERROR, "DNS Resolver: Bad Reply - reply flag is not set\n");
        return -1;
    }

    /* Is a reply and result is ok */
    if (res->rcode)
    {
        logg(LOGG_ERROR, "DNS Resolver: Bad Reply [%s] - %s (%d)\n", dname, rcode_to_string(res->rcode), res->rcode);
        return -1;
    }

    if (ntohs(res->ancount) < 1)
    {
        logg(LOGG_ERROR, "DNS Resolver: No replies\n");
        return -1;
    }

    return len;
}

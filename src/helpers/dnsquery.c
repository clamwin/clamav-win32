/*
 * Clamav Native Windows Port: dns queries for freshclam
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

#include "platform.h"

#include <windns.h>

#include "others.h"
#include "output.h"

char *dnsquery(const char *domain, int qtype, unsigned int *ttl)
{
    PDNS_RECORD pRec, pRecOrig;
    char *result = NULL;

    logg(LOGG_INFO, "DNS Resolver (dnsapi): Querying %s\n", domain);

    if (DnsQuery_A(domain, (WORD)qtype, DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_DONT_RESET_TTL_VALUES, NULL, &pRec, NULL) != ERROR_SUCCESS)
    {
        logg(LOGG_ERROR, "DNS Resolver: Can't query %s\n", domain);
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
                    result = (char *) malloc(len + 1);
                    if (!result)
                    {
                        logg(LOGG_ERROR, "DNS Resolver: Out of memory\n");
                        return NULL;
                    }
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
    DnsRecordListFree(pRecOrig, DnsFreeRecordList);
    if (!result)
        logg(LOGG_INFO, "Not a TXT record\n");
    return result;
}

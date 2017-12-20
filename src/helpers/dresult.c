/*
 * Clamav Native Windows Port: dresult() replacement not using fgets
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
#if 0
#include <platform.h>
#include <output.h>
#include <assert.h>
#include <optparser.h>

extern int notremoved, notmoved;

int dsresult(int sockd, const struct optstruct *opts)
{
    ULONG iMode = 1;
    FD_SET Reader;
    int inq = 1, received = 0, idx = 0, len;
    int infected = 0;
    struct timeval timeout;
    char buffer[8192], line[1024];
    char filename[1024];
    char *found, *pt;

    if ((ioctlsocket(sockd, FIONBIO, &iMode) == SOCKET_ERROR))
    {
        logg("!Cannot set socked in non/block mode\n");
        return -1;
    }

    FD_ZERO(&Reader);
    FD_SET(sockd, &Reader);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while ((inq = select(0, &Reader, NULL, NULL, &timeout) != SOCKET_ERROR))
    {
        if ((inq > 0) && FD_ISSET(sockd, &Reader))
        {
            assert((received >= 0) && (received < sizeof(buffer) - 1));
            if (!((received >= 0) && (received < sizeof(buffer) - 1)))
            {
                logg("!Internal error, please report: received = %d\n", received);
                return -1;
            }

            received = recv(sockd, buffer + received, (sizeof(buffer) - received - 1), 0);
            if (received == SOCKET_ERROR)
            {
                logg("!Error in while receiving data from clamd, %s\n", strerror(errno));
                return -1;
            }

            if (!received) break; /* Done */

            buffer[received] = 0;
            while ((received > 0) && (found = strchr(buffer, '\n')))
            {
                len = (int) (found - buffer);
                received -= len;
                len++; /* \0 */

                if (len >= sizeof(line))
                {
                    logg("!Oversized line received from clamd\n");
                    return -1;
                }
                memcpy(line, buffer, len);
                line[len] = 0;
                found++; received--;

                /* move the remaining buffer data back */
                if (received) memmove(buffer, found, received);
                buffer[received] = 0;

                if(strstr(line, "FOUND\n"))
                {
                    if (!(pt = strrchr(line, ':')))
                    {
                        logg("!Invalid clamd reply\n");
                        return -1;
                    }
                    logg("%s", line);
                    infected++;
                    strncpy(filename, line, pt - line);
                    filename[pt - line] = 0;

                    if (optget(opts, "remove")->enabled)
                        cw_unlink(filename);
                    else
                        cw_moveinfected(filename, optget(opts, "move")->enabled,
                            optget(opts, "move")->strarg, optget(opts, "copy")->strarg, &notmoved, &notremoved);
                }
            }
        }
        FD_ZERO(&Reader);
        FD_SET(sockd, &Reader);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }
    return infected;
}
#endif

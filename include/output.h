/*
 *  Copyright (C) 2013-2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *  Copyright (C) 2007-2013 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifndef __OUTPUT_H
#define __OUTPUT_H

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdlib.h>
#include <sys/types.h>

#ifdef __GNUC__
int mdprintf(int desc, const char *str, ...) __attribute__((format(printf, 2, 3)));
#else
int mdprintf(int desc, const char *str, ...);
#endif

/*
 * legend:
 * NAME          EXPLAIN
 * LOGG_INFO     normal
 * LOGG_INFO_NF  normal, no foreground (logfile and syslog only)
 * LOGG_DEBUG    debug, verbose
 * LOGG_DEBUG_NV debug, non-verbose
 * LOGG_WARNING  warning
 * LOGG_ERROR    ERROR
 */
typedef enum loglevel {
    LOGG_INFO,
    LOGG_INFO_NF,
    LOGG_DEBUG,
    LOGG_DEBUG_NV,
    LOGG_WARNING,
    LOGG_ERROR
} loglevel_t;

/*
 * @param loglevel legend:
 * NAME          EXPLAIN
 * LOGG_INFO     normal
 * LOGG_INFO_NF  normal, no foreground (logfile and syslog only)
 * LOGG_DEBUG    debug, verbose
 * LOGG_DEBUG_NV debug, non-verbose
 * LOGG_WARNING  warning
 * LOGG_ERROR    ERROR
 *
 * @return 0 fur success and -1 for error, e.g. log file access problems
 */
#ifdef __GNUC__
int logg(loglevel_t loglevel, const char *str, ...) __attribute__((format(printf, 2, 3)));
#else
int logg(loglevel_t loglevel, const char *str, ...);
#endif

void logg_close(void);
LIBCLAMAV_EXPORT extern short int logg_verbose, logg_nowarn, logg_lock, logg_time, logg_noflush, logg_rotate;
LIBCLAMAV_EXPORT extern off_t logg_size;
LIBCLAMAV_EXPORT extern const char *logg_file;

#ifdef __GNUC__
void mprintf(loglevel_t loglevel, const char *str, ...) __attribute__((format(printf, 2, 3)));
#else
void mprintf(loglevel_t loglevel, const char *str, ...);
#endif

LIBCLAMAV_EXPORT extern short int mprintf_disabled, mprintf_verbose, mprintf_quiet, mprintf_nowarn, mprintf_stdout, mprintf_send_timeout, mprintf_progress;

#endif

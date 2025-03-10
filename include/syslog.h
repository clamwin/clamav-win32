/*
 * Clamav Native Windows Port: syslog emulation
 *
 * Copyright (c) 2008-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#ifndef _SYSLOG_H_
#define _SYSLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#define LOG_PID 0x01
#define LOG_LOCAL6 (22<<3)

	void openlog(const char* ident, int opt, int facility);
	void closelog();
	void syslog(int priority, const char* message, ...);

#ifdef __cplusplus
}
#endif

#endif /* _SYSLOG_H_ */

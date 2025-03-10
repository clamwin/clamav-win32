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

#include "syslog.h"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#define FACILITY_CUSTOM 0x001  

 // Macro to build a 32-bit event identifier:
 // Format: Bits 31-30: Severity, Bit 29: Customer flag (0), Bits 28-16: Facility, Bits 15-0: Code.
#define MAKE_EVENT_ID(severity, facility, code) \
    (((severity & 0x3) << 30) | (((facility) & 0xFFF) << 16) | ((code) & 0xFFFF))

static HANDLE hEventLog;

WORD MapSyslogToWindowsEventType(int syslogLevel)
{
	switch (syslogLevel)
	{
	case LOG_EMERG:    // Emergency - highest severity
	case LOG_CRIT:     // Critical error conditions
	case LOG_ERR:      // Standard error message
		return EVENTLOG_ERROR_TYPE;       // Windows error event type (0x0001)

	case LOG_ALERT:    // Action must be taken immediately
		return EVENTLOG_AUDIT_FAILURE;      // Audit failure event type (0x0010)

	case LOG_WARNING:  // Warning messages
		return EVENTLOG_WARNING_TYPE;       // Windows warning event type (0x0002)

	case LOG_NOTICE:   // Normal but significant condition
		return EVENTLOG_AUDIT_SUCCESS;      // Audit success event type (0x0008)

	case LOG_INFO:     // Informational messages
	case LOG_DEBUG:    // Debug messages
	default:
		return EVENTLOG_INFORMATION_TYPE;   // Informational event type (0x0004)
	}
}

DWORD CreateCustomEventID(WORD wType)
{
	DWORD severity;
	switch (wType)
	{
	case EVENTLOG_ERROR_TYPE:
	case EVENTLOG_AUDIT_FAILURE:
		severity = 3;  // Error
		break;
	case EVENTLOG_WARNING_TYPE:
		severity = 2;  // Warning
		break;
	case EVENTLOG_INFORMATION_TYPE:
	case EVENTLOG_AUDIT_SUCCESS:
	default:
		severity = 1;  // Informational
		break;
	}
	DWORD code = 1000;
	return MAKE_EVENT_ID(severity, FACILITY_CUSTOM, code);
}

void openlog(const char* ident, int opt, int facility)
{
	if (!(hEventLog = RegisterEventSourceA(NULL, ident)))
		fprintf(stderr, "RegisterEventSourceA() failed with %ld\n", GetLastError());
}

void closelog()
{
	if (hEventLog)
		DeregisterEventSource(hEventLog);
}

static void _vsyslog(int priority, const char* message, va_list ap)
{
	char buffer[1024];

	if (vsnprintf(buffer, sizeof(buffer), message, ap) < 0) {
		fprintf(stderr, "vsnprintf failed\n");
		buffer[0] = '\0';
	}

	WORD wType = MapSyslogToWindowsEventType(priority);
	DWORD eventID = CreateCustomEventID(wType);
	LPCSTR lpStrings[1] = { buffer };

	if (!ReportEventA(hEventLog, wType, 0, eventID, NULL, 1, 0, lpStrings, NULL))
		fprintf(stderr, "RegisterEventSourceA() failed with %ld\n", GetLastError());
}

void syslog(int priority, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	_vsyslog(priority, message, ap);
	va_end(ap);
}

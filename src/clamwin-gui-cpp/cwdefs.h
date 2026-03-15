/*
 * ClamWin Free Antivirus — Platform version defines
 *
 * Must be included before <windows.h> to set WINAPI target correctly.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#ifndef CWDEFS_H
#define CWDEFS_H

/* Target Windows XP (5.1) minimum — for InitCommonControlsEx, etc. */
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501
#endif

#ifndef WINVER
#  define WINVER 0x0501
#endif

/* Require IE5.0+ headers for balloon tips, even on Win98 */
#ifndef _WIN32_IE
#  define _WIN32_IE 0x0500
#endif

/* Suppress rarely-used Windows headers */
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif

#endif /* CWDEFS_H */

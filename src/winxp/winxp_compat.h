#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdlib.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A000002
#include <windows.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP

#include <wchar.h>
#include <strsafe.h>

#if 1// def TRACE_COMPAT
#include <stdio.h>
#define TRACE(format, ...) fwprintf(stderr, L"[winxp] " format, ##__VA_ARGS__)
#else
#define TRACE(format, ...)
#endif

#endif

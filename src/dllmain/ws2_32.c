/* MSVC import thunk fallback; MinGW uses the legacy compatibility library. */
#ifdef _MSC_VER
#define NO_FUNCTION_REDEFINITION
#include "../legacy/shared/ws2_32.c"
#include "dynload.h"

imp_GetHostNameW pGetHostNameW = GetHostNameW_compat;
#endif

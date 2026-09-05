/*
 * Clamav Native Windows Port: delayload hook
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

#ifdef _MSC_VER
#include <windows.h>
#include <delayimp.h>

#define RtlGenRandom SystemFunction036
BOOLEAN WINAPI RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);

BOOL WINAPI ProcessPrng_compat(void *buffer, size_t size)
{
    return RtlGenRandom(buffer, (ULONG)size);
}

#define NO_FUNCTION_REDEFINITION
#define WRAP_SUFFIX _compat
#include "../legacy/shared/synchapi.c"

FARPROC WINAPI MyDelayLoadFailureHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    if ((dliNotify != dliFailGetProc) || (!pdli->dlp.szProcName))
        return NULL;

    if (strcmp(pdli->dlp.szProcName, "ProcessPrng") == 0)
        return (FARPROC)ProcessPrng_compat;

    if (strcmp(pdli->dlp.szProcName, "WakeByAddressSingle") == 0)
        return (FARPROC)WakeByAddressSingle_compat;

    if (strcmp(pdli->dlp.szProcName, "WaitOnAddress") == 0)
        return (FARPROC)WaitOnAddress_compat;

    if (strcmp(pdli->dlp.szProcName, "WakeByAddressAll") == 0)
        return (FARPROC)WakeByAddressAll_compat;

    MessageBoxA(NULL, pdli->dlp.szProcName, "Missing symbol", MB_OK | MB_ICONERROR);
    abort();
}

ExternC const PfnDliHook __pfnDliFailureHook2 = MyDelayLoadFailureHook;
#endif // _MSC_VER

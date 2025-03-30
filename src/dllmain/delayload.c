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

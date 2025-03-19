#include "rar.hpp"

DWORD WinNT()
{
    static int dwPlatformId = -1;
    static DWORD dwMajorVersion, dwMinorVersion;

    if (dwPlatformId == -1)
    {
        OSVERSIONINFO WinVer;
        WinVer.dwOSVersionInfoSize = sizeof(WinVer);
        GetVersionEx(&WinVer);
        dwPlatformId = WinVer.dwPlatformId;
        dwMajorVersion = WinVer.dwMajorVersion;
        dwMinorVersion = WinVer.dwMinorVersion;
    }

    DWORD Result = 0;
    if (dwPlatformId == VER_PLATFORM_WIN32_NT)
        Result = dwMajorVersion * 0x100 + dwMinorVersion;

    return Result;
}

bool IsWindows11OrGreater()
{
#if _WIN32_WINNT >= _WIN32_WINNT_WINXP
    OSVERSIONINFOEXW vi = {0};
    vi.dwOSVersionInfoSize = sizeof(vi);
    vi.dwMajorVersion = 10;
    vi.dwMinorVersion = 0;
    // Use build 21996 as a threshold (any build >= 22000 indicates Windows 11)
    vi.dwBuildNumber = 22000;

    ULONGLONG cond = 0;
    cond = VerSetConditionMask(cond, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);

    return VerifyVersionInfoW(&vi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, cond);
#else
    return false;
#endif
}

#if _WIN32_WINNT < _WIN32_WINNT_WINXP
#include "dynload.h"

DWORD WINAPI GetLongPathNameW_compat(LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer)
{
    return 0;
}

imp_GetLongPathNameW pGetLongPathNameW = GetLongPathNameW_compat;

#ifdef __GNUC__
__attribute__((constructor))
#endif
static void
init()
{
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32"));
    IMPORT_KERNEL32_FUNC(GetLongPathNameW);
}
#endif

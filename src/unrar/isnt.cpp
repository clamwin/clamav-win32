#include <libclamunrar/rar.hpp>

DWORD WinNT()
{
  static int dwPlatformId=-1;
  static DWORD dwMajorVersion,dwMinorVersion;
  if (dwPlatformId==-1)
  {
    OSVERSIONINFO WinVer;
    WinVer.dwOSVersionInfoSize=sizeof(WinVer);
    GetVersionEx(&WinVer);
    dwPlatformId=WinVer.dwPlatformId;
    dwMajorVersion=WinVer.dwMajorVersion;
    dwMinorVersion=WinVer.dwMinorVersion;

  }
  DWORD Result=0;
  if (dwPlatformId==VER_PLATFORM_WIN32_NT)
    Result=dwMajorVersion*0x100+dwMinorVersion;

  return Result;
}

// Replace it with actual check when available.
bool IsWindows11OrGreater()
{
  return false;
}

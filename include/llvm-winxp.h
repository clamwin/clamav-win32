#ifndef LLVM_WINXP_H
#define LLVM_WINXP_H

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN

#define GetFinalPathNameByHandleW NO_GetFinalPathNameByHandleW
#define SetFileInformationByHandle NO_SetFileInformationByHandle
#define RegGetValueW NO_RegGetValueW
#include <windows.h>
#include <cstdlib>

#undef GetFinalPathNameByHandleW
#undef SetFileInformationByHandle
#undef RegGetValueW

static inline DWORD GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    abort();
}

static inline BOOL SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    abort();
}

static inline long RegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData)
{
    abort();
}

#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        printf("Invalid arguments\n");
        return 1;
    }

    HANDLE hFile = CreateFileW(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFileW (%ld)\n", GetLastError());
        return 1;
    }

    wchar_t lpszFilePath[MAX_PATH];

    if (GetFinalPathNameByHandleW(hFile, lpszFilePath, MAX_PATH, VOLUME_NAME_DOS))
        wprintf(L"Result ->[%ls]\n", lpszFilePath);
    else
        wprintf(L"GetFinalPathNameByHandleW() failed with %d\n", GetLastError());

    CloseHandle(hFile);

    return 0;
}

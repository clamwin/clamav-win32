#define ReOpenFile NO_ReOpenFile
#include <windows.h>
#undef ReOpenFile
#include <stdio.h>

HANDLE WINAPI ReOpenFile(HANDLE hOriginalFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes);

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        wprintf(L"Invalid arguments\n");
        return 1;
    }

    // Step 1: Create original file handle with more permissive sharing
    HANDLE hFileOriginal = CreateFileW(
        argv[1],                            // File name
        GENERIC_READ,                       // Access mode (read only)
        FILE_SHARE_READ | FILE_SHARE_WRITE, // More permissive share mode
        NULL,                               // Security attributes
        OPEN_ALWAYS,                        // Creation disposition
        FILE_ATTRIBUTE_NORMAL,              // File attributes
        NULL                                // Template file
    );

    if (hFileOriginal == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed with error %ld\n", GetLastError());
        return 1;
    }

    printf("Original file opened with read access\n");

    // Step 2: Reopen the file with write access
    HANDLE hFileReopened = ReOpenFile(
        hFileOriginal,                      // Original handle
        GENERIC_WRITE,                      // New desired access (write)
        FILE_SHARE_READ | FILE_SHARE_WRITE, // New share mode
        FILE_FLAG_WRITE_THROUGH             // New flags
    );

    if (hFileReopened == INVALID_HANDLE_VALUE)
    {
        printf("ReOpenFile failed with error %ld\n", GetLastError());
        CloseHandle(hFileOriginal);
        return 1;
    }

    printf("File reopened with write access\n");

    // Clean up
    CloseHandle(hFileOriginal);
    CloseHandle(hFileReopened);

    return 0;
}

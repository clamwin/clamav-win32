#include <windows.h>
#include <stdio.h>
#include <winternl.h>

typedef enum _FILE_INFO_BY_HANDLE_CLASS
{
    FileBasicInfo,
    FileStandardInfo,
    FileNameInfo,
    FileRenameInfo
} FILE_INFO_BY_HANDLE_CLASS,
    *PFILE_INFO_BY_HANDLE_CLASS;

typedef struct _FILE_RENAME_INFO
{
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFO, *PFILE_RENAME_INFO;

WINBOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 3)
    {
        wprintf(L"Invalid arguments\n");
        return 1;
    }

    // Path to the file we want to rename
    const wchar_t *oldFileName = argv[1];
    // New file name (not path, just the name)
    const wchar_t *newFileName = argv[2];

    // Open the file with required permissions
    HANDLE hFile = CreateFileW(
        oldFileName,
        DELETE,                             // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // Share mode
        NULL,                               // Security attributes
        OPEN_EXISTING,                      // Open existing file
        FILE_ATTRIBUTE_NORMAL,              // File attributes
        NULL                                // Template file handle
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Failed to open file. Error code: %lu\n", GetLastError());
        return 1;
    }

    // Prepare the rename info structure
    DWORD bufferSize = sizeof(FILE_RENAME_INFO) + (wcslen(newFileName) * sizeof(WCHAR));
    FILE_RENAME_INFO *renameInfo = malloc(bufferSize);

    if (!renameInfo)
    {
        wprintf(L"Memory allocation failed\n");
        CloseHandle(hFile);
        return 1;
    }

    // Fill the structure
    renameInfo->ReplaceIfExists = FALSE; // Don't replace if file with new name exists
    renameInfo->RootDirectory = NULL;    // Not using a root directory handle
    renameInfo->FileNameLength = wcslen(newFileName) * sizeof(WCHAR);
    memcpy(renameInfo->FileName, newFileName, renameInfo->FileNameLength);

    if (SetFileInformationByHandle(hFile, FileRenameInfo, renameInfo, bufferSize))
        wprintf(L"File renamed successfully to %ls\n", newFileName);
    else
        wprintf(L"Failed to rename file. Error code: %lu\n", GetLastError());

    // Clean up
    free(renameInfo);
    CloseHandle(hFile);

    return 0;
}

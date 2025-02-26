#define CompareStringOrdinal NO_CompareStringOrdinal
#define GetFileInformationByHandleEx NO_GetFileInformationByHandleEx
#define SetFileInformationByHandle NO_SetFileInformationByHandle
#define GetFinalPathNameByHandleW NO_GetFinalPathNameByHandleW
#include "winxp_compat.h"
#undef CompareStringOrdinal
#undef GetFileInformationByHandleEx
#undef SetFileInformationByHandle
#undef GetFinalPathNameByHandleW

#include <fileapi.h>
#include <ntdef.h>
#include <winternl.h>
#include <psapi.h>
#include <strsafe.h>
#include <ddk/mountmgr.h>

DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

int WINAPI CompareStringOrdinal(
    LPCWCH lpString1,
    int cchCount1,
    LPCWCH lpString2,
    int cchCount2,
    WINBOOL bIgnoreCase)
{
    int i, minCount;
    WCHAR ch1, ch2;

    TRACE(L"CompareStringOrdinal(%ls, %d, %ls, %d, %d)\n", lpString1, cchCount1, lpString2, cchCount2, bIgnoreCase);

    // Validate input parameters.
    if (!lpString1 || !lpString2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // If the count is negative, assume the string is null-terminated.
    if (cchCount1 < 0)
        cchCount1 = lstrlenW(lpString1);
    if (cchCount2 < 0)
        cchCount2 = lstrlenW(lpString2);

    // Determine the number of characters to compare.
    minCount = (cchCount1 < cchCount2) ? cchCount1 : cchCount2;

    // Compare each character.
    for (i = 0; i < minCount; i++)
    {
        ch1 = lpString1[i];
        ch2 = lpString2[i];

        if (bIgnoreCase)
        {
            ch1 = towlower(ch1);
            ch2 = towlower(ch2);
        }

        if (ch1 < ch2)
            return CSTR_LESS_THAN;
        if (ch1 > ch2)
            return CSTR_GREATER_THAN;
    }

    // If all compared characters are equal, determine result based on string lengths.
    if (cchCount1 < cchCount2)
        return CSTR_LESS_THAN;
    if (cchCount1 > cchCount2)
        return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}

// Fallback implementation of GetFileInformationByHandleEx for Windows XP
WINBOOL WINAPI GetFileInformationByHandleEx(HANDLE hFile,
                                            FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                            LPVOID lpFileInformation,
                                            DWORD dwBufferSize)
{
    TRACE(L"GetFileInformationByHandleEx(0x%p, %d, 0x%p, %d)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);

    // Validate input parameters
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL)
    {
        TRACE(L"GetFileInformationByHandleEx -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (FileInformationClass == FileBasicInfo)
    {
        // Check if the provided buffer is large enough for FILE_BASIC_INFO
        if (dwBufferSize < sizeof(FILE_BASIC_INFO))
        {
            TRACE(L"GetFileInformationByHandleEx -> ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        FILE_BASIC_INFO *pInfo = (FILE_BASIC_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE(L"GetFileInformationByHandleEx[FileBasicInfo] -> GetFileInformationByHandle Failed (%d)\n", GetLastError());
            return FALSE;
        }

        // Copy the time and attribute data from the handle information
        pInfo->CreationTime = *(LARGE_INTEGER *)&fileInfo.ftCreationTime;
        pInfo->LastAccessTime = *(LARGE_INTEGER *)&fileInfo.ftLastAccessTime;
        pInfo->LastWriteTime = *(LARGE_INTEGER *)&fileInfo.ftLastWriteTime;
        // Windows XP does not provide a ChangeTime; using LastWriteTime as a fallback
        pInfo->ChangeTime = *(LARGE_INTEGER *)&fileInfo.ftLastWriteTime;
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        return TRUE;
    }
    else if (FileInformationClass == FileStandardInfo)
    {
        // Check if the provided buffer is large enough for FILE_STANDARD_INFO
        if (dwBufferSize < sizeof(FILE_STANDARD_INFO))
        {
            TRACE(L"GetFileInformationByHandleEx[FileStandardInfo] -> ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_STANDARD_INFO *pInfo = (FILE_STANDARD_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE(L"GetFileInformationByHandleEx[FileStandardInfo] -> GetFileInformationByHandle Failed (%d)\n", GetLastError());
            return FALSE;
        }

        // Compute the file size from its low and high parts
        LARGE_INTEGER fileSize;
        fileSize.LowPart = fileInfo.nFileSizeLow;
        fileSize.HighPart = fileInfo.nFileSizeHigh;

        pInfo->AllocationSize = fileSize;
        pInfo->EndOfFile = fileSize;
        pInfo->NumberOfLinks = fileInfo.nNumberOfLinks;
        pInfo->DeletePending = FALSE; // Not determinable via this API
        pInfo->Directory = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
        return TRUE;
    }
    else if (FileInformationClass == FileAttributeTagInfo)
    {
        // Check if the provided buffer is large enough for FILE_ATTRIBUTE_TAG_INFO
        if (dwBufferSize < sizeof(FILE_ATTRIBUTE_TAG_INFO))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_ATTRIBUTE_TAG_INFO *pInfo = (FILE_ATTRIBUTE_TAG_INFO *)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
        {
            TRACE(L"GetFileInformationByHandleEx[FileAttributeTagInfo] -> GetFileInformationByHandle Failed (%d)\n", GetLastError());
            return FALSE;
        }

        // Copy the file attributes from the handle information
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        pInfo->ReparseTag = 0; // Default value if not a reparse point

        // If the file is a reparse point, try to retrieve the reparse tag
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            // Allocate a buffer for the reparse point data.
            // Although the maximum size is defined by MAXIMUM_REPARSE_DATA_BUFFER,
            // we use a static buffer of 16 KB for simplicity.
            BYTE buffer[16 * 1024];
            DWORD dwBytesReturned = 0;
            if (DeviceIoControl(hFile,
                                FSCTL_GET_REPARSE_POINT,
                                NULL,
                                0,
                                buffer,
                                sizeof(buffer),
                                &dwBytesReturned,
                                NULL))
            {
                // Cast the buffer to a REPARSE_DATA_BUFFER pointer to retrieve the ReparseTag.
                // Note: REPARSE_DATA_BUFFER has a variable layout; here we only extract the ReparseTag.
                pInfo->ReparseTag = ((PREPARSE_DATA_BUFFER)buffer)->ReparseTag;
            }
            // If DeviceIoControl fails, ReparseTag remains 0.
        }
        return TRUE;
    }
    else
    {
        // Unsupported FileInformationClass
        TRACE(L"GetFileInformationByHandleEx: Unsupported FileInformationClass %d\n", FileInformationClass);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

WINBOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    TRACE(L"SetFileInformationByHandle(0x%p, %d, 0x%p, %d)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);

    // Validate parameters.
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL)
    {
        TRACE(L"SetFileInformationByHandle: hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (FileInformationClass)
    {
    case FileBasicInfo:
    {
        if (dwBufferSize < sizeof(FILE_BASIC_INFO))
        {
            TRACE(L"SetFileInformationByHandle[FileBasicInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        PFILE_BASIC_INFO pBasicInfo = (PFILE_BASIC_INFO)lpFileInformation;
        // Update file times using SetFileTime.
        // Note: SetFileTime only allows updating Creation, LastAccess, and LastWrite times.
        if (!SetFileTime(
                hFile,
                (const FILETIME *)&pBasicInfo->CreationTime,
                (const FILETIME *)&pBasicInfo->LastAccessTime,
                (const FILETIME *)&pBasicInfo->LastWriteTime))
        {
            // If SetFileTime fails, it sets the proper error code.
            TRACE(L"SetFileInformationByHandle[FileBasicInfo]->SetFileTime Failed (%d)\n", GetLastError());
            return FALSE;
        }
        // Note: pBasicInfo->ChangeTime and pBasicInfo->FileAttributes are not supported.
        return TRUE;
    }
    case FileRenameInfo:
    case FileRenameInfoEx:
    {
        if (dwBufferSize < sizeof(FILE_RENAME_INFO))
        {
            TRACE(L"SetFileInformationByHandle[FileRenameInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        UNICODE_STRING NtPathName = {0, 0, 0};
        FILE_RENAME_INFO *pInputRenameInfo = (FILE_RENAME_INFO *)lpFileInformation;
        PFILE_RENAME_INFO pRename = (PFILE_RENAME_INFO)lpFileInformation;

        // Only support RootDirectory == NULL.
        if (pRename->RootDirectory != NULL)
        {
            TRACE(L"SetFileInformationByHandle[FileRenameInfo]: pRename->RootDirectory != NULL\n");
            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        NtPathName.MaximumLength = (USHORT)pInputRenameInfo->FileNameLength;
        NtPathName.Length = (USHORT)pInputRenameInfo->FileNameLength;
        NtPathName.Buffer = pInputRenameInfo->FileName;

        ULONG size = (NtPathName.Length * sizeof(wchar_t)) + sizeof(FILE_RENAME_INFO);
        FILE_RENAME_INFO *RenameBuffer = malloc(size);
        if (!RenameBuffer)
        {
            TRACE(L"SetFileInformationByHandle[FileRenameInfo]: ERROR_OUTOFMEMORY\n");
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        memcpy(RenameBuffer->FileName, NtPathName.Buffer, NtPathName.Length * sizeof(wchar_t));
        RenameBuffer->FileName[NtPathName.Length] = L'\0';

        wchar_t sourcePath[MAX_PATH];
        GetFinalPathNameByHandleW(hFile, sourcePath, MAX_PATH, 0);
        TRACE(L"[%ls] -> [%ls]\n", sourcePath, RenameBuffer->FileName);

#if 1
        DWORD moveFlags = 0;
        if (pRename->ReplaceIfExists)
            moveFlags |= MOVEFILE_REPLACE_EXISTING;

        BOOL result = MoveFileExW(sourcePath, RenameBuffer->FileName, moveFlags);
        free(RenameBuffer);
        return result;
#else
        RenameBuffer->ReplaceIfExists = pInputRenameInfo->ReplaceIfExists;
        RenameBuffer->RootDirectory = pInputRenameInfo->RootDirectory;
        RenameBuffer->FileNameLength = NtPathName.Length;
        IO_STATUS_BLOCK IoStatusBlock;

        // 0xc0000033 STATUS_OBJECT_NAME_INVALID
        // 0xc00000cb on wine
        // SHARING VIOLATION on ProcMon
        TRACE(L"RenameBuffer->FileName -> %ls\n", RenameBuffer->FileName);
        NTSTATUS ntRes = NtSetInformationFile(hFile,
                                              &IoStatusBlock,
                                              RenameBuffer,
                                              size,
                                              FileRenameInformation);

        if (NtPathName.Buffer != pInputRenameInfo->FileName)
            free(NtPathName.Buffer);

        if (!NT_SUCCESS(ntRes))
        {
            TRACE(L"SetFileInformationByHandle[FileRenameInfo]: NtSetInformationFile failed (0x%08x)\n", ntRes);
            SetLastError(RtlNtStatusToDosError(ntRes));
            return FALSE;
        }

        return TRUE;
#endif
    }
    default:
        TRACE(L"SetFileInformationByHandle: Unsupported FileInformationClass %d\n", FileInformationClass);
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
}

// https://github.com/zeroclear/xpext/blob/master/xpext_ver4/k32_file.cpp#L445
// https://stackoverflow.com/questions/65170/how-to-get-name-associated-with-open-handle/5286888#5286888
union ANY_BUFFER
{
    MOUNTMGR_TARGET_NAME TargetName;
    MOUNTMGR_VOLUME_PATHS TargetPaths;
    FILE_NAME_INFORMATION NameInfo;
    UNICODE_STRING UnicodeString;
    WCHAR Buffer[USHRT_MAX];
};

// This implementation supports only basic DOS paths
DWORD WINAPI
GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    TRACE(L"GetFinalPathNameByHandleW(0x%p, 0x%p, %d, %d)\n", hFile, lpszFilePath, cchFilePath, dwFlags);

    // Validate input parameters.
    if (hFile == INVALID_HANDLE_VALUE)
    {
        TRACE(L"GetFinalPathNameByHandleW: -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwFlags != VOLUME_NAME_DOS)
    {
        TRACE(L"GetFinalPathNameByHandleW: Unsupported dwFlags 0x%08x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // FIXME: too big?
    union ANY_BUFFER nameFull, nameRel, nameMnt;

    NTSTATUS status = NtQueryObject(hFile, ObjectNameInformation, nameFull.Buffer, sizeof(nameFull.Buffer), NULL);
    if (!NT_SUCCESS(status))
    {
        TRACE(L"GetFinalPathNameByHandleW->NtQueryObject failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    IO_STATUS_BLOCK iosb;
    status = NtQueryInformationFile(hFile, &iosb, nameRel.Buffer, sizeof(nameRel.Buffer), FileNameInformation);

    if (!NT_SUCCESS(status))
    {
        TRACE(L"GetFinalPathNameByHandleW->NtQueryInformationFile failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    if (nameFull.UnicodeString.Length < nameRel.NameInfo.FileNameLength)
    {
        TRACE(L"nameFull.UnicodeString.Length < nameRel.NameInfo.FileNameLength\n");
        // FIXME: WTF
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    nameMnt.TargetName.DeviceNameLength = nameFull.UnicodeString.Length - nameRel.NameInfo.FileNameLength;
    wcsncpy(nameMnt.TargetName.DeviceName,
            nameFull.UnicodeString.Buffer,
            nameMnt.TargetName.DeviceNameLength / sizeof(wchar_t));

    HANDLE hDevice = CreateFileW(
        MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        TRACE(L"GetFinalPathNameByHandleW->CreateFileW(MOUNTMGR_DOS_DEVICE_NAME): failed (%d)\n", GetLastError());
        return 0;
    }

    TRACE(L"DevicePath: [%ls]\n", nameMnt.TargetName.DeviceName);
    TRACE(L"FileName: [%ls]\n", nameRel.NameInfo.FileName);

    DWORD rl;
    BOOL success = DeviceIoControl(hDevice,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                   &nameMnt,
                                   sizeof(nameMnt),
                                   &nameMnt,
                                   sizeof(nameMnt),
                                   &rl,
                                   NULL);

    if (success)
        CloseHandle(hDevice);

    int cchReq = 0;
    int nameLength = nameRel.NameInfo.FileNameLength / sizeof(wchar_t);

    if (success)
    {
        if (nameMnt.TargetPaths.MultiSzLength == 0)
        {
            TRACE(L"GetFinalPathNameByHandleW->DeviceIoControl nameMnt.TargetPaths.MultiSzLength == 0\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
        TRACE(L"Matched MountMgr: %ls\n", nameRel.NameInfo.FileName);
        wcsncat(nameMnt.TargetPaths.MultiSz, nameRel.NameInfo.FileName, nameLength);
        cchReq = wcslen(nameMnt.TargetPaths.MultiSz);
    }
    else
    {
        DWORD le = GetLastError();
        if ((le != ERROR_INVALID_FUNCTION) && (le != ERROR_NOT_SUPPORTED))
        {
            TRACE(L"GetFinalPathNameByHandleW->DeviceIoControl failed (%d)\n", le);
            return 0;
        }

        DWORD dwSize = MAX_PATH;
        wchar_t szLogicalDrives[MAX_PATH] = {0};
        DWORD dwResult = GetLogicalDriveStringsW(dwSize, szLogicalDrives);

        if (!dwResult || dwResult > MAX_PATH)
        {
            TRACE(L"GetFinalPathNameByHandleW->GetLogicalDriveStringsW failed or oversize (%d)\n", GetLastError());
            return 0;
        }

        wchar_t TargetDevice[MAX_PATH + 1];
        wchar_t *drive = szLogicalDrives;
        while (*drive)
        {
            wchar_t driveLetter[3] = {drive[0], drive[1], L'\0'};
            if (QueryDosDeviceW(driveLetter, TargetDevice, sizeof(TargetDevice)))
            {
                TRACE(L"%ls is {%ls}\n", driveLetter, TargetDevice);
                if (wcsncmp(TargetDevice, nameMnt.TargetName.DeviceName, nameMnt.TargetName.DeviceNameLength / sizeof(wchar_t)) == 0)
                {
                    wcsncpy(nameMnt.TargetPaths.MultiSz, driveLetter, sizeof(driveLetter));
                    int off;

                    if (TargetDevice == wcsstr(TargetDevice, L"\\Device\\LanmanRedirector\\;"))
                    {
                        /* \Device\LanmanRedirector\;C:0000000000000000\Complete Path\To File.ext */
                        if ((TargetDevice[26] == drive[0]) && (TargetDevice[27] == ':'))
                        {
                            wchar_t *path = wcschr(&TargetDevice[28], L'\\');
                            if (path == NULL)
                            {
                                SetLastError(ERROR_BAD_PATHNAME);
                                return 0;
                            }
                            TRACE(L"Matched LanmanRedirector: %ls\n", path);
                            off = wcslen(path);
                        }
                        else
                        {
                            TRACE(L"LanmanRedirector: Invalid pattern\n");
                            SetLastError(ERROR_BAD_PATHNAME);
                            return 0;
                        }
                    }
                    else // \Device\VBoxMiniRdr\;Z:\VBoxSvr\shared
                    {
                        TRACE(L"Matched Network Provider: %ls\n", driveLetter, nameMnt.TargetName.DeviceName);
                        wchar_t *semicolon = wcschr(TargetDevice, L';');
                        off = semicolon ? wcslen(semicolon + 3) : 0; // Z:\ (3)
                    }

                    wcsncat(nameMnt.TargetPaths.MultiSz, nameRel.NameInfo.FileName + off, nameLength - off);
                    cchReq = wcslen(nameMnt.TargetPaths.MultiSz);
                    break;
                }
            }
            drive += wcslen(drive) + 1;
        }

        if (!cchReq)
        {
            TRACE(L"DosPath Not Found\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (lpszFilePath && (cchFilePath >= cchReq))
    {
        wcsncpy(lpszFilePath, nameMnt.TargetPaths.MultiSz, cchReq);
        lpszFilePath[cchReq] = L'\0';
    }

    TRACE(L"GetFinalPathNameByHandleW->%ls (%d)\n", nameMnt.TargetPaths.MultiSz, cchReq);
    // Return the length of the final path (excluding the terminating null).

    return cchReq;
}

WINBASEAPI HANDLE WINAPI ReOpenFile(
    HANDLE hOriginalFile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwFlags)
{
    TRACE(L"ReOpenFile(0x%p, 0x%08x, 0x%08x, 0x%08x)\n", hOriginalFile, dwDesiredAccess, dwShareMode, dwFlags);

    // Validate the original handle.
    if (hOriginalFile == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    // Retrieve the file's final path.
    WCHAR filePath[MAX_PATH] = {0};
    DWORD ret = GetFinalPathNameByHandleW(hOriginalFile, filePath, MAX_PATH, VOLUME_NAME_DOS);
    if (ret == 0 || ret > MAX_PATH)
    {
        // Could not retrieve the path.
        return INVALID_HANDLE_VALUE;
    }

    // Remove any "\\?\" prefix if present. CreateFileW cannot use paths with this prefix.
    WCHAR *pPath = filePath;
    if (wcsncmp(filePath, L"\\\\?\\", 4) == 0)
    {
        pPath += 4;
        // Special handling for UNC paths: a UNC path may start as "\\?\UNC\server\share..."
        if (wcsncmp(pPath, L"UNC\\", 4) == 0)
        {
            pPath += 3; // Skip "UNC"
            // Prepend "\\" to form a standard UNC path.
            WCHAR uncPath[MAX_PATH];
            HRESULT hr = StringCchPrintfW(uncPath, MAX_PATH, L"\\\\%s", pPath);
            if (FAILED(hr))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
            }
            hr = StringCchCopyW(filePath, MAX_PATH, uncPath);
            if (FAILED(hr))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
            }
            pPath = filePath;
        }
    }

    // Reopen the file using CreateFileW with the new parameters.
    HANDLE hNew = CreateFileW(
        pPath,
        dwDesiredAccess,
        dwShareMode,
        NULL,          // default security attributes
        OPEN_EXISTING, // file must exist
        dwFlags,       // flags and attributes for the new handle
        NULL           // no template file
    );

    return hNew;
}

WINBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
    // Static variables to hold the baseline values.
    // They are initialized on the first call.
    static volatile LONG initialized = 0;
    static LARGE_INTEGER qpcBase = {0};      // Baseline performance counter value.
    static FILETIME ftBase = {0};            // Baseline system time corresponding to qpcBase.
    static LARGE_INTEGER qpcFrequency = {0}; // Performance counter frequency.

    TRACE(L"GetSystemTimePreciseAsFileTime(0x%p)\n", lpSystemTimeAsFileTime);

    // If not yet initialized, set the baseline.
    if (initialized == 0)
    {
        // Retrieve the performance counter frequency.
        QueryPerformanceFrequency(&qpcFrequency);
        // Record the current performance counter value.
        QueryPerformanceCounter(&qpcBase);
        // Retrieve the system time as a FILETIME.
        GetSystemTimeAsFileTime(&ftBase);
        // Mark as initialized.
        InterlockedExchange(&initialized, 1);
    }

    // Get the current performance counter value.
    LARGE_INTEGER qpcNow = {0};
    QueryPerformanceCounter(&qpcNow);

    // Calculate the difference in counter ticks.
    LONGLONG ticksElapsed = qpcNow.QuadPart - qpcBase.QuadPart;
    // Convert the tick difference to 100-nanosecond intervals.
    // 1 second = 10,000,000 (10^7) 100-ns intervals.
    ULONGLONG timeOffset = (ULONGLONG)((ticksElapsed * 10000000ULL) / qpcFrequency.QuadPart);

    // Convert the baseline FILETIME to a 64-bit integer.
    ULONGLONG baseTime = (((ULONGLONG)ftBase.dwHighDateTime) << 32) | ftBase.dwLowDateTime;
    // Compute the high-resolution time.
    ULONGLONG preciseTime = baseTime + timeOffset;

    // Store the result back into the FILETIME structure.
    lpSystemTimeAsFileTime->dwLowDateTime = (DWORD)(preciseTime & 0xFFFFFFFF);
    lpSystemTimeAsFileTime->dwHighDateTime = (DWORD)(preciseTime >> 32);
}

// Helper: Returns the number of days in a given month for a specified year.
static int DaysInMonth(int year, int month)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int d = days[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        d = 29;
    return d;
}

// Helper: Returns the day of week using Sakamoto's algorithm.
// 0 = Sunday, 1 = Monday, ... 6 = Saturday.
static int day_of_week(int year, int month, int day)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3)
        year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

// Helper: Given a recurring transition rule (expressed as a SYSTEMTIME with zero year)
// and a target year, compute the absolute date of the transition.
// For rules:
//   - wMonth: month in which the transition occurs (0 means no transition)
//   - wDay: occurrence of the day-of-week (1–4 for nth occurrence, 5 for last occurrence)
//   - wDayOfWeek: the day-of-week (0 = Sunday, …, 6 = Saturday)
// The time fields (wHour, wMinute, etc.) are copied as-is.
static void ComputeTransitionDate(USHORT wYear, const SYSTEMTIME *pRule, SYSTEMTIME *pResult)
{
    // Copy year and time components.
    pResult->wYear = wYear;
    pResult->wMonth = pRule->wMonth;
    pResult->wHour = pRule->wHour;
    pResult->wMinute = pRule->wMinute;
    pResult->wSecond = pRule->wSecond;
    pResult->wMilliseconds = pRule->wMilliseconds;

    // If no transition is defined, set day values to zero.
    if (pRule->wMonth == 0)
    {
        pResult->wDay = 0;
        pResult->wDayOfWeek = 0;
        return;
    }

    if (pRule->wDay < 5)
    {
        // pRule->wDay indicates the nth occurrence of pRule->wDayOfWeek in the month.
        int nth = pRule->wDay;
        int firstDow = day_of_week(wYear, pRule->wMonth, 1);
        int desiredDow = pRule->wDayOfWeek;
        int offset = (desiredDow - firstDow + 7) % 7;
        int day = 1 + offset + (nth - 1) * 7;
        int dim = DaysInMonth(wYear, pRule->wMonth);
        // If calculated day exceeds month length, step back one week.
        if (day > dim)
            day -= 7;
        pResult->wDay = (WORD)day;
        pResult->wDayOfWeek = (WORD)desiredDow;
    }
    else
    {
        // pRule->wDay == 5 means the last occurrence of the specified day-of-week.
        int dim = DaysInMonth(wYear, pRule->wMonth);
        int lastDow = day_of_week(wYear, pRule->wMonth, dim);
        int desiredDow = pRule->wDayOfWeek;
        int offset = (lastDow - desiredDow + 7) % 7;
        int day = dim - offset;
        pResult->wDay = (WORD)day;
        pResult->wDayOfWeek = (WORD)desiredDow;
    }
}

// Fallback implementation of GetTimeZoneInformationForYear for Windows XP.
// Parameters:
//   wYear   - The local year for which the time zone settings are to be retrieved.
//   pdtzi   - Optional pointer to a DYNAMIC_TIME_ZONE_INFORMATION structure specifying the time zone.
//             If NULL, the current system time zone is used.
//   ptzi    - Pointer to a TIME_ZONE_INFORMATION structure to receive the time zone settings.
// Return Value:
//   Returns nonzero (TRUE) on success, or zero (FALSE) on failure (with GetLastError set).
WINBOOL WINAPI GetTimeZoneInformationForYear(
    USHORT wYear,
    PDYNAMIC_TIME_ZONE_INFORMATION pdtzi,
    LPTIME_ZONE_INFORMATION ptzi)
{
    TRACE(L"GetTimeZoneInformationForYear(%d, 0x%p, 0x%p)\n", wYear, pdtzi, ptzi);

    if (ptzi == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DYNAMIC_TIME_ZONE_INFORMATION dtziLocal;
    PDYNAMIC_TIME_ZONE_INFORMATION pDTZI;

    // If no dynamic TZ info is provided, use the current time zone.
    if (pdtzi == NULL)
    {
        TIME_ZONE_INFORMATION tzi;
        GetTimeZoneInformation(&tzi);
        // Map the TIME_ZONE_INFORMATION fields into our dynamic structure.
        memset(&dtziLocal, 0, sizeof(dtziLocal));
        dtziLocal.Bias = tzi.Bias;
        dtziLocal.StandardBias = tzi.StandardBias;
        dtziLocal.DaylightBias = tzi.DaylightBias;
        memcpy(dtziLocal.StandardName, tzi.StandardName, sizeof(dtziLocal.StandardName));
        memcpy(dtziLocal.DaylightName, tzi.DaylightName, sizeof(dtziLocal.DaylightName));
        dtziLocal.StandardDate = tzi.StandardDate;
        dtziLocal.DaylightDate = tzi.DaylightDate;
        // For the dynamic fields not present in TIME_ZONE_INFORMATION, set defaults.
        dtziLocal.TimeZoneKeyName[0] = L'\0';
        dtziLocal.DynamicDaylightTimeDisabled = FALSE;
        pDTZI = &dtziLocal;
    }
    else
        pDTZI = pdtzi;

    // Copy basic bias and name information to the output structure.
    ptzi->Bias = pDTZI->Bias;
    ptzi->StandardBias = pDTZI->StandardBias;
    ptzi->DaylightBias = pDTZI->DaylightBias;
    memcpy(ptzi->StandardName, pDTZI->StandardName, sizeof(ptzi->StandardName));
    memcpy(ptzi->DaylightName, pDTZI->DaylightName, sizeof(ptzi->DaylightName));

    // Compute the absolute transition dates for Standard and Daylight rules for the specified year.
    SYSTEMTIME stStandard, stDaylight;
    ComputeTransitionDate(wYear, &pDTZI->StandardDate, &stStandard);
    ComputeTransitionDate(wYear, &pDTZI->DaylightDate, &stDaylight);

    ptzi->StandardDate = stStandard;
    ptzi->DaylightDate = stDaylight;

    // Even if the time zone does not observe DST (DaylightDate.wMonth == 0),
    // we consider the function to have succeeded.
    return TRUE;
}

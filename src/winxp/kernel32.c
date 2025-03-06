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
        memcpy(&pInfo->CreationTime, &fileInfo.ftCreationTime, sizeof(pInfo->CreationTime));
        memcpy(&pInfo->LastAccessTime, &fileInfo.ftLastAccessTime, sizeof(pInfo->LastAccessTime));
        memcpy(&pInfo->LastWriteTime, &fileInfo.ftLastWriteTime, sizeof(pInfo->LastWriteTime));
        // Windows XP does not provide a ChangeTime; using LastWriteTime as a fallback
        memcpy(&pInfo->ChangeTime, &fileInfo.ftLastWriteTime, sizeof(pInfo->ChangeTime));
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

    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    BOOL success = FALSE;

    // Validate parameters.
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL)
    {
        TRACE(L"SetFileInformationByHandle: hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Map Win32 file information class to NT file information class
    // and prepare the appropriate structure
    PVOID ntBuffer = NULL;
    ULONG ntBufferSize = 0;
    FILE_INFORMATION_CLASS ntInfoClass;

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

        PFILE_BASIC_INFO win32BasicInfo = (PFILE_BASIC_INFO)lpFileInformation;
        PFILE_BASIC_INFORMATION ntBasicInfo;

        ntBufferSize = sizeof(FILE_BASIC_INFORMATION);
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntBasicInfo = (PFILE_BASIC_INFORMATION)ntBuffer;
        ntBasicInfo->CreationTime = win32BasicInfo->CreationTime;
        ntBasicInfo->LastAccessTime = win32BasicInfo->LastAccessTime;
        ntBasicInfo->LastWriteTime = win32BasicInfo->LastWriteTime;
        ntBasicInfo->ChangeTime = win32BasicInfo->ChangeTime;
        ntBasicInfo->FileAttributes = win32BasicInfo->FileAttributes;

        ntInfoClass = FileBasicInformation;
        break;
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

        PFILE_RENAME_INFO win32RenameInfo = (PFILE_RENAME_INFO)lpFileInformation;
#if 0
        UNICODE_STRING ntPath;
        // Convert the DOS path to an NT native path.
        if (!RtlDosPathNameToNtPathName_U(win32RenameInfo->FileName, &ntPath, NULL, NULL))
        {
            TRACE(L"RtlDosPathNameToNtPathName_U failed\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        // Print the converted NT path.
        TRACE(L"NT Path: [%ls]\n", ntPath.Buffer);

        // Map FileRenameInfo to FileRenameInformation
        PFILE_RENAME_INFORMATION ntRenameInfo;

        // Calculate the NT buffer size
        ntBufferSize = sizeof(FILE_RENAME_INFORMATION) - sizeof(wchar_t) + ntPath.Length;
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntRenameInfo = (PFILE_RENAME_INFORMATION)ntBuffer;
        ntRenameInfo->ReplaceIfExists = win32RenameInfo->ReplaceIfExists;
        ntRenameInfo->RootDirectory = win32RenameInfo->RootDirectory;
        ntRenameInfo->FileNameLength = ntPath.Length;
        memcpy(ntRenameInfo->FileName, ntPath.Buffer, ntPath.Length);

        status = NtSetInformationFile(
            hFile,
            &ioStatusBlock,
            ntBuffer,
            ntBufferSize,
            FileRenameInformation);

        // Clean up
        free(ntBuffer);

        // Convert NT status to Win32 error and set return value
        if (NT_SUCCESS(status))
        {
            TRACE(L"SetFileInformationByHandle -> NtSetInformationFile Rename OK\n");
            return TRUE;
        }
        else
        {
            TRACE(L"SetFileInformationByHandle -> NtSetInformationFile Rename failed: 0x%08lx (%ld)\n",
                  status, RtlNtStatusToDosError(status));
#else
        {
#endif
            // try with RenameFileEx
            wchar_t sourcePath[MAX_PATH + 1];

            if (!GetFinalPathNameByHandleW(hFile, sourcePath, MAX_PATH, VOLUME_NAME_DOS))
                return FALSE;

            DWORD moveFlags = 0;
            if (win32RenameInfo->ReplaceIfExists)
                moveFlags |= MOVEFILE_REPLACE_EXISTING;

            return MoveFileExW(sourcePath, win32RenameInfo->FileName, moveFlags);
        }
    }
    case FileDispositionInfo:
    {
        if (dwBufferSize < sizeof(FILE_DISPOSITION_INFORMATION))
        {
            TRACE(L"SetFileInformationByHandle[FileRenameInfo]: ERROR_INSUFFICIENT_BUFFER\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        // Map FileDispositionInfo to FileDispositionInformation
        PFILE_DISPOSITION_INFO win32DispInfo = (PFILE_DISPOSITION_INFO)lpFileInformation;
        PFILE_DISPOSITION_INFORMATION ntDispInfo;

        ntBufferSize = sizeof(FILE_DISPOSITION_INFORMATION);
        if (!(ntBuffer = malloc(ntBufferSize)))
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        ntDispInfo = (PFILE_DISPOSITION_INFORMATION)ntBuffer;
        ntDispInfo->DoDeleteFile = win32DispInfo->DeleteFile;

        ntInfoClass = FileDispositionInformation;
        break;
    }
    default:
        TRACE(L"SetFileInformationByHandle: Unsupported FileInformationClass %d\n", FileInformationClass);
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    // Call NtSetInformationFile with the prepared NT structures
    status = NtSetInformationFile(
        hFile,
        &ioStatusBlock,
        ntBuffer,
        ntBufferSize,
        ntInfoClass);

    // Convert NT status to Win32 error and set return value
    if (NT_SUCCESS(status))
    {
        TRACE(L"SetFileInformationByHandle -> NtSetInformationFile OK\n");
        success = TRUE;
    }
    else
    {
        // Convert NTSTATUS to Win32 error code
        TRACE(L"SetFileInformationByHandle -> NtSetInformationFile failed: 0x%08lx (%ld)\n", status, RtlNtStatusToDosError(status));
        SetLastError(RtlNtStatusToDosError(status));
        success = FALSE;
    }

    // Clean up
    if (ntBuffer)
        free(ntBuffer);

    return success;
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

static HANDLE hMountMgr = INVALID_HANDLE_VALUE;

__attribute__((constructor)) static void open_mount_manager()
{
    hMountMgr = CreateFileW(
        MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    TRACE(L"MountMgr HANDLE: %p\n", hMountMgr);
}

__attribute__((destructor)) static void close_mount_manager()
{
    if (hMountMgr != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hMountMgr);
        TRACE(L"Closed MountMgr HANDLE\n");
    }
}

/*
 * GetFinalPathNameByHandleW - Retrieves the final path for the specified file
 *
 * @param hFile       - Handle to a file or directory
 * @param lpszFilePath - Buffer to receive the path
 * @param cchFilePath - Size of the buffer in characters
 * @param dwFlags     - Format of the returned path (currently only VOLUME_NAME_DOS supported)
 *
 * @return Number of characters in the final path (excluding terminator), or 0 on failure
 */
HOTFUNC DWORD WINAPI
GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    DWORD requiredLength = 0;

    TRACE(L"GetFinalPathNameByHandleW(0x%p, 0x%p, %d, %d)\n", hFile, lpszFilePath, cchFilePath, dwFlags);

    // Validate input parameters.
    if (hFile == INVALID_HANDLE_VALUE)
    {
        TRACE(L"GetFinalPathNameByHandleW: -> ERROR_INVALID_PARAMETER (invalid handle)\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (dwFlags != VOLUME_NAME_DOS)
    {
        TRACE(L"GetFinalPathNameByHandleW: Unsupported dwFlags 0x%08x\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Allocate buffers for path information
    // FIXME: too big?
    union ANY_BUFFER nameFull, nameRel, nameMnt;

    // pointer refs for readability
    wchar_t *deviceName = nameMnt.TargetName.DeviceName;
    wchar_t *fileName = nameRel.NameInfo.FileName;
    wchar_t targetPath[MAX_PATH + 1] = L"\\\\?\\";

    // Get object name information (full NT path)
    status = NtQueryObject(hFile, ObjectNameInformation, nameFull.Buffer, sizeof(nameFull.Buffer), NULL);
    if (!NT_SUCCESS(status))
    {
        TRACE(L"GetFinalPathNameByHandleW->NtQueryObject failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    // Get file name information (relative path)
    status = NtQueryInformationFile(hFile, &iosb, nameRel.Buffer, sizeof(nameRel.Buffer), FileNameInformation);
    if (!NT_SUCCESS(status))
    {
        TRACE(L"GetFinalPathNameByHandleW->NtQueryInformationFile failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    if (nameFull.UnicodeString.Length < nameRel.NameInfo.FileNameLength)
    {
        TRACE(L"Path length validation failed: full (%d) < relative (%d)\n",
              nameFull.UnicodeString.Length, nameRel.NameInfo.FileNameLength);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t nameLength = nameRel.NameInfo.FileNameLength / sizeof(wchar_t);

    // Extract the device path portion
    nameMnt.TargetName.DeviceNameLength = nameFull.UnicodeString.Length - nameRel.NameInfo.FileNameLength;
    wcsncpy(nameMnt.TargetName.DeviceName,
            nameFull.UnicodeString.Buffer,
            nameMnt.TargetName.DeviceNameLength / sizeof(wchar_t));

    TRACE(L"deviceName: [%ls]\n", deviceName);
    TRACE(L"fileName: [%ls]\n", fileName);

    // skip Mup Device
    if (wcsncmp(deviceName, L"\\Device\\Mup", 11))
    {
        if (hMountMgr != INVALID_HANDLE_VALUE)
        {
            DWORD bytesReturned = 0;
            BOOL success = DeviceIoControl(hMountMgr,
                                           IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                           &nameMnt,
                                           sizeof(nameMnt),
                                           &nameMnt,
                                           sizeof(nameMnt),
                                           &bytesReturned,
                                           NULL);

            if (success && nameMnt.TargetPaths.MultiSzLength > 0)
            {
                TRACE(L"Resolved via MountMgr: %ls\n", targetPath);
                wcsncat(targetPath, nameMnt.TargetPaths.MultiSz, nameMnt.TargetPaths.MultiSzLength);
                wcsncat(targetPath, fileName, nameLength);
                requiredLength = wcslen(targetPath);
            }
        }
    }

    if (requiredLength == 0)
    {
        wcsncat(targetPath, L"UNC", 3);
        wcsncat(targetPath, fileName, nameLength);
        requiredLength = wcslen(targetPath);
    }

    if (lpszFilePath && (cchFilePath >= requiredLength))
    {
        wcsncpy(lpszFilePath, targetPath, requiredLength);
        lpszFilePath[requiredLength] = L'\0';
    }

    TRACE(L"GetFinalPathNameByHandleW -> %ls (%d chars)\n", targetPath, requiredLength);
    // Return the length of the final path (excluding the terminating null).
    return requiredLength;
}

#define VALID_FLAGS 0x5AFFB7

WINBASEAPI HANDLE WINAPI ReOpenFile(
    HANDLE hOriginalFile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwFlagsAndAttributes)
{
    TRACE(L"ReOpenFile(0x%p, 0x%08x, 0x%08x, 0x%08x)\n", hOriginalFile, dwDesiredAccess, dwShareMode, dwFlagsAndAttributes);

    SECURITY_QUALITY_OF_SERVICE qos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DestinationString = {0};
    IO_STATUS_BLOCK IoStatusBlock;

    if ((dwFlagsAndAttributes & VALID_FLAGS) != 0)
    {
        SetLastError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    ULONG CreateOptions = FILE_NON_DIRECTORY_FILE;

    if ((dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH) != 0)
        CreateOptions |= FILE_WRITE_THROUGH;

    if ((dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN) != 0)
        CreateOptions |= FILE_SEQUENTIAL_ONLY;

    if ((dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS) != 0)
        CreateOptions |= FILE_RANDOM_ACCESS;

    if ((dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) != 0)
        CreateOptions |= FILE_OPEN_FOR_BACKUP_INTENT;

    if ((dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL) != 0)
        CreateOptions |= FILE_OPEN_NO_RECALL;

    if ((dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT) != 0)
        CreateOptions |= FILE_FLAG_OPEN_REPARSE_POINT;

    if ((dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING) != 0)
        CreateOptions |= FILE_NO_INTERMEDIATE_BUFFERING;

    if ((dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED) != 0)
        CreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;

    if ((dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE) != 0)
    {
        CreateOptions |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
    }

    ULONG Attributes = 0;
    if ((dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS) == 0)
        Attributes = OBJ_CASE_INSENSITIVE;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DestinationString,
        Attributes,
        hOriginalFile,
        0);

    if (dwFlagsAndAttributes & SECURITY_SQOS_PRESENT)
    {
        qos.Length = sizeof(qos);
        qos.ImpersonationLevel = (dwFlagsAndAttributes >> 16) & 0x3;
        qos.ContextTrackingMode = dwFlagsAndAttributes & SECURITY_CONTEXT_TRACKING ? SECURITY_DYNAMIC_TRACKING : SECURITY_STATIC_TRACKING;
        qos.EffectiveOnly = (dwFlagsAndAttributes & SECURITY_EFFECTIVE_ONLY) != 0;
        ObjectAttributes.SecurityQualityOfService = &qos;
    }

    HANDLE FileHandle;

    NTSTATUS status = NtCreateFile(
        &FileHandle,                                          // FileHandle
        dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES, // DesiredAccess
        &ObjectAttributes,                                    // ObjectAttributes
        &IoStatusBlock,                                       // IoStatusBlock
        0,                                                    // AllocationSize
        0,                                                    // FileAttributes
        dwShareMode,                                          // ShareAccess
        FILE_OPEN,                                            // CreateDisposition
        CreateOptions,                                        // CreateOptions
        0,                                                    // EaBuffer
        0                                                     // EaLength
    );

    if (!NT_SUCCESS(status))
    {
        TRACE(L"ReOpenFile->NtCreateFile failed (0x%08x)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return INVALID_HANDLE_VALUE;
    }

    SetLastError(0);
    return FileHandle;
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
        if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID)
            return FALSE;

        // Map the TIME_ZONE_INFORMATION fields into our dynamic structure.
        memcpy(&dtziLocal, &tzi, sizeof(tzi));
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

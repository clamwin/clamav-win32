#define _WIN32_WINNT 0x0501
#define _SYNCHAPI_H_

#include <windows.h>
#include <ntdef.h>
#include <psapi.h>
#include <strsafe.h>

WINBASEAPI VOID WINAPI Sleep (DWORD dwMilliseconds);

int WINAPI CompareStringOrdinal(
    LPCWCH lpString1,
    int cchCount1,
    LPCWCH lpString2,
    int cchCount2,
    WINBOOL bIgnoreCase
)
{
    int i, minCount;
    WCHAR ch1, ch2;

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

// Fallback implementation of SetThreadStackGuarantee for Windows XP.
// Since XP does not support changing the thread stack guarantee, this stub simply returns TRUE.
// The value pointed to by StackSizeInBytes is left unchanged.
WINBASEAPI WINBOOL WINAPI SetThreadStackGuarantee(PULONG StackSizeInBytes)
{
    if (StackSizeInBytes == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // On Windows XP, this API is not supported. Do nothing.
    return TRUE;
}

BOOLEAN APIENTRY CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

HANDLE WINAPI CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES lpTimerAttributes, LPCWSTR lpTimerName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

// Define FILE_BASIC_INFO structure for XP (not available by default)
typedef struct _FILE_BASIC_INFO {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    DWORD FileAttributes;
} FILE_BASIC_INFO, *PFILE_BASIC_INFO;

// Define FILE_STANDARD_INFO structure for XP (not available by default)
typedef struct _FILE_STANDARD_INFO {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    DWORD NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFO, *PFILE_STANDARD_INFO;

// Define FILE_ATTRIBUTE_TAG_INFO structure (if not already defined)
typedef struct _FILE_ATTRIBUTE_TAG_INFO {
    DWORD FileAttributes;
    DWORD ReparseTag;
} FILE_ATTRIBUTE_TAG_INFO, *PFILE_ATTRIBUTE_TAG_INFO;

// Define an enum for the file information classes (similar to Vista+)
typedef enum _FILE_INFO_BY_HANDLE_CLASS {
    FileBasicInfo = 0,
    FileStandardInfo = 1,
    FileAttributeTagInfo = 2,
    // Other info classes are not supported in this fallback implementation
} FILE_INFO_BY_HANDLE_CLASS;

// Fallback implementation of GetFileInformationByHandleEx for Windows XP
WINBOOL WINAPI GetFileInformationByHandleEx(HANDLE hFile,
    FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    LPVOID lpFileInformation,
    DWORD dwBufferSize)
{
    // Validate input parameters
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (FileInformationClass == FileBasicInfo) {
        // Check if the provided buffer is large enough for FILE_BASIC_INFO
        if (dwBufferSize < sizeof(FILE_BASIC_INFO)) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_BASIC_INFO *pInfo = (FILE_BASIC_INFO*)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
            return FALSE;

        // Copy the time and attribute data from the handle information
        pInfo->CreationTime   = *(LARGE_INTEGER*)&fileInfo.ftCreationTime;
        pInfo->LastAccessTime = *(LARGE_INTEGER*)&fileInfo.ftLastAccessTime;
        pInfo->LastWriteTime  = *(LARGE_INTEGER*)&fileInfo.ftLastWriteTime;
        // Windows XP does not provide a ChangeTime; using LastWriteTime as a fallback
        pInfo->ChangeTime     = *(LARGE_INTEGER*)&fileInfo.ftLastWriteTime;
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        return TRUE;
    }
    else if (FileInformationClass == FileStandardInfo) {
        // Check if the provided buffer is large enough for FILE_STANDARD_INFO
        if (dwBufferSize < sizeof(FILE_STANDARD_INFO)) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_STANDARD_INFO *pInfo = (FILE_STANDARD_INFO*)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
            return FALSE;

        // Compute the file size from its low and high parts
        LARGE_INTEGER fileSize;
        fileSize.LowPart  = fileInfo.nFileSizeLow;
        fileSize.HighPart = fileInfo.nFileSizeHigh;

        pInfo->AllocationSize = fileSize;
        pInfo->EndOfFile      = fileSize;
        pInfo->NumberOfLinks  = fileInfo.nNumberOfLinks;
        pInfo->DeletePending  = FALSE; // Not determinable via this API
        pInfo->Directory      = (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
        return TRUE;
    }
    else if (FileInformationClass == FileAttributeTagInfo) {
        // Check if the provided buffer is large enough for FILE_ATTRIBUTE_TAG_INFO
        if (dwBufferSize < sizeof(FILE_ATTRIBUTE_TAG_INFO)) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        FILE_ATTRIBUTE_TAG_INFO *pInfo = (FILE_ATTRIBUTE_TAG_INFO*)lpFileInformation;
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if (!GetFileInformationByHandle(hFile, &fileInfo))
            return FALSE;

        // Copy the file attributes from the handle information
        pInfo->FileAttributes = fileInfo.dwFileAttributes;
        pInfo->ReparseTag = 0;  // Default value if not a reparse point

        // If the file is a reparse point, try to retrieve the reparse tag
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
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
                                NULL)) {
                // Cast the buffer to a REPARSE_DATA_BUFFER pointer to retrieve the ReparseTag.
                // Note: REPARSE_DATA_BUFFER has a variable layout; here we only extract the ReparseTag.
                pInfo->ReparseTag = ((PREPARSE_DATA_BUFFER)buffer)->ReparseTag;
            }
            // If DeviceIoControl fails, ReparseTag remains 0.
        }
        return TRUE;
    }
    else {
        // Unsupported FileInformationClass
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}

WINBOOL WINAPI SetFileInformationByHandle(
    HANDLE hFile,
    FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    LPVOID lpFileInformation,
    DWORD dwBufferSize)
{
    // Validate parameters.
    if (hFile == INVALID_HANDLE_VALUE || lpFileInformation == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (FileInformationClass)
    {
    case FileBasicInfo:
    {
        if (dwBufferSize < sizeof(FILE_BASIC_INFO))
        {
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
            return FALSE;
        }
        // Note: pBasicInfo->ChangeTime and pBasicInfo->FileAttributes are not supported.
        return TRUE;
    }
    default:
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
}

DWORD WINAPI GetFinalPathNameByHandleW(
    HANDLE hFile,
    LPWSTR lpszFilePath,
    DWORD cchFilePath,
    DWORD dwFlags   // This implementation supports only basic DOS paths
) {
    // Validate input parameters.
    if (hFile == INVALID_HANDLE_VALUE || lpszFilePath == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Create a file mapping object for the file handle.
    // We only need a mapping of 1 byte.
    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
    if (hMapping == NULL) {
        return 0;
    }

    // Map a view of the file into memory.
    LPVOID pMappingView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 1);
    if (pMappingView == NULL) {
        CloseHandle(hMapping);
        return 0;
    }

    // Retrieve the device path of the mapped file.
    // The device path will be something like:
    // "\\Device\\HarddiskVolume2\\Windows\\system32\\kernel32.dll"
    WCHAR devicePath[MAX_PATH];
    DWORD devicePathLen = GetMappedFileNameW(GetCurrentProcess(), pMappingView, devicePath, MAX_PATH);

    // Clean up the mapping.
    UnmapViewOfFile(pMappingView);
    CloseHandle(hMapping);

    if (devicePathLen == 0) {
        // Failed to get the mapped file name.
        return 0;
    }

    // Convert the device path to a DOS path.
    // Iterate over possible drive letters (A: to Z:) and compare the device name.
    WCHAR drive[3] = L"A:";  // Buffer for the drive letter (e.g., "C:")
    WCHAR deviceName[MAX_PATH];
    BOOL found = FALSE;
    WCHAR finalPath[MAX_PATH] = {0};
    size_t deviceNameLen = 0;
    DWORD i;

    for (i = L'A'; i <= L'Z'; i++) {
        drive[0] = (WCHAR)i;
        drive[1] = L':';
        drive[2] = L'\0';
        // QueryDosDeviceW retrieves the device name for the drive letter.
        if (QueryDosDeviceW(drive, deviceName, MAX_PATH)) {
            deviceNameLen = wcslen(deviceName);
            // Check if the beginning of devicePath matches the device name.
            if (wcsncmp(devicePath, deviceName, deviceNameLen) == 0) {
                // Build the final DOS path.
                // For example, if devicePath is "\\Device\\HarddiskVolume2\\Windows\\system32\\file.dll"
                // and deviceName is "\\Device\\HarddiskVolume2", then finalPath becomes "C:\\Windows\\system32\\file.dll".
                HRESULT hr = StringCchPrintfW(finalPath, MAX_PATH, L"%s%s", drive, devicePath + deviceNameLen);
                if (FAILED(hr)) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return 0;
                }
                found = TRUE;
                break;
            }
        }
    }

    // If no matching drive letter was found, use the original device path.
    LPWSTR outputPath = found ? finalPath : devicePath;
    size_t outputPathLen = wcslen(outputPath);

    // Check if the provided output buffer is large enough.
    if (outputPathLen + 1 > cchFilePath) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (DWORD)(outputPathLen + 1);
    }

    // Copy the resulting path to the caller's buffer using safe string copy.
    HRESULT hrCopy = StringCchCopyW(lpszFilePath, cchFilePath, outputPath);
    if (FAILED(hrCopy)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    // Return the length of the final path (excluding the terminating null).
    return (DWORD)outputPathLen;
}

WINBASEAPI HANDLE WINAPI ReOpenFile(
    HANDLE hOriginalFile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwFlags)
{
    // Validate the original handle.
    if (hOriginalFile == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    // Retrieve the file's final path.
    WCHAR filePath[MAX_PATH] = {0};
    DWORD ret = GetFinalPathNameByHandleW(hOriginalFile, filePath, MAX_PATH, 0);
    if (ret == 0 || ret > MAX_PATH) {
        // Could not retrieve the path.
        return INVALID_HANDLE_VALUE;
    }

    // Remove any "\\?\" prefix if present. CreateFileW cannot use paths with this prefix.
    WCHAR *pPath = filePath;
    if (wcsncmp(filePath, L"\\\\?\\", 4) == 0) {
        pPath += 4;
        // Special handling for UNC paths: a UNC path may start as "\\?\UNC\server\share..."
        if (wcsncmp(pPath, L"UNC\\", 4) == 0) {
            pPath += 3; // Skip "UNC"
            // Prepend "\\" to form a standard UNC path.
            WCHAR uncPath[MAX_PATH];
            HRESULT hr = StringCchPrintfW(uncPath, MAX_PATH, L"\\\\%s", pPath);
            if (FAILED(hr)) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
            }
            hr = StringCchCopyW(filePath, MAX_PATH, uncPath);
            if (FAILED(hr)) {
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
        NULL,            // default security attributes
        OPEN_EXISTING,   // file must exist
        dwFlags,         // flags and attributes for the new handle
        NULL             // no template file
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

// Define our fallback INIT_ONCE structure for one-time initialization.
// It must be zero-initialized (e.g. as a static/global variable).
typedef struct _INIT_ONCE {
    volatile LONG state; // 0 = not initialized, 1 = initializing, 2 = initialized
} INIT_ONCE, *PINIT_ONCE;

// Fallback implementation of InitOnceBeginInitialize for Windows XP.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - reserved, must be 0
//   lpPending - output flag that indicates whether the calling thread should perform initialization (TRUE)
//   lpContext - reserved, must be NULL
// Returns TRUE on success, FALSE on error (with an appropriate error code set)
WINBOOL WINAPI InitOnceBeginInitialize(PINIT_ONCE pInitOnce, DWORD dwFlags, PBOOL lpPending, LPVOID *lpContext)
{
    // Validate parameters.
    // lpContext must be NULL and dwFlags must be 0.
    if (pInitOnce == NULL || lpPending == NULL || lpContext != NULL || dwFlags != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // If initialization is already complete, indicate no pending initialization.
    if (pInitOnce->state == 2) {
        *lpPending = FALSE;
        return TRUE;
    }

    // Attempt to mark the INIT_ONCE structure as "initializing".
    // If the current state is 0 (not initialized), atomically set it to 1.
    if (InterlockedCompareExchange(&pInitOnce->state, 1, 0) == 0) {
        // The current thread is responsible for performing the initialization.
        *lpPending = TRUE;
        return TRUE;
    }
    else {
        // Another thread is performing initialization.
        // Spin-wait until the state becomes 2 (initialized).
        while (pInitOnce->state != 2) {
            Sleep(0); // Yield execution to other threads.
        }
        *lpPending = FALSE;
        return TRUE;
    }
}

// Fallback implementation of InitOnceComplete for Windows XP.
// This function should be called by the thread that performed the initialization to mark it as complete.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - reserved, must be 0
//   lpContext - reserved, must be NULL
// Returns TRUE on success, or FALSE if an invalid parameter is provided.
WINBOOL WINAPI  InitOnceComplete(PINIT_ONCE pInitOnce, DWORD dwFlags, LPVOID lpContext)
{
    // Validate parameters: pInitOnce must not be NULL, lpContext must be NULL, and dwFlags must be 0.
    if (pInitOnce == NULL || lpContext != NULL || dwFlags != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Mark the initialization as complete by setting the state to 2.
    InterlockedExchange(&pInitOnce->state, 2);

    return TRUE;
}


// Helper: Returns the number of days in a given month for a specified year.
static int DaysInMonth(int year, int month)
{
    static const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int d = days[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        d = 29;
    return d;
}

// Helper: Returns the day of week using Sakamoto's algorithm.
// 0 = Sunday, 1 = Monday, ... 6 = Saturday.
static int day_of_week(int year, int month, int day)
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (month < 3)
        year -= 1;
    return (year + year/4 - year/100 + year/400 + t[month - 1] + day) % 7;
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
    if (pRule->wMonth == 0) {
        pResult->wDay = 0;
        pResult->wDayOfWeek = 0;
        return;
    }

    if (pRule->wDay < 5) {
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
    } else {
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
    LPTIME_ZONE_INFORMATION ptzi
)
{
    if (ptzi == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DYNAMIC_TIME_ZONE_INFORMATION dtziLocal;
    PDYNAMIC_TIME_ZONE_INFORMATION pDTZI;

    // If no dynamic TZ info is provided, use the current time zone.
    if (pdtzi == NULL) {
        TIME_ZONE_INFORMATION tzi;
        DWORD res = GetTimeZoneInformation(&tzi);
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
    } else {
        pDTZI = pdtzi;
    }

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


// Define a minimal dummy attribute list structure.
// This structure is only used as a placeholder because Windows XP does not support extended attributes.
typedef struct _PROC_THREAD_ATTRIBUTE_LIST {
    SIZE_T cbSize;
    // No attribute storage is provided in this fallback.
} PROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

// Fallback implementation of InitializeProcThreadAttributeList for XP.
// If lpAttributeList is NULL, the required size is returned via lpSize and the function fails
// (as per the normal pattern). Otherwise, the dummy attribute list is initialized.
WINBOOL WINAPI InitializeProcThreadAttributeList(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwAttributeCount,   // Ignored in this fallback.
    DWORD dwFlags,            // Must be zero.
    PSIZE_T lpSize)
{
    if (lpSize == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Our dummy attribute list requires only a fixed size.
    SIZE_T requiredSize = sizeof(PROC_THREAD_ATTRIBUTE_LIST);

    // If caller is requesting the required size, return it.
    if (lpAttributeList == NULL) {
        *lpSize = requiredSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Verify that the provided buffer is large enough.
    if (*lpSize < requiredSize) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialize the dummy attribute list.
    lpAttributeList->cbSize = requiredSize;

    // Ignore dwAttributeCount and dwFlags (since no attributes are supported).
    return TRUE;
}

// Fallback implementation of DeleteProcThreadAttributeList for XP.
// In this dummy implementation, no resources are allocated, so this function simply clears the structure.
VOID WINAPI DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList)
{
    if (lpAttributeList) {
        // Clear the structure.
        lpAttributeList->cbSize = 0;
    }
}

// Fallback implementation of UpdateProcThreadAttribute for XP.
// Since extended process/thread attributes are not supported on XP,
// this function always fails and sets the error to ERROR_NOT_SUPPORTED.
WINBOOL WINAPI UpdateProcThreadAttribute(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwFlags,
    DWORD_PTR Attribute,
    PVOID lpValue,
    SIZE_T cbSize,
    PVOID lpPreviousValue,
    PSIZE_T lpReturnSize)
{
    UNREFERENCED_PARAMETER(lpAttributeList);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(lpValue);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(lpPreviousValue);
    UNREFERENCED_PARAMETER(lpReturnSize);

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

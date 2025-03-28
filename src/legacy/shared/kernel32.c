/*
 * Legacy Windows Compatibility Layer
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "legacy.h"

int WINAPI CompareStringOrdinal(
    LPCWCH lpString1,
    int cchCount1,
    LPCWCH lpString2,
    int cchCount2,
    WINBOOL bIgnoreCase)
{
    int i, minCount;
    WCHAR ch1, ch2;

    TRACE("CompareStringOrdinal(%ls, %d, %ls, %d, %d)\n", lpString1, cchCount1, lpString2, cchCount2, bIgnoreCase);

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

#define VALID_FLAGS 0x5AFFB7

HANDLE WINAPI ReOpenFile(
    HANDLE hOriginalFile,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    DWORD dwFlagsAndAttributes)
{
    TRACE("ReOpenFile(0x%p, 0x%08lx, 0x%08lx, 0x%08lx)\n", hOriginalFile, dwDesiredAccess, dwShareMode, dwFlagsAndAttributes);

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
        TRACE("ReOpenFile->NtCreateFile failed (0x%08lx)\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return INVALID_HANDLE_VALUE;
    }

    SetLastError(0);
    return FileHandle;
}

// Static variables to hold the baseline values.
// They are initialized on the first call.
static LARGE_INTEGER qpcBase = { 0 };      // Baseline performance counter value.
static FILETIME ftBase = { 0 };            // Baseline system time corresponding to qpcBase.
static LARGE_INTEGER qpcFrequency = { 0 }; // Performance counter frequency.

#if defined(__GNUC__) || defined(__clang__)
__attribute__((constructor))
#endif
static void
init()
{
    // Set the baseline.
    // Retrieve the performance counter frequency.
    QueryPerformanceFrequency(&qpcFrequency);
    // Record the current performance counter value.
    QueryPerformanceCounter(&qpcBase);
    // Retrieve the system time as a FILETIME.
    GetSystemTimeAsFileTime(&ftBase);
}

VOID WINAPI GetSystemTimePreciseAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
    TRACE("GetSystemTimePreciseAsFileTime(0x%p)\n", lpSystemTimeAsFileTime);

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

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x2
#endif

HANDLE WINAPI CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES lpTimerAttributes, LPCWSTR lpTimerName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    TRACE("CreateWaitableTimerExW(0x%p, %ls, %ld, %ld)\n",
          lpTimerAttributes, lpTimerName, dwFlags, dwDesiredAccess);

    BOOL bManualReset = (dwFlags & CREATE_WAITABLE_TIMER_MANUAL_RESET) != 0;
#ifdef UNICODE
    HANDLE hTimer = CreateWaitableTimerW(lpTimerAttributes, bManualReset, lpTimerName);
#else
    char *lpTimerNameA = NULL;

    if (lpTimerName)
    {
        int size = WideCharToMultiByte(CP_ACP, 0, lpTimerName, -1, NULL, 0, NULL, NULL);
        if (size)
        {
            lpTimerNameA = RtlAllocateHeap(GetProcessHeap(), 0, size);
            WideCharToMultiByte(CP_ACP, 0, lpTimerName, -1, NULL, 0, NULL, NULL);
        }
    }

    HANDLE hTimer = CreateWaitableTimerA(lpTimerAttributes, bManualReset, lpTimerNameA);
    if (lpTimerNameA)
        RtlFreeHeap(GetProcessHeap(), 0, lpTimerNameA);
#endif
    // If the timer was created successfully but HIGH_RESOLUTION was requested,
    if ((hTimer != NULL) && (dwFlags & CREATE_WAITABLE_TIMER_HIGH_RESOLUTION))
    {
        TRACE("Warning: High-resolution timer requested but not supported\n");
        OutputDebugString(TEXT("Warning: High-resolution timer requested but not supported\n"));
    }

    return hTimer;
}

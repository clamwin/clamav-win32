/*
 * Legacy Windows Compatibility Layer: Windows 7
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

#include <windows.h>
#include "dynload.h"

// Static variables to hold the baseline values.
// They are initialized on the first call.
static LARGE_INTEGER qpcBase = { 0 };      // Baseline performance counter value.
static FILETIME ftBase = { 0 };            // Baseline system time corresponding to qpcBase.
static LARGE_INTEGER qpcFrequency = { 0 }; // Performance counter frequency.

void init_sysinfoapi(void)
{
    // Set the baseline.
    // Retrieve the performance counter frequency.
    QueryPerformanceFrequency(&qpcFrequency);
    // Record the current performance counter value.
    QueryPerformanceCounter(&qpcBase);
    // Retrieve the system time as a FILETIME.
    GetSystemTimeAsFileTime(&ftBase);
}

VOID WINAPI GetSystemTimePreciseAsFileTime_compat(LPFILETIME lpSystemTimeAsFileTime)
{
    // Get the current performance counter value.
    LARGE_INTEGER qpcNow = { 0 };
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

imp_GetSystemTimePreciseAsFileTime pGetSystemTimePreciseAsFileTime = GetSystemTimePreciseAsFileTime_compat;

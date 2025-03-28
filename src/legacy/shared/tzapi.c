/*
 * Legacy Windows Compatibility Layer: TZ Api
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

// Helper: Returns the number of days in a given month for a specified year.
int DaysInMonth(int year, int month)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int d = days[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        d = 29;
    return d;
}

// Helper: Returns the day of week using Sakamoto's algorithm.
// 0 = Sunday, 1 = Monday, ... 6 = Saturday.
int day_of_week(int year, int month, int day)
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
void ComputeTransitionDate(USHORT wYear, const SYSTEMTIME *pRule, SYSTEMTIME *pResult)
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

// Helper to compare two SYSTEMTIME structures.
int CompareSystemTime(const SYSTEMTIME *a, const SYSTEMTIME *b)
{
    if (a->wYear != b->wYear)
        return a->wYear - b->wYear;
    if (a->wMonth != b->wMonth)
        return a->wMonth - b->wMonth;
    if (a->wDay != b->wDay)
        return a->wDay - b->wDay;
    if (a->wHour != b->wHour)
        return a->wHour - b->wHour;
    if (a->wMinute != b->wMinute)
        return a->wMinute - b->wMinute;
    if (a->wSecond != b->wSecond)
        return a->wSecond - b->wSecond;
    return a->wMilliseconds - b->wMilliseconds;
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
    TRACE("GetTimeZoneInformationForYear(%d, 0x%p, 0x%p)\n", wYear, pdtzi, ptzi);

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

WINBOOL WINAPI SystemTimeToTzSpecificLocalTime_compat(
    const TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    const SYSTEMTIME *lpUniversalTime,
    LPSYSTEMTIME lpLocalTime)
{
    FILETIME ftUniversal, ftLocal;
    ULONGLONG ullUniversal, ullNew;
    ULONGLONG biasIntervals, effectiveBiasIntervals;
    SYSTEMTIME stPrelim, stStandard, stDaylight;
    int effectiveBias;
    BOOL isDST = FALSE;

    // Convert the provided UTC SYSTEMTIME into FILETIME.
    if (!SystemTimeToFileTime(lpUniversalTime, &ftUniversal))
        return FALSE;

    ullUniversal = (((ULONGLONG)ftUniversal.dwHighDateTime) << 32) | ftUniversal.dwLowDateTime;

    // Compute preliminary local time using the base Bias (in minutes).
    biasIntervals = (ULONGLONG)lpTimeZoneInformation->Bias * 600000000ULL;
    ullNew = ullUniversal - biasIntervals;
    ftLocal.dwLowDateTime = (DWORD)(ullNew & 0xFFFFFFFF);
    ftLocal.dwHighDateTime = (DWORD)(ullNew >> 32);

    if (!FileTimeToSystemTime(&ftLocal, &stPrelim))
        return FALSE;

    // If no DST information is available, use the preliminary local time.
    if (lpTimeZoneInformation->StandardDate.wMonth == 0 || lpTimeZoneInformation->DaylightDate.wMonth == 0)
    {
        *lpLocalTime = stPrelim;
        return TRUE;
    }

    // Compute the absolute transition dates for the current year using your helper.
    ComputeTransitionDate(stPrelim.wYear, &lpTimeZoneInformation->StandardDate, &stStandard);
    ComputeTransitionDate(stPrelim.wYear, &lpTimeZoneInformation->DaylightDate, &stDaylight);

    // Determine whether DST is in effect.
    // When the Daylight transition occurs before the Standard transition,
    // DST is active from stDaylight (inclusive) to stStandard (exclusive).
    if (CompareSystemTime(&stDaylight, &stStandard) < 0)
    {
        if (CompareSystemTime(&stPrelim, &stDaylight) >= 0 &&
            CompareSystemTime(&stPrelim, &stStandard) < 0)
            isDST = TRUE;
    }
    else
    {
        // For zones where the DST period spans year-end.
        if (CompareSystemTime(&stPrelim, &stStandard) < 0 ||
            CompareSystemTime(&stPrelim, &stDaylight) >= 0)
            isDST = TRUE;
    }

    // Choose the effective bias.
    effectiveBias = lpTimeZoneInformation->Bias +
                    (isDST ? lpTimeZoneInformation->DaylightBias : lpTimeZoneInformation->StandardBias);

    effectiveBiasIntervals = (ULONGLONG)effectiveBias * 600000000ULL;

    // Recompute local time from UTC using the effective bias.
    ullNew = ullUniversal - effectiveBiasIntervals;
    ftLocal.dwLowDateTime = (DWORD)(ullNew & 0xFFFFFFFF);
    ftLocal.dwHighDateTime = (DWORD)(ullNew >> 32);

    if (!FileTimeToSystemTime(&ftLocal, lpLocalTime))
        return FALSE;

    return TRUE;
}

WINBOOL WINAPI TzSpecificLocalTimeToSystemTime_compat(
    const TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    const SYSTEMTIME *lpLocalTime,
    LPSYSTEMTIME lpUniversalTime)
{
    FILETIME ftLocal, ftCandidate;
    ULONGLONG ullLocal, candidateUTCIntervals;
    SYSTEMTIME stCandidate, stBackConverted;
    int effectiveBias;

    // Convert the given local SYSTEMTIME to FILETIME.
    if (!SystemTimeToFileTime(lpLocalTime, &ftLocal))
        return FALSE;

    ullLocal = (((ULONGLONG)ftLocal.dwHighDateTime) << 32) | ftLocal.dwLowDateTime;

    // First attempt: assume that the local time is in DST.
    effectiveBias = lpTimeZoneInformation->Bias + lpTimeZoneInformation->DaylightBias;
    candidateUTCIntervals = ullLocal + ((ULONGLONG)effectiveBias * 600000000ULL);
    ftCandidate.dwLowDateTime = (DWORD)(candidateUTCIntervals & 0xFFFFFFFF);
    ftCandidate.dwHighDateTime = (DWORD)(candidateUTCIntervals >> 32);

    if (!FileTimeToSystemTime(&ftCandidate, &stCandidate))
        return FALSE;

    // Convert the candidate UTC time back to local time.
    if (!SystemTimeToTzSpecificLocalTime(lpTimeZoneInformation, &stCandidate, &stBackConverted))
        return FALSE;

    // If the round-trip conversion matches, we've found the correct UTC.
    if (CompareSystemTime(lpLocalTime, &stBackConverted) == 0)
    {
        *lpUniversalTime = stCandidate;
        return TRUE;
    }

    // Second attempt: assume the local time is standard (non-DST).
    effectiveBias = lpTimeZoneInformation->Bias + lpTimeZoneInformation->StandardBias;
    candidateUTCIntervals = ullLocal + ((ULONGLONG)effectiveBias * 600000000ULL);
    ftCandidate.dwLowDateTime = (DWORD)(candidateUTCIntervals & 0xFFFFFFFF);
    ftCandidate.dwHighDateTime = (DWORD)(candidateUTCIntervals >> 32);

    if (!FileTimeToSystemTime(&ftCandidate, &stCandidate))
        return FALSE;

    if (!SystemTimeToTzSpecificLocalTime(lpTimeZoneInformation, &stCandidate, &stBackConverted))
        return FALSE;

    if (CompareSystemTime(lpLocalTime, &stBackConverted) == 0)
    {
        *lpUniversalTime = stCandidate;
        return TRUE;
    }

    // Neither candidate produced the original local time. The input may be invalid
    // (for example, during a DST gap).
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

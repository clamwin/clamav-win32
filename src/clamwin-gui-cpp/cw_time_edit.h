#pragma once
/*
 * cw_time_edit.h — helpers for hour/minute edit controls
 *
 * Hours are written per the OS regional setting (24h or 12h AM/PM).
 * Minutes are always zero-padded to two digits.
 * The hour edit must NOT have ES_NUMBER so that "AM"/"PM" text is accepted.
 */
#include <windows.h>

inline bool CW_ContainsNoCase(const char* haystack, const char* needle)
{
    if (!haystack || !needle || !*needle)
        return false;

    int nlen = lstrlenA(needle);
    if (nlen <= 0)
        return false;

    for (const char* p = haystack; *p; ++p)
    {
        int i = 0;
        while (i < nlen)
        {
            char a = p[i];
            char b = needle[i];
            if (!a)
                return false;
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b)
                break;
            ++i;
        }
        if (i == nlen)
            return true;
    }

    return false;
}

/* Write hour (0–23) formatted per LOCALE_ITIME ("1"→24h, "0"→12h AM/PM). */
inline void CW_WriteHourToEdit(HWND edit, int hour24)
{
    if (!edit) return;
    char itime[4] = {0};
    char am[32] = "AM";
    char pm[32] = "PM";
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME, itime, sizeof(itime));
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S1159, am, sizeof(am));
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S2359, pm, sizeof(pm));
    char buf[16];
    if (itime[0] == '1')                         /* 24-hour */
    {
        wsprintfA(buf, "%d", hour24);
    }
    else                                          /* 12-hour */
    {
        int h12 = hour24 % 12;
        if (h12 == 0) h12 = 12;
        wsprintfA(buf, "%d %s", h12, hour24 < 12 ? am : pm);
    }
    SetWindowTextA(edit, buf);
}

/* Read hour back.  Strips optional AM/PM; returns 24-hour value 0–23. */
inline int CW_ReadHourFromEdit(HWND edit, int fallback)
{
    if (!edit) return fallback;
    char buf[32] = {0};
    GetWindowTextA(edit, buf, sizeof(buf));
    if (!buf[0]) return fallback;

    char am[32] = "AM";
    char pm[32] = "PM";
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S1159, am, sizeof(am));
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_S2359, pm, sizeof(pm));

    bool isPM = CW_ContainsNoCase(buf, pm);
    bool isAM = CW_ContainsNoCase(buf, am);
    int h = atoi(buf);
    if (isPM && h != 12) h += 12;
    if (isAM && h == 12) h  = 0;
    if (h < 0)  h = 0;
    if (h > 23) h = 23;
    return h;
}

/* Write minute always zero-padded ("00"–"59"). */
inline void CW_WriteMinuteToEdit(HWND edit, int minute)
{
    if (!edit) return;
    char buf[8];
    wsprintfA(buf, "%02d", minute);
    SetWindowTextA(edit, buf);
}

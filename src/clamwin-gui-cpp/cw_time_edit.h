#pragma once
/*
 * cw_time_edit.h — helpers for hour/minute edit controls
 *
 * Hours are written per the OS regional setting (24h or 12h AM/PM).
 * Minutes are always zero-padded to two digits.
 * The hour edit must NOT have ES_NUMBER so that "AM"/"PM" text is accepted.
 */
#include <windows.h>

/* Write hour (0–23) formatted per LOCALE_ITIME ("1"→24h, "0"→12h AM/PM). */
inline void CW_WriteHourToEdit(HWND edit, int hour24)
{
    if (!edit) return;
    char itime[4] = {0};
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ITIME, itime, sizeof(itime));
    char buf[16];
    if (itime[0] == '1')                         /* 24-hour */
    {
        wsprintfA(buf, "%d", hour24);
    }
    else                                          /* 12-hour */
    {
        int h12 = hour24 % 12;
        if (h12 == 0) h12 = 12;
        wsprintfA(buf, "%d %s", h12, hour24 < 12 ? "AM" : "PM");
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

    bool isPM = false, isAM = false;
    for (int i = 0; buf[i]; ++i)
    {
        if (buf[i] == 'P' || buf[i] == 'p') { isPM = true; break; }
        if (buf[i] == 'A' || buf[i] == 'a') { isAM = true; break; }
    }
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

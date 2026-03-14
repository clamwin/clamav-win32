/*
 * ClamWin Free Antivirus — Utility Functions
 *
 * DB info parsing and protection status evaluation.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "cw_gui_shared.h"
#include <time.h>

/* ─── Parse a ClamAV .cvd/.cld header for version/sig info ─── */

/*
 * ClamAV CVD/CLD file header format (first 512 bytes):
 *   ClamAV-VDB:build_time:version:num_sigs:...
 * Fields separated by ':'
 */
int CW_GetDBInfo(const char *db_path, CW_DBInfo *info)
{
    char main_path[CW_MAX_PATH];
    char daily_path[CW_MAX_PATH];
    char header[512];
    HANDLE hFile;
    DWORD bytesRead;
    char *p, *fields[8];
    int i;

    memset(info, 0, sizeof(*info));

    /* Try main.cld first, then main.cvd */
    wsprintfA(main_path, "%s\\main.cld", db_path);
    if (GetFileAttributesA(main_path) == INVALID_FILE_ATTRIBUTES)
        wsprintfA(main_path, "%s\\main.cvd", db_path);

    hFile = CreateFileA(main_path, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (ReadFile(hFile, header, sizeof(header) - 1, &bytesRead, NULL))
        {
            header[bytesRead] = '\0';
            /* Parse: ClamAV-VDB:time:ver:sigs:... */
            p = header;
            for (i = 0; i < 8 && p; i++)
            {
                fields[i] = p;
                p = strchr(p, ':');
                if (p) *p++ = '\0';
            }
            if (i >= 4)
            {
                info->main_ver  = atoi(fields[2]);
                info->main_sigs = atoi(fields[3]);
            }
        }
        CloseHandle(hFile);
    }

    /* Try daily.cld first, then daily.cvd */
    wsprintfA(daily_path, "%s\\daily.cld", db_path);
    if (GetFileAttributesA(daily_path) == INVALID_FILE_ATTRIBUTES)
        wsprintfA(daily_path, "%s\\daily.cvd", db_path);

    hFile = CreateFileA(daily_path, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (ReadFile(hFile, header, sizeof(header) - 1, &bytesRead, NULL))
        {
            header[bytesRead] = '\0';
            p = header;
            for (i = 0; i < 8 && p; i++)
            {
                fields[i] = p;
                p = strchr(p, ':');
                if (p) *p++ = '\0';
            }
            if (i >= 4)
            {
                info->daily_ver  = atoi(fields[2]);
                info->daily_sigs = atoi(fields[3]);
            }
            /* Field[1] is the build time string, parse it to epoch */
            if (i >= 2)
            {
                /* ClamAV uses "DD Mon YYYY HH-MM +0000" format */
                /* For now, use the file modification time instead */
                FILETIME ft;
                SYSTEMTIME st;
                if (GetFileTime(hFile, NULL, NULL, &ft))
                {
                    FileTimeToSystemTime(&ft, &st);
                    struct tm tm_val = {0};
                    tm_val.tm_year = st.wYear - 1900;
                    tm_val.tm_mon  = st.wMonth - 1;
                    tm_val.tm_mday = st.wDay;
                    tm_val.tm_hour = st.wHour;
                    tm_val.tm_min  = st.wMinute;
                    tm_val.tm_sec  = st.wSecond;
                    info->updated_time = (long)mktime(&tm_val);
                }
            }
        }
        CloseHandle(hFile);
    }

    info->total_sigs = info->main_sigs + info->daily_sigs;
    return (info->main_ver > 0 || info->daily_ver > 0) ? 1 : 0;
}

/* ─── Evaluate protection status ────────────────────────────── */

CW_ProtectionStatus CW_GetProtectionStatus(const CWConfig *cfg)
{
    CW_DBInfo info;

    if (!CW_GetDBInfo(cfg->databasePath.c_str(), &info))
        return CW_STATUS_ERROR;

    /* Check how old the daily DB is */
    if (info.updated_time > 0)
    {
        time_t now = time(NULL);
        double days = difftime(now, (time_t)info.updated_time) / 86400.0;

        if (days > 14.0)
            return CW_STATUS_ERROR;
        if (days > 5.0)
            return CW_STATUS_WARN;
    }

    return CW_STATUS_OK;
}

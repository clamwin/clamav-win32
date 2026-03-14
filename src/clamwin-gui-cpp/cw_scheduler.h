/*
 * ClamWin Free Antivirus — CWScheduler
 *
 * Timer-based scheduled scanning and database updates.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include <windows.h>
#include "cw_config.h"

class CWScheduler
{
public:
    CWScheduler();
    ~CWScheduler();

    void start(HWND hwnd, CWConfig* config);
    void stop();
    void check();

public:
    static int isDue(time_t now, int scheduled, int hour, int minute, int frequency, int day, bool runMissed, long long& last_run_time);

private:
    /* No copy */
    CWScheduler(const CWScheduler&);
    CWScheduler& operator=(const CWScheduler&);

    HWND      m_hwnd;
    CWConfig* m_config;
    UINT_PTR  m_timerId;
};

/*
 * ClamWin Free Antivirus — CWScheduler
 *
 * Timer-based scheduled scanning and database updates.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_scheduler.h"
#include "cw_gui_shared.h"
#include <time.h>

#define SCHEDULER_TIMER_ID  42
#define SCHEDULER_INTERVAL  60000  /* 60 seconds */

CWScheduler::CWScheduler()
    : m_hwnd(NULL)
    , m_config(NULL)
    , m_timerId(0)
{
}

CWScheduler::~CWScheduler()
{
    stop();
}

int CWScheduler::isDue(time_t now, int scheduled, int hour, int minute, int frequency, int day, bool runMissed, long long& last_run_time)
{
    if (!scheduled) return 0;

    struct tm* tm_ptr = localtime(&now);
    if (!tm_ptr) return 0;
    
    struct tm tm_val = *tm_ptr;
    struct tm* tm = &tm_val;

    time_t last_rt = (time_t)last_run_time;
    struct tm ltm = {0};
    if (last_rt > 0) {
        struct tm* ltm_ptr = localtime(&last_rt);
        if (ltm_ptr) ltm = *ltm_ptr;
    }

    /* Determine current interval identifier to prevent running twice */
    bool ran_this_interval = false;
    if (last_rt > 0) {
        if (frequency == 3) {
            ran_this_interval = (ltm.tm_year == tm->tm_year && ltm.tm_yday == tm->tm_yday && ltm.tm_hour == tm->tm_hour);
            if (now - last_rt < 3000) ran_this_interval = true;
        } else {
            ran_this_interval = (ltm.tm_year == tm->tm_year && ltm.tm_yday == tm->tm_yday);
        }
    }

    if (ran_this_interval) return 0;

    /* Is today an eligible day? */
    bool eligible_day = false;
    switch (frequency) {
        case 0: /* Daily */     eligible_day = true; break;
        case 1: /* Workdays */  eligible_day = (tm->tm_wday >= 1 && tm->tm_wday <= 5); break;
        case 2: /* Weekly */    eligible_day = (tm->tm_wday == day); break;
        case 3: /* Hourly */    eligible_day = true; break;
    }

    if (!eligible_day) return 0;

    bool time_to_run = false;

    if (frequency == 3) {
        if (tm->tm_min >= minute) time_to_run = true;
    } else {
        if (runMissed) {
            /* If runMissed is true, run at the exact hour OR ANYTIME AFTER the exact hour */
            if (tm->tm_hour > hour || (tm->tm_hour == hour && tm->tm_min >= minute)) {
                time_to_run = true;
            }
        } else {
            /* If runMissed is false, strictly only run IN the exact hour */
            if (tm->tm_hour == hour && tm->tm_min >= minute) {
                time_to_run = true;
            }
        }
    }

    if (time_to_run) {
        last_run_time = now;
        return 1;
    }

    /* Catch up on missed tasks if computer was off */
    if (runMissed && last_rt > 0)
    {
        int gap = (int)(now - last_rt);
        if (frequency == 3 && gap > 5400) time_to_run = true;
        if (frequency == 0 && gap > 129600) time_to_run = true; /* 36h */
        if (frequency == 1 && gap > 302400) time_to_run = true; /* 84h (3.5 days) */
        if (frequency == 2 && gap > 648000) time_to_run = true; /* 7.5 days */
    }

    if (time_to_run) {
        last_run_time = now;
        return 1;
    }

    return 0;
}

void CWScheduler::check()
{
    if (!m_config || !m_hwnd) return;

    time_t now = time(NULL);
    bool conf_changed = false;

    /* Check scan schedule */
    if (isDue(now, m_config->scanScheduled,
              m_config->scanHour, m_config->scanMinute,
              m_config->scanFrequency, m_config->scanDay,
              m_config->scanRunMissed, m_config->scanLastRunTime))
    {
        conf_changed = true;
        PostMessage(m_hwnd, WM_COMMAND, IDM_TRAY_SCHEDULED_SCAN, 0);
    }

    /* Check update schedule */
    if (isDue(now, m_config->updateScheduled,
              m_config->updateHour, m_config->updateMinute,
              m_config->updateFrequency, 0,
              m_config->updateRunMissed, m_config->updateLastRunTime))
    {
        conf_changed = true;
        PostMessage(m_hwnd, WM_COMMAND, IDM_TRAY_SCHEDULED_UPDATE, 0);
    }

    if (conf_changed) {
        m_config->save();
    }
}

void CWScheduler::start(HWND hwnd, CWConfig *cfg)
{
    m_hwnd = hwnd;
    m_config = cfg;

    m_timerId = SetTimer(hwnd, SCHEDULER_TIMER_ID, SCHEDULER_INTERVAL, NULL);

    /* Do an initial check immediately */
    check();
}

void CWScheduler::stop()
{
    if (m_timerId && m_hwnd)
    {
        KillTimer(m_hwnd, SCHEDULER_TIMER_ID);
        m_timerId = 0;
    }
    m_hwnd = NULL;
    m_config = NULL;
}

/* ─── Global C wrappers for compatibility ───────────────────────── */
static CWScheduler s_globalScheduler;

void CW_SchedulerStart(HWND hwnd, CWConfig *cfg)
{
    s_globalScheduler.start(hwnd, cfg);
}

void CW_SchedulerStop(void)
{
    s_globalScheduler.stop();
}

void CW_SchedulerCheck(void)
{
    s_globalScheduler.check();
}

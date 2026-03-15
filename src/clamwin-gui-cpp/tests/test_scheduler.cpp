#include "doctest.h"
#include "cw_scheduler.h"
#include <time.h>

TEST_SUITE("Scheduler")
{
    TEST_CASE("isDue daily scheduling")
    {
        // 2026-03-10 10:00:00 (Tuesday)
        struct tm tm_mock = {0};
        tm_mock.tm_year = 2026 - 1900;
        tm_mock.tm_mon = 2; // March
        tm_mock.tm_mday = 10;
        tm_mock.tm_hour = 10;
        tm_mock.tm_min = 0;
        tm_mock.tm_sec = 0;
        tm_mock.tm_isdst = -1;
        time_t now = mktime(&tm_mock);
        
        long long last_run = 0;

        // Scheduled for 10:00 daily
        CHECK(CWScheduler::isDue(now, 1, 10, 0, 0, 0, false, last_run) == 1);
        CHECK(last_run == now);

        // Same interval, should not run again
        CHECK(CWScheduler::isDue(now, 1, 10, 0, 0, 0, false, last_run) == 0);
        
        // 10:05, exact same interval, shouldn't run
        time_t now_10_05 = now + 300;
        CHECK(CWScheduler::isDue(now_10_05, 1, 10, 0, 0, 0, false, last_run) == 0);

        // Run missed false: 11:00, shouldn't run (past exact run hour)
        time_t now_11_00 = now + 3600;
        long long last_run_yesterday = now - 86400; // ran yesterday
        CHECK(CWScheduler::isDue(now_11_00, 1, 10, 0, 0, 0, false, last_run_yesterday) == 0);

        // Run missed true: 11:00, missed 10:00 today. Should run.
        last_run_yesterday = now - 86400;
        CHECK(CWScheduler::isDue(now_11_00, 1, 10, 0, 0, 0, true, last_run_yesterday) == 1);
        CHECK(last_run_yesterday == now_11_00);
    }

    TEST_CASE("isDue hourly scheduling")
    {
        // 2026-03-10 10:15:00
        struct tm tm_mock = {0};
        tm_mock.tm_year = 2026 - 1900;
        tm_mock.tm_mon = 2;
        tm_mock.tm_mday = 10;
        tm_mock.tm_hour = 10;
        tm_mock.tm_min = 15;
        tm_mock.tm_isdst = -1;
        time_t now = mktime(&tm_mock);
        
        long long last_run = 0;

        // Scheduled for hh:15 hourly
        CHECK(CWScheduler::isDue(now, 1, 0, 15, 3, 0, false, last_run) == 1);
        CHECK(last_run == now);

        // 10:20 (same hour, same interval), should not run
        time_t now_10_20 = now + 300;
        CHECK(CWScheduler::isDue(now_10_20, 1, 0, 15, 3, 0, false, last_run) == 0);

        // 11:10 (next hour, hasn't reached minute), should not run
        time_t now_11_10 = now + 3300;
        CHECK(CWScheduler::isDue(now_11_10, 1, 0, 15, 3, 0, false, last_run) == 0);

        // 11:15 (next hour, reached minute), should run
        time_t now_11_15 = now + 3600;
        CHECK(CWScheduler::isDue(now_11_15, 1, 0, 15, 3, 0, false, last_run) == 1);
        CHECK(last_run == now_11_15);

        // Run missed true: exact same hour, missed it by 10 mins (PC was off, turned on at 10:25)
        last_run = now - 3600; // Last ran 9:15
        time_t now_10_25 = now + 600;
        CHECK(CWScheduler::isDue(now_10_25, 1, 0, 15, 3, 0, true, last_run) == 1);
        CHECK(last_run == now_10_25);
    }

    TEST_CASE("isDue workdays scheduling")
    {
        // 2026-03-09 = Monday
        struct tm tm_mock = {0};
        tm_mock.tm_year = 2026 - 1900;
        tm_mock.tm_mon = 2;
        tm_mock.tm_mday = 9;
        tm_mock.tm_hour = 10;
        tm_mock.tm_isdst = -1;
        time_t monday_10_00 = mktime(&tm_mock);

        long long last_run = 0;

        // Monday 10:00 -> should run
        CHECK(CWScheduler::isDue(monday_10_00, 1, 10, 0, 1, 0, false, last_run) == 1);
        
        // Advance to Saturday 10:00
        time_t saturday_10_00 = monday_10_00 + 5 * 86400;
        CHECK(CWScheduler::isDue(saturday_10_00, 1, 10, 0, 1, 0, false, last_run) == 0);

        // Test Workdays RunMissed gap > 3.5 days.
        // Last run Friday 10:00. Now Monday 8:00 AM (missed gap of 2.9 days)
        long long friday_10_00 = monday_10_00 - 3 * 86400;
        time_t monday_08_00 = monday_10_00 - 7200;
        last_run = friday_10_00;
        
        // Gap is < 84h, should NOT run as "missed", because Friday to Monday isn't a missed interval!
        CHECK(CWScheduler::isDue(monday_08_00, 1, 10, 0, 1, 0, true, last_run) == 0);
        
        // Wait, what if someone turns on PC at 11:00 AM Monday?
        time_t monday_11_00 = monday_10_00 + 3600;
        // Should run because tm_hour > hour
        CHECK(CWScheduler::isDue(monday_11_00, 1, 10, 0, 1, 0, true, last_run) == 1);
        CHECK(last_run == monday_11_00);
    }

    TEST_CASE("CWScheduler integration check")
    {
        // Simple test to ensure check() correctly posts messages and updates config's last run times
        // Fixed "now" for deterministic behavior: 2026-03-10 10:00:00
        struct tm tm_mock = {0};
        tm_mock.tm_year = 2026 - 1900;
        tm_mock.tm_mon = 2; // March
        tm_mock.tm_mday = 10;
        tm_mock.tm_hour = 10;
        tm_mock.tm_min = 0;
        tm_mock.tm_sec = 0;
        tm_mock.tm_isdst = -1;
        time_t now = mktime(&tm_mock);

        CWConfig cfg;
        cfg.defaults(); // Apply defaults
        cfg.scanScheduled = true;
        cfg.scanRunMissed = true;
        cfg.scanFrequency = 0; // Daily
        cfg.scanLastRunTime = 0;
        cfg.updateScheduled = true;
        cfg.updateRunMissed = true;
        cfg.updateFrequency = 0;
        cfg.updateLastRunTime = 0;
        cfg.scanHour = tm_mock.tm_hour;
        cfg.scanMinute = tm_mock.tm_min;
        cfg.scanDay = tm_mock.tm_wday;
        cfg.updateHour = tm_mock.tm_hour;
        cfg.updateMinute = tm_mock.tm_min;

        // Simulate that the last runs were 3 days ago, so they definitely trigger runMissed logic.
        time_t past = now - (3 * 86400);
        cfg.scanLastRunTime = past;
        cfg.updateLastRunTime = past;

        // Create a dummy message loop window to catch postmessages
        HWND hwnd = CreateWindowA("STATIC", "Test", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        REQUIRE(hwnd != NULL);

        long long dummy_scan_last = past;
        struct tm* tm = localtime(&now);
        int tm_hour = tm ? tm->tm_hour : -1;
        int tm_yday = tm ? tm->tm_yday : -1;
        
        time_t last_rt = (time_t)dummy_scan_last;
        struct tm* ltm = localtime(&last_rt);
        int ltm_yday = ltm ? ltm->tm_yday : -1;
        
        int gap = (int)(now - last_rt);
        int due = CWScheduler::isDue(now, cfg.scanScheduled, cfg.scanHour, cfg.scanMinute, cfg.scanFrequency, cfg.scanDay, cfg.scanRunMissed, dummy_scan_last);

        MESSAGE("now: ", now, " past: ", past, " gap: ", gap, " freq: ", cfg.scanFrequency, " scheduled: ", cfg.scanScheduled, " runMissed: ", cfg.scanRunMissed, " tm_hour: ", tm_hour, " cfg.scanHour: ", cfg.scanHour, " tm_yday: ", tm_yday, " ltm_yday: ", ltm_yday);
        CHECK_MESSAGE(due == 1, "isDue directly failed to return 1. dummy_scan_last changed? " << (dummy_scan_last != past));

        CWScheduler sched;
        sched.start(hwnd, &cfg);
        
        // Let's verify start() -> check() triggered runMissed logic.
        // If they ran, the last run times in the config should be updated to a positive timestamp.
        CHECK(cfg.scanLastRunTime > 0);
        CHECK(cfg.updateLastRunTime > 0);
        CHECK(cfg.scanLastRunTime != past);
        CHECK(cfg.updateLastRunTime != past);

        sched.stop();
        DestroyWindow(hwnd);
    }
}

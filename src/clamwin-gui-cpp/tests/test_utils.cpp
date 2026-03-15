#include "doctest.h"
#include "cw_gui_shared.h"
#include "cw_config.h"
#include "test_support.h"

#include <time.h>

namespace
{
bool setFileMTimeDaysAgo(const std::string& path, int daysAgo)
{
    HANDLE h = CreateFileA(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    SYSTEMTIME stNow;
    GetSystemTime(&stNow);

    struct tm tmUtc;
    memset(&tmUtc, 0, sizeof(tmUtc));
    tmUtc.tm_year = stNow.wYear - 1900;
    tmUtc.tm_mon  = stNow.wMonth - 1;
    tmUtc.tm_mday = stNow.wDay;
    tmUtc.tm_hour = stNow.wHour;
    tmUtc.tm_min  = stNow.wMinute;
    tmUtc.tm_sec  = stNow.wSecond;

    time_t nowUtc = _mkgmtime(&tmUtc);
    if (nowUtc == (time_t)-1)
    {
        CloseHandle(h);
        return false;
    }

    time_t targetUtc = nowUtc - (time_t)daysAgo * 86400;
    struct tm* targetTm = gmtime(&targetUtc);
    if (!targetTm)
    {
        CloseHandle(h);
        return false;
    }

    SYSTEMTIME stTarget;
    memset(&stTarget, 0, sizeof(stTarget));
    stTarget.wYear   = (WORD)(targetTm->tm_year + 1900);
    stTarget.wMonth  = (WORD)(targetTm->tm_mon + 1);
    stTarget.wDay    = (WORD)targetTm->tm_mday;
    stTarget.wHour   = (WORD)targetTm->tm_hour;
    stTarget.wMinute = (WORD)targetTm->tm_min;
    stTarget.wSecond = (WORD)targetTm->tm_sec;

    FILETIME ft;
    bool ok = (SystemTimeToFileTime(&stTarget, &ft) != 0) &&
              (SetFileTime(h, NULL, NULL, &ft) != 0);

    CloseHandle(h);
    return ok;
}
}

TEST_SUITE("utils")
{
    TEST_CASE("CW_GetDBInfo returns 0 when DB files missing")
    {
        TestTempDir temp;
        CW_DBInfo info;
        CHECK(CW_GetDBInfo(temp.path.c_str(), &info) == 0);
        CHECK(info.main_ver == 0);
        CHECK(info.daily_ver == 0);
    }

    TEST_CASE("CW_GetProtectionStatus returns WARN/ERROR by DB age")
    {
        TestTempDir temp;

        std::string daily = testJoinPath(temp.path, "daily.cvd");
        REQUIRE(testWriteFile(daily, "ClamAV-VDB:dummy:1:123:rest\n"));

        CWConfig cfg;
        cfg.defaults();
        cfg.databasePath = temp.path;

        REQUIRE(setFileMTimeDaysAgo(daily, 6));
        CHECK(CW_GetProtectionStatus(&cfg) == CW_STATUS_WARN);

        REQUIRE(setFileMTimeDaysAgo(daily, 16));
        CHECK(CW_GetProtectionStatus(&cfg) == CW_STATUS_ERROR);
    }
}

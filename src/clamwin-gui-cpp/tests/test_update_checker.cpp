#include "doctest.h"
#include "cw_update_checker.h"
#include "cw_gui_shared.h"

#include <stdio.h>

namespace
{
void makeVersion(char* out, size_t outCap, int maj, int min, int pat)
{
    if (!out || outCap == 0)
        return;
    snprintf(out, outCap, "%d.%d.%d", maj, min, pat);
    out[outCap - 1] = '\0';
}
}

TEST_SUITE("UpdateChecker")
{

TEST_CASE("parseVersion: standard major.minor.patch")
{
    int maj = 0, min = 0, pat = 0;
    CHECK(CWUpdateChecker::parseVersion("1.2.3", maj, min, pat));
    CHECK(maj == 1);
    CHECK(min == 2);
    CHECK(pat == 3);
}

TEST_CASE("parseVersion: leading v prefix")
{
    int maj = 0, min = 0, pat = 0;
    CHECK(CWUpdateChecker::parseVersion("v1.0.5", maj, min, pat));
    CHECK(maj == 1);
    CHECK(min == 0);
    CHECK(pat == 5);
}

TEST_CASE("parseVersion: epoch prefix like 0:0.103.2")
{
    int maj = 0, min = 0, pat = 0;
    CHECK(CWUpdateChecker::parseVersion("0:0.103.2", maj, min, pat));
    CHECK(maj == 0);
    CHECK(min == 103);
    CHECK(pat == 2);
}

TEST_CASE("parseVersion: major.minor only")
{
    int maj = 0, min = 0, pat = 0;
    CHECK(CWUpdateChecker::parseVersion("2.5", maj, min, pat));
    CHECK(maj == 2);
    CHECK(min == 5);
    CHECK(pat == 0);
}

TEST_CASE("parseVersion: invalid input returns false")
{
    int maj = 0, min = 0, pat = 0;
    CHECK_FALSE(CWUpdateChecker::parseVersion("", maj, min, pat));
    CHECK_FALSE(CWUpdateChecker::parseVersion(NULL, maj, min, pat));
    CHECK_FALSE(CWUpdateChecker::parseVersion("abc", maj, min, pat));
}

TEST_CASE("isNewerVersion: newer major")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char remote[32];
    makeVersion(remote, sizeof(remote), maj + 1, 0, 0);
    CHECK(CWUpdateChecker::isNewerVersion(remote));
}

TEST_CASE("isNewerVersion: newer minor")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char remote[32];
    makeVersion(remote, sizeof(remote), maj, min + 1, 0);
    CHECK(CWUpdateChecker::isNewerVersion(remote));
}

TEST_CASE("isNewerVersion: newer patch")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char remote[32];
    makeVersion(remote, sizeof(remote), maj, min, pat + 1);
    CHECK(CWUpdateChecker::isNewerVersion(remote));
}

TEST_CASE("isNewerVersion: same version is not newer")
{
    CHECK_FALSE(CWUpdateChecker::isNewerVersion(CLAMWIN_VERSION_STR));
}

TEST_CASE("isNewerVersion: older version is not newer")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char olderPatch[32];
    if (pat > 0)
    {
        makeVersion(olderPatch, sizeof(olderPatch), maj, min, pat - 1);
        CHECK_FALSE(CWUpdateChecker::isNewerVersion(olderPatch));
    }

    if (min > 0)
    {
        char olderMinor[32];
        makeVersion(olderMinor, sizeof(olderMinor), maj, min - 1, pat);
        CHECK_FALSE(CWUpdateChecker::isNewerVersion(olderMinor));
    }
}

TEST_CASE("isNewerVersion: v-prefix handled")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char newer[32];
    char same[32];
    makeVersion(newer, sizeof(newer), maj, min + 1, 0);
    makeVersion(same, sizeof(same), maj, min, pat);

    char newerPrefixed[36];
    char samePrefixed[36];
    snprintf(newerPrefixed, sizeof(newerPrefixed), "v%s", newer);
    snprintf(samePrefixed, sizeof(samePrefixed), "v%s", same);

    CHECK(CWUpdateChecker::isNewerVersion(newerPrefixed));
    CHECK_FALSE(CWUpdateChecker::isNewerVersion(samePrefixed));
}

TEST_CASE("isNewerVersion: epoch prefix handled")
{
    int maj = 0, min = 0, pat = 0;
    REQUIRE(CWUpdateChecker::parseVersion(CLAMWIN_VERSION_STR, maj, min, pat));

    char newer[32];
    char same[32];
    makeVersion(newer, sizeof(newer), maj, min + 1, 0);
    makeVersion(same, sizeof(same), maj, min, pat);

    char newerEpoch[40];
    char sameEpoch[40];
    snprintf(newerEpoch, sizeof(newerEpoch), "0:%s", newer);
    snprintf(sameEpoch, sizeof(sameEpoch), "0:%s", same);

    CHECK(CWUpdateChecker::isNewerVersion(newerEpoch));
    CHECK_FALSE(CWUpdateChecker::isNewerVersion(sameEpoch));
}

TEST_CASE("hardcoded URLs are HTTPS")
{
    const char* api = CWUpdateChecker::apiUrl();
    const char* dl  = CWUpdateChecker::downloadUrl();
    bool apiHttps = (strncmp(api, "https://", 8) == 0);
    bool dlHttps  = (strncmp(dl,  "https://", 8) == 0);
    CHECK(apiHttps);
    CHECK(dlHttps);
}

TEST_CASE("download URL points to clamwin.com")
{
    const char* dl = CWUpdateChecker::downloadUrl();
    bool hasClamwin = (strstr(dl, "clamwin.com") != nullptr);
    CHECK(hasClamwin);
}

} /* TEST_SUITE */

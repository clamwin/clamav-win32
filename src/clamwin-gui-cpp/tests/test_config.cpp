#include "doctest.h"
#include "cw_config.h"
#include "test_support.h"

namespace
{
void checkConfigEqual(const CWConfig& expected, const CWConfig& actual)
{
    CHECK(expected.scanRecursive == actual.scanRecursive);
    CHECK(expected.scanArchives == actual.scanArchives);
    CHECK(expected.scanOle2 == actual.scanOle2);
    CHECK(expected.scanMail == actual.scanMail);
    CHECK(expected.infectedAction == actual.infectedAction);
    CHECK(expected.maxScanSizeMb == actual.maxScanSizeMb);
    CHECK(expected.maxFileSizeMb == actual.maxFileSizeMb);
    CHECK(expected.maxFiles == actual.maxFiles);
    CHECK(expected.maxDepth == actual.maxDepth);
    CHECK(expected.databasePath == actual.databasePath);
    CHECK(expected.quarantinePath == actual.quarantinePath);
    CHECK(expected.scanLogFile == actual.scanLogFile);
    CHECK(expected.updateLogFile == actual.updateLogFile);
    CHECK(expected.dbMirror == actual.dbMirror);
    CHECK(expected.updateOnStartup == actual.updateOnStartup);
    CHECK(expected.proxyEnabled == actual.proxyEnabled);
    CHECK(expected.proxyHost == actual.proxyHost);
    CHECK(expected.proxyPort == actual.proxyPort);
    CHECK(expected.proxyUser == actual.proxyUser);
    CHECK(expected.proxyPass == actual.proxyPass);
    CHECK(expected.emailEnabled == actual.emailEnabled);
    CHECK(expected.emailFrom == actual.emailFrom);
    CHECK(expected.emailTo == actual.emailTo);
    CHECK(expected.emailSmtp == actual.emailSmtp);
    CHECK(expected.scanScheduled == actual.scanScheduled);
    CHECK(expected.scanHour == actual.scanHour);
    CHECK(expected.scanMinute == actual.scanMinute);
    CHECK(expected.scanFrequency == actual.scanFrequency);
    CHECK(expected.scanDay == actual.scanDay);
    CHECK(expected.scanPath == actual.scanPath);
    CHECK(expected.scanDescription == actual.scanDescription);
    CHECK(expected.scanMemory == actual.scanMemory);
    CHECK(expected.updateScheduled == actual.updateScheduled);
    CHECK(expected.updateHour == actual.updateHour);
    CHECK(expected.updateMinute == actual.updateMinute);
    CHECK(expected.updateFrequency == actual.updateFrequency);
    CHECK(expected.includePatterns == actual.includePatterns);
    CHECK(expected.excludePatterns == actual.excludePatterns);
    CHECK(expected.priority == actual.priority);
}
}

TEST_SUITE("config")
{
    TEST_CASE("load missing file uses defaults")
    {
        TestTempDir tempDir;
        CWConfig cfg;
        std::string iniPath = testJoinPath(tempDir.path, "missing.conf");

        bool loaded = cfg.load(iniPath);
        CHECK_FALSE(loaded);
        CHECK(cfg.scanRecursive);
        CHECK(cfg.dbMirror == "database.clamav.net");
    }

    TEST_CASE("save and reload preserves all fields")
    {
        TestTempDir tempDir;
        CWConfig expected;
        expected.iniPath = testJoinPath(tempDir.path, "roundtrip.conf");
        expected.scanRecursive = false;
        expected.scanArchives = false;
        expected.scanOle2 = false;
        expected.scanMail = true;
        expected.infectedAction = 2;
        expected.maxScanSizeMb = 321;
        expected.maxFileSizeMb = 123;
        expected.maxFiles = 654;
        expected.maxDepth = 33;
        expected.databasePath = "C:\\db folder\\main";
        expected.quarantinePath = "C:\\Quarantine Folder";
        expected.scanLogFile = "C:\\logs\\scan log.txt";
        expected.updateLogFile = "C:\\logs\\update log.txt";
        expected.dbMirror = "mirror.example.test";
        expected.updateOnStartup = true;
        expected.proxyEnabled = true;
        expected.proxyHost = "proxy.example.test";
        expected.proxyPort = 8080;
        expected.proxyUser = "proxy-user";
        expected.proxyPass = "proxy-pass";
        expected.emailEnabled = true;
        expected.emailFrom = "from@example.test";
        expected.emailTo = "to@example.test";
        expected.emailSmtp = "smtp.example.test";
        expected.scanScheduled = true;
        expected.scanHour = 6;
        expected.scanMinute = 45;
        expected.scanFrequency = 2;
        expected.scanDay = 4;
        expected.scanPath = "C:\\Scan Root";
        expected.scanDescription = "Nightly scan";
        expected.scanMemory = true;
        expected.updateScheduled = false;
        expected.updateHour = 9;
        expected.updateMinute = 15;
        expected.updateFrequency = 3;
        expected.includePatterns = "*.exe|CLAMWIN_SEP|<^important$>";
        expected.excludePatterns = "*.tmp|CLAMWIN_SEP|*.bak";
        expected.priority = "l";

        REQUIRE(expected.save());

        CWConfig actual;
        REQUIRE(actual.load(expected.iniPath));
        checkConfigEqual(expected, actual);
    }

    TEST_CASE("load normalizes legacy filter patterns")
    {
        TestTempDir tempDir;
        std::string iniPath = testJoinPath(tempDir.path, "legacy.conf");
        std::string content =
            "[ClamAV]\r\n"
            "IncludePatterns=*.txt\r\n"
            "ExcludePatterns=*.dbx|CLAMWIN_SEP|bb|CLAMWIN_SEP|st\r\n";

        REQUIRE(testWriteFile(iniPath, content));

        CWConfig cfg;
        REQUIRE(cfg.load(iniPath));
        CHECK(cfg.includePatterns == "*.txt");
        CHECK(cfg.excludePatterns == "*.dbx|CLAMWIN_SEP|*.tbb|CLAMWIN_SEP|*.pst");
    }

    TEST_CASE("save creates parent directory")
    {
        TestTempDir tempDir;
        std::string nestedDir = testJoinPath(tempDir.path, "nested");
        std::string iniPath = testJoinPath(nestedDir, "created.conf");

        CWConfig cfg;
        cfg.iniPath = iniPath;
        REQUIRE(cfg.save());
        CHECK(GetFileAttributesA(nestedDir.c_str()) != INVALID_FILE_ATTRIBUTES);
        CHECK(GetFileAttributesA(iniPath.c_str()) != INVALID_FILE_ATTRIBUTES);
    }

    TEST_CASE("blank log path values fall back to defaults")
    {
        TestTempDir tempDir;
        std::string iniPath = testJoinPath(tempDir.path, "blank-logs.conf");
        std::string content =
            "[ClamAV]\r\n"
            "LogFile=\r\n"
            "[Updates]\r\n"
            "UpdateLog=\r\n";

        REQUIRE(testWriteFile(iniPath, content));

        CWConfig cfg;
        REQUIRE(cfg.load(iniPath));
        CHECK_FALSE(cfg.scanLogFile.empty());
        CHECK_FALSE(cfg.updateLogFile.empty());
        CHECK(cfg.scanLogFile.find("ClamScan.log") != std::string::npos);
        CHECK(cfg.updateLogFile.find("FreshClam.log") != std::string::npos);
    }
}

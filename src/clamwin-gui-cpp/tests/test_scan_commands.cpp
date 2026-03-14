#include "doctest.h"
#include "cw_scan_logic.h"
#include "test_support.h"

#include <string>

namespace
{
CWConfig makeBaseConfig(void)
{
    CWConfig cfg;
    cfg.databasePath = "C:\\db path";
    cfg.quarantinePath = "C:\\Quarantine Path";
    cfg.scanLogFile = "C:\\logs\\scan report.log";
    cfg.updateLogFile = "C:\\logs\\update report.log";
    cfg.includePatterns = "*.exe|CLAMWIN_SEP|<^important$>";
    cfg.excludePatterns = "*.tmp|CLAMWIN_SEP|*.bak";
    return cfg;
}
}

TEST_SUITE("scan_commands")
{
    TEST_CASE("clamscan command for directory includes expected switches")
    {
        TestTempDir tempDir;
        std::string targetDir = testJoinPath(tempDir.path, "scan root");
        REQUIRE(testMakeDirectory(targetDir));

        CWConfig cfg = makeBaseConfig();
        cfg.infectedAction = 2;

        std::string cmd = CWScanLogic::buildClamscanCommand(cfg, targetDir, "C:\\Program Files\\ClamWin\\");

        CHECK(cmd.find("\"C:\\Program Files\\ClamWin\\clamscan.exe\"") != std::string::npos);
        CHECK(cmd.find("--database=\"C:\\db path\"") != std::string::npos);
        CHECK(cmd.find("--max-files=500") != std::string::npos);
        CHECK(cmd.find("--max-scansize=150M") != std::string::npos);
        CHECK(cmd.find("--max-recursion=50") != std::string::npos);
        CHECK(cmd.find("--max-filesize=100M") != std::string::npos);
        CHECK(cmd.find("--move=\"C:\\Quarantine Path\"") != std::string::npos);
        CHECK(cmd.find("--recursive") != std::string::npos);
        CHECK(cmd.find("--include=[^\\\\/]*\\.exe$") != std::string::npos);
        CHECK(cmd.find("--include=^important$") != std::string::npos);
        CHECK(cmd.find("--exclude=[^\\\\/]*\\.tmp$") != std::string::npos);
        CHECK(cmd.find("--log=\"C:\\logs\\scan report.log\"") != std::string::npos);
        CHECK(cmd.find(std::string("\"") + targetDir + "\"") != std::string::npos);
    }

    TEST_CASE("clamscan command for file skips preference filters")
    {
        TestTempDir tempDir;
        std::string targetFile = testJoinPath(tempDir.path, "sample.txt");
        REQUIRE(testWriteFile(targetFile, "sample"));

        CWConfig cfg = makeBaseConfig();
        std::string cmd = CWScanLogic::buildClamscanCommand(cfg, targetFile, "C:\\ClamWin");

        CHECK(cmd.find("--include=") == std::string::npos);
        CHECK(cmd.find("--exclude=") == std::string::npos);
    }

    TEST_CASE("clamscan command handles archive and remove modes")
    {
        CWConfig cfg = makeBaseConfig();
        cfg.scanArchives = false;
        cfg.scanOle2 = false;
        cfg.scanRecursive = false;
        cfg.infectedAction = 1;

        std::string cmd = CWScanLogic::buildClamscanCommand(cfg, "C:\\scan-root", "C:\\ClamWin");

        CHECK(cmd.find("--scan-archive=no") != std::string::npos);
        CHECK(cmd.find("--scan-ole2=no") != std::string::npos);
        CHECK(cmd.find("--remove") != std::string::npos);
        CHECK(cmd.find("--recursive") == std::string::npos);
    }

    TEST_CASE("freshclam command uses optional local paths")
    {
        TestTempDir tempDir;
        REQUIRE(testWriteFile(testJoinPath(tempDir.path, "freshclam.exe"), ""));
        REQUIRE(testWriteFile(testJoinPath(tempDir.path, "freshclam.conf"), "DatabaseMirror database.clamav.net\n"));
        REQUIRE(testMakeDirectory(testJoinPath(tempDir.path, "certs")));

        CWConfig cfg = makeBaseConfig();
        std::string cmd = CWScanLogic::buildFreshclamCommand(cfg, tempDir.path);

        CHECK(cmd.find(std::string("\"") + testJoinPath(tempDir.path, "freshclam.exe") + "\"") != std::string::npos);
        CHECK(cmd.find(std::string("--config-file=\"") + testJoinPath(tempDir.path, "freshclam.conf") + "\"") != std::string::npos);
        CHECK(cmd.find(std::string("--cvdcertsdir=\"") + testJoinPath(tempDir.path, "certs") + "\"") != std::string::npos);
        CHECK(cmd.find("--datadir=\"C:\\db path\"") != std::string::npos);
        CHECK(cmd.find("--log=\"C:\\logs\\update report.log\"") != std::string::npos);
    }

    TEST_CASE("freshclam executable preflight detects missing and present binaries")
    {
        TestTempDir tempDir;

        CHECK_FALSE(CWScanLogic::hasFreshclamExecutable(tempDir.path));

        std::string freshclamPath = CWScanLogic::buildFreshclamExecutablePath(tempDir.path);
        REQUIRE(testWriteFile(freshclamPath, ""));
        CHECK(CWScanLogic::hasFreshclamExecutable(tempDir.path));
    }

    TEST_CASE("clamscan command escapes embedded quotes")
    {
        CWConfig cfg = makeBaseConfig();
        cfg.infectedAction = 2;
        cfg.databasePath = "C:\\unsafe\"path";
        cfg.quarantinePath = "C:\\quarantine\"folder";

        std::string cmd = CWScanLogic::buildClamscanCommand(cfg, "C:\\scan-root", "C:\\ClamWin");

        CHECK(cmd.find("--database=\"C:\\unsafe\\\"path\"") != std::string::npos);
        CHECK(cmd.find("--move=\"C:\\quarantine\\\"folder\"") != std::string::npos);
    }

    TEST_CASE("clamscan memory command uses memory switch without path filters")
    {
        CWConfig cfg = makeBaseConfig();
        cfg.scanRecursive = true;

        std::string cmd = CWScanLogic::buildClamscanCommand(cfg, "", "C:\\ClamWin", true);

        CHECK(cmd.find("\"C:\\ClamWin\\clamscan.exe\"") != std::string::npos);
        CHECK(cmd.find("--memory") != std::string::npos);
        CHECK(cmd.find("--recursive") == std::string::npos);
        CHECK(cmd.find("--include=") == std::string::npos);
        CHECK(cmd.find("--exclude=") == std::string::npos);
        CHECK(cmd.find("\"\"") == std::string::npos);
    }
}

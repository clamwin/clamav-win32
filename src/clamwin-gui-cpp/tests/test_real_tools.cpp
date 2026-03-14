#include "doctest.h"
#include "cw_scan_logic.h"
#include "test_support.h"

#include <string>
#include <vector>

namespace
{
bool realToolsEnabled(void)
{
    return testGetEnv("CLAMWIN_REAL_TOOLS") == "1";
}

bool realFreshclamUpdateEnabled(void)
{
    return testGetEnv("CLAMWIN_REAL_FRESHCLAM_UPDATE") == "1";
}

bool realFreshclamNegativeEnabled(void)
{
    return testGetEnv("CLAMWIN_REAL_FRESHCLAM_NEGATIVE") == "1";
}

std::string resolveToolPath(const char* envVar, const char* fileName)
{
    std::string fromEnv = testGetEnv(envVar);
    if (!fromEnv.empty())
        return fromEnv;
    return testJoinPath(testExecutableDir(), fileName);
}

std::vector<std::string> splitLines(const std::string& text)
{
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= text.size())
    {
        size_t end = text.find_first_of("\r\n", start);
        std::string line = (end == std::string::npos) ? text.substr(start) : text.substr(start, end - start);
        if (!line.empty())
            lines.push_back(line);
        if (end == std::string::npos)
            break;
        start = end + 1;
        if (start < text.size() && text[start] == '\n' && text[end] == '\r')
            ++start;
    }
    return lines;
}
}

TEST_SUITE("real_tools")
{
    TEST_CASE("real clamscan version output is available" * doctest::skip(!realToolsEnabled()))
    {
        std::string clamscanPath = resolveToolPath("CLAMWIN_CLAMSCAN_EXE", "clamscan.exe");
        REQUIRE(testPathExists(clamscanPath));

        std::string output;
        int exitCode = -1;
        REQUIRE(testRunCommandCapture(std::string("\"") + clamscanPath + "\" --version",
                                      testExecutableDir(),
                                      output,
                                      exitCode));
        CHECK(exitCode == 0);
        CHECK(output.find("ClamAV") != std::string::npos);
    }

    TEST_CASE("real clamscan clean fixture scan emits parseable output" * doctest::skip(!realToolsEnabled()))
    {
        std::string clamscanPath = resolveToolPath("CLAMWIN_CLAMSCAN_EXE", "clamscan.exe");
        std::string dbDir = testGetEnv("CLAMWIN_DB_DIR");
        if (dbDir.empty())
            dbDir = testJoinPath(testExecutableDir(), "db");

        REQUIRE(testPathExists(clamscanPath));
        REQUIRE(testPathExists(dbDir));

        TestTempDir tempDir;
        std::string cleanDir = testJoinPath(tempDir.path, "clean");
        REQUIRE(testMakeDirectory(cleanDir));
        REQUIRE(testWriteFile(testJoinPath(cleanDir, "sample.txt"), "clean file\n"));

        std::string output;
        int exitCode = -1;
        std::string cmd = std::string("\"") + clamscanPath + "\" --stdout --show-progress --database=\"" + dbDir + "\" \"" + cleanDir + "\"";
        REQUIRE(testRunCommandCapture(cmd, testExecutableDir(), output, exitCode));
        CHECK((exitCode == 0 || exitCode == 1));

        CWScanLogic::ScanOutputState state;
        CWScanLogic::initScanOutputState(state, false);
        std::vector<std::string> lines = splitLines(output);
        for (size_t i = 0; i < lines.size(); ++i)
            CWScanLogic::processOutputLine(state, lines[i].c_str(), false);

        CHECK(output.find("Scanned files:") != std::string::npos);
        CHECK(state.filesScanned >= 1);
    }

    TEST_CASE("real freshclam version output is available" * doctest::skip(!realToolsEnabled()))
    {
        std::string freshclamPath = resolveToolPath("CLAMWIN_FRESHCLAM_EXE", "freshclam.exe");
        std::string configPath = testJoinPath(testExecutableDir(), "freshclam.conf");
        REQUIRE(testPathExists(freshclamPath));
        REQUIRE(testPathExists(configPath));

        std::string output;
        int exitCode = -1;
        REQUIRE(testRunCommandCapture(std::string("\"") + freshclamPath + "\" --config-file=\"" + configPath + "\" --version",
                                      testExecutableDir(),
                                      output,
                                      exitCode));
        bool recognizedExit = (exitCode == 0 || exitCode == 2);
        bool recognizedBanner = (output.find("freshclam") != std::string::npos ||
                                 output.find("ClamAV") != std::string::npos);
        CHECK(recognizedExit);
        CHECK(recognizedBanner);
    }

    TEST_CASE("real freshclam update output is parseable when enabled" * doctest::skip(!realToolsEnabled() || !realFreshclamUpdateEnabled()))
    {
        std::string freshclamPath = resolveToolPath("CLAMWIN_FRESHCLAM_EXE", "freshclam.exe");
        std::string configPath = testJoinPath(testExecutableDir(), "freshclam.conf");
        std::string dbDir = testGetEnv("CLAMWIN_DB_DIR");
        std::string certsDir = testJoinPath(testExecutableDir(), "certs");
        if (dbDir.empty())
            dbDir = testJoinPath(testExecutableDir(), "db");

        REQUIRE(testPathExists(freshclamPath));
        REQUIRE(testPathExists(configPath));
        REQUIRE(testPathExists(dbDir));
        REQUIRE(testPathExists(certsDir));

        std::string output;
        int exitCode = -1;
        std::string cmd = std::string("\"") + freshclamPath + "\" --config-file=\"" + configPath +
            "\" --stdout --show-progress --datadir=\"" + dbDir + "\" --cvdcertsdir=\"" + certsDir +
            "\" --checks=1 --update-db=daily";
        REQUIRE(testRunCommandCapture(cmd, testExecutableDir(), output, exitCode));

        bool sawParseableProgress = false;
        bool sawRelevantStatus = false;
        std::vector<std::string> lines = splitLines(output);
        CWScanLogic::ScanOutputState state;
        CWScanLogic::initScanOutputState(state, true);

        for (size_t i = 0; i < lines.size(); ++i)
        {
            double downloaded = 0.0;
            double total = 0.0;
            if (CWScanLogic::parseFreshclamProgressLine(lines[i].c_str(), &downloaded, &total))
                sawParseableProgress = true;

            if (lines[i].find(" database available for download") != std::string::npos ||
                lines[i].find(" database is up-to-date") != std::string::npos ||
                lines[i].find(" updated (version") != std::string::npos)
                sawRelevantStatus = true;

            CWScanLogic::processOutputLine(state, lines[i].c_str(), false);
        }

        bool recognizedExit = (exitCode == 0 || exitCode == 1 || exitCode == 52 || exitCode == 56);
        bool parseableOutput = (sawParseableProgress || sawRelevantStatus || state.updateUpToDateCount > 0 || state.updateHadChanges);
        CHECK(recognizedExit);
        CHECK(parseableOutput);
    }

    TEST_CASE("real freshclam invalid config emits parseable error output when enabled" * doctest::skip(!realToolsEnabled() || !realFreshclamNegativeEnabled()))
    {
        std::string freshclamPath = resolveToolPath("CLAMWIN_FRESHCLAM_EXE", "freshclam.exe");
        REQUIRE(testPathExists(freshclamPath));

        TestTempDir tempDir;
        std::string configPath = testJoinPath(tempDir.path, "freshclam-invalid.conf");
        REQUIRE(testWriteFile(configPath, "ThisDirectiveDoesNotExist yes\r\n"));

        std::string output;
        int exitCode = -1;
        std::string cmd = std::string("\"") + freshclamPath + "\" --config-file=\"" + configPath + "\" --version";
        REQUIRE(testRunCommandCapture(cmd, testExecutableDir(), output, exitCode));

        CHECK(exitCode != 0);
        CHECK(output.find("Parse error") != std::string::npos);
        CHECK(output.find("Unknown option ThisDirectiveDoesNotExist") != std::string::npos);
        CHECK(output.find("Can't open/parse the config file") != std::string::npos);

        CWScanLogic::ScanOutputState state;
        CWScanLogic::initScanOutputState(state, true);
        std::vector<std::string> lines = splitLines(output);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            CWScanLogic::ScanLineEffects effects = CWScanLogic::processOutputLine(state, lines[i].c_str(), true);
            CHECK(effects.appendToLog);
            CHECK_FALSE(effects.statusChanged);
            CHECK_FALSE(effects.progressChanged);
        }

        CHECK_FALSE(state.updateProgressKnown);
        CHECK_FALSE(state.updateHadChanges);
        CHECK(state.updateUpToDateCount == 0);
    }
}

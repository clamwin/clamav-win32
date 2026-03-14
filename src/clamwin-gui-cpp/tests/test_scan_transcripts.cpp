#include "doctest.h"
#include "cw_scan_logic.h"
#include "test_support.h"

#include <string>
#include <vector>

namespace
{
CWScanLogic::ScanLineEffects feedLine(CWScanLogic::ScanOutputState& state, const char* text, bool isError)
{
    return CWScanLogic::processOutputLine(state, text, isError);
}

std::vector<std::string> readTranscript(const char* relativePath)
{
    std::string content;
    std::string path = testJoinPath(testFixturesDir(), relativePath);
    std::vector<std::string> lines;
    if (!testReadFile(path, content))
        return lines;

    size_t start = 0;
    while (start <= content.size())
    {
        size_t end = content.find_first_of("\r\n", start);
        std::string line = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        if (!line.empty())
            lines.push_back(line);
        if (end == std::string::npos)
            break;
        start = end + 1;
        if (start < content.size() && content[start] == '\n' && content[end] == '\r')
            ++start;
    }
    return lines;
}

std::vector<std::string> requireTranscript(const char* relativePath, size_t expectedLineCount)
{
    std::vector<std::string> lines = readTranscript(relativePath);
    REQUIRE(lines.size() == expectedLineCount);
    return lines;
}

CWScanLogic::ScanOutputState makeScanState(int expectedFiles)
{
    CWScanLogic::ScanOutputState state;
    CWScanLogic::initScanOutputState(state, false);
    state.scanExpectedFiles = expectedFiles;
    return state;
}

CWScanLogic::ScanOutputState makeUpdateState(void)
{
    CWScanLogic::ScanOutputState state;
    CWScanLogic::initScanOutputState(state, true);
    return state;
}
}

TEST_SUITE("scan_transcripts")
{
    TEST_CASE("clamscan transcript scenarios")
    {
        SUBCASE("updates db and file state")
        {
            CWScanLogic::ScanOutputState state = makeScanState(2);
            std::vector<std::string> lines = requireTranscript("transcripts\\clamscan\\load_and_scan.txt", 3);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Loading virus definitions... (50%)");
            CHECK_FALSE(effects.appendToLog);

            effects = feedLine(state, lines[1].c_str(), false);
            CHECK(state.scanCompletedFiles == 1);
            CHECK(state.filesScanned == 1);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "File scanned: 1/2");
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 50);

            effects = feedLine(state, lines[2].c_str(), false);
            CHECK(state.scanCompletedFiles == 2);
            CHECK(state.filesScanned == 2);
            CHECK(state.threatsFound == 1);
            CHECK(effects.progressPos == 100);
        }

        SUBCASE("ignores excluded results")
        {
            CWScanLogic::ScanOutputState state = makeScanState(3);
            std::vector<std::string> lines = requireTranscript("transcripts\\clamscan\\excluded.txt", 1);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(state.scanCompletedFiles == 0);
            CHECK(state.filesScanned == 0);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "File scanned: 0/3");
        }

        SUBCASE("partial failure keeps summary and progress consistent")
        {
            CWScanLogic::ScanOutputState state = makeScanState(2);
            std::vector<std::string> lines = requireTranscript("transcripts\\clamscan\\partial_failure.txt", 5);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(effects.statusChanged);
            CHECK_FALSE(effects.appendToLog);

            effects = feedLine(state, lines[1].c_str(), true);
            CHECK(state.scanCompletedFiles == 0);
            CHECK(state.filesScanned == 0);
            CHECK(effects.appendToLog);
            CHECK_FALSE(effects.progressChanged);

            effects = feedLine(state, lines[2].c_str(), false);
            CHECK(state.scanCompletedFiles == 1);
            CHECK(state.filesScanned == 1);
            CHECK(effects.progressPos == 50);

            effects = feedLine(state, lines[3].c_str(), false);
            CHECK(state.filesScanned == 1);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 50);

            effects = feedLine(state, lines[4].c_str(), false);
            CHECK(state.threatsFound == 0);
            CHECK(effects.progressPos == 50);
        }

        SUBCASE("unknown total uses running scanned count")
        {
            CWScanLogic::ScanOutputState state = makeScanState(0);
            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "C:\\WINDOWS\\system32\\napinsp.dll: OK",
                false);

            CHECK(state.scanCompletedFiles == 1);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Files scanned: 1");
            CHECK_FALSE(effects.statusAnimate);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 100);
        }
    }

    TEST_CASE("freshclam transcript scenarios")
    {
        SUBCASE("tracks progress and suppresses transient log lines")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();
            std::vector<std::string> lines = requireTranscript("transcripts\\freshclam\\download_progress.txt", 3);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(state.updateHadChanges);
            CHECK(effects.statusText == "Downloading daily.cvd...");

            effects = feedLine(state, lines[1].c_str(), false);
            CHECK(state.updateProgressKnown);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos > 0);
            CHECK(effects.statusText.find("Downloading daily.cvd:") == 0);
            CHECK(effects.appendToLog);

            effects = feedLine(state, lines[2].c_str(), false);
            CHECK_FALSE(effects.appendToLog);
        }

        SUBCASE("tracks up to date and updates installed db")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();
            std::vector<std::string> lines = requireTranscript("transcripts\\freshclam\\up_to_date.txt", 2);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(state.updateUpToDateCount == 1);
            CHECK(effects.statusText == "Virus definitions are already up to date");

            effects = feedLine(state, lines[1].c_str(), false);
            CHECK(state.updateCurrentDb == "daily.cvd");
            CHECK(effects.statusText == "Installed daily.cvd");
        }

        SUBCASE("resets progress across database switches")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();
            std::vector<std::string> lines = requireTranscript("transcripts\\freshclam\\multi_db_progress.txt", 4);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(effects.statusText == "Downloading daily.cvd...");

            effects = feedLine(state, lines[1].c_str(), false);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 90);
            CHECK(state.updateDownloadedBytes == doctest::Approx(90.0 * 1024.0 * 1024.0));
            CHECK(state.updateTransferredBytes == doctest::Approx(0.0));

            effects = feedLine(state, lines[2].c_str(), false);
            CHECK(effects.statusText == "Downloading bytecode.cvd...");
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 0);
            CHECK(state.updateDownloadedBytes == doctest::Approx(0.0));
            CHECK(state.updateTransferredBytes == doctest::Approx(90.0 * 1024.0 * 1024.0));

            effects = feedLine(state, lines[3].c_str(), false);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 5);
            CHECK(effects.statusText == "Downloading bytecode.cvd: 10.00 MiB / 200.00 MiB (5%)");
            CHECK(state.updateDownloadedBytes == doctest::Approx(10.0 * 1024.0 * 1024.0));
            CHECK(state.updateTransferredBytes == doctest::Approx(90.0 * 1024.0 * 1024.0));
        }

        SUBCASE("failure transcript preserves update context across stderr")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();
            std::vector<std::string> lines = requireTranscript("transcripts\\freshclam\\mirror_failure.txt", 4);

            CWScanLogic::ScanLineEffects effects = feedLine(state, lines[0].c_str(), false);
            CHECK(state.updateHadChanges);
            CHECK(state.updateCurrentDb == "daily.cvd");
            CHECK(effects.statusText == "Downloading daily.cvd...");

            effects = feedLine(state, lines[1].c_str(), true);
            CHECK(effects.appendToLog);
            CHECK_FALSE(effects.statusChanged);
            CHECK_FALSE(effects.progressChanged);

            effects = feedLine(state, lines[2].c_str(), true);
            CHECK(effects.appendToLog);
            CHECK_FALSE(effects.statusChanged);

            effects = feedLine(state, lines[3].c_str(), true);
            CHECK_FALSE(state.updateProgressKnown);
            CHECK(state.updateHadChanges);
            CHECK(state.updateUpToDateCount == 0);
            CHECK(state.updateCurrentDb == "daily.cvd");
        }

        SUBCASE("detects CDN cooldown and unsupported-version warnings")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();

            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "This means that you have been rate limited or blocked by the CDN.",
                true);
            CHECK(state.updateBlocked);
            CHECK_FALSE(state.updateUnsupportedVersion);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Update blocked by CDN cooldown");

            effects = feedLine(
                state,
                "Verify that you're running a supported ClamAV version.",
                true);
            CHECK(state.updateBlocked);
            CHECK(state.updateUnsupportedVersion);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Update blocked: unsupported ClamAV version");
        }

        SUBCASE("detects server or DNS resolution failures")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();

            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "WARNING: Failed to get main database version information from server:",
                false);
            CHECK(state.updateServerError);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Update failed: server/DNS resolution error");

            effects = feedLine(
                state,
                "ERROR: check_for_new_database_version: Failed to find main database using server https://database.clamav.net.",
                false);
            CHECK(state.updateServerError);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Update failed: server/DNS resolution error");
        }

        SUBCASE("suppresses freshclam time table rows")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();

            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "main database available for download (remote version: 63)",
                false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Downloading main.cvd...");

            effects = feedLine(
                state,
                "Time:    0.1s               [                         ]         0B/84.95MiB",
                false);
            CHECK_FALSE(effects.appendToLog);
            CHECK(effects.progressChanged);
            CHECK(effects.progressPos == 0);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Downloading main.cvd: 0.00 B / 84.95 MiB (0%)");
        }

        SUBCASE("switches status to verification after tiny download")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();

            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "main database available for download (remote version: 63)",
                false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Downloading main.cvd...");

            effects = feedLine(state, "8.87KiB/8.87KiB", false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Downloading main.cvd: 8.87 KiB / 8.87 KiB (100%)");

            effects = feedLine(
                state,
                "Testing database: 'C:\\\\tmp\\\\main.cvd' ...",
                false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Verifying main.cvd...");
        }

        SUBCASE("throttles status updates within same percent")
        {
            CWScanLogic::ScanOutputState state = makeUpdateState();

            CWScanLogic::ScanLineEffects effects = feedLine(
                state,
                "daily database available for download (remote version: 27933)",
                false);
            CHECK(effects.statusChanged);
            CHECK(effects.statusText == "Downloading daily.cvd...");

            effects = feedLine(state, "8.10MiB/100.0MiB", false);
            CHECK(effects.progressChanged);
            CHECK(effects.statusChanged);
            CHECK(effects.progressPos == 8);

            effects = feedLine(state, "8.20MiB/100.0MiB", false);
            CHECK_FALSE(effects.progressChanged);
            CHECK_FALSE(effects.statusChanged);
        }
    }

    TEST_CASE("stderr output handling")
    {
        CWScanLogic::ScanOutputState state = makeScanState(0);
        state.filesScanned = 4;
        state.threatsFound = 1;
        state.scanCompletedFiles = 4;
        state.scanUiProgress = 80;
        state.scanDbPhaseSeen = true;
        state.scanFilePhaseSeen = true;

        CWScanLogic::ScanLineEffects effects = feedLine(state, "ERROR: Can't open file C:\\scan\\locked.dat", true);
        CHECK(effects.appendToLog);
        CHECK_FALSE(effects.statusChanged);
        CHECK_FALSE(effects.progressChanged);
        CHECK(state.filesScanned == 4);
        CHECK(state.threatsFound == 1);
        CHECK(state.scanCompletedFiles == 4);
        CHECK(state.scanUiProgress == 80);
    }
}

#include "doctest.h"
#include "cw_scan_logic.h"

#include <math.h>
#include <string>

TEST_SUITE("scan_parsers")
{
    TEST_CASE("parseSizeToBytes handles valid and invalid tokens")
    {
        CHECK(CWScanLogic::parseSizeToBytes("22.29MiB") == doctest::Approx(22.29 * 1024.0 * 1024.0));
        CHECK(CWScanLogic::parseSizeToBytes("84.95MB") == doctest::Approx(84.95 * 1024.0 * 1024.0));
        CHECK(CWScanLogic::parseSizeToBytes("bogus") < 0.0);
    }

    TEST_CASE("parseFreshclamProgressLine recognizes byte progress")
    {
        double downloaded = 0.0;
        double total = 0.0;

        CHECK(CWScanLogic::parseFreshclamProgressLine("Downloading... 22.29MiB/84.95MiB", &downloaded, &total));
        CHECK(downloaded > 0.0);
        CHECK(total > downloaded);
        CHECK_FALSE(CWScanLogic::parseFreshclamProgressLine("Downloading... done", &downloaded, &total));
    }

    TEST_CASE("parseRatioProgressLine handles plain and scaled counters")
    {
        int pct = 0;
        CHECK(CWScanLogic::parseRatioProgressLine("Loading: 50/100 sigs", "sigs", &pct));
        CHECK(pct == 50);
        CHECK(CWScanLogic::parseRatioProgressLine("Compiling: 3K/6K tasks", "tasks", &pct));
        CHECK(pct == 50);
        CHECK_FALSE(CWScanLogic::parseRatioProgressLine("Loading: done", "sigs", &pct));
    }

    TEST_CASE("file progress helpers split percent and spinner output")
    {
        int pct = 0;
        std::string file;
        char spinner = '\0';

        CHECK(CWScanLogic::parseFilePercentLine("C:\\scan\\a.txt: [50%]", &pct, file));
        CHECK(pct == 50);
        CHECK(file == "C:\\scan\\a.txt");

        CHECK(CWScanLogic::parseFileSpinnerLine("C:\\scan\\a.txt: [ / ]", file, &spinner));
        CHECK(spinner == '/');
        CHECK_FALSE(CWScanLogic::parseFileSpinnerLine("C:\\scan\\a.txt: [50%]", file, &spinner));
    }

    TEST_CASE("file result and summary helpers extract structured values")
    {
        std::string file;
        std::string status;
        int value = 0;

        CHECK(CWScanLogic::parseFileResultLine("C:\\scan\\infected.txt: Win.Test.EICAR_HDB-1 FOUND", file, status));
        CHECK(file == "C:\\scan\\infected.txt");
        CHECK(status == "Win.Test.EICAR_HDB-1 FOUND");
        CHECK(CWScanLogic::parseSummaryInt("Scanned files: 42", "Scanned files:", &value));
        CHECK(value == 42);
        CHECK(CWScanLogic::isExcludedFileResult("Excluded by pattern"));
    }

    TEST_CASE("database label helpers distinguish available and updated lines")
    {
        std::string dbLabel;

        CHECK(CWScanLogic::isSizeOnlyProgressLine("22.29MiB/84.95MiB"));
        CHECK_FALSE(CWScanLogic::isSizeOnlyProgressLine("Time: 00:01 ETA: 00:02"));
        CHECK(CWScanLogic::extractDbLabelFromAvailableLine("daily database available for download", dbLabel));
        CHECK(dbLabel == "daily.cvd");
        CHECK(CWScanLogic::extractDbLabelFromAvailableLine("daily.cvd database available for download", dbLabel));
        CHECK(dbLabel == "daily.cvd");
        CHECK(CWScanLogic::extractDbLabelFromUpdatedLine("daily.cvd updated (version: 12345, sigs: 67890)", dbLabel));
        CHECK(dbLabel == "daily.cvd");
        CHECK(CWScanLogic::isDbLoadOrCompileLine("Loading: 12/20 sigs"));
        CHECK(CWScanLogic::isDbFileLabel("daily.cvd"));
    }
}

#include "doctest.h"
#include "cw_cli_args.h"

TEST_SUITE("cli_args")
{
    TEST_CASE("parses scanner mode and repeated path arguments")
    {
        CWCliArgs args;
        CW_ParseCommandLineArgs("--mode=scanner --path=\"C:\\scan one\" --path=\\\\srv\\share --close", args);

        CHECK(args.hasSwitches);
        CHECK(args.close);
        CHECK(args.mode == "scanner");
        REQUIRE(args.paths.size() == 2);
        CHECK(args.paths[0] == "C:\\scan one");
        CHECK(args.paths[1] == "\\\\srv\\share");
    }

    TEST_CASE("parses config file switch")
    {
        CWCliArgs args;
        CW_ParseCommandLineArgs("--mode=update --config_file=\"C:\\Users\\alex\\ClamWin.conf\"", args);

        CHECK(args.hasSwitches);
        CHECK(args.mode == "update");
        CHECK(args.configFile == "C:\\Users\\alex\\ClamWin.conf");
    }

    TEST_CASE("keeps legacy non-switch command line untouched")
    {
        CWCliArgs args;
        CW_ParseCommandLineArgs("C:\\Users\\alex\\.clamwin\\ClamWin.conf", args);

        CHECK_FALSE(args.hasSwitches);
        CHECK(args.mode == "main");
        CHECK(args.paths.empty());
    }
}

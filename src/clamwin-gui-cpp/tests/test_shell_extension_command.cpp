#include "doctest.h"
#include "cw_shell_extension_command.h"

TEST_SUITE("shell_extension_command")
{
    TEST_CASE("trim trailing slash handles spaces and quotes")
    {
        CHECK(CWTrimTrailingSlash("\"C:\\Program Files\\ClamWin\\\"") == "C:\\Program Files\\ClamWin");
        CHECK(CWTrimTrailingSlash("C:\\ClamWin\\") == "C:\\ClamWin");
        CHECK(CWTrimTrailingSlash("  C:\\ClamWin  ") == "C:\\ClamWin");
    }

    TEST_CASE("build scanner command emits expected mode and path args")
    {
        std::string cmd = CWBuildShellScannerCommand(
            "C:\\Program Files\\ClamWin\\",
            "--path=\"C:\\data\" --path=\"D:\\more\"",
            "--close");

        CHECK(cmd.find("\"C:\\Program Files\\ClamWin\\ClamWin.exe\"") == 0);
        CHECK(cmd.find("--mode=scanner") != std::string::npos);
        CHECK(cmd.find("--path=\"C:\\data\"") != std::string::npos);
        CHECK(cmd.find("--path=\"D:\\more\"") != std::string::npos);
        CHECK(cmd.find("--close") != std::string::npos);
    }
}

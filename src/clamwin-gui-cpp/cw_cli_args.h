/*
 * ClamWin Free Antivirus — command-line parsing helpers
 *
 * Keeps legacy argument compatibility for shell extension invocation.
 */

#pragma once

#include <string>
#include <vector>

struct CWCliArgs
{
    bool hasSwitches;
    bool close;
    std::string mode;
    std::string configFile;
    std::vector<std::string> paths;

    CWCliArgs()
        : hasSwitches(false)
        , close(false)
        , mode("main")
    {
    }
};

void CW_ParseCommandLineArgs(const char* cmdLine, CWCliArgs& out);

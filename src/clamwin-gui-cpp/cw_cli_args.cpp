#include "cw_cli_args.h"

#include <string.h>

namespace
{
void splitCommandLine(const char* cmdLine, std::vector<std::string>& out)
{
    out.clear();
    if (!cmdLine)
        return;

    std::string current;
    bool inQuotes = false;

    for (const char* p = cmdLine; *p; ++p)
    {
        const char ch = *p;
        if (ch == '"')
        {
            inQuotes = !inQuotes;
            continue;
        }

        if ((ch == ' ' || ch == '\t') && !inQuotes)
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty())
        out.push_back(current);
}

bool startsWith(const std::string& value, const char* prefix)
{
    const size_t n = strlen(prefix);
    return value.size() >= n && value.compare(0, n, prefix) == 0;
}
}

void CW_ParseCommandLineArgs(const char* cmdLine, CWCliArgs& out)
{
    out = CWCliArgs();

    std::vector<std::string> args;
    splitCommandLine(cmdLine, args);

    for (size_t i = 0; i < args.size(); ++i)
    {
        const std::string& arg = args[i];

        if (arg == "--close")
        {
            out.hasSwitches = true;
            out.close = true;
            continue;
        }

        if (arg == "--open-dashboard")
        {
            out.hasSwitches = true;
            out.openDashboard = true;
            continue;
        }

        if (startsWith(arg, "--mode="))
        {
            out.hasSwitches = true;
            out.mode = arg.substr(strlen("--mode="));
            continue;
        }

        if (startsWith(arg, "--path="))
        {
            out.hasSwitches = true;
            out.paths.push_back(arg.substr(strlen("--path=")));
            continue;
        }

        if (startsWith(arg, "--config_file="))
        {
            out.hasSwitches = true;
            out.configFile = arg.substr(strlen("--config_file="));
            continue;
        }
    }
}

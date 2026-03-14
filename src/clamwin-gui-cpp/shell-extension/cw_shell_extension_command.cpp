#include "cw_shell_extension_command.h"

#include <ctype.h>
#include <cwctype>

namespace
{
std::string trimSpaces(const std::string& value)
{
    size_t first = 0;
    while (first < value.size() && isspace((unsigned char)value[first]))
        ++first;

    size_t last = value.size();
    while (last > first && isspace((unsigned char)value[last - 1]))
        --last;

    return value.substr(first, last - first);
}

std::string trimOuterQuotes(const std::string& value)
{
    if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
        return value.substr(1, value.size() - 2);
    return value;
}
}

std::string CWTrimTrailingSlash(const std::string& path)
{
    std::string out = trimOuterQuotes(trimSpaces(path));
    while (!out.empty() && (out[out.size() - 1] == '\\' || out[out.size() - 1] == '/'))
        out.erase(out.size() - 1);
    return out;
}

std::string CWBuildShellScannerCommand(const std::string& clamwinPath,
                                       const std::string& pathArgs,
                                       const std::string& extraParams)
{
    std::string basePath = CWTrimTrailingSlash(clamwinPath);
    std::string cmd = "\"" + basePath + "\\ClamWin.exe\" --mode=scanner";

    if (!pathArgs.empty())
    {
        cmd += " ";
        cmd += pathArgs;
    }

    std::string params = trimSpaces(extraParams);
    if (!params.empty())
    {
        cmd += " ";
        cmd += params;
    }

    return cmd;
}

std::wstring CWTrimTrailingSlashW(const std::wstring& path)
{
    std::wstring out = path;

    while (!out.empty() && iswspace(out.front()))
        out.erase(out.begin());
    while (!out.empty() && iswspace(out.back()))
        out.erase(out.end() - 1);

    if (out.size() >= 2 && out.front() == L'"' && out.back() == L'"')
        out = out.substr(1, out.size() - 2);

    while (!out.empty() && (out[out.size() - 1] == L'\\' || out[out.size() - 1] == L'/'))
        out.erase(out.size() - 1);

    return out;
}

std::wstring CWBuildShellScannerCommand(const std::wstring& clamwinPath,
                                        const std::wstring& pathArgs,
                                        const std::wstring& extraParams)
{
    std::wstring basePath = CWTrimTrailingSlashW(clamwinPath);
    std::wstring cmd = L"\"" + basePath + L"\\ClamWin.exe\" --mode=scanner";

    if (!pathArgs.empty())
    {
        cmd += L" ";
        cmd += pathArgs;
    }

    std::wstring params = extraParams;
    while (!params.empty() && iswspace(params.front()))
        params.erase(params.begin());
    while (!params.empty() && iswspace(params.back()))
        params.erase(params.end() - 1);

    if (!params.empty())
    {
        cmd += L" ";
        cmd += params;
    }

    return cmd;
}

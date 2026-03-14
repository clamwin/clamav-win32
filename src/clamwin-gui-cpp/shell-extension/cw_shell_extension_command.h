#pragma once

#include <string>

std::string CWTrimTrailingSlash(const std::string& path);
std::string CWBuildShellScannerCommand(const std::string& clamwinPath,
                                       const std::string& pathArgs,
                                       const std::string& extraParams);

std::wstring CWTrimTrailingSlashW(const std::wstring& path);
std::wstring CWBuildShellScannerCommand(const std::wstring& clamwinPath,
                                        const std::wstring& pathArgs,
                                        const std::wstring& extraParams);

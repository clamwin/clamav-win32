/*
 * ClamWin Free Antivirus — scan/update command and parsing helpers
 *
 * Extracted from CWScanDialog so the fragile stdout and command logic can be
 * tested without standing up the Win32 dialog.
 */

#pragma once

#include "cw_config.h"
#include <windows.h>
#include <string>
#include <vector>

namespace CWScanLogic
{
struct ScanOutputState
{
    bool isUpdate;
    int filesScanned;
    int threatsFound;
    int scanExpectedFiles;
    ULONGLONG scanExpectedBytes;
    int scanCompletedFiles;
    ULONGLONG scanScannedBytes;
    int scanUiProgress;
    bool scanDbPhaseSeen;
    bool scanFilePhaseSeen;
    bool updateProgressKnown;
    double updateDownloadedBytes;
    double updateTotalBytes;
    double updateTransferredBytes;
    int updateUiProgress;
    bool updateHadChanges;
    int updateUpToDateCount;
    std::string updateCurrentDb;
    bool updateBlocked;
    bool updateUnsupportedVersion;
    bool updateServerError;
};

struct ScanLineEffects
{
    bool appendToLog;
    bool statusChanged;
    bool statusAnimate;
    std::string statusText;
    bool progressChanged;
    int progressPos;

    ScanLineEffects()
        : appendToLog(true)
        , statusChanged(false)
        , statusAnimate(false)
        , progressChanged(false)
        , progressPos(0)
    {
    }
};

struct ScanPattern
{
    std::string text;
    bool isRegex;

    ScanPattern(const std::string& patternText, bool regex)
        : text(patternText)
        , isRegex(regex)
    {
    }
};

std::string trimOuterQuotes(const std::string& s);
std::string trimSpaces(const std::string& s);
void splitPatternList(const std::string& raw, std::vector<ScanPattern>& out);
bool shouldApplyPreferenceFilters(const std::string& rawTarget);
std::string globToClamscanRegex(const std::string& pattern);
std::string buildClamscanCommand(const CWConfig& cfg,
                                 const std::string& targetPath,
                                 const std::string& exeDir,
                                 bool scanMemoryOnly = false);
std::string buildFreshclamCommand(const CWConfig& cfg,
                                  const std::string& exeDir);
std::string buildFreshclamExecutablePath(const std::string& exeDir);
bool hasFreshclamExecutable(const std::string& exeDir);
void initScanOutputState(ScanOutputState& state, bool isUpdate);
ScanLineEffects processOutputLine(ScanOutputState& state, const char* text, bool isError);

double parseSizeToBytes(const char* token);
void formatBytes(double bytes, char* out, size_t outSize);
bool parseFreshclamProgressLine(const char* text, double* downloaded, double* total);
int countSubstring(const char* text, const char* needle);
bool isSizeOnlyProgressLine(const char* text);
bool extractDbLabelFromAvailableLine(const char* text, std::string& out);
bool extractDbLabelFromUpdatedLine(const char* text, std::string& out);
bool parseSummaryInt(const char* text, const char* label, int* outValue);
bool parseRatioProgressLine(const char* text, const char* suffix, int* outPct);
bool parseFilePercentLine(const char* text, int* outPct, std::string& outFile);
bool parseFileSpinnerLine(const char* text, std::string& outFile, char* outSpinner);
bool isDbLoadOrCompileLine(const char* text);
bool parseFileResultLine(const char* text, std::string& outFile, std::string& outStatus);
bool isExcludedFileResult(const std::string& status);
bool isDbFileLabel(const std::string& s);
}

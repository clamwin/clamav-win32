#include "cw_scan_logic.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace CWScanLogic
{
namespace
{
const char* const kPatternSep = "|CLAMWIN_SEP|";
const int kPatternSepLen = (int)(sizeof("|CLAMWIN_SEP|") - 1);

bool containsInsensitive(const char* text, const char* needle)
{
    if (!text || !needle || !*needle)
        return false;

    size_t n = strlen(needle);
    const char* p = text;
    while (*p)
    {
        if (_strnicmp(p, needle, n) == 0)
            return true;
        ++p;
    }
    return false;
}

bool isFreshclamRateLimitText(const char* text)
{
    return containsInsensitive(text, "rate limit") ||
           containsInsensitive(text, "rate limited") ||
           containsInsensitive(text, "cool-down") ||
           containsInsensitive(text, "cooldown") ||
           containsInsensitive(text, "blocked by the CDN");
}

bool isFreshclamUnsupportedText(const char* text)
{
    return containsInsensitive(text, "unsupported ClamAV version") ||
           containsInsensitive(text, "supported ClamAV version") ||
           containsInsensitive(text, "faq-eol");
}

bool isFreshclamServerErrorText(const char* text)
{
    return containsInsensitive(text, "Failed to get main database version information from server") ||
           containsInsensitive(text, "Failed to find main database using server") ||
           containsInsensitive(text, "Can't query DNS") ||
           containsInsensitive(text, "Temporary DNS error");
}

bool isPathSeparator(char ch)
{
    return ch == '\\' || ch == '/';
}

std::string ensureTrailingSlash(const std::string& path)
{
    if (path.empty())
        return std::string();

    char tail = path[path.size() - 1];
    if (tail == '\\' || tail == '/')
        return path;

    return path + "\\";
}

bool isDriveRootPath(const std::string& path)
{
    return path.size() == 2 ||
           (path.size() == 3 && isalpha((unsigned char)path[0]) && path[1] == ':' &&
            (path[2] == '\\' || path[2] == '/'));
}

size_t findGlobClassEnd(const char* pattern)
{
    size_t index = 1;
    if (pattern[index] == '!' || pattern[index] == '^')
        ++index;
    if (pattern[index] == ']')
        ++index;

    while (pattern[index] && pattern[index] != ']')
        ++index;

    return pattern[index] == ']' ? index : 0;
}

bool globClassMatchNoCase(const char* pattern, size_t classEnd, char textCh)
{
    bool negate = false;
    bool matched = false;
    size_t index = 1;
    unsigned char value = (unsigned char)toupper((unsigned char)textCh);

    if (pattern[index] == '!' || pattern[index] == '^')
    {
        negate = true;
        ++index;
    }

    if (index < classEnd && pattern[index] == ']')
    {
        matched = (textCh == ']');
        ++index;
    }

    while (index < classEnd)
    {
        unsigned char start = (unsigned char)toupper((unsigned char)pattern[index]);
        if (index + 2 < classEnd && pattern[index + 1] == '-' && pattern[index + 2] != ']')
        {
            unsigned char end = (unsigned char)toupper((unsigned char)pattern[index + 2]);
            if (start <= value && value <= end)
                matched = true;
            index += 3;
        }
        else
        {
            if (start == value)
                matched = true;
            ++index;
        }
    }

    return negate ? !matched : matched;
}

bool globMatchNoCase(const char* pattern, const char* text)
{
    if (!pattern || !text)
        return false;

    while (*pattern)
    {
        if (*pattern == '*')
        {
            while (*pattern == '*')
                ++pattern;
            if (!*pattern)
                return true;

            while (*text)
            {
                if (!isPathSeparator(*text) && globMatchNoCase(pattern, text))
                    return true;
                if (isPathSeparator(*text))
                    break;
                ++text;
            }
            return false;
        }

        if (!*text)
            return false;

        if (*pattern == '?')
        {
            if (isPathSeparator(*text))
                return false;
            ++pattern;
            ++text;
            continue;
        }

        if (*pattern == '[')
        {
            size_t classEnd = findGlobClassEnd(pattern);
            if (!classEnd)
            {
                if (*text != '[')
                    return false;
                ++pattern;
                ++text;
                continue;
            }

            if (!globClassMatchNoCase(pattern, classEnd, *text))
                return false;

            pattern += classEnd + 1;
            ++text;
            continue;
        }

        if (toupper((unsigned char)*pattern) != toupper((unsigned char)*text))
            return false;

        ++pattern;
        ++text;
    }

    return *text == '\0';
}

bool matchesAnyWildcardPattern(const std::vector<ScanPattern>& patterns,
                               const std::string& fileName,
                               const std::string& fullPath)
{
    for (size_t i = 0; i < patterns.size(); ++i)
    {
        if (patterns[i].isRegex)
            continue;

        const char* pat = patterns[i].text.c_str();
        if (globMatchNoCase(pat, fileName.c_str()) ||
            globMatchNoCase(pat, fullPath.c_str()))
        {
            return true;
        }
    }
    return false;
}

std::string normalizeExeDir(const std::string& exeDir)
{
    return ensureTrailingSlash(trimOuterQuotes(trimSpaces(exeDir)));
}

std::string quoteCommandArg(const std::string& value)
{
    std::string quoted = "\"";
    int backslashCount = 0;

    for (size_t i = 0; i < value.size(); ++i)
    {
        char ch = value[i];
        if (ch == '\\')
        {
            ++backslashCount;
            continue;
        }

        if (ch == '"')
        {
            quoted.append(backslashCount * 2 + 1, '\\');
            quoted += '"';
            backslashCount = 0;
            continue;
        }

        if (backslashCount > 0)
        {
            quoted.append(backslashCount, '\\');
            backslashCount = 0;
        }

        quoted += ch;
    }

    if (backslashCount > 0)
        quoted.append(backslashCount * 2, '\\');

    quoted += '"';
    return quoted;
}

double parseProgressToken(const char* token)
{
    if (!token || !*token)
        return -1.0;

    char* endNum = NULL;
    double value = strtod(token, &endNum);
    if (endNum == token || value < 0.0)
        return -1.0;

    while (*endNum && isspace((unsigned char)*endNum))
        ++endNum;

    if (!*endNum)
        return value;

    char unit = (char)toupper((unsigned char)*endNum);
    if (unit == 'K')
        return value * 1000.0;
    if (unit == 'M')
        return value * 1000.0 * 1000.0;
    if (unit == 'G')
        return value * 1000.0 * 1000.0 * 1000.0;

    return -1.0;
}

bool endsWithNoCase(const std::string& s, const char* suffix)
{
    if (!suffix)
        return false;
    size_t n = strlen(suffix);
    if (s.size() < n)
        return false;
    for (size_t i = 0; i < n; ++i)
    {
        char a = (char)tolower((unsigned char)s[s.size() - n + i]);
        char b = (char)tolower((unsigned char)suffix[i]);
        if (a != b)
            return false;
    }
    return true;
}
}

std::string trimOuterQuotes(const std::string& s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

std::string trimSpaces(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start]))
        ++start;

    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1]))
        --end;

    return s.substr(start, end - start);
}

void splitPatternList(const std::string& raw, std::vector<ScanPattern>& out)
{
    out.clear();

    size_t pos = 0;
    while (pos <= raw.size())
    {
        size_t next = raw.find(kPatternSep, pos);
        std::string item = (next == std::string::npos)
            ? raw.substr(pos)
            : raw.substr(pos, next - pos);

        item = trimSpaces(item);
        if (!item.empty())
        {
            bool isRegex = item.size() >= 2 && item.front() == '<' && item.back() == '>';
            if (isRegex)
                item = item.substr(1, item.size() - 2);

            if (!item.empty())
                out.push_back(ScanPattern(item, isRegex));
        }

        if (next == std::string::npos)
            break;
        pos = next + kPatternSepLen;
    }
}

bool shouldApplyPreferenceFilters(const std::string& rawTarget)
{
    std::string target = trimOuterQuotes(trimSpaces(rawTarget));
    if (target.empty())
        return false;

    DWORD attr = GetFileAttributesA(target.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES)
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;

    return target.size() >= 2 && isalpha((unsigned char)target[0]) && target[1] == ':' &&
           isDriveRootPath(target);
}

std::string globToClamscanRegex(const std::string& pattern)
{
    std::string regex;
    const bool hasPathSeparator = pattern.find('\\') != std::string::npos ||
                                  pattern.find('/') != std::string::npos;

    if (hasPathSeparator)
        regex += "^";

    for (size_t i = 0; i < pattern.size(); ++i)
    {
        char ch = pattern[i];
        if (ch == '*')
        {
            regex += "[^\\\\/]*";
        }
        else if (ch == '?')
        {
            regex += "[^\\\\/]";
        }
        else if (ch == '[')
        {
            size_t j = i + 1;
            if (j < pattern.size() && (pattern[j] == '!' || pattern[j] == '^'))
                ++j;
            if (j < pattern.size() && pattern[j] == ']')
                ++j;
            while (j < pattern.size() && pattern[j] != ']')
                ++j;

            if (j >= pattern.size())
            {
                regex += "\\[";
            }
            else
            {
                std::string stuff = pattern.substr(i + 1, j - i - 1);
                if (!stuff.empty() && stuff[0] == '!')
                    stuff[0] = '^';
                else if (!stuff.empty() && stuff[0] == '^')
                    stuff.insert(stuff.begin(), '\\');

                size_t slashPos = 0;
                while ((slashPos = stuff.find('\\', slashPos)) != std::string::npos)
                {
                    stuff.insert(slashPos, "\\");
                    slashPos += 2;
                }

                regex += '[';
                regex += stuff;
                regex += ']';
                i = j;
            }
        }
        else if (ch == '/' || ch == '\\')
        {
            regex += "[/\\\\]";
        }
        else
        {
            switch (ch)
            {
                case '.':
                case '^':
                case '$':
                case '+':
                case '(':
                case ')':
                case '{':
                case '}':
                case '[':
                case ']':
                case '|':
                    regex += '\\';
                    break;
            }
            regex += ch;
        }
    }
    regex += "$";
    return regex;
}

std::string buildClamscanCommand(const CWConfig& cfg,
                                 const std::string& targetPath,
                                 const std::string& exeDir,
                                 bool scanMemoryOnly)
{
    std::string normalizedExeDir = normalizeExeDir(exeDir);
    std::string cmd = quoteCommandArg(normalizedExeDir + "clamscan.exe");
    cmd += " --stdout --show-progress";

    if (!cfg.databasePath.empty())
    {
        cmd += " --database=";
        cmd += quoteCommandArg(cfg.databasePath);
    }

    if (!cfg.scanOle2)
        cmd += " --scan-ole2=no";

    if (cfg.scanArchives)
    {
        char limits[160];
        _snprintf(limits, sizeof(limits),
                  " --max-files=%d --max-scansize=%dM --max-recursion=%d",
                  cfg.maxFiles,
                  cfg.maxScanSizeMb,
                  cfg.maxDepth);
        limits[sizeof(limits) - 1] = '\0';
        cmd += limits;
    }
    else
    {
        cmd += " --scan-archive=no";
    }

    {
        char maxFileSize[64];
        _snprintf(maxFileSize, sizeof(maxFileSize), " --max-filesize=%dM", cfg.maxFileSizeMb);
        maxFileSize[sizeof(maxFileSize) - 1] = '\0';
        cmd += maxFileSize;
    }

    if (cfg.infectedAction == 1)
    {
        cmd += " --remove";
    }
    else if (cfg.infectedAction == 2 && !cfg.quarantinePath.empty())
    {
        cmd += " --move=";
        cmd += quoteCommandArg(cfg.quarantinePath);
    }

    if (scanMemoryOnly)
        cmd += " --memory";

    if (!scanMemoryOnly && cfg.scanRecursive)
        cmd += " --recursive";

    if (!scanMemoryOnly && shouldApplyPreferenceFilters(targetPath))
    {
        std::vector<ScanPattern> includePatterns;
        std::vector<ScanPattern> excludePatterns;
        splitPatternList(cfg.includePatterns, includePatterns);
        splitPatternList(cfg.excludePatterns, excludePatterns);

        for (size_t i = 0; i < includePatterns.size(); ++i)
        {
            cmd += " --include=";
            cmd += includePatterns[i].isRegex
                ? includePatterns[i].text
                : globToClamscanRegex(includePatterns[i].text);
        }

        for (size_t i = 0; i < excludePatterns.size(); ++i)
        {
            cmd += " --exclude=";
            cmd += excludePatterns[i].isRegex
                ? excludePatterns[i].text
                : globToClamscanRegex(excludePatterns[i].text);
        }
    }

    if (!cfg.scanLogFile.empty())
    {
        cmd += " --log=";
        cmd += quoteCommandArg(cfg.scanLogFile);
    }

    if (!scanMemoryOnly)
    {
        cmd += " ";
        cmd += quoteCommandArg(targetPath);
    }

    return cmd;
}

std::string buildFreshclamCommand(const CWConfig& cfg,
                                  const std::string& exeDir)
{
    std::string normalizedExeDir = normalizeExeDir(exeDir);
    std::string cmd = quoteCommandArg(buildFreshclamExecutablePath(normalizedExeDir));
    cmd += " --stdout --show-progress";

    /* Always pass --config-file pointing to the profile-dir freshclam.conf
     * (generated by CWConfig::writeFreshclamConf on prefs save / first load). */
    std::string cfgPath = cfg.freshclamConfPath();
    if (!cfgPath.empty())
    {
        cmd += " --config-file=";
        cmd += quoteCommandArg(cfgPath);
    }

    std::string certsDir = normalizedExeDir + "certs";
    DWORD certsAttr = GetFileAttributesA(certsDir.c_str());
    if (certsAttr != INVALID_FILE_ATTRIBUTES && (certsAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        cmd += " --cvdcertsdir=";
        cmd += quoteCommandArg(certsDir);
    }

    if (!cfg.databasePath.empty())
    {
        cmd += " --datadir=";
        cmd += quoteCommandArg(cfg.databasePath);
    }

    if (!cfg.updateLogFile.empty())
    {
        cmd += " --log=";
        cmd += quoteCommandArg(cfg.updateLogFile);
    }

    return cmd;
}

std::string buildFreshclamExecutablePath(const std::string& exeDir)
{
    return normalizeExeDir(exeDir) + "freshclam.exe";
}

bool hasFreshclamExecutable(const std::string& exeDir)
{
    std::string exePath = buildFreshclamExecutablePath(exeDir);
    DWORD attr = GetFileAttributesA(exePath.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void initScanOutputState(ScanOutputState& state, bool isUpdate)
{
    state.isUpdate = isUpdate;
    state.filesScanned = 0;
    state.threatsFound = 0;
    state.scanExpectedFiles = 0;
    state.scanExpectedBytes = 0;
    state.scanCompletedFiles = 0;
    state.scanScannedBytes = 0;
    state.scanUiProgress = 0;
    state.scanDbPhaseSeen = false;
    state.scanFilePhaseSeen = false;
    state.updateProgressKnown = false;
    state.updateDownloadedBytes = 0.0;
    state.updateTotalBytes = 0.0;
    state.updateTransferredBytes = 0.0;
    state.updateUiProgress = 0;
    state.updateHadChanges = false;
    state.updateUpToDateCount = 0;
    state.updateCurrentDb.clear();
    state.updateBlocked = false;
    state.updateUnsupportedVersion = false;
    state.updateServerError = false;
}

ScanLineEffects processOutputLine(ScanOutputState& state, const char* text, bool isError)
{
    ScanLineEffects effects;

    if (!text)
        return effects;

    double downloaded = 0.0;
    double total = 0.0;
    bool hasProgress = state.isUpdate && !isError && parseFreshclamProgressLine(text, &downloaded, &total);

    if (state.isUpdate && !isError && strstr(text, "Time:") && strstr(text, "ETA:"))
        effects.appendToLog = false;

    if (state.isUpdate && !isError && strstr(text, "ETA:") && strchr(text, '[') && strchr(text, ']'))
        effects.appendToLog = false;

    /* freshclam table rows like: Time: 0.1s [ ... ] 0B/84.95MiB */
    if (state.isUpdate && !isError && strstr(text, "Time:") && strchr(text, '[') && strchr(text, ']') && strchr(text, '/'))
        effects.appendToLog = false;

    if (state.isUpdate && !isError && isSizeOnlyProgressLine(text))
        effects.appendToLog = false;

    if (state.isUpdate && isFreshclamRateLimitText(text))
    {
        state.updateBlocked = true;
        effects.statusChanged = true;
        effects.statusText = "Update blocked by CDN cooldown";
    }

    if (state.isUpdate && isFreshclamUnsupportedText(text))
    {
        state.updateBlocked = true;
        state.updateUnsupportedVersion = true;
        effects.statusChanged = true;
        effects.statusText = "Update blocked: unsupported ClamAV version";
    }

    if (state.isUpdate && isFreshclamServerErrorText(text))
    {
        state.updateServerError = true;
        effects.statusChanged = true;
        effects.statusText = "Update failed: server/DNS resolution error";
    }

    if (isError)
        return effects;

    int summaryValue = 0;
    if (parseSummaryInt(text, "Scanned files:", &summaryValue))
        state.filesScanned = summaryValue;
    if (parseSummaryInt(text, "Infected files:", &summaryValue))
        state.threatsFound = summaryValue;

    if (!state.isUpdate)
    {
        int pct = 0;
        std::string fileLabel;
        char spinnerGlyph = '|';
        std::string fileResultPath;
        std::string fileResultText;
        int uiProgress = state.scanUiProgress;
        bool isTransient = false;
        bool isFileScan = false;

        if (parseRatioProgressLine(text, "sigs", &pct))
        {
            isTransient = true;
            state.scanDbPhaseSeen = true;
            char status[128];
            _snprintf(status, sizeof(status), "Loading virus definitions... (%d%%)", pct);
            status[sizeof(status) - 1] = '\0';
            effects.statusChanged = true;
            effects.statusAnimate = true;
            effects.statusText = status;
        }
        else if (parseRatioProgressLine(text, "tasks", &pct))
        {
            isTransient = true;
            state.scanDbPhaseSeen = true;
            char status[128];
            _snprintf(status, sizeof(status), "Compiling signatures... (%d%%)", pct);
            status[sizeof(status) - 1] = '\0';
            effects.statusChanged = true;
            effects.statusAnimate = true;
            effects.statusText = status;
        }
        else if (parseFilePercentLine(text, &pct, fileLabel))
        {
            isTransient = true;
            isFileScan = true;
            state.scanFilePhaseSeen = true;
        }
        else if (parseFileSpinnerLine(text, fileLabel, &spinnerGlyph))
        {
            isTransient = true;
            isFileScan = true;
            state.scanFilePhaseSeen = true;
        }
        else if (isDbLoadOrCompileLine(text))
        {
            isTransient = true;
            state.scanDbPhaseSeen = true;
            effects.statusChanged = true;
            effects.statusAnimate = true;
            effects.statusText = strstr(text, "Compiling:") ? "Compiling signatures..."
                                                              : "Loading virus definitions...";
        }

        if (parseFileResultLine(text, fileResultPath, fileResultText))
        {
            state.scanFilePhaseSeen = true;
            if (!isExcludedFileResult(fileResultText))
            {
                ++state.scanCompletedFiles;
                state.filesScanned = state.scanCompletedFiles;
                if (strstr(fileResultText.c_str(), "FOUND"))
                    ++state.threatsFound;

                WIN32_FILE_ATTRIBUTE_DATA fad;
                memset(&fad, 0, sizeof(fad));
                if (GetFileAttributesExA(fileResultPath.c_str(), GetFileExInfoStandard, &fad))
                    state.scanScannedBytes += ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;

                if (state.scanExpectedBytes > 0)
                {
                    uiProgress = (int)((state.scanScannedBytes * 100) / state.scanExpectedBytes);
                }
                else
                {
                    int expected = (state.scanExpectedFiles > 0) ? state.scanExpectedFiles : state.scanCompletedFiles;
                    if (expected < state.scanCompletedFiles)
                        expected = state.scanCompletedFiles;
                    uiProgress = (state.scanCompletedFiles * 100) / (expected > 0 ? expected : 1);
                }
                if (uiProgress < 0)
                    uiProgress = 0;
                if (uiProgress > 100)
                    uiProgress = 100;
            }
            isFileScan = true;
        }

        if (isFileScan)
        {
            char status[128];
            if (state.scanExpectedBytes > 0)
            {
                char scanned[32];
                char totalText[32];
                formatBytes((double)state.scanScannedBytes, scanned, sizeof(scanned));
                formatBytes((double)state.scanExpectedBytes, totalText, sizeof(totalText));
                _snprintf(status, sizeof(status), "Scanned: %s / %s", scanned, totalText);
            }
            else
            {
                int expected = (state.scanExpectedFiles > 0) ? state.scanExpectedFiles : state.scanCompletedFiles;
                if (expected < state.scanCompletedFiles)
                    expected = state.scanCompletedFiles;
                if (state.scanExpectedFiles > 0)
                {
                    _snprintf(status, sizeof(status), "File scanned: %d/%d",
                              state.scanCompletedFiles, expected > 0 ? expected : 1);
                }
                else
                {
                    _snprintf(status, sizeof(status), "Files scanned: %d",
                              state.scanCompletedFiles);
                }
            }
            status[sizeof(status) - 1] = '\0';
            effects.statusChanged = true;
            effects.statusAnimate = false;
            effects.statusText = status;
        }

        if (isTransient)
            effects.appendToLog = false;

        if (uiProgress > state.scanUiProgress)
            state.scanUiProgress = uiProgress;
        if (state.scanUiProgress > 100)
            state.scanUiProgress = 100;
        effects.progressChanged = true;
        effects.progressPos = state.scanUiProgress;

        if (!state.scanDbPhaseSeen && !state.scanFilePhaseSeen && !isFileScan)
        {
            effects.statusChanged = true;
            effects.statusAnimate = true;
            effects.statusText = "Loading scanner engine...";
        }

        return effects;
    }

    std::string dbLabel;

    if (strstr(text, " database available for download"))
        state.updateHadChanges = true;

    if (strstr(text, " updated (version"))
        state.updateHadChanges = true;

    if (strstr(text, " database is up-to-date"))
    {
        state.updateUpToDateCount += countSubstring(text, " database is up-to-date");
        if (!state.updateHadChanges)
        {
            effects.statusChanged = true;
            effects.statusText = "Virus definitions are already up to date";
        }
    }

    if (strstr(text, "Testing database:"))
    {
        char status[256];
        const char* activeDb = isDbFileLabel(state.updateCurrentDb)
            ? state.updateCurrentDb.c_str()
            : "definitions";
        _snprintf(status, sizeof(status), "Verifying %s...", activeDb);
        status[sizeof(status) - 1] = '\0';
        effects.statusChanged = true;
        effects.statusText = status;
    }

    if (extractDbLabelFromUpdatedLine(text, dbLabel))
    {
        state.updateCurrentDb = dbLabel;
        char status[256];
        _snprintf(status, sizeof(status), "Installed %s", state.updateCurrentDb.c_str());
        status[sizeof(status) - 1] = '\0';
        effects.statusChanged = true;
        effects.statusText = status;
    }

    if (extractDbLabelFromAvailableLine(text, dbLabel))
    {
        /* Roll current file progress into the session total before switching files. */
        if (state.updateDownloadedBytes > 0.0)
            state.updateTransferredBytes += state.updateDownloadedBytes;

        state.updateCurrentDb = dbLabel;
        /* Each database file should show independent progress starting at 0%. */
        state.updateProgressKnown = false;
        state.updateDownloadedBytes = 0.0;
        state.updateTotalBytes = 0.0;
        state.updateUiProgress = 0;
        effects.progressChanged = true;
        effects.progressPos = 0;
        char status[256];
        _snprintf(status, sizeof(status), "Downloading %s...", state.updateCurrentDb.c_str());
        status[sizeof(status) - 1] = '\0';
        effects.statusChanged = true;
        effects.statusText = status;
    }

    if (hasProgress)
    {
        bool hadKnownProgress = state.updateProgressKnown;
        int prevPct = state.updateUiProgress;
        state.updateProgressKnown = true;

        /* Safety net for stream formats that omit an explicit file-switch line. */
        if (downloaded + 0.01 < state.updateDownloadedBytes)
            state.updateTransferredBytes += state.updateDownloadedBytes;

        state.updateDownloadedBytes = downloaded;
        state.updateTotalBytes = total;
        int filePct = (int)((downloaded * 100.0 / total) + 0.5);
        if (filePct < 0)
            filePct = 0;
        if (filePct > 100)
            filePct = 100;

        bool shouldRefreshProgressUi = !hadKnownProgress || (filePct != prevPct) || (filePct >= 100);
        state.updateUiProgress = filePct;
        if (shouldRefreshProgressUi)
        {
            effects.progressChanged = true;
            effects.progressPos = state.updateUiProgress;
        }

        char done[32];
        char all[32];
        char status[256];
        formatBytes(downloaded, done, sizeof(done));
        formatBytes(total, all, sizeof(all));
        const char* activeDb = isDbFileLabel(state.updateCurrentDb)
            ? state.updateCurrentDb.c_str()
            : "definitions";
        _snprintf(status, sizeof(status), "Downloading %s: %s / %s (%d%%)",
                  activeDb, done, all, filePct);
        status[sizeof(status) - 1] = '\0';
        if (shouldRefreshProgressUi)
        {
            effects.statusChanged = true;
            effects.statusText = status;
        }
    }

    return effects;
}

double parseSizeToBytes(const char* token)
{
    if (!token || !*token)
        return -1.0;

    char* endNum = NULL;
    double value = strtod(token, &endNum);
    if (endNum == token || value < 0.0)
        return -1.0;

    while (*endNum && isspace((unsigned char)*endNum))
        ++endNum;

    char unit[8];
    int i = 0;
    while (*endNum && i < (int)sizeof(unit) - 1)
    {
        if (!isalpha((unsigned char)*endNum))
            break;
        unit[i++] = (char)toupper((unsigned char)*endNum);
        ++endNum;
    }
    unit[i] = '\0';

    double mult = 1.0;
    if (strcmp(unit, "KIB") == 0 || strcmp(unit, "KB") == 0)
        mult = 1024.0;
    else if (strcmp(unit, "MIB") == 0 || strcmp(unit, "MB") == 0)
        mult = 1024.0 * 1024.0;
    else if (strcmp(unit, "GIB") == 0 || strcmp(unit, "GB") == 0)
        mult = 1024.0 * 1024.0 * 1024.0;
    else if (strcmp(unit, "TIB") == 0 || strcmp(unit, "TB") == 0)
        mult = 1024.0 * 1024.0 * 1024.0 * 1024.0;

    return value * mult;
}

void formatBytes(double bytes, char* out, size_t outSize)
{
    static const char* kUnits[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int unit = 0;
    double v = (bytes < 0.0) ? 0.0 : bytes;
    while (v >= 1024.0 && unit < 4)
    {
        v /= 1024.0;
        ++unit;
    }
    _snprintf(out, outSize, "%.2f %s", v, kUnits[unit]);
    out[outSize - 1] = '\0';
}

bool parseFreshclamProgressLine(const char* text, double* downloaded, double* total)
{
    if (!text || !downloaded || !total)
        return false;

    const char* slash = strchr(text, '/');
    if (!slash)
        return false;

    const char* leftStart = slash;
    while (leftStart > text && !isspace((unsigned char)leftStart[-1]))
        --leftStart;

    const char* rightStart = slash + 1;
    while (*rightStart && isspace((unsigned char)*rightStart))
        ++rightStart;

    const char* rightEnd = rightStart;
    while (*rightEnd && !isspace((unsigned char)*rightEnd) && *rightEnd != '\r' && *rightEnd != '\n')
        ++rightEnd;

    char left[32];
    char right[32];
    int leftLen = (int)(slash - leftStart);
    int rightLen = (int)(rightEnd - rightStart);
    if (leftLen <= 0 || rightLen <= 0 || leftLen >= (int)sizeof(left) || rightLen >= (int)sizeof(right))
        return false;

    memcpy(left, leftStart, leftLen);
    left[leftLen] = '\0';
    memcpy(right, rightStart, rightLen);
    right[rightLen] = '\0';

    double doneBytes = parseSizeToBytes(left);
    double totalBytes = parseSizeToBytes(right);
    if (doneBytes < 0.0 || totalBytes <= 0.0)
        return false;

    *downloaded = doneBytes;
    *total = totalBytes;
    return true;
}

int countSubstring(const char* text, const char* needle)
{
    if (!text || !needle || !*needle)
        return 0;
    int count = 0;
    const char* p = text;
    size_t n = strlen(needle);
    while ((p = strstr(p, needle)) != NULL)
    {
        ++count;
        p += n;
    }
    return count;
}

bool isSizeOnlyProgressLine(const char* text)
{
    if (!text)
        return false;

    const char* start = text;
    while (*start && isspace((unsigned char)*start))
        ++start;

    const char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1]))
        --end;

    int len = (int)(end - start);
    if (len <= 0 || len >= 64)
        return false;

    char token[64];
    memcpy(token, start, (size_t)len);
    token[len] = '\0';

    if (!strchr(token, ' ') && !strchr(token, '/'))
        return parseSizeToBytes(token) >= 0.0;

    const char* slash = strchr(token, '/');
    if (!slash || slash == token || slash[1] == '\0' || strchr(slash + 1, '/'))
        return false;
    if (strchr(token, ' '))
        return false;

    char left[32];
    char right[32];
    int leftLen = (int)(slash - token);
    int rightLen = (int)strlen(slash + 1);
    if (leftLen <= 0 || rightLen <= 0 || leftLen >= (int)sizeof(left) || rightLen >= (int)sizeof(right))
        return false;

    memcpy(left, token, (size_t)leftLen);
    left[leftLen] = '\0';
    memcpy(right, slash + 1, (size_t)rightLen);
    right[rightLen] = '\0';

    return parseSizeToBytes(left) >= 0.0 && parseSizeToBytes(right) > 0.0;
}

bool isDbFileLabel(const std::string& s)
{
    return endsWithNoCase(s, ".cvd") || endsWithNoCase(s, ".cld");
}

bool extractDbLabelFromAvailableLine(const char* text, std::string& out)
{
    if (!text)
        return false;

    const char* marker = strstr(text, " database available for download");
    if (!marker)
        return false;

    const char* end = marker;
    while (end > text && isspace((unsigned char)end[-1]))
        --end;
    if (end <= text)
        return false;

    const char* start = end;
    while (start > text)
    {
        char c = start[-1];
        if (isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.')
            --start;
        else
            break;
    }
    if (end <= start)
        return false;

    std::string stem(start, (size_t)(end - start));
    for (size_t i = 0; i < stem.size(); ++i)
    {
        unsigned char c = (unsigned char)stem[i];
        if (!isalnum(c) && c != '_' && c != '-' && c != '.')
            return false;
    }

    if (isDbFileLabel(stem))
        out = stem;
    else
        out = stem + ".cvd";
    return true;
}

bool extractDbLabelFromUpdatedLine(const char* text, std::string& out)
{
    if (!text)
        return false;

    const char* marker = strstr(text, " updated (version");
    if (!marker)
        return false;

    const char* end = marker;
    const char* start = end;
    while (start > text && !isspace((unsigned char)start[-1]))
        --start;
    if (end <= start)
        return false;

    std::string token(start, (size_t)(end - start));
    if (!isDbFileLabel(token))
        return false;

    out = token;
    return true;
}

bool parseSummaryInt(const char* text, const char* label, int* outValue)
{
    if (!text || !label || !outValue)
        return false;

    const char* p = strstr(text, label);
    if (!p)
        return false;

    p += strlen(label);
    while (*p && isspace((unsigned char)*p))
        ++p;

    if (!isdigit((unsigned char)*p))
        return false;

    *outValue = atoi(p);
    return true;
}

bool parseRatioProgressLine(const char* text, const char* suffix, int* outPct)
{
    if (!text || !suffix || !outPct)
        return false;

    const char* marker = strstr(text, suffix);
    if (!marker)
        return false;

    const char* slash = marker;
    while (slash > text && *slash != '/')
        --slash;
    if (*slash != '/')
        return false;

    const char* leftStart = slash;
    while (leftStart > text && !isspace((unsigned char)leftStart[-1]))
        --leftStart;

    const char* rightStart = slash + 1;
    while (*rightStart && isspace((unsigned char)*rightStart))
        ++rightStart;

    const char* rightEnd = rightStart;
    while (rightEnd < marker && !isspace((unsigned char)*rightEnd))
        ++rightEnd;

    int leftLen = (int)(slash - leftStart);
    int rightLen = (int)(rightEnd - rightStart);
    if (leftLen <= 0 || rightLen <= 0 || leftLen >= 31 || rightLen >= 31)
        return false;

    char left[32];
    char right[32];
    memcpy(left, leftStart, (size_t)leftLen);
    left[leftLen] = '\0';
    memcpy(right, rightStart, (size_t)rightLen);
    right[rightLen] = '\0';

    double done = parseProgressToken(left);
    double total = parseProgressToken(right);
    if (done < 0.0 || total <= 0.0)
        return false;

    int pct = (int)((done * 100.0 / total) + 0.5);
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    *outPct = pct;
    return true;
}

bool parseFilePercentLine(const char* text, int* outPct, std::string& outFile)
{
    if (!text || !outPct)
        return false;

    const char* sep = strstr(text, ": [");
    if (!sep)
        return false;

    const char* lbr = strchr(sep, '[');
    const char* pctSign = strchr(sep, '%');
    if (!lbr || !pctSign || pctSign <= lbr + 1)
        return false;

    char pctBuf[8];
    int pctLen = (int)(pctSign - (lbr + 1));
    if (pctLen <= 0 || pctLen >= (int)sizeof(pctBuf))
        return false;

    memcpy(pctBuf, lbr + 1, (size_t)pctLen);
    pctBuf[pctLen] = '\0';

    int pct = atoi(pctBuf);
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;

    outFile.assign(text, (size_t)(sep - text));
    *outPct = pct;
    return true;
}

bool parseFileSpinnerLine(const char* text, std::string& outFile, char* outSpinner)
{
    if (!text || !outSpinner)
        return false;

    const char* sep = strstr(text, ": [");
    if (!sep)
        return false;

    const char* lbr = strchr(sep, '[');
    const char* rbr = strchr(sep, ']');
    if (!lbr || !rbr || rbr <= lbr + 1)
        return false;

    if (strchr(lbr, '%'))
        return false;

    const char* p = lbr + 1;
    while (p < rbr && isspace((unsigned char)*p))
        ++p;
    if (p >= rbr)
        return false;

    char spinner = *p;
    if (spinner != '|' && spinner != '/' && spinner != '-' && spinner != '\\')
        return false;

    outFile.assign(text, (size_t)(sep - text));
    *outSpinner = spinner;
    return !outFile.empty();
}

bool isDbLoadOrCompileLine(const char* text)
{
    if (!text)
        return false;
    return strstr(text, "Loading:") != NULL || strstr(text, "Compiling:") != NULL;
}

bool parseFileResultLine(const char* text, std::string& outFile, std::string& outStatus)
{
    if (!text)
        return false;

    const char* sep = strstr(text, ": ");
    if (!sep)
        return false;

    const char* status = sep + 2;
    if (*status == '[')
        return false;

    outFile.assign(text, (size_t)(sep - text));
    outStatus.assign(status);

    bool looksLikePath = (outFile.find('\\') != std::string::npos) ||
                         (outFile.find('/') != std::string::npos) ||
                         (outFile.size() >= 2 && isalpha((unsigned char)outFile[0]) && outFile[1] == ':');
    if (!looksLikePath)
        return false;

    while (!outStatus.empty() && isspace((unsigned char)outStatus[outStatus.size() - 1]))
        outStatus.erase(outStatus.size() - 1);

    return !outFile.empty() && !outStatus.empty();
}

bool isExcludedFileResult(const std::string& status)
{
    return status.size() >= 8 && _strnicmp(status.c_str(), "Excluded", 8) == 0;
}
}

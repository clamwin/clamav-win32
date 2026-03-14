/*
 * ClamWin Free Antivirus — CWScanDialog implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_scan_dialog.h"
#include "cw_dpi.h"
#include "cw_scan_logic.h"
#include "cw_theme.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#ifndef PBS_MARQUEE
#define PBS_MARQUEE 0x08
#endif

#ifndef PBM_SETMARQUEE
#define PBM_SETMARQUEE (WM_USER + 10)
#endif

namespace
{
const int kScanProgressMax = 100;
const char* const kPatternSep = "|CLAMWIN_SEP|";
const int kPatternSepLen = (int)(sizeof("|CLAMWIN_SEP|") - 1);

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

std::string fileNameFromPath(const std::string& path)
{
    size_t slash = path.find_last_of("\\/");
    if (slash == std::string::npos)
        return path;
    return path.substr(slash + 1);
}

bool isPathSeparator(char ch)
{
    return ch == '\\' || ch == '/';
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

void splitPatternList(const std::string& raw, std::vector<ScanPattern>& out)
{
    out.clear();

    size_t pos = 0;
    while (pos <= raw.size())
    {
        size_t next = raw.find(kPatternSep, pos);
        std::string item;
        if (next == std::string::npos)
            item = raw.substr(pos);
        else
            item = raw.substr(pos, next - pos);

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

bool isDriveRootPath(const std::string& path)
{
    return path.size() == 2 ||
           (path.size() == 3 && isalpha((unsigned char)path[0]) && path[1] == ':' &&
            (path[2] == '\\' || path[2] == '/'));
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
        else if (ch == '/')
        {
            regex += "[/\\\\]";
        }
        else if (ch == '\\')
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

bool shouldCountPath(const std::string& path,
                     const std::vector<ScanPattern>& includePatterns,
                     const std::vector<ScanPattern>& excludePatterns)
{
    const std::string fileName = fileNameFromPath(path);

    if (!includePatterns.empty() && !matchesAnyWildcardPattern(includePatterns, fileName, path))
        return false;

    if (matchesAnyWildcardPattern(excludePatterns, fileName, path))
        return false;

    return true;
}

void appendDebugLineToFile(const std::string& filePath, const std::string& line)
{
    if (filePath.empty() || line.empty())
        return;

    HANDLE hFile = CreateFileA(filePath.c_str(), FILE_APPEND_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    DWORD written = 0;
    WriteFile(hFile, line.c_str(), (DWORD)line.size(), &written, NULL);
    (void)written;
    CloseHandle(hFile);
}

void enumScanPath(const std::string& path,
                  const std::vector<ScanPattern>& includePatterns,
                  const std::vector<ScanPattern>& excludePatterns,
                  bool applyFilters,
                  int& fileCount,
                  ULONGLONG& totalBytes)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
        return;

    if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        WIN32_FIND_DATAA ffd0;
        HANDLE h = FindFirstFileA(path.c_str(), &ffd0);
        if (h != INVALID_HANDLE_VALUE)
        {
            if (!applyFilters || shouldCountPath(path, includePatterns, excludePatterns))
            {
                ++fileCount;
                totalBytes += ((ULONGLONG)ffd0.nFileSizeHigh << 32) | ffd0.nFileSizeLow;
            }
            FindClose(h);
        }
        return;
    }

    std::string pattern = path;
    if (!pattern.empty())
    {
        char tail = pattern[pattern.size() - 1];
        if (tail != '\\' && tail != '/')
            pattern += "\\";
    }
    pattern += "*";

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
            continue;

        std::string child = path;
        if (!child.empty())
        {
            char tail = child[child.size() - 1];
            if (tail != '\\' && tail != '/')
                child += "\\";
        }
        child += ffd.cFileName;

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            enumScanPath(child, includePatterns, excludePatterns, applyFilters, fileCount, totalBytes);
        }
        else
        {
            if (!applyFilters || shouldCountPath(child, includePatterns, excludePatterns))
            {
                ++fileCount;
                totalBytes += ((ULONGLONG)ffd.nFileSizeHigh << 32) | ffd.nFileSizeLow;
            }
        }
    }
    while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
}

void enumScanTargets(const std::string& rawTarget,
                     const CWConfig& cfg,
                     int& fileCount,
                     ULONGLONG& totalBytes)
{
    fileCount = 0;
    totalBytes = 0;

    std::vector<ScanPattern> includePatterns;
    std::vector<ScanPattern> excludePatterns;
    bool applyFilters = shouldApplyPreferenceFilters(rawTarget);
    if (applyFilters)
    {
        splitPatternList(cfg.includePatterns, includePatterns);
        splitPatternList(cfg.excludePatterns, excludePatterns);
        applyFilters = !includePatterns.empty() || !excludePatterns.empty();
    }

    std::string target = trimOuterQuotes(rawTarget);
    if (!target.empty())
        enumScanPath(target, includePatterns, excludePatterns, applyFilters, fileCount, totalBytes);
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

    /* Single size token, e.g. "22.29MiB" */
    if (!strchr(token, ' ') && !strchr(token, '/'))
    {
        return parseSizeToBytes(token) >= 0.0;
    }

    /* Size pair token, e.g. "22.29MiB/84.95MiB" */
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
        if (isalnum((unsigned char)c) || c == '_' || c == '-')
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
        if (!isalnum(c) && c != '_' && c != '-')
            return false;
    }

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
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
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
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

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

/* ─── Constructor / Destructor ──────────────────────────────── */

CWScanDialog::CWScanDialog(CWConfig& cfg, const std::string& targetPath,
                           bool isUpdate, bool scanMemoryOnly)
    : m_cfg(cfg)
    , m_targetPath(targetPath)
    , m_isUpdate(isUpdate)
    , m_scanMemoryOnly(scanMemoryOnly)
    , m_hwndStatus(NULL), m_hwndProgress(NULL)
    , m_hwndStats(NULL),  m_hwndLog(NULL)
    , m_hwndBtnStop(NULL),m_hwndBtnSave(NULL)
    , m_hFont(NULL), m_hFontBold(NULL)
    , m_finished(0), m_cancelled(0), m_exitCode(-1)
    , m_showMnemonics(false)
    , m_scanExpectedFiles(0)
    , m_scanExpectedBytes(0)
    , m_scanCompletedFiles(0)
    , m_scanScannedBytes(0)
    , m_scanUiProgress(0)
    , m_scanDbPhaseSeen(false)
    , m_scanFilePhaseSeen(false)
    , m_scanStatusBase("")
    , m_scanAnimate(false)
    , m_scanAnimFrame(0)
    , m_lastStatusText("")
    , m_memoryLoopProgressPos(0)
    , m_memoryUseMarquee(false)
    , m_updateProgressKnown(false)
    , m_updateDownloadedBytes(0.0)
    , m_updateTotalBytes(0.0)
    , m_updateTransferredBytes(0.0)
    , m_updateUiProgress(0)
    , m_updateHadChanges(false)
    , m_updateUpToDateCount(0)
    , m_updateCurrentDb("")
    , m_updateBlocked(false)
    , m_updateUnsupportedVersion(false)
    , m_updateServerError(false)
    , m_updateToolMissing(false)
    , m_updateLastErrorLine("")
    , m_hWorker(NULL)
{
    memset(&m_stats, 0, sizeof(m_stats));
}

CWScanDialog::~CWScanDialog()
{
    /* Ensure process is stopped before fonts are freed */
    m_process.stop();

    if (m_hWorker)
    {
        WaitForSingleObject(m_hWorker, 2000);
        CloseHandle(m_hWorker);
    }

    auto del = [](HFONT& hf) {
        if (hf && hf != (HFONT)GetStockObject(DEFAULT_GUI_FONT))
            DeleteObject(hf);
        hf = NULL;
    };
    del(m_hFont);
    del(m_hFontBold);
}

/* ─── run ───────────────────────────────────────────────────── */

int CWScanDialog::run(HWND parent)
{
    return (int)runModal(parent, 600, 450);
}

/* ─── Process callbacks (called from reader thread — must PostMessage) */

void CWScanDialog::outputCb(const char* text, void* userdata)
{
    CWScanDialog* self = static_cast<CWScanDialog*>(userdata);
    OutputMsg* msg = (OutputMsg*)malloc(sizeof(OutputMsg));
    if (!msg) return;
    lstrcpynA(msg->text, text, sizeof(msg->text));
    msg->is_error = 0;
    PostMessageA(self->m_hwnd, WM_SCAN_UPDATE, 0, (LPARAM)msg);
}

void CWScanDialog::errorCb(const char* text, void* userdata)
{
    CWScanDialog* self = static_cast<CWScanDialog*>(userdata);
    OutputMsg* msg = (OutputMsg*)malloc(sizeof(OutputMsg));
    if (!msg) return;
    lstrcpynA(msg->text, text, sizeof(msg->text));
    msg->is_error = 1;
    PostMessageA(self->m_hwnd, WM_SCAN_UPDATE, 0, (LPARAM)msg);
}

void CWScanDialog::finishedCb(int exitCode, void* userdata)
{
    CWScanDialog* self = static_cast<CWScanDialog*>(userdata);
    PostMessageA(self->m_hwnd, WM_SCAN_FINISHED, (WPARAM)exitCode, 0);
}

/* ─── Worker threads ─────────────────────────────────────────── */

DWORD WINAPI CWScanDialog::scanWorker(LPVOID param)
{
    CWScanDialog* self = static_cast<CWScanDialog*>(param);

    if (!self->m_scanMemoryOnly)
    {
        int fc = 0;
        ULONGLONG tb = 0;
        enumScanTargets(self->m_targetPath, self->m_cfg, fc, tb);
        self->m_scanExpectedFiles = fc;
        self->m_scanExpectedBytes = tb;
    }

    char exeDir[CW_MAX_PATH];
    GetModuleFileNameA(NULL, exeDir, sizeof(exeDir));
    char* slash = strrchr(exeDir, '\\');
    if (slash) *(slash + 1) = '\0';

    std::string cmd = CWScanLogic::buildClamscanCommand(self->m_cfg,
                                                        self->m_targetPath,
                                                        exeDir,
                                                        self->m_scanMemoryOnly);

    {
        std::string debugLine = "[CWDebug] clamscan command: ";
        debugLine += cmd;
        debugLine += "\r\n";
        OutputDebugStringA(debugLine.c_str());
        appendDebugLineToFile(self->m_cfg.scanLogFile, debugLine);
    }

    if (!self->m_process.start(cmd, self->m_cfg.priority,
                               outputCb, errorCb, finishedCb, self))
    {
        PostMessageA(self->m_hwnd, WM_SCAN_FINISHED, (WPARAM)-1, 0);
    }
    return 0;
}

DWORD WINAPI CWScanDialog::updateWorker(LPVOID param)
{
    CWScanDialog* self = static_cast<CWScanDialog*>(param);

    /* Build freshclam command line using std::string (no fixed buffer overflows) */
    char exeDir[CW_MAX_PATH];
    GetModuleFileNameA(NULL, exeDir, sizeof(exeDir));
    char* slash = strrchr(exeDir, '\\');
    if (slash) *(slash + 1) = '\0';

    if (!CWScanLogic::hasFreshclamExecutable(exeDir))
    {
        self->m_updateToolMissing = true;
        std::string err = "ERROR: freshclam.exe not found at: ";
        err += CWScanLogic::buildFreshclamExecutablePath(exeDir);
        err += "\r\n";
        errorCb(err.c_str(), self);
        PostMessageA(self->m_hwnd, WM_SCAN_FINISHED, (WPARAM)-1, 0);
        return 0;
    }

    std::string cmd = CWScanLogic::buildFreshclamCommand(self->m_cfg, exeDir);

    if (!self->m_process.start(cmd, self->m_cfg.priority,
                               outputCb, errorCb, finishedCb, self))
    {
        PostMessageA(self->m_hwnd, WM_SCAN_FINISHED, (WPARAM)-1, 0);
    }
    return 0;
}


void CWScanDialog::startWorker()
{
    m_stats.start_tick = GetTickCount();
    m_hWorker = CreateThread(NULL, 0,
                              m_isUpdate ? updateWorker : scanWorker,
                              this, 0, NULL);
}

/* ─── onInit ─────────────────────────────────────────────────── */

bool CWScanDialog::onInit()
{
    /* Icon */
    HICON hIcon = LoadIconA(GetModuleHandleA(NULL),
                             MAKEINTRESOURCEA(IDI_CLAMWIN));
    SendMessageA(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessageA(m_hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);

    /* Title */
    SetWindowTextA(m_hwnd, m_isUpdate ? "ClamWin - Updating Database"
                                      : "ClamWin - Scanning");

    /* Fonts */
    m_hFont = CreateFontA(-CW_Scale(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                           "Tahoma");
    m_hFontBold = CreateFontA(-CW_Scale(13), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                               "Tahoma");
    if (!m_hFont)     m_hFont     = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!m_hFontBold) m_hFontBold = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    /* Layout controls */
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int w = rc.right - CW_Scale(24);

    m_hwndStatus = CreateWindowExA(0, "STATIC",
        m_isUpdate ? "Checking update metadata..."
                   : (m_scanMemoryOnly ? "Scanning computer memory..."
                                       : "Enumerating files..."),
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CW_Scale(12), CW_Scale(12), w, CW_Scale(20), m_hwnd, (HMENU)IDC_SCAN_STATUS, NULL, NULL);
    SendMessageA(m_hwndStatus, WM_SETFONT, (WPARAM)m_hFont, 0);

    DWORD progressStyle = WS_CHILD | WS_VISIBLE | PBS_SMOOTH;
    if (!m_isUpdate && m_scanMemoryOnly)
        progressStyle |= PBS_MARQUEE;

    m_hwndProgress = CreateWindowExA(0, PROGRESS_CLASSA, NULL,
        progressStyle,
        CW_Scale(12), CW_Scale(38), w, CW_Scale(22), m_hwnd, (HMENU)IDC_SCAN_PROGRESS, NULL, NULL);
    SendMessageA(m_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageA(m_hwndProgress, PBM_SETPOS, 0, 0);
    if (!m_isUpdate && m_scanMemoryOnly)
    {
        LRESULT marqueeOk = SendMessageA(m_hwndProgress, PBM_SETMARQUEE, TRUE, 30);
        m_memoryUseMarquee = (marqueeOk != 0);
    }

    m_hwndStats = CreateWindowExA(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        CW_Scale(12), CW_Scale(68), w, CW_Scale(20), m_hwnd, (HMENU)IDC_SCAN_STATS, NULL, NULL);
    SendMessageA(m_hwndStats, WM_SETFONT, (WPARAM)m_hFont, 0);
    if (m_isUpdate)
        SetWindowTextA(m_hwndStats, "Downloaded: 0 B / 0 B    Speed: 0 B/s    Elapsed: 00:00");

    m_hwndLog = CreateWindowExA(0, RICHEDIT_CLASSA, "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        CW_Scale(12), CW_Scale(96), w, rc.bottom - CW_Scale(140), m_hwnd, (HMENU)IDC_SCAN_LOG, NULL, NULL);
    SendMessageA(m_hwndLog, WM_SETFONT, (WPARAM)m_hFont, 0);
    CWTheme* theme = CW_GetTheme();
    SendMessageA(m_hwndLog, EM_SETBKGNDCOLOR, 0,
                 theme ? theme->colorSurface() : RGB(250, 250, 250));

    m_hwndBtnSave = CreateWindowExA(0, "BUTTON", "&Save Report",
        WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_OWNERDRAW,
        rc.right - CW_Scale(220), rc.bottom - CW_Scale(36), CW_Scale(100), CW_Scale(28),
        m_hwnd, (HMENU)IDC_SCAN_SAVE, NULL, NULL);
    SendMessageA(m_hwndBtnSave, WM_SETFONT, (WPARAM)m_hFont, 0);

    m_hwndBtnStop = CreateWindowExA(0, "BUTTON", "&Stop",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
        rc.right - CW_Scale(112), rc.bottom - CW_Scale(36), CW_Scale(100), CW_Scale(28),
        m_hwnd, (HMENU)IDC_SCAN_STOP, NULL, NULL);
    SendMessageA(m_hwndBtnStop, WM_SETFONT, (WPARAM)m_hFont, 0);

    setDialogMnemonicCues(m_hwnd, false);
    SetTimer(m_hwnd, 1, 200, NULL);
    SetTimer(m_hwnd, 2, 30, NULL);
    startWorker();
    return TRUE;
}

/* ─── onCommand ─────────────────────────────────────────────── */

bool CWScanDialog::onCommand(int id, HWND src)
{
    (void)src;
    switch (id)
    {
        case IDC_SCAN_STOP:
            if (!m_finished)
            {
                m_cancelled = 1;
                m_process.stop();
            }
            else
            {
                endDialog(m_cancelled ? -1 : m_exitCode);
            }
            return true;

        case IDC_SCAN_SAVE:
            saveReport();
            return true;

        case IDCANCEL:
            if (!m_finished)
            {
                m_cancelled = 1;
                m_process.stop();
            }
            endDialog(-1);
            return true;
    }
    return false;
}

/* ─── onClose ────────────────────────────────────────────────── */

void CWScanDialog::onClose()
{
    if (!m_finished)
    {
        m_cancelled = 1;
        m_process.stop();
    }
    endDialog(m_cancelled ? -1 : m_exitCode);
}

/* ─── handleMessage ─────────────────────────────────────────── */

INT_PTR CWScanDialog::handleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_TIMER:
            if (wp == 1)
            {
                updateStatsDisplay();
                refreshScanStatusAnimation();
                if (!m_isUpdate && m_scanMemoryOnly && !m_finished && m_hwndProgress && !m_memoryUseMarquee)
                {
                    m_memoryLoopProgressPos += 4;
                    if (m_memoryLoopProgressPos > 100)
                        m_memoryLoopProgressPos = 0;
                    SendMessageA(m_hwndProgress, PBM_SETPOS, m_memoryLoopProgressPos, 0);
                }
            }
            else if (wp == 2)
            {
                const bool showNow = (GetKeyState(VK_MENU) & 0x8000) != 0;
                if (showNow != m_showMnemonics)
                {
                    m_showMnemonics = showNow;
                    setDialogMnemonicCues(m_hwnd, m_showMnemonics);
                    InvalidateRect(m_hwnd, NULL, TRUE);
                }
            }
            return TRUE;

        case WM_SYSKEYDOWN:
        {
            const bool showNow = ((GetKeyState(VK_MENU) & 0x8000) != 0) || (wp == VK_MENU);
            if (showNow != m_showMnemonics)
            {
                m_showMnemonics = showNow;
                setDialogMnemonicCues(m_hwnd, m_showMnemonics);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            if (wp == VK_MENU)
                return TRUE;
            break;
        }

        case WM_SYSKEYUP:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                setDialogMnemonicCues(m_hwnd, false);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            if (wp == VK_MENU)
                return TRUE;
            break;

        case WM_SYSCOMMAND:
            if ((wp & 0xFFF0) == SC_KEYMENU)
                return TRUE;
            break;

        case WM_KILLFOCUS:
            if (m_showMnemonics)
            {
                m_showMnemonics = false;
                setDialogMnemonicCues(m_hwnd, false);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            break;

        case WM_DESTROY:
            KillTimer(m_hwnd, 2);
            break;

        case WM_SCAN_UPDATE:
        {
            OutputMsg* om = reinterpret_cast<OutputMsg*>(lp);
            if (om)
            {
                onScanOutput(om->text, om->is_error != 0);
                free(om);
            }
            return TRUE;
        }

        case WM_SCAN_FINISHED:
            onScanFinished((int)wp);
            return TRUE;

        case WM_SIZE:
            layoutControls(LOWORD(lp), HIWORD(lp));
            return TRUE;
    }
    return CWDialog::handleMessage(msg, wp, lp);
}

/* ─── onScanOutput ──────────────────────────────────────────── */

void CWScanDialog::onScanOutput(const char* text, bool isError)
{
    if (!m_hwndLog) return;

    if (m_isUpdate && text)
    {
        bool looksLikeError = isError || strstr(text, "ERROR:") != NULL || strstr(text, "WARNING:") != NULL;
        if (looksLikeError)
        {
            std::string line = trimSpaces(text);
            while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n'))
                line.erase(line.size() - 1);
            if (!line.empty())
                m_updateLastErrorLine = line;
        }
    }

    CWScanLogic::ScanOutputState state;
    CWScanLogic::initScanOutputState(state, m_isUpdate);
    state.filesScanned = m_stats.files_scanned;
    state.threatsFound = m_stats.threats_found;
    state.scanExpectedFiles = m_scanExpectedFiles;
    state.scanExpectedBytes = m_scanExpectedBytes;
    state.scanCompletedFiles = m_scanCompletedFiles;
    state.scanScannedBytes = m_scanScannedBytes;
    state.scanUiProgress = m_scanUiProgress;
    state.scanDbPhaseSeen = m_scanDbPhaseSeen;
    state.scanFilePhaseSeen = m_scanFilePhaseSeen;
    state.updateProgressKnown = m_updateProgressKnown;
    state.updateDownloadedBytes = m_updateDownloadedBytes;
    state.updateTotalBytes = m_updateTotalBytes;
    state.updateTransferredBytes = m_updateTransferredBytes;
    state.updateUiProgress = m_updateUiProgress;
    state.updateHadChanges = m_updateHadChanges;
    state.updateUpToDateCount = m_updateUpToDateCount;
    state.updateCurrentDb = m_updateCurrentDb;
    state.updateBlocked = m_updateBlocked;
    state.updateUnsupportedVersion = m_updateUnsupportedVersion;
    state.updateServerError = m_updateServerError;

    CWScanLogic::ScanLineEffects effects = CWScanLogic::processOutputLine(state, text, isError);

    m_stats.files_scanned = state.filesScanned;
    m_stats.threats_found = state.threatsFound;
    m_scanExpectedFiles = state.scanExpectedFiles;
    m_scanExpectedBytes = state.scanExpectedBytes;
    m_scanCompletedFiles = state.scanCompletedFiles;
    m_scanScannedBytes = state.scanScannedBytes;
    m_scanUiProgress = state.scanUiProgress;
    m_scanDbPhaseSeen = state.scanDbPhaseSeen;
    m_scanFilePhaseSeen = state.scanFilePhaseSeen;
    m_updateProgressKnown = state.updateProgressKnown;
    m_updateDownloadedBytes = state.updateDownloadedBytes;
    m_updateTotalBytes = state.updateTotalBytes;
    m_updateTransferredBytes = state.updateTransferredBytes;
    m_updateUiProgress = state.updateUiProgress;
    m_updateHadChanges = state.updateHadChanges;
    m_updateUpToDateCount = state.updateUpToDateCount;
    m_updateCurrentDb = state.updateCurrentDb;
    m_updateBlocked = state.updateBlocked;
    m_updateUnsupportedVersion = state.updateUnsupportedVersion;
    m_updateServerError = state.updateServerError;

    if (effects.statusChanged)
    {
        if (m_isUpdate)
            setStatusTextIfChanged(effects.statusText);
        else
            setScanStatusText(effects.statusText, effects.statusAnimate);
    }

    if (effects.progressChanged && !(m_scanMemoryOnly && !m_isUpdate && !m_finished))
        SendMessageA(m_hwndProgress, PBM_SETPOS, effects.progressPos, 0);

    if (effects.appendToLog)
        appendLog(text, isError);
}

/* ─── appendLog ─────────────────────────────────────────────── */

void CWScanDialog::appendLog(const char* text, bool isError)
{
    CWTheme* theme = CW_GetTheme();
    CHARRANGE cr;
    cr.cpMin = -1; cr.cpMax = -1;
    SendMessageA(m_hwndLog, EM_EXSETSEL, 0, (LPARAM)&cr);

    CHARFORMAT2A cf;
    memset(&cf, 0, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;

    bool isThreat = !isError && strstr(text, "FOUND") != NULL;
    if (isThreat)
    {
        cf.dwEffects  = CFE_BOLD;
        cf.crTextColor = theme ? theme->colorWarning() : RGB(198, 40, 40);
    }
    else if (isError)
    {
        cf.dwEffects  = 0;
        cf.crTextColor = theme ? theme->colorTextMuted() : RGB(117, 117, 117);
    }
    else
    {
        cf.dwEffects  = 0;
        cf.crTextColor = theme ? theme->colorText() : RGB(33, 33, 33);
    }
    SendMessageA(m_hwndLog, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageA(m_hwndLog, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageA(m_hwndLog, WM_VSCROLL, SB_BOTTOM, 0);
}

/* ─── onScanFinished ─────────────────────────────────────────── */

void CWScanDialog::onScanFinished(int exitCode)
{
    m_finished = 1;
    m_exitCode = exitCode;

    KillTimer(m_hwnd, 1);
    updateStatsDisplay();

    if (!m_isUpdate && m_scanMemoryOnly && m_memoryUseMarquee)
        SendMessageA(m_hwndProgress, PBM_SETMARQUEE, FALSE, 0);

    bool noUpdatesNeeded = m_isUpdate && !m_cancelled && (m_exitCode == 0) &&
                           (!m_updateHadChanges) && (m_updateUpToDateCount > 0);
    bool updateToolMissing = m_isUpdate && !m_cancelled && m_updateToolMissing;
    bool updateBlocked = m_isUpdate && !m_cancelled && m_updateBlocked;
    bool updateServerError = m_isUpdate && !m_cancelled && m_updateServerError;
    bool updateExitFailed = m_isUpdate && !m_cancelled && (m_exitCode != 0) &&
                            !updateToolMissing && !updateBlocked && !updateServerError;
    bool updateFailed = updateToolMissing || updateServerError || updateExitFailed;
    bool updateErrorVisual = updateFailed || updateBlocked;

    if (m_isUpdate && updateErrorVisual)
        SendMessageA(m_hwndProgress, PBM_SETPOS, 0, 0);
    else
        SendMessageA(m_hwndProgress, PBM_SETPOS, 100, 0);

    m_scanAnimate = false;
    const char* statusText = m_cancelled      ? "Cancelled"
                           : updateToolMissing ? "Update failed: freshclam.exe not found (check ClamWin folder)"
                           : updateServerError ? "Update failed: server/DNS resolution error"
                           : updateBlocked    ? (m_updateUnsupportedVersion
                                                  ? "Update blocked: unsupported ClamAV version"
                                                  : "Update blocked by CDN cooldown")
                           : updateExitFailed ? "Update failed"
                           : noUpdatesNeeded  ? "Virus definitions are already up to date"
                           : m_isUpdate       ? "Update complete"
                                              : "Scan complete";
    setStatusTextIfChanged(statusText);
    SetWindowTextA(m_hwndBtnStop, "&Close");
    EnableWindow(m_hwndBtnSave, TRUE);

    const char* resultLabel = m_cancelled ? "Cancelled"
                           : updateFailed ? "Failed"
                           : updateBlocked ? "Blocked"
                           : "Completed";
    char footer[160];
    _snprintf(footer, sizeof(footer),
              "\r\n------------------------------------\r\n%s\r\n------------------------------------\r\n",
              resultLabel);
    footer[sizeof(footer) - 1] = '\0';
    appendLog(footer, false);

    if (m_isUpdate && !m_cancelled)
    {
        DWORD elapsed = (GetTickCount() - m_stats.start_tick) / 1000;
        double totalDone = m_updateTransferredBytes + m_updateDownloadedBytes;
        double avgSpeed = (elapsed > 0) ? (totalDone / elapsed) : 0.0;
        char summary[256];

        if (updateToolMissing)
        {
            _snprintf(summary, sizeof(summary),
                      "Update failed: freshclam.exe was not found. Place freshclam.exe in the ClamWin folder and retry. No new virus definitions were downloaded.\r\n");
        }
        else if (updateServerError)
        {
            _snprintf(summary, sizeof(summary),
                      "Update failed: server or DNS resolution problem prevented database download. No new virus definitions were downloaded.\r\n");
        }
        else if (updateBlocked)
        {
            _snprintf(summary, sizeof(summary),
                      m_updateUnsupportedVersion
                          ? "Update blocked: unsupported ClamAV version. No new virus definitions were downloaded.\r\n"
                          : "Update blocked by CDN cooldown/rate limit. No new virus definitions were downloaded.\r\n");
        }
        else if (updateExitFailed)
        {
            _snprintf(summary, sizeof(summary),
                      "Update failed (exit code: %d). No new virus definitions were downloaded.\r\n",
                      m_exitCode);
        }
        else if (noUpdatesNeeded || totalDone <= 0.0)
        {
            _snprintf(summary, sizeof(summary),
                      "No new virus definitions were downloaded.\r\n");
        }
        else
        {
            char done[32];
            char speed[32];
            formatBytes(totalDone, done, sizeof(done));
            formatBytes(avgSpeed, speed, sizeof(speed));
            _snprintf(summary, sizeof(summary),
                      "Total downloaded: %s    Average speed: %s/s\r\n",
                      done, speed);
        }
        summary[sizeof(summary) - 1] = '\0';
        appendLog(summary, false);

        if ((updateErrorVisual || updateExitFailed) && !m_updateLastErrorLine.empty())
        {
            char detail[600];
            _snprintf(detail, sizeof(detail), "Error detail: %s\r\n", m_updateLastErrorLine.c_str());
            detail[sizeof(detail) - 1] = '\0';
            appendLog(detail, true);
        }
    }

    if (m_isUpdate && updateErrorVisual)
    {
        HICON hErrorIcon = LoadIconA(NULL, MAKEINTRESOURCEA(32513)); /* IDI_ERROR / IDI_HAND */
        if (hErrorIcon)
        {
            SendMessageA(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hErrorIcon);
            SendMessageA(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hErrorIcon);
        }
    }

    SetWindowTextA(m_hwnd,
                   m_isUpdate
                       ? (updateFailed ? "ClamWin - Update Failed"
                                       : (updateBlocked ? "ClamWin - Update Blocked"
                                                        : "ClamWin - Update Complete"))
                       : "ClamWin - Scan Complete");

    if (m_isUpdate && !m_cancelled && !updateFailed && !updateBlocked && noUpdatesNeeded)
        m_exitCode = CW_UPDATE_RC_NO_CHANGES;

    if (m_hWorker)
    {
        WaitForSingleObject(m_hWorker, 2000);
        CloseHandle(m_hWorker);
        m_hWorker = NULL;
    }
}

/* ─── updateStatsDisplay ─────────────────────────────────────── */

void CWScanDialog::updateStatsDisplay()
{
    if (!m_hwndStats) return;
    DWORD elapsed = (GetTickCount() - m_stats.start_tick) / 1000;
    int mins = (int)(elapsed / 60);
    int secs = (int)(elapsed % 60);
    char buf[256];
    if (m_isUpdate)
    {
        char done[32];
        char all[32];
        char speedText[32];
        double sessionDone = m_updateTransferredBytes + m_updateDownloadedBytes;
        double sessionTotal = m_updateTransferredBytes + m_updateTotalBytes;
        formatBytes(sessionDone, done, sizeof(done));
        formatBytes(sessionTotal, all, sizeof(all));
        formatBytes((elapsed > 0) ? (sessionDone / elapsed) : 0.0,
                    speedText, sizeof(speedText));

        if (m_updateProgressKnown)
        {
            wsprintfA(buf, "Downloaded total: %s    Throughput: %s/s    Elapsed: %02d:%02d",
                      done, speedText, mins, secs);
        }
        else
        {
            if (!m_updateHadChanges && m_updateUpToDateCount > 0)
                wsprintfA(buf, "No new virus definitions were downloaded.    Elapsed: %02d:%02d", mins, secs);
            else
                wsprintfA(buf, "Preparing secure download channel...    Elapsed: %02d:%02d", mins, secs);
        }
    }
    else
    {
        int expected = (m_scanExpectedFiles > 0) ? m_scanExpectedFiles : m_scanCompletedFiles;
        if (m_scanExpectedFiles > 0)
        {
            wsprintfA(buf, "Files: %d / %d    Threats: %d    Elapsed: %02d:%02d",
                      m_scanCompletedFiles, expected, m_stats.threats_found, mins, secs);
        }
        else
        {
            wsprintfA(buf, "Files scanned: %d    Threats: %d    Elapsed: %02d:%02d",
                      m_scanCompletedFiles, m_stats.threats_found, mins, secs);
        }

        if (!m_isUpdate && !m_scanDbPhaseSeen && !m_scanFilePhaseSeen &&
            m_scanCompletedFiles == 0 && m_hwndStatus)
        {
            char status[196];
            _snprintf(status, sizeof(status),
                      "File enumeration complete: %d files. Loading virus definitions...",
                      m_scanExpectedFiles);
            status[sizeof(status) - 1] = '\0';
            setScanStatusText(status, true);
        }
    }
    SetWindowTextA(m_hwndStats, buf);
}

void CWScanDialog::setStatusTextIfChanged(const std::string& text)
{
    if (!m_hwndStatus)
        return;

    if (text == m_lastStatusText)
        return;

    SetWindowTextA(m_hwndStatus, text.c_str());
    m_lastStatusText = text;
}

void CWScanDialog::setScanStatusText(const std::string& baseText, bool animate)
{
    if (m_isUpdate || !m_hwndStatus)
    {
        if (m_hwndStatus)
            setStatusTextIfChanged(baseText);
        return;
    }

    m_scanStatusBase = baseText;
    m_scanAnimate = animate;

    if (!animate)
    {
        setStatusTextIfChanged(m_scanStatusBase);
        return;
    }

    static const char kSpinner[] = {'|', '/', '-', '\\'};
    char status[700];
    char frame = kSpinner[m_scanAnimFrame % 4];
    _snprintf(status, sizeof(status), "%s [%c]", m_scanStatusBase.c_str(), frame);
    status[sizeof(status) - 1] = '\0';
    setStatusTextIfChanged(status);
}

void CWScanDialog::refreshScanStatusAnimation()
{
    if (m_isUpdate || !m_scanAnimate || m_scanStatusBase.empty() || !m_hwndStatus)
        return;

    ++m_scanAnimFrame;
    static const char kSpinner[] = {'|', '/', '-', '\\'};
    char status[700];
    char frame = kSpinner[m_scanAnimFrame % 4];
    _snprintf(status, sizeof(status), "%s [%c]", m_scanStatusBase.c_str(), frame);
    status[sizeof(status) - 1] = '\0';
    setStatusTextIfChanged(status);
}

std::string CWScanDialog::fitPathForStatus(const std::string& path, int reservedChars) const
{
    if (path.empty() || !m_hwndStatus)
        return path;

    RECT rc;
    GetClientRect(m_hwndStatus, &rc);
    int widthPx = rc.right - rc.left;
    if (widthPx <= 0)
        return path;

    HDC hdc = GetDC(m_hwndStatus);
    if (!hdc)
        return path;

    HFONT useFont = m_hFont ? m_hFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldFont = (HFONT)SelectObject(hdc, useFont);

    TEXTMETRICA tm;
    memset(&tm, 0, sizeof(tm));
    GetTextMetricsA(hdc, &tm);

    SelectObject(hdc, oldFont);
    ReleaseDC(m_hwndStatus, hdc);

    int avgCharWidth = (tm.tmAveCharWidth > 0) ? tm.tmAveCharWidth : 7;
    int maxChars = (widthPx / avgCharWidth) - reservedChars;
    if (maxChars < 8)
        maxChars = 8;

    if ((int)path.size() <= maxChars)
        return path;

    int tailChars = maxChars - 3;
    if (tailChars < 1)
        tailChars = 1;

    return std::string("...") + path.substr(path.size() - (size_t)tailChars);
}

/* ─── layoutControls ─────────────────────────────────────────── */

void CWScanDialog::layoutControls(int w, int h)
{
    int iw = w - CW_Scale(24);
    if (m_hwndStatus)   MoveWindow(m_hwndStatus,   CW_Scale(12),     CW_Scale(12),     iw, CW_Scale(20),   TRUE);
    if (m_hwndProgress) MoveWindow(m_hwndProgress, CW_Scale(12),     CW_Scale(38),     iw, CW_Scale(22),   TRUE);
    if (m_hwndStats)    MoveWindow(m_hwndStats,    CW_Scale(12),     CW_Scale(68),     iw, CW_Scale(20),   TRUE);
    if (m_hwndLog)      MoveWindow(m_hwndLog,      CW_Scale(12),     CW_Scale(96),     iw, h - CW_Scale(140), TRUE);
    if (m_hwndBtnSave)  MoveWindow(m_hwndBtnSave,  w - CW_Scale(220), h - CW_Scale(36), CW_Scale(100), CW_Scale(28), TRUE);
    if (m_hwndBtnStop)  MoveWindow(m_hwndBtnStop,  w - CW_Scale(112), h - CW_Scale(36), CW_Scale(100), CW_Scale(28), TRUE);
}

/* ─── saveReport ─────────────────────────────────────────────── */

void CWScanDialog::saveReport()
{
    char filename[CW_MAX_PATH];
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(filename, sizeof(filename),
             "clamav_report_%d%m%y_%H%M%S.txt", tm);

    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hwnd;
    ofn.lpstrFilter = "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = sizeof(filename);
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "txt";

    if (!GetSaveFileNameA(&ofn)) return;

    int len = GetWindowTextLengthA(m_hwndLog);
    char* text = (char*)malloc(len + 2);
    if (!text) return;

    GetWindowTextA(m_hwndLog, text, len + 1);
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0,
                                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        WriteFile(hFile, text, (DWORD)len, &written, NULL);
        CloseHandle(hFile);
    }
    free(text);
}

/* ─── Public C wrappers (called by CWApplication) ─────────────── */

int CW_ScanDialogRun(HWND hwndParent, CWConfig* cfg, const char* targetPath)
{
    CWScanDialog dlg(*cfg, targetPath ? std::string(targetPath) : "", false, false);
    return dlg.run(hwndParent);
}

int CW_ScanMemoryDialogRun(HWND hwndParent, CWConfig* cfg)
{
    CWScanDialog dlg(*cfg, "", false, true);
    return dlg.run(hwndParent);
}

int CW_UpdateDialogRun(HWND hwndParent, CWConfig* cfg)
{
    CWScanDialog dlg(*cfg, "", true, false);
    return dlg.run(hwndParent);
}

/*
 * ClamWin Free Antivirus — CWConfig implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_config.h"
#include "cw_gui_shared.h"   /* for CW_INI_SECTION_* defines */

#include <stdio.h>
#include <string.h>

/* ─── Section names (same as legacy Python ClamWin) ─────────── */

static const char* SEC_CLAMAV   = "ClamAV";
static const char* SEC_UPDATES  = "Updates";
static const char* SEC_PROXY    = "Proxy";
static const char* SEC_SCHEDULE = "Schedule";
static const char* SEC_ALERTS   = "EmailAlerts";
static const char* SEC_UI       = "UI";

static std::string normalizeFilterPatterns(const std::string& raw)
{
    if (raw == "*.dbx|CLAMWIN_SEP|bb|CLAMWIN_SEP|st")
        return "*.dbx|CLAMWIN_SEP|*.tbb|CLAMWIN_SEP|*.pst";
    return raw;
}

/* ─── Constructor: apply defaults ────────────────────────────── */

CWConfig::CWConfig()
{
    defaults();
}

/* ─── Defaults ──────────────────────────────────────────────── */

void CWConfig::defaults()
{
    char exedir[MAX_PATH];
    GetModuleFileNameA(NULL, exedir, MAX_PATH);
    char* slash = strrchr(exedir, '\\');
    if (slash) *(slash + 1) = '\0';

    databasePath    = std::string(exedir) + "db";
    quarantinePath  = std::string(exedir) + "Quarantine";
    scanLogFile     = std::string(exedir) + "ClamScan.log";
    updateLogFile   = std::string(exedir) + "FreshClam.log";
    iniPath         = defaultIniPath();

    scanRecursive   = true;
    scanArchives    = true;
    scanOle2        = true;
    scanMail        = false;
    infectedAction  = 0;
    maxScanSizeMb   = 150;
    maxFileSizeMb   = 100;
    maxFiles        = 500;
    maxDepth        = 50;

    dbMirror        = "database.clamav.net";
    updateOnStartup = false;
    checkVersion    = true;

    proxyEnabled    = false;
    proxyHost       = "";
    proxyPort       = 3128;
    proxyUser       = "";
    proxyPass       = "";

    emailEnabled    = false;
    emailFrom = emailTo = emailSmtp = "";

    scanScheduled   = false;
    scanHour        = 22;
    scanMinute      = 0;
    scanFrequency   = 0;
    scanDay         = 0;
    scanPath        = "";
    scanDescription = "";
    scanMemory      = false;
    scanRunMissed   = true;
    scanLastRunTime = 0;
    updateScheduled = true;
    updateHour      = 4;
    updateMinute    = 0;
    updateFrequency = 0;
    updateRunMissed = true;
    updateLastRunTime = 0;

    closeOnExit     = false;
    trayNotify      = true;
    priority        = "n";

    includePatterns = "";
    excludePatterns = "*.dbx|CLAMWIN_SEP|*.tbb|CLAMWIN_SEP|*.pst";
}

/* ─── Default INI path ──────────────────────────────────────── */

std::string CWConfig::defaultIniPath()
{
    char profile[MAX_PATH];
    DWORD len = GetEnvironmentVariableA("USERPROFILE", profile, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        return std::string(profile) + "\\.clamwin\\ClamWin.conf";
    }
    /* Fallback: same directory as executable */
    char exedir[MAX_PATH];
    GetModuleFileNameA(NULL, exedir, MAX_PATH);
    char* slash = strrchr(exedir, '\\');
    if (slash) *(slash + 1) = '\0';
    return std::string(exedir) + "ClamWin.conf";
}

/* ─── Load ──────────────────────────────────────────────────── */

bool CWConfig::load(const std::string& path)
{
    defaults();

    if (!path.empty())
        iniPath = path;

    if (GetFileAttributesA(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        return false;  /* no file — defaults already applied */

    databasePath   = getStr(SEC_CLAMAV, "Database",      databasePath);
    scanRecursive  = getInt(SEC_CLAMAV, "ScanRecursive", scanRecursive) != 0;
    scanArchives   = getInt(SEC_CLAMAV, "ScanArchives",  scanArchives)  != 0;
    scanOle2       = getInt(SEC_CLAMAV, "ScanOle2",      scanOle2)      != 0;
    scanMail       = getInt(SEC_CLAMAV, "ScanMail",      scanMail)      != 0;
    infectedAction = getInt(SEC_CLAMAV, "InfectedAction", infectedAction);
    maxScanSizeMb  = getInt(SEC_CLAMAV, "MaxScanSize",   maxScanSizeMb);
    maxFileSizeMb  = getInt(SEC_CLAMAV, "MaxFileSize",   maxFileSizeMb);
    maxFiles       = getInt(SEC_CLAMAV, "MaxFiles",      maxFiles);
    maxDepth       = getInt(SEC_CLAMAV, "MaxRecursion",  maxDepth);
    quarantinePath = getStr(SEC_CLAMAV, "Quarantine",    quarantinePath);
    scanLogFile    = getStr(SEC_CLAMAV, "LogFile",       scanLogFile);
    priority       = getStr(SEC_CLAMAV, "Priority",      priority);

    dbMirror        = getStr(SEC_UPDATES, "DBMirror",        dbMirror);
    updateLogFile   = getStr(SEC_UPDATES, "UpdateLog",       updateLogFile);
    updateOnStartup = getInt(SEC_UPDATES, "UpdateOnStartup", updateOnStartup) != 0;
    checkVersion    = getInt(SEC_UPDATES, "CheckVersion",    checkVersion)    != 0;

    proxyEnabled = getInt(SEC_PROXY, "Enabled",  proxyEnabled) != 0;
    proxyHost    = getStr(SEC_PROXY, "Host",     proxyHost);
    proxyPort    = getInt(SEC_PROXY, "Port",     proxyPort);
    proxyUser    = getStr(SEC_PROXY, "User",     proxyUser);
    proxyPass    = getStr(SEC_PROXY, "Password", proxyPass);

    emailEnabled = getInt(SEC_ALERTS, "Enabled", emailEnabled) != 0;
    emailFrom    = getStr(SEC_ALERTS, "From",    emailFrom);
    emailTo      = getStr(SEC_ALERTS, "To",      emailTo);
    emailSmtp    = getStr(SEC_ALERTS, "SMTP",    emailSmtp);

    scanScheduled   = getInt(SEC_SCHEDULE, "ScanEnabled",    scanScheduled)   != 0;
    scanHour        = getInt(SEC_SCHEDULE, "ScanHour",       scanHour);
    scanMinute      = getInt(SEC_SCHEDULE, "ScanMinute",     scanMinute);
    scanFrequency   = getInt(SEC_SCHEDULE, "ScanFrequency",  scanFrequency);
    scanDay         = getInt(SEC_SCHEDULE, "ScanDay",        scanDay);
    scanPath        = getStr(SEC_SCHEDULE, "ScanPath",        scanPath);
    scanDescription = getStr(SEC_SCHEDULE, "ScanDescription", scanDescription);
    scanMemory      = getInt(SEC_SCHEDULE, "ScanMemory",      scanMemory) != 0;
    scanRunMissed   = getInt(SEC_SCHEDULE, "RunMissed",       scanRunMissed) != 0;
    scanLastRunTime = getInt64(SEC_SCHEDULE, "ScanLastRun",   scanLastRunTime);
    updateScheduled = getInt(SEC_SCHEDULE, "UpdateEnabled",  updateScheduled) != 0;
    updateHour      = getInt(SEC_SCHEDULE, "UpdateHour",     updateHour);
    updateMinute    = getInt(SEC_SCHEDULE, "UpdateMinute",   updateMinute);
    updateFrequency = getInt(SEC_SCHEDULE, "UpdateFrequency",updateFrequency);
    updateRunMissed = getInt(SEC_SCHEDULE, "UpdateRunMissed",updateRunMissed) != 0;
    updateLastRunTime = getInt64(SEC_SCHEDULE,"UpdateLastRun",updateLastRunTime);

    closeOnExit     = getInt(SEC_UI, "CloseOnExit", closeOnExit)  != 0;
    trayNotify      = getInt(SEC_UI, "TrayNotify",  trayNotify)   != 0;

    /* Preserve backward compatibility with INI files that contain empty log path values. */
    if (scanLogFile.empty())
    {
        char exedir[MAX_PATH];
        GetModuleFileNameA(NULL, exedir, MAX_PATH);
        char* slash = strrchr(exedir, '\\');
        if (slash)
            *(slash + 1) = '\0';
        scanLogFile = std::string(exedir) + "ClamScan.log";
    }
    if (updateLogFile.empty())
    {
        char exedir[MAX_PATH];
        GetModuleFileNameA(NULL, exedir, MAX_PATH);
        char* slash = strrchr(exedir, '\\');
        if (slash)
            *(slash + 1) = '\0';
        updateLogFile = std::string(exedir) + "FreshClam.log";
    }

    includePatterns = getStr(SEC_CLAMAV, "IncludePatterns", includePatterns);
    excludePatterns = getStr(SEC_CLAMAV, "ExcludePatterns", excludePatterns);
    includePatterns = normalizeFilterPatterns(includePatterns);
    excludePatterns = normalizeFilterPatterns(excludePatterns);

    return true;
}

/* ─── Save ──────────────────────────────────────────────────── */

bool CWConfig::save() const
{
    /* Create directory if needed */
    std::string dir = iniPath;
    size_t slash = dir.rfind('\\');
    if (slash != std::string::npos)
    {
        dir.resize(slash);
        CreateDirectoryA(dir.c_str(), NULL);
    }

    setStr(SEC_CLAMAV, "Database",       databasePath);
    setInt(SEC_CLAMAV, "ScanRecursive",  scanRecursive  ? 1 : 0);
    setInt(SEC_CLAMAV, "ScanArchives",   scanArchives   ? 1 : 0);
    setInt(SEC_CLAMAV, "ScanOle2",       scanOle2       ? 1 : 0);
    setInt(SEC_CLAMAV, "ScanMail",       scanMail       ? 1 : 0);
    setInt(SEC_CLAMAV, "InfectedAction", infectedAction);
    setInt(SEC_CLAMAV, "MaxScanSize",    maxScanSizeMb);
    setInt(SEC_CLAMAV, "MaxFileSize",    maxFileSizeMb);
    setInt(SEC_CLAMAV, "MaxFiles",       maxFiles);
    setInt(SEC_CLAMAV, "MaxRecursion",   maxDepth);
    setStr(SEC_CLAMAV, "Quarantine",     quarantinePath);
    setStr(SEC_CLAMAV, "LogFile",        scanLogFile);
    setStr(SEC_CLAMAV, "Priority",       priority);

    setStr(SEC_UPDATES, "DBMirror",         dbMirror);
    setStr(SEC_UPDATES, "UpdateLog",        updateLogFile);
    setInt(SEC_UPDATES, "UpdateOnStartup",  updateOnStartup ? 1 : 0);
    setInt(SEC_UPDATES, "CheckVersion",     checkVersion    ? 1 : 0);

    setInt(SEC_PROXY, "Enabled",  proxyEnabled ? 1 : 0);
    setStr(SEC_PROXY, "Host",     proxyHost);
    setInt(SEC_PROXY, "Port",     proxyPort);
    setStr(SEC_PROXY, "User",     proxyUser);
    setStr(SEC_PROXY, "Password", proxyPass);

    setInt(SEC_ALERTS, "Enabled", emailEnabled ? 1 : 0);
    setStr(SEC_ALERTS, "From",    emailFrom);
    setStr(SEC_ALERTS, "To",      emailTo);
    setStr(SEC_ALERTS, "SMTP",    emailSmtp);

    setInt(SEC_SCHEDULE, "ScanEnabled",    scanScheduled   ? 1 : 0);
    setInt(SEC_SCHEDULE, "ScanHour",       scanHour);
    setInt(SEC_SCHEDULE, "ScanMinute",     scanMinute);
    setInt(SEC_SCHEDULE, "ScanFrequency",  scanFrequency);
    setInt(SEC_SCHEDULE, "ScanDay",        scanDay);
    setStr(SEC_SCHEDULE, "ScanPath",        scanPath);
    setStr(SEC_SCHEDULE, "ScanDescription", scanDescription);
    setInt(SEC_SCHEDULE, "ScanMemory",      scanMemory ? 1 : 0);
    setInt(SEC_SCHEDULE, "RunMissed",       scanRunMissed ? 1 : 0);
    setInt64(SEC_SCHEDULE, "ScanLastRun",   scanLastRunTime);
    setInt(SEC_SCHEDULE, "UpdateEnabled",  updateScheduled ? 1 : 0);
    setInt(SEC_SCHEDULE, "UpdateHour",     updateHour);
    setInt(SEC_SCHEDULE, "UpdateMinute",   updateMinute);
    setInt(SEC_SCHEDULE, "UpdateFrequency",updateFrequency);
    setInt(SEC_SCHEDULE, "UpdateRunMissed",updateRunMissed ? 1 : 0);
    setInt64(SEC_SCHEDULE, "UpdateLastRun",updateLastRunTime);

    setInt(SEC_UI, "CloseOnExit", closeOnExit ? 1 : 0);
    setInt(SEC_UI, "TrayNotify",  trayNotify  ? 1 : 0);
    setStr(SEC_CLAMAV, "IncludePatterns", includePatterns);
    setStr(SEC_CLAMAV, "ExcludePatterns", excludePatterns);

    return true;
}

/* ─── Private helpers ────────────────────────────────────────── */

std::string CWConfig::getStr(const char* sec, const char* key,
                              const std::string& def) const
{
    char buf[1024];
    GetPrivateProfileStringA(sec, key, def.c_str(),
                              buf, sizeof(buf), iniPath.c_str());
    return std::string(buf);
}

int CWConfig::getInt(const char* sec, const char* key, int def) const
{
    return GetPrivateProfileIntA(sec, key, def, iniPath.c_str());
}

void CWConfig::setStr(const char* sec, const char* key,
                       const std::string& val) const
{
    WritePrivateProfileStringA(sec, key, val.c_str(), iniPath.c_str());
}

void CWConfig::setInt(const char* sec, const char* key, int val) const
{
    char buf[32];
    wsprintfA(buf, "%d", val);
    WritePrivateProfileStringA(sec, key, buf, iniPath.c_str());
}

long long CWConfig::getInt64(const char* sec, const char* key, long long def) const
{
    std::string valStr = getStr(sec, key, "");
    if (valStr.empty()) return def;
    return _atoi64(valStr.c_str());
}

void CWConfig::setInt64(const char* sec, const char* key, long long val) const
{
    char buf[64];
    _snprintf(buf, sizeof(buf), "%I64d", val);
    WritePrivateProfileStringA(sec, key, buf, iniPath.c_str());
}

/*
 * ClamWin Free Antivirus — CWConfig
 *
 * Loads and saves ClamWin.conf INI files, backward-compatible
 * with the legacy Python ConfigParser format.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>
#include <string>

class CWConfig
{
public:
    /* Scan options */
    bool scanRecursive;
    bool scanArchives;
    bool scanOle2;
    bool scanMail;
    int  infectedAction;   /* 0=report, 1=remove, 2=quarantine */
    int  maxScanSizeMb;
    int  maxFileSizeMb;
    int  maxFiles;
    int  maxDepth;

    /* Paths */
    std::string databasePath;
    std::string quarantinePath;
    std::string scanLogFile;
    std::string updateLogFile;
    std::string iniPath;

    /* Updates */
    std::string dbMirror;
    bool updateOnStartup;
    bool checkVersion;     /* notify about new ClamWin releases */

    /* Proxy */
    bool        proxyEnabled;
    std::string proxyHost;
    int         proxyPort;
    std::string proxyUser;
    std::string proxyPass;

    /* Email alerts */
    bool        emailEnabled;
    std::string emailFrom;
    std::string emailTo;
    std::string emailSmtp;

    /* Schedule */
    bool scanScheduled;
    int  scanHour;
    int  scanMinute;
    int  scanFrequency;   /* 0=daily 1=workdays 2=weekly 3=hourly */
    int  scanDay;         /* 0=Monday … 6=Sunday */
    std::string scanPath;
    std::string scanDescription;
    bool scanMemory;
    bool scanRunMissed;
    long long scanLastRunTime;
    bool updateScheduled;
    int  updateHour;
    int  updateMinute;
    int  updateFrequency;
    bool updateRunMissed;
    long long updateLastRunTime;

    /* Filters */
    std::string includePatterns;  /* |CLAMWIN_SEP|-delimited wildcard patterns */
    std::string excludePatterns;  /* |CLAMWIN_SEP|-delimited wildcard patterns */

    /* UI */
    bool        closeOnExit;
    bool        trayNotify;  /* show balloon tips on scan/update completion */
    std::string priority;   /* "l"=low, "n"=normal */

    CWConfig();

    /* Load from file. Returns true on success, false = file not found (defaults used). */
    bool load(const std::string& iniPath = "");

    /* Save to current iniPath. Returns true on success. */
    bool save() const;

    /* Reset all fields to defaults. */
    void defaults();

    /* Returns the default INI path (%USERPROFILE%\.clamwin\ClamWin.conf) */
    static std::string defaultIniPath();

private:
    std::string getStr(const char* section, const char* key,
                       const std::string& def) const;
    int         getInt(const char* section, const char* key, int def) const;
    void        setStr(const char* section, const char* key,
                       const std::string& val) const;
    void        setInt(const char* section, const char* key, int val) const;
    long long   getInt64(const char* section, const char* key, long long def) const;
    void        setInt64(const char* section, const char* key, long long val) const;
};

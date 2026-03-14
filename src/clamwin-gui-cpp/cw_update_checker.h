/*
 * ClamWin Free Antivirus — CWUpdateChecker
 *
 * Checks GitHub Releases for a newer ClamWin version via HTTPS.
 * Runs on a background thread, posts WM_CW_VERSION_RESULT to the
 * target HWND when done.  The download URL is hardcoded — the API
 * response only provides the version number.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>

/* Custom message posted to the target HWND when the check completes.
 * WPARAM = 1 if a newer version is available, 0 if not (or on error).
 * LPARAM = pointer to a heap-allocated CWVersionResult (receiver must delete). */
#define WM_CW_VERSION_RESULT  (WM_USER + 200)

struct CWVersionResult
{
    bool  available;           /* true = newer version exists */
    int   major, minor, patch; /* remote version components   */
    char  versionStr[64];      /* e.g. "1.2.0"               */
};

class CWUpdateChecker
{
public:
    CWUpdateChecker();
    ~CWUpdateChecker();

    /* Hardcoded constants — change the repo slug here, once. */
    static const char* apiUrl();       /* GitHub Releases API endpoint */
    static const char* downloadUrl();  /* Always clamwin.com           */

    /* Launch a background HTTPS check.  When complete a WM_CW_VERSION_RESULT
     * message is posted to |hwndTarget|.  Only one check runs at a time;
     * calling again while one is in flight is a no-op. */
    void startCheck(HWND hwndTarget);

    /* Block until the background thread finishes (used during shutdown). */
    void waitForThread();

    /* Compare a version string (e.g. "1.2.0") against CLAMWIN_VERSION_STR.
     * Returns true if |remote| is strictly newer. Pure logic, no I/O. */
    static bool isNewerVersion(const char* remote);

    /* Parse "1.2.3" into components. Returns false on bad input. */
    static bool parseVersion(const char* str, int& major, int& minor, int& patch);

private:
    CWUpdateChecker(const CWUpdateChecker&);
    CWUpdateChecker& operator=(const CWUpdateChecker&);

    HANDLE m_hThread;
    HWND   m_hwndTarget;

    static DWORD WINAPI threadProc(LPVOID param);
    void doCheck();
};

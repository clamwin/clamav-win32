/*
 * ClamWin Free Antivirus — CWProcess
 *
 * RAII wrapper for spawning clamscan.exe / freshclam.exe with
 * stdout/stderr pipe capture via a reader thread.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cwdefs.h"
#include <windows.h>
#include <string>

/* Callback types — plain function pointers for Win98 compat */
typedef void (*CW_OutputCb)(const char *line, void *userdata);
typedef void (*CW_FinishedCb)(int exitCode, void *userdata);

class CWProcess
{
public:
    CWProcess();
    ~CWProcess();  /* RAII: calls stop() */

    /* Start a process. cmdline is copied internally.
     * priority: "l" = idle, "n" = normal.
     * Returns true on success. */
    bool start(const std::string& cmdline,
               const std::string& priority,
               CW_OutputCb   onOutput,
               CW_OutputCb   onError,
               CW_FinishedCb onFinished,
               void*         userdata);

    /* Terminate process and wait for reader thread to exit. */
    void stop();

    /* Returns true if the process is still running. */
    bool isRunning() const;

private:
    /* No copy */
    CWProcess(const CWProcess&);
    CWProcess& operator=(const CWProcess&);

    struct Ctx {
        HANDLE        hProcess;
        HANDLE        hStdoutRead;
        HANDLE        hStderrRead;
        CW_OutputCb   onOutput;
        CW_OutputCb   onError;
        CW_FinishedCb onFinished;
        void*         userdata;
        volatile int  stopping;
    };

    Ctx*   m_ctx;
    HANDLE m_hThread;

    static DWORD WINAPI readerThread(LPVOID param);
};

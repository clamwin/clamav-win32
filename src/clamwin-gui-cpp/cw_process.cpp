/*
 * ClamWin Free Antivirus — CWProcess implementation
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_process.h"
#include <string.h>
#include <stdlib.h>

namespace
{
void emitBufferedLine(CW_OutputCb cb, void* userdata, const std::string& line)
{
    if (!cb || line.empty())
        return;

    std::string out = line;
    out += "\r\n";
    cb(out.c_str(), userdata);
}

void processPipeChunk(const char* data, DWORD len,
                      std::string& pending,
                      CW_OutputCb cb,
                      void* userdata)
{
    if (!data || len == 0)
        return;

    pending.append(data, (size_t)len);

    size_t lineStart = 0;
    for (size_t i = 0; i < pending.size(); ++i)
    {
        char c = pending[i];
        if (c != '\r' && c != '\n')
            continue;

        if (i > lineStart)
            emitBufferedLine(cb, userdata, pending.substr(lineStart, i - lineStart));

        if (c == '\r' && (i + 1) < pending.size() && pending[i + 1] == '\n')
            ++i;

        lineStart = i + 1;
    }

    if (lineStart > 0)
        pending.erase(0, lineStart);

    const size_t kMaxPendingBytes = 8192;
    if (pending.size() > kMaxPendingBytes)
    {
        emitBufferedLine(cb, userdata, pending);
        pending.clear();
    }
}

void flushPipeBuffer(std::string& pending, CW_OutputCb cb, void* userdata)
{
    if (pending.empty())
        return;

    emitBufferedLine(cb, userdata, pending);
    pending.clear();
}
}

CWProcess::CWProcess()
    : m_ctx(NULL)
    , m_hThread(NULL)
{
}

CWProcess::~CWProcess()
{
    stop();
}

bool CWProcess::start(const std::string& cmdline,
                      const std::string& priority,
                      CW_OutputCb   onOutput,
                      CW_OutputCb   onError,
                      CW_FinishedCb onFinished,
                      void*         userdata)
{
    stop();  /* clean up any previous process */

    SECURITY_ATTRIBUTES sa;
    HANDLE hStdoutRead = NULL, hStdoutWrite = NULL;
    HANDLE hStderrRead = NULL, hStderrWrite = NULL;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0))
        return false;

    /* Make read end non-inheritable */
    {
        HANDLE hTmp;
        BOOL ok = DuplicateHandle(GetCurrentProcess(), hStdoutRead,
                                   GetCurrentProcess(), &hTmp,
                                   0, FALSE, DUPLICATE_SAME_ACCESS);
        CloseHandle(hStdoutRead);
        if (!ok) { CloseHandle(hStdoutWrite); return false; }
        hStdoutRead = hTmp;
    }

    if (!CreatePipe(&hStderrRead, &hStderrWrite, &sa, 0))
    {
        CloseHandle(hStdoutRead); CloseHandle(hStdoutWrite);
        return false;
    }

    {
        HANDLE hTmp;
        BOOL ok = DuplicateHandle(GetCurrentProcess(), hStderrRead,
                                   GetCurrentProcess(), &hTmp,
                                   0, FALSE, DUPLICATE_SAME_ACCESS);
        CloseHandle(hStderrRead);
        if (!ok)
        {
            CloseHandle(hStdoutRead); CloseHandle(hStdoutWrite);
            CloseHandle(hStderrWrite);
            return false;
        }
        hStderrRead = hTmp;
    }

    /* Set up startup info */
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput  = hStdoutWrite;
    si.hStdError   = hStderrWrite;
    si.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);

    /* CreateProcess needs a mutable command line */
    std::string cmdCopy = cmdline;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL created = CreateProcessA(NULL,
                                   const_cast<char*>(cmdCopy.c_str()),
                                   NULL, NULL, TRUE,
                                   CREATE_NO_WINDOW, NULL, NULL,
                                   &si, &pi);

    /* Close write ends — child has them */
    CloseHandle(hStdoutWrite);
    CloseHandle(hStderrWrite);

    if (!created)
    {
        CloseHandle(hStdoutRead);
        CloseHandle(hStderrRead);
        return false;
    }

    CloseHandle(pi.hThread);

    /* Apply priority */
    if (!priority.empty() && priority[0] == 'l')
        SetPriorityClass(pi.hProcess, IDLE_PRIORITY_CLASS);

    /* Build context */
    m_ctx = new Ctx;
    m_ctx->hProcess    = pi.hProcess;
    m_ctx->hStdoutRead = hStdoutRead;
    m_ctx->hStderrRead = hStderrRead;
    m_ctx->onOutput    = onOutput;
    m_ctx->onError     = onError;
    m_ctx->onFinished  = onFinished;
    m_ctx->userdata    = userdata;
    m_ctx->stopping    = 0;

    m_hThread = CreateThread(NULL, 0, readerThread, m_ctx, 0, NULL);
    if (!m_hThread)
    {
        TerminateProcess(m_ctx->hProcess, 1);
        CloseHandle(m_ctx->hProcess);
        CloseHandle(m_ctx->hStdoutRead);
        CloseHandle(m_ctx->hStderrRead);
        delete m_ctx;
        m_ctx = NULL;
        return false;
    }

    return true;
}

void CWProcess::stop()
{
    if (!m_ctx) return;

    m_ctx->stopping = 1;
    TerminateProcess(m_ctx->hProcess, 1);

    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    /* Reader thread closes handles; just free ctx */
    delete m_ctx;
    m_ctx = NULL;
}

bool CWProcess::isRunning() const
{
    if (!m_ctx || !m_ctx->hProcess) return false;
    return (WaitForSingleObject(m_ctx->hProcess, 0) == WAIT_TIMEOUT);
}

/* ─── Reader thread ─────────────────────────────────────────── */

DWORD WINAPI CWProcess::readerThread(LPVOID param)
{
    Ctx* ctx = static_cast<Ctx*>(param);
    char buf[4096];
    DWORD bytesRead;
    std::string pendingStdout;
    std::string pendingStderr;

    while (!ctx->stopping)
    {
        /* Read stdout */
        while (PeekNamedPipe(ctx->hStdoutRead, NULL, 0, NULL, &bytesRead, NULL)
               && bytesRead > 0)
        {
            DWORD toRead = (bytesRead < sizeof(buf) - 1)
                            ? bytesRead : sizeof(buf) - 1;
            if (ReadFile(ctx->hStdoutRead, buf, toRead, &bytesRead, NULL)
                && bytesRead > 0)
            {
                processPipeChunk(buf, bytesRead,
                                 pendingStdout,
                                 ctx->onOutput,
                                 ctx->userdata);
            }
        }

        /* Read stderr */
        while (PeekNamedPipe(ctx->hStderrRead, NULL, 0, NULL, &bytesRead, NULL)
               && bytesRead > 0)
        {
            DWORD toRead = (bytesRead < sizeof(buf) - 1)
                            ? bytesRead : sizeof(buf) - 1;
            if (ReadFile(ctx->hStderrRead, buf, toRead, &bytesRead, NULL)
                && bytesRead > 0)
            {
                processPipeChunk(buf, bytesRead,
                                 pendingStderr,
                                 ctx->onError,
                                 ctx->userdata);
            }
        }

        if (WaitForSingleObject(ctx->hProcess, 50) != WAIT_TIMEOUT)
            break;
    }

    /* Drain remaining output */
    while (ReadFile(ctx->hStdoutRead, buf, sizeof(buf) - 1, &bytesRead, NULL)
           && bytesRead > 0)
    {
        processPipeChunk(buf, bytesRead,
                         pendingStdout,
                         ctx->onOutput,
                         ctx->userdata);
    }
    while (ReadFile(ctx->hStderrRead, buf, sizeof(buf) - 1, &bytesRead, NULL)
           && bytesRead > 0)
    {
        processPipeChunk(buf, bytesRead,
                         pendingStderr,
                         ctx->onError,
                         ctx->userdata);
    }

    flushPipeBuffer(pendingStdout, ctx->onOutput, ctx->userdata);
    flushPipeBuffer(pendingStderr, ctx->onError, ctx->userdata);

    DWORD exitCode = (DWORD)-1;
    GetExitCodeProcess(ctx->hProcess, &exitCode);

    if (ctx->onFinished) ctx->onFinished((int)exitCode, ctx->userdata);

    CloseHandle(ctx->hStdoutRead);
    CloseHandle(ctx->hStderrRead);
    CloseHandle(ctx->hProcess);

    return 0;
}

/* ─── C-compatible wrappers (called by legacy clamwin_*.cpp) ── */

#include "cw_gui_shared.h"  /* CW_OutputCallback, CW_FinishedCallback */

HANDLE CW_ProcessStart(const char* cmdline, const char* priority,
                        CW_OutputCallback on_output,
                        CW_OutputCallback on_error,
                        CW_FinishedCallback on_finished,
                        void* userdata)
{
    CWProcess* proc = new CWProcess();
    if (!proc->start(cmdline ? std::string(cmdline) : "",
                     priority ? std::string(priority) : "n",
                     on_output, on_error, on_finished, userdata))
    {
        delete proc;
        return NULL;
    }
    return reinterpret_cast<HANDLE>(proc);
}

void CW_ProcessStop(HANDLE hCtx)
{
    if (!hCtx) return;
    CWProcess* proc = reinterpret_cast<CWProcess*>(hCtx);
    proc->stop();
    delete proc;
}

int CW_ProcessIsRunning(HANDLE hCtx)
{
    if (!hCtx) return 0;
    return reinterpret_cast<CWProcess*>(hCtx)->isRunning() ? 1 : 0;
}

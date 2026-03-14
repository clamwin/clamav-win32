/*
 * ClamWin Free Antivirus — CWScanDialog
 *
 * Scan / DB-update progress dialog built on CWDialog.
 * Spawns clamscan.exe or freshclam.exe via CWProcess (RAII).
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#pragma once
#include "cw_dialog.h"
#include "cw_process.h"
#include "cw_gui_shared.h"   /* CW_Config, CW_ScanStats, IDC_* */
#include <string>

class CWScanDialog : public CWDialog
{
public:
    /* mode = false → scan target_path; mode = true → DB update */
    CWScanDialog(CWConfig& cfg, const std::string& targetPath, bool isUpdate, bool scanMemoryOnly);
    virtual ~CWScanDialog();

    /* Returns process exit code (0 = clean, 1 = threats, -1 = error/cancel) */
    int run(HWND parent);

protected:
    /* CWDialog overrides */
    virtual bool onInit();
    virtual bool onCommand(int id, HWND src);
    virtual void onClose();
    virtual INT_PTR handleMessage(UINT msg, WPARAM wp, LPARAM lp);

private:
    /* No copy */
    CWScanDialog(const CWScanDialog&);
    CWScanDialog& operator=(const CWScanDialog&);

    /* Callbacks from CWProcess reader thread — post to dialog */
    static void outputCb(const char* text, void* userdata);
    static void errorCb (const char* text, void* userdata);
    static void finishedCb(int exitCode, void* userdata);

    /* Worker threads */
    static DWORD WINAPI scanWorker(LPVOID param);
    static DWORD WINAPI updateWorker(LPVOID param);

    void startWorker();
    void onScanOutput(const char* text, bool isError);
    void onScanFinished(int exitCode);
    void updateStatsDisplay();
    void setStatusTextIfChanged(const std::string& text);
    void setScanStatusText(const std::string& baseText, bool animate);
    void refreshScanStatusAnimation();
    std::string fitPathForStatus(const std::string& path, int reservedChars) const;
    void appendLog(const char* text, bool isError);
    void layoutControls(int w, int h);
    void saveReport();

    CWConfig&    m_cfg;
    std::string  m_targetPath;
    bool         m_isUpdate;
    bool         m_scanMemoryOnly;

    /* Process */
    CWProcess    m_process;

    /* Dialog child controls */
    HWND m_hwndStatus;
    HWND m_hwndProgress;
    HWND m_hwndStats;
    HWND m_hwndLog;
    HWND m_hwndBtnStop;
    HWND m_hwndBtnSave;

    /* Fonts — owned, cleaned up in destructor */
    HFONT m_hFont;
    HFONT m_hFontBold;

    /* Scan stats */
    CW_ScanStats m_stats;

    /* State flags */
    volatile int m_finished;
    volatile int m_cancelled;
    int          m_exitCode;

    /* Scan-mode progress state (file scan mode only) */
    int          m_scanExpectedFiles;
    ULONGLONG    m_scanExpectedBytes;
    int          m_scanCompletedFiles;
    ULONGLONG    m_scanScannedBytes;
    int          m_scanUiProgress;
    bool         m_scanDbPhaseSeen;
    bool         m_scanFilePhaseSeen;
    std::string  m_scanStatusBase;
    bool         m_scanAnimate;
    int          m_scanAnimFrame;
    std::string  m_lastStatusText;
    int          m_memoryLoopProgressPos;
    bool         m_memoryUseMarquee;

    /* FreshClam live progress state (update mode only) */
    bool         m_updateProgressKnown;
    double       m_updateDownloadedBytes;
    double       m_updateTotalBytes;
    double       m_updateTransferredBytes;
    int          m_updateUiProgress;
    bool         m_updateHadChanges;
    int          m_updateUpToDateCount;
    std::string  m_updateCurrentDb;
    bool         m_updateBlocked;
    bool         m_updateUnsupportedVersion;
    bool         m_updateServerError;
    bool         m_updateToolMissing;
    std::string  m_updateLastErrorLine;

    /* Worker thread handle (for cleanup) */
    HANDLE m_hWorker;

    /* Mnemonic cue visibility (Alt-key tracking) */
    bool m_showMnemonics;
};

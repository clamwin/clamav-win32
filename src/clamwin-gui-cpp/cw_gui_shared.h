/*
 * ClamWin Free Antivirus - Shared Win32 GUI definitions
 *
 * Shared resource IDs, message constants, lightweight structs, and
 * compatibility entry points used across the class-based frontend.
 */

#ifndef CW_GUI_SHARED_H
#define CW_GUI_SHARED_H

#include "cwdefs.h"

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <richedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cw_build_version.h"

#ifndef CLAMWIN_VERSION_STR
#define CLAMWIN_VERSION_STR     "1.5.1.0"
#endif
#define CLAMWIN_APPNAME         "ClamWin Free Antivirus"
#define CLAMWIN_APPNAME_W       L"ClamWin Free Antivirus"
#define CLAMWIN_WEBSITE         "http://www.clamwin.com"
#define CLAMWIN_CLASS_MAIN      "ClamWinMainWnd"
#define CLAMWIN_CLASS_MAIN_W    L"ClamWinMainWnd"
#define CLAMWIN_MUTEX_NAME      L"ClamWinGUI_SingleInstance"

#define IDI_CLAMWIN             100
#define IDI_TRAY_OK             101
#define IDI_TRAY_WARN           102
#define IDI_TRAY_ERROR          103

#define IDB_CLAMAV_LOGO         200
#define IDB_CLAMWIN_LOGO        201
#define IDB_NETFARM_LOGO        202

#define IDM_TRAY_SCAN           1001
#define IDM_TRAY_SCANMEM        1002
#define IDM_TRAY_UPDATE         1003
#define IDM_TRAY_SCANREPORT     1004
#define IDM_TRAY_UPDATEREPORT   1005
#define IDM_TRAY_PREFS          1006
#define IDM_TRAY_SCHEDULE       1007
#define IDM_TRAY_ABOUT          1008
#define IDM_TRAY_EXIT           1009
#define IDM_TRAY_OPEN           1010
#define IDM_TRAY_REPORTS        1011
#define IDM_TRAY_SCHEDULED_SCAN 1012
#define IDM_TRAY_SCHEDULED_UPDATE 1013
#define IDM_TRAY_HELP           1014

#define IDC_CARD_SCAN           2001
#define IDC_CARD_UPDATE         2002
#define IDC_CARD_SCANMEM        2003
#define IDC_CARD_REPORTS        2004
#define IDC_CARD_PREFS          2005
#define IDC_CARD_SCHEDULE       2006
#define IDC_STATUS_BANNER       2007
#define IDC_CARD_HELP           2008
#define IDC_CARD_ABOUT          2009

#define IDC_SCAN_PROGRESS       3001
#define IDC_SCAN_LOG            3002
#define IDC_SCAN_STATUS         3003
#define IDC_SCAN_STATS          3004
#define IDC_SCAN_STOP           3005
#define IDC_SCAN_SAVE           3006

#define WM_TRAYICON             (WM_USER + 100)
#define WM_SCAN_UPDATE          (WM_USER + 101)
#define WM_SCAN_FINISHED        (WM_USER + 102)

#define CW_INI_SECTION_CLAMAV   "ClamAV"
#define CW_INI_SECTION_UI       "UI"
#define CW_INI_SECTION_PROXY    "Proxy"
#define CW_INI_SECTION_UPDATES  "Updates"
#define CW_INI_SECTION_SCHEDULE "Schedule"
#define CW_INI_SECTION_ALERTS   "EmailAlerts"

typedef enum {
    CW_STATUS_OK = 0,
    CW_STATUS_WARN,
    CW_STATUS_ERROR
} CW_ProtectionStatus;

#define CW_MAX_PATH 1024

#include "cw_config.h"

typedef struct {
    int  main_ver;
    int  main_sigs;
    int  daily_ver;
    int  daily_sigs;
    long updated_time;
    int  total_sigs;
} CW_DBInfo;

typedef struct {
    int  files_scanned;
    int  threats_found;
    int  errors;
    DWORD start_tick;
    char current_file[CW_MAX_PATH];
} CW_ScanStats;

typedef struct {
    char text[4096];
    int  is_error;
} OutputMsg;

int  CW_TrayCreate(HWND hwnd, HICON hIcon);
void CW_TrayDestroy(HWND hwnd);
void CW_TraySetIcon(HWND hwnd, HICON hIcon, const WCHAR *tooltip);
void CW_TrayShowContextMenu(HWND hwnd);


int  CW_ScanDialogRun(HWND hwndParent, CWConfig *cfg, const char *target_path);
int  CW_ScanMemoryDialogRun(HWND hwndParent, CWConfig *cfg);
int  CW_UpdateDialogRun(HWND hwndParent, CWConfig *cfg);

/* Update dialog return codes for caller-side notification behavior. */
#define CW_UPDATE_RC_SUCCESS      0
#define CW_UPDATE_RC_CANCELLED   -1
#define CW_UPDATE_RC_NO_CHANGES   2

int  CW_GetDBInfo(const char *db_path, CW_DBInfo *info);
CW_ProtectionStatus CW_GetProtectionStatus(const CWConfig *cfg);

void CW_AboutDialogRun(HWND hwndParent, CWConfig *cfg);
void CW_LogViewerRun(HWND hwndParent, const char *logfile, const char *title);
int  CW_PrefsDialogRun(HWND hwndParent, CWConfig *cfg);
int  CW_ScheduleDialogRun(HWND hwndParent, CWConfig *cfg);
void CW_SchedulerStart(HWND hwnd, CWConfig *cfg);
void CW_SchedulerStop(void);
void CW_SchedulerCheck(void);

typedef void (*CW_OutputCallback)(const char *text, void *userdata);
typedef void (*CW_FinishedCallback)(int exit_code, void *userdata);

HANDLE CW_ProcessStart(const char *cmdline, const char *priority,
                        CW_OutputCallback on_output,
                        CW_OutputCallback on_error,
                        CW_FinishedCallback on_finished,
                        void *userdata);
void CW_ProcessStop(HANDLE hCtx);
int  CW_ProcessIsRunning(HANDLE hCtx);

#endif /* CW_GUI_SHARED_H */
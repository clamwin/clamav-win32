/*
 * ClamWin Free Antivirus — CWUpdateChecker implementation
 *
 * Uses WinINet (InternetOpenA / HttpOpenRequestA) for HTTPS GET to
 * the GitHub Releases API.  Parses the "tag_name" field from the
 * JSON response for a semantic version comparison.
 *
 * Copyright (c) 2004-2026 ClamWin Pty Ltd
 * License: GPLv2
 */

#include "cw_update_checker.h"
#include "cw_gui_shared.h"   /* CLAMWIN_VERSION_STR */

#include <wininet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "wininet.lib")

/* ─── Hardcoded endpoints ───────────────────────────────────── */

static const char* CW_GITHUB_API_HOST = "api.github.com";
static const char* CW_GITHUB_API_PATH = "/repos/clamwin/clamwin/releases/latest";
static const char* CW_DOWNLOAD_PAGE   = "https://www.clamwin.com/download";

/* GitHub API requires modern TLS; skip check on legacy OS families (Win9x/XP). */
static bool canUseGithubTlsOnThisOs()
{
    OSVERSIONINFOA osv;
    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);

    if (!GetVersionExA(&osv))
        return true; /* If detection fails, keep behavior unchanged. */

    return osv.dwMajorVersion >= 6;
}

const char* CWUpdateChecker::apiUrl()
{
    return "https://api.github.com/repos/clamwin/clamwin/releases/latest";
}

const char* CWUpdateChecker::downloadUrl()
{
    return CW_DOWNLOAD_PAGE;
}

/* ─── Version parsing / comparison ──────────────────────────── */

bool CWUpdateChecker::parseVersion(const char* str, int& major, int& minor, int& patch)
{
    if (!str)
        return false;

    /* Skip leading 'v' or 'V' (common in Git tags like "v1.2.3") */
    if (*str == 'v' || *str == 'V')
        ++str;

    /* Skip legacy epoch prefix like "0:" */
    const char* colon = strchr(str, ':');
    if (colon)
        str = colon + 1;

    major = minor = patch = 0;
    int n = sscanf(str, "%d.%d.%d", &major, &minor, &patch);
    return n >= 2;  /* at least major.minor */
}

bool CWUpdateChecker::isNewerVersion(const char* remote)
{
    int rMaj = 0, rMin = 0, rPat = 0;
    int lMaj = 0, lMin = 0, lPat = 0;

    if (!parseVersion(remote, rMaj, rMin, rPat))
        return false;
    if (!parseVersion(CLAMWIN_VERSION_STR, lMaj, lMin, lPat))
        return false;

    if (rMaj != lMaj) return rMaj > lMaj;
    if (rMin != lMin) return rMin > lMin;
    return rPat > lPat;
}

/* ─── Constructor / Destructor ──────────────────────────────── */

CWUpdateChecker::CWUpdateChecker()
    : m_hThread(NULL)
    , m_hwndTarget(NULL)
{
}

CWUpdateChecker::~CWUpdateChecker()
{
    waitForThread();
}

void CWUpdateChecker::waitForThread()
{
    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

/* ─── Start background check ───────────────────────────────── */

void CWUpdateChecker::startCheck(HWND hwndTarget)
{
    if (m_hThread)
        return;   /* already in flight */

    m_hwndTarget = hwndTarget;
    m_hThread = CreateThread(NULL, 0, threadProc, this, 0, NULL);
}

DWORD WINAPI CWUpdateChecker::threadProc(LPVOID param)
{
    CWUpdateChecker* self = static_cast<CWUpdateChecker*>(param);
    if (self)
        self->doCheck();
    return 0;
}

/* ─── Minimal JSON extraction ───────────────────────────────── *
 * We only need the "tag_name" value from the GitHub response.
 * A full JSON parser is overkill — we search for the key and
 * extract the quoted string value that follows it.
 */

static bool extractTagName(const char* json, int jsonLen, char* out, int outCap)
{
    if (!json || jsonLen <= 0 || !out || outCap <= 0)
        return false;

    const char* key = "\"tag_name\"";
    const char* pos = NULL;

    /* Search safely within the known length */
    for (int i = 0; i <= jsonLen - 10; ++i)
    {
        if (strncmp(json + i, key, 10) == 0)
        {
            pos = json + i + 10;
            break;
        }
    }
    if (!pos)
        return false;

    /* Skip whitespace and colon */
    const char* end = json + jsonLen;
    while (pos < end && (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\n' || *pos == '\r'))
        ++pos;

    if (pos >= end || *pos != '"')
        return false;
    ++pos;  /* skip opening quote */

    int i = 0;
    while (pos < end && *pos != '"' && i < outCap - 1)
    {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return i > 0;
}

/* ─── The actual HTTPS check ────────────────────────────────── */

void CWUpdateChecker::doCheck()
{
    CWVersionResult* result = new CWVersionResult();
    memset(result, 0, sizeof(*result));

    HINTERNET hInet = NULL;
    HINTERNET hConn = NULL;
    HINTERNET hReq  = NULL;

    do
    {
        if (!canUseGithubTlsOnThisOs())
            break;

        hInet = InternetOpenA("ClamWin/" CLAMWIN_VERSION_STR,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL, NULL, 0);
        if (!hInet)
            break;

        /* Set a reasonable timeout so we don't block forever */
        DWORD timeout = 15000;
        InternetSetOptionA(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        hConn = InternetConnectA(hInet, CW_GITHUB_API_HOST,
                                 INTERNET_DEFAULT_HTTPS_PORT,
                                 NULL, NULL,
                                 INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConn)
            break;

        const char* acceptTypes[] = { "application/json", NULL };
        hReq = HttpOpenRequestA(hConn, "GET", CW_GITHUB_API_PATH,
                                NULL, NULL, acceptTypes,
                                INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE |
                                INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_UI,
                                0);
        if (!hReq)
            break;

        if (!HttpSendRequestA(hReq, NULL, 0, NULL, 0))
            break;

        /* Check HTTP status code */
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (!HttpQueryInfoA(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                            &statusCode, &statusSize, NULL))
            break;
        if (statusCode != 200)
            break;

        /* Read response body (capped at 32 KB — the real response is ~5 KB) */
        const int MAX_RESPONSE = 32768;
        char* body = (char*)malloc(MAX_RESPONSE + 1);
        if (!body)
            break;

        int totalRead = 0;
        DWORD bytesRead = 0;
        while (totalRead < MAX_RESPONSE)
        {
            if (!InternetReadFile(hReq, body + totalRead,
                                  MAX_RESPONSE - totalRead, &bytesRead))
                break;
            if (bytesRead == 0)
                break;
            totalRead += (int)bytesRead;
        }
        body[totalRead] = '\0';

        char tagName[64] = "";
        if (extractTagName(body, totalRead, tagName, sizeof(tagName)))
        {
            if (isNewerVersion(tagName))
            {
                result->available = true;
                parseVersion(tagName, result->major, result->minor, result->patch);
                _snprintf(result->versionStr, sizeof(result->versionStr), "%s", tagName);
                result->versionStr[sizeof(result->versionStr) - 1] = '\0';
            }
        }

        free(body);

    } while (false);

    if (hReq)  InternetCloseHandle(hReq);
    if (hConn) InternetCloseHandle(hConn);
    if (hInet) InternetCloseHandle(hInet);

    /* Post result to the UI thread */
    if (m_hwndTarget && IsWindow(m_hwndTarget))
    {
        PostMessageA(m_hwndTarget, WM_CW_VERSION_RESULT,
                     (WPARAM)(result->available ? 1 : 0),
                     (LPARAM)result);
    }
    else
    {
        delete result;
    }
}

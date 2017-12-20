/*
 * Clamav Native Windows Port: executables signature check
 *
 * Copyright (c) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <osdeps.h>
#include <others.h>
#include <wincrypt.h>

#include <pshpack8.h>

typedef struct WINTRUST_FILE_INFO_
{
    DWORD cbStruct;
    LPCWSTR pcwszFilePath;
    HANDLE hFile;
    GUID *pgKnownSubject;
} WINTRUST_FILE_INFO, *PWINTRUCT_FILE_INFO;

typedef struct WINTRUST_CATALOG_INFO_
{
    DWORD cbStruct;
    DWORD dwCatalogVersion;
    LPCWSTR pcwszCatalogFilePath;
    LPCWSTR pcwszMemberTag;
    LPCWSTR pcwszMemberFilePath;
    HANDLE hMemberFile;
    BYTE *pbCalculatedFileHash;
    DWORD cbCalculatedFileHash;
    PCCTL_CONTEXT pcCatalogContext;
} WINTRUST_CATALOG_INFO, *PWINTRUST_CATALOG_INFO;

typedef struct _WINTRUST_DATA
{
    DWORD cbStruct;
    LPVOID pPolicyCallbackData;
    LPVOID pSIPClientData;
    DWORD dwUIChoice;
    DWORD fdwRevocationChecks;
    DWORD dwUnionChoice;
    union
    {
        struct WINTRUST_FILE_INFO_ *pFile;
        struct WINTRUST_CATALOG_INFO_ *pCatalog;
        struct WINTRUST_BLOB_INFO_ *pBlob;
        struct WINTRUST_SGNR_INFO_ *pSgnr;
        struct WINTRUST_CERT_INFO_ *pCert;
    };
    DWORD dwStateAction;
    HANDLE hWVTStateData;
    WCHAR *pwszURLReference;
    DWORD dwProvFlags;
    DWORD dwUIContext;
} WINTRUST_DATA, *PWINTRUST_DATA;


#include <poppack.h>

/* MinGW */
#ifndef CERT_QUERY_OBJECT_FILE
typedef struct _CRYPT_ATTRIBUTES
{
    DWORD                cAttr;
    PCRYPT_ATTRIBUTE     rgAttr;
} CRYPT_ATTRIBUTES, *PCRYPT_ATTRIBUTES;

typedef struct _CMSG_SIGNER_INFO
{
    DWORD                       dwVersion;
    CERT_NAME_BLOB              Issuer;
    CRYPT_INTEGER_BLOB          SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER  HashAlgorithm;
    CRYPT_ALGORITHM_IDENTIFIER  HashEncryptionAlgorithm;
    CRYPT_DATA_BLOB             EncryptedHash;
    CRYPT_ATTRIBUTES            AuthAttrs;
    CRYPT_ATTRIBUTES            UnauthAttrs;
} CMSG_SIGNER_INFO, *PCMSG_SIGNER_INFO;

#define CERT_QUERY_OBJECT_FILE                      0x1
#define CERT_NAME_ISSUER_FLAG                       0x1
#define CERT_QUERY_FORMAT_FLAG_BINARY               0x2
#define CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED  0x400
#define CMSG_SIGNER_INFO_PARAM                      0x6
#endif

#define WINTRUST_ACTION_GENERIC_VERIFY_V2 { 0xaac56b, 0xcd44, 0x11d0, { 0x8c, 0xc2, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } }
#define WTD_UI_ALL              1
#define WTD_UI_NONE             2
#define WTD_STATEACTION_VERIFY  1
#define WTD_STATEACTION_CLOSE   2
#define WTD_CHOICE_FILE         1
#define WTD_CHOICE_CATALOG      2

#define WTD_REVOKE_NONE         0
#define WTD_REVOKE_WHOLECHAIN   1

#ifndef TRUST_E_NOSIGNATURE
#define TRUST_E_NOSIGNATURE 0x800B0100L
#endif

static BOOL isIssuerTrusted(wchar_t *filename)
{
    BOOL fResult = FALSE;
    DWORD dwSize;
    DWORD lErr;
    CERT_INFO CertInfo;
    LPSTR szName = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwEncoding, dwContentType, dwFormatType;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;

    if (!cw_helpers.wt.CryptQueryObject(CERT_QUERY_OBJECT_FILE, filename, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY, 0, &dwEncoding, &dwContentType, &dwFormatType, &hStore, &hMsg, NULL))
    {
        if ((lErr = GetLastError()) == CRYPT_E_NO_MATCH)
            cli_dbgmsg("sigcheck: CryptQueryObject() returns CRYPT_E_NO_MATCH\n");
        else
            cli_errmsg("sigcheck: CryptQueryObject() failed: 0x%08x\n", lErr);
        return FALSE;
    }

    if (!cw_helpers.wt.CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSize))
    {
        cli_errmsg("sigcheck: CryptMsgGetParam() for size failed: 0x%08x\n", GetLastError());
        return FALSE;
    }

    if (!(pSignerInfo = (PCMSG_SIGNER_INFO) LocalAlloc(LPTR, dwSize)))
    {
        cli_errmsg("sigcheck: LocalAlloc() pSignerInfo failed: %d\n", GetLastError());
        return FALSE;
    }

    do
    {
        if (!cw_helpers.wt.CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID) pSignerInfo, &dwSize))
        {
            cli_errmsg("sigcheck: CryptMsgGetParam() failed: 0x%08x\n", GetLastError());
            break;
        }

        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        if (!(pCertContext = cw_helpers.wt.CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
            CERT_FIND_SUBJECT_CERT, (PVOID)& CertInfo, NULL)))
        {
            cli_errmsg("sigcheck: CertFindCertificateInStore() failed: 0x%08x\n", GetLastError());
            break;
        }

        if (!(dwSize = cw_helpers.wt.CertGetNameStringA(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0)))
        {
            cli_errmsg("sigcheck: CertGetNameStringA() for size failed: 0x%08x\n", GetLastError());
            break;
        }

        if (!(szName = (LPTSTR) LocalAlloc(LPTR, dwSize)))
        {
            cli_errmsg("sigcheck: LocalAlloc() szName failed: %d\n", GetLastError());
            break;
        }

        if (!cw_helpers.wt.CertGetNameStringA(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, szName, dwSize))
        {
            cli_errmsg("sigcheck: CertGetNameStringA() failed: 0x%08x\n", GetLastError());
            break;
        }

        fResult = strncmp("Microsoft ", szName, sizeof("Microsoft ") - 1) == 0;
        cli_dbgmsg("sigcheck: %s issuer [%s]\n", fResult ? "Trusted" : "Untrusted", szName);

    } while (0);

    if (pSignerInfo)
        LocalFree(pSignerInfo);
    if (szName)
        LocalFree(szName);
    return fResult;
}

static int sigcheck(int fd, const char *virname, int warnfp)
{
    const char *scanning = cw_get_currentfile();
    BOOL TrustIssuer = FALSE;
    LONG lstatus, lsigned = TRUST_E_NOSIGNATURE;
    HANDLE hFile = (HANDLE) _get_osfhandle(fd);
    HCATINFO *phCatInfo = NULL;
    CATALOG_INFO sCatInfo;
    WINTRUST_FILE_INFO wtfi;
    WINTRUST_CATALOG_INFO wtci;
    WINTRUST_DATA wtd;
    BYTE bHash[20];

    int i;
    wchar_t mTag[41];

    wchar_t *filename = NULL;
    GUID pgActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    DWORD cbHash = sizeof(bHash);

    if (!scanning || (GetFileAttributesA(scanning) == INVALID_FILE_ATTRIBUTES))
        return TRUST_E_NOSIGNATURE;

    do
    {
        if (!cw_helpers.wt.CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, bHash, 0))
        {
            cli_dbgmsg("sigcheck: CryptCATAdminCalcHashFromFileHandle() failed: 0x%08x\n", GetLastError());
            break;
        }

        if (!(filename = cw_mb2wc(scanning)))
        {
            cli_errmsg("sigcheck: out of memory for unicode filename\n");
            break;
        }

        for (i = 0; i < sizeof(bHash); i++)
            _snwprintf(&mTag[i * 2], 2, L"%02X", bHash[i]);
        mTag[i * 2] = 0;

        memset(&wtd, 0, sizeof(wtd));
        wtd.cbStruct = sizeof(wtd);
        wtd.dwStateAction = WTD_STATEACTION_VERIFY;
        wtd.fdwRevocationChecks = (isWin9x() || isOldOS()) ? WTD_REVOKE_NONE : WTD_REVOKE_WHOLECHAIN;
        wtd.dwUIChoice = WTD_UI_NONE;

        phCatInfo = cw_helpers.wt.CryptCATAdminEnumCatalogFromHash(cw_helpers.wt.hCatAdmin, bHash, cbHash, 0, NULL);

        if (phCatInfo)
        {
            memset(&sCatInfo, 0, sizeof(sCatInfo));
            sCatInfo.cbStruct = sizeof(sCatInfo);

            lstatus = cw_helpers.wt.CryptCATCatalogInfoFromContext(phCatInfo, &sCatInfo, 0);
            cw_helpers.wt.CryptCATAdminReleaseCatalogContext(cw_helpers.wt.hCatAdmin, phCatInfo, 0);

            if (!lstatus)
            {
                cli_errmsg("sigcheck: CryptCATCatalogInfoFromContext() failed: 0x%08x\n", GetLastError());
                break;
            }

            memset(&wtci, 0, sizeof(wtci));
            wtci.cbStruct = sizeof(wtci);
            wtci.cbCalculatedFileHash = sizeof(bHash);
            wtci.pbCalculatedFileHash = bHash;
            wtci.pcwszMemberTag = mTag;
            wtci.pcwszCatalogFilePath = sCatInfo.wszCatalogFile;
            wtci.pcwszMemberFilePath = filename;

            wtd.dwUnionChoice = WTD_CHOICE_CATALOG;
            wtd.pCatalog = &wtci;
            TrustIssuer = isIssuerTrusted(sCatInfo.wszCatalogFile);
        }
        else
        {
            DWORD err = GetLastError();
            if (err != ERROR_NOT_FOUND)
            {
                cli_dbgmsg("sigcheck: CryptCATAdminEnumCatalogFromHash() failed: 0x%08x\n", GetLastError());
                break;
            }

            cli_dbgmsg("sigcheck: hash not found in catalog trying embedded signature\n");

            memset(&wtfi, 0, sizeof(wtfi));
            wtfi.cbStruct = sizeof(wtfi);
            wtfi.pcwszFilePath = filename;
            wtfi.hFile = hFile;

            wtd.dwUnionChoice = WTD_CHOICE_FILE;
            wtd.pFile = &wtfi;
            TrustIssuer = isIssuerTrusted(filename);
        }

        lsigned = cw_helpers.wt.WinVerifyTrust(0, &pgActionID, (LPVOID) &wtd);
        cli_dbgmsg("sigcheck: WinVerifyTrust 0x%08x\n", lsigned);

        wtd.dwStateAction = WTD_STATEACTION_CLOSE;
        lstatus = cw_helpers.wt.WinVerifyTrust(0, &pgActionID, (LPVOID) &wtd);

    } while (0);

    if (!TrustIssuer)
        lsigned = TRUST_E_NOSIGNATURE;
    else if (warnfp && (lsigned == ERROR_SUCCESS))
        fprintf(stderr, "%s: [%s] FALSE POSITIVE FOUND\n", scanning, virname);

    if (filename)
        free(filename);

    cw_set_currentfile(NULL);
    return lsigned;
}

static int sigcheck_dummy(int fd, const char *virname, int warnfp)
{
    return TRUST_E_NOSIGNATURE;
}

typedef int (*pfn_sigcheck)(int fd, const char *virname, int warnfp);
static pfn_sigcheck pf_sigcheck = sigcheck_dummy;

cl_error_t cw_postscan_check(int fd, int result, const char *virname, void *context)
{
    if ((result == CL_VIRUS) && (pf_sigcheck(fd, virname, 1) == ERROR_SUCCESS))
        return CL_BREAK;
    return CL_CLEAN;
}

int cw_sig_init(void)
{
    if (!cw_helpers.wt.ok)
        return 1;

    if (!cw_helpers.wt.CryptCATAdminAcquireContext(&cw_helpers.wt.hCatAdmin, NULL, 0))
    {
        cli_errmsg("sigcheck: CryptCATAdminAcquireContext() failed: 0x%08x\n", GetLastError());
        return 1;
    }

    cli_dbgmsg("sigcheck: Engine enabled\n");
    pf_sigcheck = sigcheck;
    return 0;
}

/* exported */
int cw_sigcheck(int fd)
{
    return pf_sigcheck(fd, NULL, 0);
}

/*
 * Clamav Native Windows Port: executables signature check
 *
 * Copyright (c) 2010-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#include "platform.h"
#include "others.h"

#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <softpub.h>

#ifdef _UNICODE
#define FAILED_RET(call, ret)                                   \
    {                                                           \
        DWORD le = GetLastError();                              \
        cli_dbgmsg("sigcheck: " call " failed: 0x%08lx\n", le); \
        ret;                                                    \
    }

#define FAILED_HRESULT(call) FAILED_RET(call, return le)
#define FAILED_BREAK(call) FAILED_RET(call, break)

static bool isIssuerTrusted(wchar_t *filename)
{
    bool fResult = false;
    DWORD dwSize = 0;
    DWORD lErr;
    CERT_INFO CertInfo;
    wchar_t *szName = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwEncoding, dwContentType, dwFormatType;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;

    if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, filename, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                          CERT_QUERY_FORMAT_FLAG_BINARY, 0, &dwEncoding, &dwContentType, &dwFormatType, &hStore, &hMsg, NULL))
    {
        if ((lErr = GetLastError()) == CRYPT_E_NO_MATCH)
            cli_dbgmsg("sigcheck: CryptQueryObject() returns CRYPT_E_NO_MATCH\n");
        else
            cli_errmsg("sigcheck: CryptQueryObject() failed: 0x%08lx\n", lErr);
        return lErr;
    }

    if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSize))
        FAILED_HRESULT("CryptMsgGetParam");

    if (!(pSignerInfo = (PCMSG_SIGNER_INFO)malloc(dwSize)))
        FAILED_HRESULT("malloc() pSignerInfo");

    do
    {
        if (!CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSize))
            FAILED_BREAK("CryptMsgGetParam");

        CertInfo.Issuer = pSignerInfo->Issuer;
        CertInfo.SerialNumber = pSignerInfo->SerialNumber;

        if (!(pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
                                                        CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo, NULL)))
            FAILED_BREAK("CertFindCertificateInStore");

        if (!(dwSize = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0)))
            FAILED_BREAK("CertGetNameString");

        if (!(szName = (wchar_t *)malloc(dwSize * sizeof(wchar_t))))
            FAILED_BREAK("malloc() szName");

        if (!CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, szName, dwSize))
            FAILED_BREAK("CertGetNameString")

        fResult = wcsncmp(L"Microsoft ", szName, 10) == 0;
        cli_dbgmsg("sigcheck: %s issuer [%ls]\n", fResult ? "Trusted" : "Untrusted", szName);
    } while (0);

    if (pSignerInfo)
        free(pSignerInfo);
    if (szName)
        free(szName);
    return fResult;
}

static long _sigcheck(HCATADMIN hCatAdmin, int fd, const char *virname, bool warnfp)
{
    BOOL TrustIssuer = FALSE;
    LONG lstatus, lsigned = TRUST_E_NOSIGNATURE;
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    HCATINFO *phCatInfo = NULL;
    CATALOG_INFO sCatInfo;
    WINTRUST_FILE_INFO wtfi;
    WINTRUST_CATALOG_INFO wtci;
    WINTRUST_DATA wtd;
    BYTE bHash[20];

    int i;
    wchar_t mTag[41];

    wchar_t filename[MAX_PATH];
    GUID pgActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    DWORD cbHash = sizeof(bHash);

    if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, bHash, 0))
        FAILED_HRESULT("CryptCATAdminCalcHashFromFileHandle");

    if (!GetFinalPathNameByHandleW(hFile, filename, MAX_PATH - 1, VOLUME_NAME_DOS))
        FAILED_HRESULT("GetFinalPathNameByHandleW");

    for (i = 0; i < sizeof(bHash); i++)
        _snwprintf(&mTag[i * 2], 2, L"%02X", bHash[i]);
    mTag[i * 2] = 0;

    memset(&wtd, 0, sizeof(wtd));
    wtd.cbStruct = sizeof(wtd);
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    wtd.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    wtd.dwUIChoice = WTD_UI_NONE;

    phCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, bHash, cbHash, 0, NULL);

    if (phCatInfo)
    {
        memset(&sCatInfo, 0, sizeof(sCatInfo));
        sCatInfo.cbStruct = sizeof(sCatInfo);

        lstatus = CryptCATCatalogInfoFromContext(phCatInfo, &sCatInfo, 0);
        CryptCATAdminReleaseCatalogContext(hCatAdmin, phCatInfo, 0);

        if (!lstatus)
            FAILED_HRESULT("CryptCATAdminReleaseCatalogContext");

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
            FAILED_HRESULT("CryptCATAdminEnumCatalogFromHash");

        cli_dbgmsg("sigcheck: hash not found in catalog trying embedded signature\n");

        memset(&wtfi, 0, sizeof(wtfi));
        wtfi.cbStruct = sizeof(wtfi);
        wtfi.pcwszFilePath = filename;
        wtfi.hFile = hFile;

        wtd.dwUnionChoice = WTD_CHOICE_FILE;
        wtd.pFile = &wtfi;
        TrustIssuer = isIssuerTrusted(filename);
    }

    lsigned = WinVerifyTrust(0, &pgActionID, (LPVOID)&wtd);
    cli_dbgmsg("sigcheck: WinVerifyTrust 0x%08lx\n", lsigned);

    wtd.dwStateAction = WTD_STATEACTION_CLOSE;
    lstatus = WinVerifyTrust(0, &pgActionID, (LPVOID)&wtd);

    if (!TrustIssuer)
        lsigned = TRUST_E_NOSIGNATURE;
    else if (warnfp && (lsigned == ERROR_SUCCESS))
        fprintf(stderr, "%ls: [%s] FALSE POSITIVE FOUND\n", filename, virname);

    return lsigned;
}

long cw_sigcheck(int fd, const char *virname, bool warnfp)
{
    HCATADMIN hCatAdmin;

    if (!CryptCATAdminAcquireContext(&hCatAdmin, NULL, 0))
        FAILED_HRESULT("CryptCATAdminAcquireContext");

    long result = _sigcheck(hCatAdmin, fd, virname, warnfp);

    CryptCATAdminReleaseContext(hCatAdmin, 0);
    return result;
}

#else
long cw_sigcheck(int fd, const char *virname, bool warnfp)
{
    return 0;
}
#endif // UNICODE

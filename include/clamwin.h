/*
 * Clamav Native Windows Port: Native Win32 Interface
 *
 * Copyright (c) 2009-2010 Gianluigi Tiesi <sherpya@netfarm.it>
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

#ifndef _CLAMWIN_H_
#define _CLAMWIN_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct CWHandle_T *CWHandle;

CWHandle CWInit(void);
void CWFree(CWHandle cwh);
int CWLoadDB_A(CWHandle cwh, const char *db);
int CWLoadDB_W(CWHandle cwh, const wchar_t *db);
int CWScanHandle_A(CWHandle cwh, HANDLE h_in, char **virname);
int CWScanHandle_W(CWHandle cwh, HANDLE h_in, wchar_t **virname);
int CWScanHandle2_A(CWHandle cwh, HANDLE h_in, char **virname);
int CWScanHandle2_W(CWHandle cwh, HANDLE h_in, wchar_t **virname);
int CWScanFile_A(CWHandle cwh, const char *filename, char **virname);
int CWScanFile_W(CWHandle cwh, const wchar_t *filename, wchar_t **virname);
const char *CWStrError_A(int clerror);
const wchar_t *CWStrError_W(int clerror);
void CWFreeBuffer(void *ptr);

#ifdef UNICODE
#define CWLoadDB CWLoadDB_W
#define CWScanHandle CWScanHandle_W
#define CWScanHandle2 CWScanHandle2_W
#define CWScanFile CWScanFile_W
#define CWStrError CWStrError_W
#else
#define CWLoadDB CWLoadDB_A
#define CWScanHandle CWScanHandle_A
#define CWScanHandle2 CWScanHandle2_A
#define CWScanFile CWScanFile_A
#define CWStrError CWStrError_A
#endif

#ifdef __cplusplus
}

#include <tchar.h>

#ifdef UNICODE
#define _TFunc(x) #x"_W"
#else
#define _TFunc(x) #x"_A"
#endif

#ifdef _DEBUG
#define _LIBCLAMAV _T("libclamavD.dll")
#else
#define _LIBCLAMAV _T("libclamav.dll")
#endif

typedef CWHandle (__cdecl *imp_CWInit)(void);
typedef void (__cdecl *imp_CWFree)(CWHandle cwh);
typedef int (__cdecl *imp_CWScanFile)(CWHandle cwh, TCHAR *filename, TCHAR **virname);
typedef int (__cdecl *imp_CWLoadDB)(CWHandle cwh, const TCHAR *db);
typedef int (__cdecl *imp_CWScanHandle)(CWHandle cwh, HANDLE h_in, TCHAR **virname);
typedef void (__cdecl *imp_CWFreeBuffer)(void *ptr);
typedef const TCHAR *(__cdecl *imp_CWStrError)(int clerror);

#define _CWSYMCHECK(x) if (!(x)) { ::FreeLibrary(hClamWin); return FALSE; }

namespace ClamWin
{
    static imp_CWInit pCWInit = NULL;
    static imp_CWFree pCWFree = NULL;
    static imp_CWLoadDB pCWLoadDB = NULL;
    static imp_CWScanHandle pCWScanHandle = NULL;
    static imp_CWScanFile pCWScanFile = NULL;
    static imp_CWFreeBuffer pCWFreeBuffer = NULL;
    static imp_CWStrError pCWStrError = NULL;
    static HINSTANCE hClamWin = NULL;

    BOOL LoadLibClamav(LPCTSTR path = NULL)
    {
        if (!(hClamWin = ::LoadLibrary(path ? path : _LIBCLAMAV)))
            return FALSE;

        _CWSYMCHECK(pCWInit = (imp_CWInit) ::GetProcAddress(hClamWin, "CWInit"));
        _CWSYMCHECK(pCWFree = (imp_CWFree) ::GetProcAddress(hClamWin, "CWFree"));
        _CWSYMCHECK(pCWLoadDB = (imp_CWLoadDB) ::GetProcAddress(hClamWin, _TFunc(CWLoadDB)));
        _CWSYMCHECK(pCWScanHandle = (imp_CWScanHandle) ::GetProcAddress(hClamWin, _TFunc(CWScanHandle)));
        _CWSYMCHECK(pCWScanFile = (imp_CWScanFile) ::GetProcAddress(hClamWin, _TFunc(CWScanFile)));
        _CWSYMCHECK(pCWFreeBuffer = (imp_CWFreeBuffer) ::GetProcAddress(hClamWin, "CWFreeBuffer"));
        _CWSYMCHECK(pCWStrError = (imp_CWStrError) ::GetProcAddress(hClamWin, _TFunc(CWStrError)));
        return TRUE;
    };

    void UnloadLibClamav(void)
    {
        if (hClamWin)
        {
            ::FreeLibrary(hClamWin);
            hClamWin = NULL;
        }
    }

    class Scanner
    {
    public:
        Scanner(void) : m_ch(pCWInit()) {}
        ~Scanner(void) { pCWFree(m_ch); }

        BOOL LoadDB(TCHAR *path = NULL)
        {
            return pCWLoadDB(m_ch, path);
        }

        int ScanFile(TCHAR *filename, PTCHAR &virname)
        {
            return pCWScanFile(m_ch, filename, &virname);
        }

        int ScanHandle(HANDLE hFile, PTCHAR &virname)
        {
            return pCWScanHandle(m_ch, hFile, &virname);
        }

        void FreeBuffer(PTCHAR &ptr)
        {
            pCWFreeBuffer(ptr);
        }

        const TCHAR *StrError(int clerror)
        {
            return pCWStrError(clerror);
        }
    private:
        CWHandle m_ch;
    };
};

#undef _LIBCLAMAV
#undef _TFunc
#undef _CWSYMCHECK

#endif

#endif /* _CLAMWIN_H_ */

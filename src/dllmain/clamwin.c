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

#include <osdeps.h>
#include <clamav.h>
#include <clamwin.h>

struct CWHandle_T
{
    struct cl_engine *engine;
    unsigned int sigs;
    unsigned int dboptions;
};

CWHandle CWInit(void)
{
    CWHandle cwh = malloc(sizeof(struct CWHandle_T));
    cl_init(CL_INIT_DEFAULT);
    cwh->engine = cl_engine_new();
    cwh->dboptions = CL_DB_CVDNOTMP;
    return cwh;
}

void CWFree(CWHandle cwh)
{
    cl_engine_free(cwh->engine);
    free(cwh);
}

int CWLoadDB_A(CWHandle cwh, const char *db)
{
    int ret;
    const char *dbdir = db ? db : cl_retdbdir();
    if ((ret = cl_load(dbdir, cwh->engine, &cwh->sigs, cwh->dboptions)) != CL_SUCCESS)
        return ret;
    return cl_engine_compile(cwh->engine);
}

int CWLoadDB_W(CWHandle cwh, const wchar_t *db)
{
    int ret;
    char *db_a = db ? cw_wc2mb(db, 0) : NULL;
    ret = CWLoadDB_A(cwh, db_a);
    if (db_a) free(db_a);
    return ret;
}

int CWScanHandle_A(CWHandle cwh, HANDLE h_in, char **virname)
{
    int ret, fd;
    unsigned long int blocks;
    const char *vn;
    HANDLE hScan;

    DuplicateHandle(GetCurrentProcess(),
                    h_in,
                    GetCurrentProcess(),
                    &hScan,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);

    fd = _open_osfhandle((intptr_t) hScan, _O_RDONLY);
    ret = cl_scandesc(fd, &vn, &blocks, cwh->engine, cwh->dboptions);
    _close(fd);
    *virname = strdup(vn);
    return ret;
}

int CWScanHandle_W(CWHandle cwh, HANDLE h_in, wchar_t **virname)
{
    char *vn;
    int ret = CWScanHandle_A(cwh, h_in, &vn);
    *virname = cw_mb2wc(vn);
    free(vn);
    return ret;
}

int CWScanHandle2_A(CWHandle cwh, HANDLE h_in, char **virname)
{
    int ret, fd;
    unsigned long int blocks;
    const char *vn;
    fd = _open_osfhandle((intptr_t) h_in, _O_RDONLY);
    ret = cl_scandesc(fd, &vn, &blocks, cwh->engine, cwh->dboptions);
    _close(fd);
    *virname = strdup(vn);
    return ret;
}

int CWScanHandle2_W(CWHandle cwh, HANDLE h_in, wchar_t **virname)
{
    char *vn;
    int ret = CWScanHandle2_A(cwh, h_in, &vn);
    *virname = cw_mb2wc(vn);
    free(vn);
    return ret;
}

int CWScanFile_A(CWHandle cwh, const char *filename, char **virname)
{
    int ret;
    HANDLE h_in = CreateFileA(filename, GENERIC_READ,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ret = CWScanHandle2_A(cwh, h_in, virname);
    return ret;
}

int CWScanFile_W(CWHandle cwh, const wchar_t *filename, wchar_t **virname)
{
    int ret;
    HANDLE h_in = CreateFileW(filename, GENERIC_READ,
        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ret = CWScanHandle_W(cwh, h_in, virname);
    CloseHandle(h_in);
    return ret;
}

void CWFreeBuffer(void *ptr)
{
    if (ptr) free(ptr);
}

const char *CWStrError_A(int clerror)
{
    return cl_strerror(clerror);
}

const wchar_t *CWStrError_W(int clerror)
{
    switch(clerror) {
    /* libclamav specific codes */
    case CL_CLEAN:
        return L"No viruses detected";
    case CL_VIRUS:
        return L"Virus(es) detected";
    case CL_ENULLARG:
        return L"Null argument passed to function";
    case CL_EARG:
        return L"Invalid argument passed to function";
    case CL_EMALFDB:
        return L"Malformed database";
    case CL_ECVD:
        return L"Broken or not a CVD file";
    case CL_EVERIFY:
        return L"Can't verify database integrity";
    case CL_EUNPACK:
        return L"Can't unpack some data";

    /* I/O and memory errors */
    case CL_EOPEN:
        return L"Can't open file or directory";
    case CL_ECREAT:
        return L"Can't create new file";
    case CL_EUNLINK:
        return L"Can't unlink file";
    case CL_ESTAT:
        return L"Can't get file status";
    case CL_EREAD:
        return L"Can't read file";
    case CL_ESEEK:
        return L"Can't set file offset";
    case CL_EWRITE:
        return L"Can't write to file";
    case CL_EDUP:
        return L"Can't duplicate file descriptor";
    case CL_EACCES:
        return L"Can't access file";
    case CL_ETMPFILE:
        return L"Can't create temporary file";
    case CL_ETMPDIR:
        return L"Can't create temporary directory";
    case CL_EMAP:
        return L"Can't map file into memory";
    case CL_EMEM:
        return L"Can't allocate memory";
    /* internal (needed for debug messages) */
    case CL_EMAXREC:
        return L"CL_EMAXREC";
    case CL_EMAXSIZE:
        return L"CL_EMAXSIZE";
    case CL_EMAXFILES:
        return L"CL_EMAXFILES";
    case CL_EFORMAT:
        return L"CL_EFORMAT: Bad format or broken data";
    case CL_EBYTECODE:
        return L"CL_EBYTECODE: error during bytecode execution";
    default:
        return L"Unknown error code";
    }
}

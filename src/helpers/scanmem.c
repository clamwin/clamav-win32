/*
 * Clamav Native Windows Port: scanner for in-memory modules/exe
 *
 * Copyright (c) 2005-2010 Gianluigi Tiesi <sherpya@netfarm.it>
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
#include <exeScanner.h>

#include <clamav.h>
#include <output.h>
#include <others.h>
#include <clamscan/manager.h>
#include <shared/actions.h>

#ifndef TH32CS_SNAPMODULE32
#define TH32CS_SNAPMODULE32 0x00000010
#endif

#define TIMEOUT_MODULE  30000

extern cl_error_t scancallback(int desc, int bytes, void *context);

/* Yes, I'm no so polite, but clamscan is single-threaded */
#include <clamscan/global.h>
extern struct s_info info;

/* cache helpers */
typedef struct _filelist_t
{
    char filename[MAX_PATH];
    int res;
    struct _filelist_t *next;
} filelist_t;

static inline int lookup_cache(filelist_t **list, const char *filename)
{
    filelist_t *current = *list;
    while (current)
    {
        /* Cache hit */
        if (!_stricmp(filename, current->filename))
        {
            /* cli_dbgmsg("ScanMem Cache [Hit]: %s (%s)\n", current->filename, cl_strerror(current->res)); */
            return current->res;
        }
        current = current->next;
    }
    /* cli_dbgmsg("ScanMem Cache [Miss]: %s\n", filename); */
    return -1;
}

static inline void insert_cache(filelist_t **list, const char *filename, int res)
{
    filelist_t *current = *list, *prev = NULL;

    if (!current) /* New */
        *list = current = malloc(sizeof(filelist_t));
    else
    {
        while (current->next) current = current->next;
        prev = current;
        prev->next = current = malloc(sizeof(filelist_t));
    }

    current->next = NULL;
    current->res = res;
    current->filename[0] = 0;
    strncat(current->filename, filename, MAX_PATH - 1 - strlen(current->filename));
    current->filename[MAX_PATH - 1] = 0;
}

static inline void free_cache(filelist_t **list)
{
    filelist_t *current, *prev;
    current = prev = *list;

    if (!current) return;

    do
    {
        prev = current;
        current = prev->next;
        free(prev);
    } while (current);
}

/* Callback */
typedef struct _cb_data_t
{
    const char *filename;
    size_t size, count;
    int oldvalue;
    int fd;
} cb_data_t;

typedef struct _scanmem_data_t
{
    filelist_t *files;
    int printclean, kill, unload, leave_temps, exclude;
    int res;
    uint32_t processes, modules;

    /* ClamAV Specific */
    struct cl_engine *engine;
    const struct optstruct *opts;
    int options;
} scanmem_data;

/* Needed to Scan System Processes */
int EnablePrivilege(LPCSTR PrivilegeName, DWORD yesno)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!cw_helpers.av32.ok)
    {
        logg("^EnablePrivilege functions are missing\n");
        return 0;
    }

    if(!cw_helpers.av32.OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
        return 0;

    if(!cw_helpers.av32.LookupPrivilegeValueA(NULL, PrivilegeName, &luid))
        return 0;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = yesno;

    cw_helpers.av32.AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

    CloseHandle(hToken);
    return (GetLastError() == ERROR_SUCCESS) ? 1 : 0;
}

static char *getaltpath(const wchar_t *filename)
{
    WIN32_FIND_DATAW wfdw;
    HANDLE hf = INVALID_HANDLE_VALUE;
    wchar_t *part = _wcsdup(filename);
    wchar_t comprev[MAX_PATH + 1] = L"", compose[MAX_PATH + 1];
    wchar_t *rev = comprev, *slash = part, *c = NULL;
    size_t l, la;
    size_t i;

    do
    {
        if (slash != part) *slash = 0;

        /* c: d: etc */
        if ((wcslen(part) == 2) && (part[1] == L':'))
        {
            *rev++ = L':';
            *rev++ = part[0];
            break;
        }

        hf = FindFirstFileW(part, &wfdw);
        if (hf == INVALID_HANDLE_VALUE) /* Network path */
        {
            for (i = wcslen(part); i > 0; i--)
                *rev++ = part[i - 1];
            break;
        }
        FindClose(hf);
        l = wcslen(wfdw.cFileName);
        la = wcslen(wfdw.cAlternateFileName);

        if (la)
            for (i = la; i > 0; i--)
                *rev++ = *(wfdw.cAlternateFileName + i - 1);
        else
            for (i = l; i > 0; i--)
                *rev++ = *(wfdw.cFileName + i - 1);
        *rev++ = '\\';

    }
    while ((slash = wcsrchr(part, L'\\')));

    rev = comprev;
    c = compose;
    for (i = wcslen(rev); i > 0; i--)
        *c++ = *(rev + i - 1);
    *c = 0;

    free(part);
    return cw_wc2mb(compose, WC_NO_BEST_FIT_CHARS);
}

int walkmodules_th(proc_callback callback, void *data)
{
    HANDLE hSnap = INVALID_HANDLE_VALUE, hModuleSnap = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 ps;
    MODULEENTRY32 me32;

    if (!cw_helpers.k32.ok)
    {
        logg("^ToolHelp not available\n");
        return -1;
    }

    logg(" *** Memory Scan: using ToolHelp ***\n\n");

    hSnap = cw_helpers.k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return -1;

    ps.dwSize = sizeof(PROCESSENTRY32);

    if (!cw_helpers.k32.Process32First(hSnap, &ps))
    {
        CloseHandle(hSnap);
        return -1;
    }

    do
    {
        /* system process */
        if (!ps.th32ProcessID) continue;
        /* if (ps.szExeFile[0] != 'c') continue; */
        /* if (!strstr(ps.szExeFile, "clam.exe")) continue; */
        hModuleSnap = cw_helpers.k32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ps.th32ProcessID);
        if (hModuleSnap == INVALID_HANDLE_VALUE) continue;

        me32.dwSize = sizeof(MODULEENTRY32);
        if(!cw_helpers.k32.Module32First(hModuleSnap, &me32))
        {
            CloseHandle(hModuleSnap);
            continue;
        }

        /* Check and transform non ANSI filenames to ANSI using altnames */
        if (!isWin9x() && cw_helpers.psapi.GetModuleFileNameExW)
        {
            HANDLE hFile = CreateFileA(me32.szExePath,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD err = GetLastError();
                wchar_t name[MAX_PATH + 1];
                char *converted = NULL;
                HANDLE p;

                if (err == ERROR_BAD_NETPATH)
                {
                    logg("^Warning scanning files on non-ansi network paths is not supported\n");
                    logg("^File: %s\n", me32.szExePath);
                    continue;
                }

                if ((err != ERROR_INVALID_NAME) && (err != ERROR_PATH_NOT_FOUND))
                {
                    logg("^Expected ERROR_INVALID_NAME/ERROR_PATH_NOT_FOUND but got %d\n", err);
                    continue;
                }

                p = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ps.th32ProcessID);
                if (!cw_helpers.psapi.GetModuleFileNameExW(p, NULL, name, MAX_PATH))
                {
                    logg("^GetModuleFileNameExW() failed %d\n", GetLastError());
                    CloseHandle(p);
                    continue;
                }
                CloseHandle(p);

                if (!(converted = getaltpath(name)))
                {
                    logg("^Cannot map filename to ANSI codepage\n");
                    continue;
                }
                strcpy(me32.szExePath, converted);
                free(converted);
            }
            else
                CloseHandle(hFile);
        }

        do if (callback(ps, me32, data)) break;
        while (cw_helpers.k32.Module32Next(hModuleSnap, &me32));

        CloseHandle(hModuleSnap);
    }
    while (cw_helpers.k32.Process32Next(hSnap, &ps));
    CloseHandle(hSnap);
    return 0;
}

int walkmodules_psapi(proc_callback callback, void *data)
{
    DWORD procs[1024], needed, nprocs, mneeded;
    HANDLE hProc;
    HMODULE mods[1024];
    PROCESSENTRY32 ps;
    MODULEENTRY32 me32;
    MODULEINFO mi;
    int i, j;

    if (!cw_helpers.psapi.ok)
    {
        logg("^PsApi not available\n");
        return -1;
    }

    logg(" *** Memory Scan: using PsApi ***\n\n");

    if (!cw_helpers.psapi.EnumProcesses(procs, sizeof(procs), &needed))
        return -1;

    nprocs = needed / sizeof(DWORD);

    memset(&ps, 0, sizeof(PROCESSENTRY32));
    memset(&me32, 0, sizeof(MODULEENTRY32));
    ps.dwSize = sizeof(PROCESSENTRY32);
    me32.dwSize = sizeof(MODULEENTRY32);

    for (i = 0; i < nprocs; i++)
    {
        if (!procs[i]) continue; /* System process */

        hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procs[i]);

        if (!hProc) continue;

        if (!cw_helpers.psapi.EnumProcessModules(hProc, mods, sizeof(mods), &mneeded))
        {
            CloseHandle(hProc);
            continue;
        }

        if (!cw_helpers.psapi.GetModuleBaseNameA(hProc, mods[0], ps.szExeFile, MAX_PATH - 1))
        {
            CloseHandle(hProc);
            continue;
        }

        ps.th32ProcessID = procs[i];

        for (j = 0; j < (mneeded / sizeof(HMODULE)); j++)
        {
            if (!cw_helpers.psapi.GetModuleBaseName(hProc, mods[j], me32.szModule, MAX_PATH - 1))
                continue;

            if (!cw_helpers.psapi.GetModuleFileNameExA(hProc, mods[j], me32.szExePath, MAX_PATH - 1))
                continue;

            if (!cw_helpers.psapi.GetModuleInformation(hProc, mods[j], &mi, sizeof(mi)))
                continue;

            me32.hModule = mods[j];
            me32.th32ProcessID = procs[i];
            me32.modBaseAddr = mi.lpBaseOfDll;
            me32.modBaseSize = mi.SizeOfImage;
            if (callback(ps, me32, data)) break;
        }
        CloseHandle(hProc);
    }
    return 0;
}

int kill_process(DWORD pid)
{
    HANDLE hProc;
    if (GetCurrentProcessId() == pid)
    {
        logg("^Don't want to kill myself\n");
        return 1;
    }

    if ((hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid)))
    {
        TerminateProcess(hProc, 0);
        if (WaitForSingleObject(hProc, TIMEOUT_MODULE) != WAIT_OBJECT_0)
            logg("^Unable to unload process from memory\n");
        CloseHandle(hProc);
    }
    else
        logg("^OpenProcess() failed %lu\n", GetLastError());
    return 1; /* Skip to next process anyway */
}

/* Not so safe ;) */
int unload_module(DWORD pid, HANDLE hModule)
{
    DWORD rc = 1;
    HANDLE ht;
    HANDLE hProc;

    if (GetCurrentProcessId() == pid)
    {
        logg("^Don't want to unload modules from myself\n");
        return 1;
    }

    hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);

    if (!hProc)
    {
        logg("^OpenProcess() failed %lu\n", GetLastError());
        return 1; /* Skip to next process */
    }

    if ((ht = cw_helpers.k32.CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE) FreeLibrary, hModule, 0, &rc)))
    {
        if(WaitForSingleObject(ht, TIMEOUT_MODULE) == WAIT_TIMEOUT)
        {
            CloseHandle(ht);
            CloseHandle(hProc);
            logg("The module may trying to trick us, killing the process, please rescan\n");
            return kill_process(pid);
        }
        CloseHandle(ht);
        rc = 0; /* Continue scanning this process */
    }
    else
    {
        DWORD res = GetLastError();
        if (res == ERROR_CALL_NOT_IMPLEMENTED)
        {
            logg("^Module unloading is not supported on this OS\n");
            rc = -1; /* Don't complain about removing/moving the file */
        }
        else
        {
            logg("!CreateRemoteThread() failed %lu\n", res);
            rc = 1; /* Skip to next process */
        }
    }

    CloseHandle(hProc);
    return rc;
}

#define FILLBYTES(dst) \
    if (IsBadReadPtr(seek, sizeof(dst))) \
    { \
        logg("!ScanMem Align: Bad pointer!!!\n"); \
        return 1; \
    } \
    memcpy(&dst, seek, sizeof(dst))

/* PE Realignment - FIXME: a lot of code is copy/paste from exeScanner.c */
int align_pe(unsigned char *buffer, size_t size)
{
    int i = 0;
    uint16_t e_mz;
    uint32_t e_lfanew, e_magic;
    unsigned char *seek = buffer;
    PIMAGE_FILE_HEADER pehdr;
    PIMAGE_OPTIONAL_HEADER32 opthdr;
    PIMAGE_SECTION_HEADER sechdr;

    FILLBYTES(e_mz);
    if (e_mz != IMAGE_DOS_SIGNATURE)
    {
        /* cli_dbgmsg("ScanMem Align: DOS Signature not found\n"); */
        return 0;
    }

    seek += 0x3c;

    FILLBYTES(e_lfanew);
    if (!e_lfanew)
    {
        /* cli_dbgmsg("ScanMem Align: Invalid PE offset\n"); */
        return 0;
    }
    seek = buffer + e_lfanew;

    /* PE Signature 'PE' */
    FILLBYTES(e_magic);
    if (e_magic != IMAGE_NT_SIGNATURE)
    {
        /* cli_dbgmsg("ScanMem Align: PE Signature not found\n"); */
        return 0;
    }
    seek += sizeof(e_magic);

    if (IsBadReadPtr(seek, sizeof(IMAGE_FILE_HEADER))) return 0;
    pehdr = (PIMAGE_FILE_HEADER) seek;
    seek += sizeof(IMAGE_FILE_HEADER);

    if (IsBadReadPtr(seek, sizeof(IMAGE_OPTIONAL_HEADER32))) return 0;
    opthdr = (PIMAGE_OPTIONAL_HEADER32) seek;
    seek += sizeof(IMAGE_OPTIONAL_HEADER32);

    /* Invalid sections number */
    if ((pehdr->NumberOfSections < 1) || (pehdr->NumberOfSections > 32))
    {
        /* cli_dbgmsg("ScanMem Align: Invalid sections number\n"); */
        return 0;
    }

    for (i = 0; i < pehdr->NumberOfSections; i++)
    {
        if (IsBadWritePtr(seek, sizeof(IMAGE_SECTION_HEADER))) return 0;
        sechdr = (PIMAGE_SECTION_HEADER) seek;
        seek += sizeof(IMAGE_SECTION_HEADER);
        sechdr->PointerToRawData = sechdr->VirtualAddress;
        sechdr->SizeOfRawData = sechdr->Misc.VirtualSize;
    }
    return 1;
}

int dump_pe(const char *filename, PROCESSENTRY32 ProcStruct, MODULEENTRY32 me32)
{
#ifdef _WIN64 /* MinGW has a broken header for ReadProcessMemory() */
    size_t bytesread = 0;
#else
    DWORD bytesread = 0;
#endif
    DWORD byteswrite = 0;
    int ret = -1;
    HANDLE hFile = INVALID_HANDLE_VALUE, hProc = NULL;
    unsigned char *buffer = NULL;

    if (!(hProc = OpenProcess(PROCESS_VM_READ, FALSE, ProcStruct.th32ProcessID))) return -1;

    buffer = malloc((size_t) me32.modBaseSize);
    if (!ReadProcessMemory(hProc, me32.modBaseAddr, buffer, (size_t) me32.modBaseSize, &bytesread))
    {
        free(buffer);
        CloseHandle(hProc);
        return ret;
    }

    CloseHandle(hProc);

    /* PE Realignment */
    align_pe(buffer, me32.modBaseSize);
    /*
    if (align_pe(buffer, me32.modBaseSize))
        cli_dbgmsg("ScanMem Align: File aligned\n");
    else
        cli_dbgmsg("ScanMem Align: Realign failed\n");
    */

    hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        logg("Error creating %s\n", filename);
        free(buffer);
        return ret;
    }

    if (WriteFile(hFile, buffer, (DWORD) bytesread, &byteswrite, NULL))
        ret = _open_osfhandle((intptr_t) hFile, O_RDONLY | O_BINARY);
    free(buffer);
    return ret;
}

static inline int excluded(const char *filename, const struct optstruct *opts)
{
    const struct optstruct *opt;

    if((opt = optget(opts, "exclude"))->enabled)
    {
        while(opt)
        {
            /* cli_dbgmsg("Matching %s vs %s\n", filename, opt->strarg); */
            if (cli_matchregex(filename, opt->strarg) == 1)
            {
                logg("~%s: Excluded\n", filename);
                return 1;
            }
            opt = opt->nextarg;
        }
    }
    return 0;
}

int cw_scanfile(const char *filename, scanmem_data *scan_data)
{
    int fd;
    int ret = CL_CLEAN;
    const char *virname = NULL;

    logg("*Scanning %s\n", filename);

    if ((fd = safe_open(filename, O_RDONLY|O_BINARY)) == -1)
    {
        logg("^Can't open file %s, %s\n", filename, strerror(errno));
        return -1;
    }

    if (scan_data->engine->cb_progress_ctx)
    {
        cb_data_t *cbdata = scan_data->engine->cb_progress_ctx;
        cbdata->count = 0;
        cbdata->fd = fd;
        cbdata->oldvalue = 0;
        cbdata->filename = filename;
        cbdata->size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
    }

    ret = cl_scandesc(fd, &virname, &info.blocks, scan_data->engine, scan_data->options);
    if (ret == CL_VIRUS)
    {
        logg("~%s: %s FOUND\n", filename, virname);
        info.ifiles++;
    }
    else if (scan_data->printclean)
        logg("~%s: OK    \n", filename);

    close(fd);
    return ret;
}

int scanmem_cb(PROCESSENTRY32 ProcStruct, MODULEENTRY32 me32, void *data)
{
    scanmem_data *scan_data = data;
    int rc = 0;
    int isprocess = 0;
    char modulename[MAX_PATH] = "";
    char expandmodule[MAX_PATH] = "";

    if (!scan_data) return 0;
    scan_data->res = CL_CLEAN;

    modulename[0] = 0;
    /* Special case, btw why I get \SystemRoot\ in process szExePath?
       There are also other cases? */
    if ((strlen(me32.szExePath) > 12) && !strncmp(me32.szExePath, "\\SystemRoot\\", 12))
    {
        expandmodule[0] = 0;
        strncat(expandmodule, me32.szExePath, MAX_PATH - 1 - strlen(expandmodule));
        expandmodule[MAX_PATH - 1] = 0;
        snprintf(expandmodule, MAX_PATH - 1, "%%SystemRoot%%\\%s", &me32.szExePath[12]);
        expandmodule[MAX_PATH - 1] = 0;
        ExpandEnvironmentStrings(expandmodule, modulename, MAX_PATH - 1);
        modulename[MAX_PATH - 1] = 0;
    }

    if (!modulename[0])
    {
        if ((strlen(me32.szExePath) > 4) && PATH_ISUN2(me32.szExePath)) /* \\??\ <-- wtf */
            strncpy(modulename, UNC_OFFSET(me32.szExePath), MAX_PATH - 1);
        else
            strncpy(modulename, me32.szExePath, MAX_PATH - 1);
        modulename[MAX_PATH - 1] = 0;
    }

    scan_data->res = lookup_cache(&scan_data->files, modulename);
    isprocess = !_stricmp(ProcStruct.szExeFile, modulename) || !_stricmp(ProcStruct.szExeFile, me32.szModule);

    if (scan_data->res == -1)
    {
        if (isprocess) scan_data->processes++;
        else scan_data->modules++;

        info.files++;

        /* check for module exclusion */
        scan_data->res = CL_CLEAN;
        if (!(scan_data->exclude && excluded(modulename, scan_data->opts)))
            scan_data->res = cw_scanfile(modulename, scan_data);

        if ((scan_data->res != CL_VIRUS) && is_packed(modulename))
        {
            char *dumped = cli_gentemp(NULL);
            int fd = -1;
            /* cli_dbgmsg("%s: appears to be compressed, saving from computer memory\n", modulename); */
            if ((fd = dump_pe(dumped, ProcStruct, me32)) > 0)
            {
                close(fd);
                scan_data->res = cw_scanfile(dumped, scan_data);
                if (!scan_data->leave_temps) DeleteFile(dumped);
            }
            free(dumped);
        }
        insert_cache(&scan_data->files, modulename, scan_data->res);
    }

    if (scan_data->res == CL_VIRUS)
    {
        if (isprocess && scan_data->kill)
        {
            logg("Unloading program %s from memory\n", modulename);
            rc = kill_process(ProcStruct.th32ProcessID);
        }
        else if (scan_data->unload)
        {
            logg("Unloading module %s from %s\n", me32.szModule, modulename);
            if ((rc = unload_module(ProcStruct.th32ProcessID, me32.hModule)) == -1)
                /* CreateProcessThread() is not implemented */
                return 0;
        }

        if (action) action(modulename);
        return rc;
    }
    return rc;
}

int scanmem(struct cl_engine *engine, const struct optstruct *opts, int options)
{
    scanmem_data data;
    cb_data_t cbdata;
    data.files = NULL;
    data.printclean = 1;
    data.kill = 0;
    data.unload = 0;
    data.leave_temps = 0;
    data.exclude = 0;
    data.res = CL_CLEAN;
    data.processes = 0;
    data.modules = 0;

    if (!(cw_helpers.k32.ok || cw_helpers.psapi.ok))
    {
        logg(" *** Memory Scanning is not supported on this OS ***\n\n");
        return -1;
    }

    if (optget(opts, "infected")->enabled) data.printclean = 0;
    if (optget(opts, "kill")->enabled) data.kill = 1;
    if (optget(opts, "unload")->enabled) data.unload = 1;
    if (optget(opts, "leave-temps")->enabled) data.leave_temps = 1;
    if (optget(opts, "exclude")->enabled) data.exclude = 1;

    /* ClamAV Specific */
    data.engine = engine;
    data.opts = opts;
    data.options = options;

    if (optget(opts, "show-progress")->enabled)
    {
        memset(&cbdata, 0, sizeof(cb_data_t));
        cl_engine_set_clcb_progress(data.engine, scancallback, &cbdata);
    }

    logg(" *** Scanning Programs in Computer Memory ***\n");

    if (!isWin9x() && !EnablePrivilege(SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED))
        logg("---Please login as an Administrator to scan System processes loaded in computer memory---\n");

    cw_revertfsredir();
    if (cw_helpers.k32.ok)
        walkmodules_th(scanmem_cb, (void *) &data);
    else
        walkmodules_psapi(scanmem_cb, (void *) &data);
    cw_disablefsredir();
    free_cache(&data.files);

    logg("\n *** Scanned %lu processes - %lu modules ***\n", data.processes, data.modules);
    logg(" *** Computer Memory Scan Completed ***\n\n");
    return data.res;
}

/*
 * Name:        profiler.c
 * Product:     ClamWin Antivirus
 *
 * Author(s):      alch [alch at users dot sourceforge dot net]
 *                 sherpya [sherpya at users dot sourceforge dot net]
 *
 * Created:     2005/12/15
 * Copyright:   Copyright ClamWin Pty Ltd (c) 2005-2008
 * Licence:
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS
#define OWN_WINSOCK
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include <fcntl.h>
#include <clamav.h>

extern int gnulib_snprintf(char *str, size_t size, const char *format, ...);

/*
    On my TB 1.2Ghz
    Average speed: 1.608 mb/s - main + daily
    Average speed: 6.752 mb/s - daily
*/

/* yes msvc 6 sucks... */
#ifndef _INTPTR_T_DEFINED
typedef long int intptr_t;
#define _INTPTR_T_DEFINED
#endif

#define BMTOMS  (1000.0 / 1048576.0)

#define DAILY_ONLY

#define FILESPEC "*.dll"

#define PADDING_50 "                                                  \n"
#define PADDING_48 "                                                \n"

static struct cl_engine *engine;

static double speed = 0.0;
static int runs = 1;
static int cycles = 0;

BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType)
{
    SetConsoleCtrlHandler(cw_stop_ctrl_handler, FALSE);
    printf("\nControl+C pressed, aborting...\n");
    if (speed > 0.0f) printf("Average speed: %4.3f mb/s" PADDING_50, (speed * BMTOMS / runs));
    exit(1);
}

double benchmark(void)
{
    const char *virname;
    double p_size = 0.0, p_speed = 0.0, r_speed = 0.0;
    WIN32_FIND_DATA info;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE fp = INVALID_HANDLE_VALUE;
    DWORD ticker, now;
    size_t i = 0, processed = 0, elapsed = 0;
    int fd = -1;
    char szWinDir[MAX_PATH], szFileSpec[MAX_PATH], szFileName[MAX_PATH];
    char szLine[MAX_PATH];

    GetWindowsDirectory(szWinDir, MAX_PATH - 1);
    strncat(szWinDir, "\\System32\\", MAX_PATH - 1 - strlen(szWinDir));
    szFileSpec[0] = '\0';
    strncat(szFileSpec, szWinDir, MAX_PATH - 1 - strlen(szFileSpec));
    strncat(szFileSpec, FILESPEC, MAX_PATH - 1 - strlen(szFileSpec));
    fp = FindFirstFile(szFileSpec, &info);

    do
    {
        if (!_stricmp(".", info.cFileName) || !_stricmp("..", info.cFileName))
        {
            if (!FindNextFile(fp, &info)) break;
            continue;
        }

        szFileName[0] = L'\0';
        strncat(szFileName, szWinDir, MAX_PATH - 1 - strlen(szFileName));
        strncat(szFileName, info.cFileName, MAX_PATH - 1 - strlen(szFileName));

        hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
        if (hFile == INVALID_HANDLE_VALUE) break;

        fd = _open_osfhandle((intptr_t) hFile, O_RDONLY | O_BINARY);

        /* Process the file */
        ticker = timeGetTime();
        cl_scandesc(fd, &virname, NULL, engine, CL_SCAN_STDOPT);

        now = timeGetTime();
        /* End processing */

        elapsed = now - ticker; /* millisec */
        p_size = (double) GetFileSize(hFile, NULL); /* bytes */
        p_speed = 0;

        /* Skip scans < 1 millisec */
        if (elapsed)
        {
            processed++;
            p_speed = p_size / (double) elapsed; /* bytes / millisec */
            r_speed += p_speed;

            gnulib_snprintf(szLine, sizeof(szLine), "[%02d/%02d] %4.3f mb/s Scanned %s", runs, cycles, p_speed * BMTOMS, szFileName);
            SetConsoleTitle(szLine);
            /* Padding */
            for (i = strlen(szLine); i < 78; i++)
                szLine[i] = ' ';
            szLine[78] = '\r';
            szLine[79] = '\0';
            printf(szLine);
        }

        _close(fd);

        if (!FindNextFile(fp, &info)) break;

    } while (fp && fp != INVALID_HANDLE_VALUE);

    FindClose(fp);
    return (r_speed / processed);
}

int main(int argc, char *argv[])
{
    int ret = -1;
    unsigned int no = 0;

    if (argc > 3)
    {
        printf("Usage %s: [cycles]\n", argv[0]);
        return -1;
    }

    if (argc == 2)
    {
        cycles = atoi(argv[1]);
        if ((cycles < 2) || (cycles > 20))
        {
            printf("Invalid number of cycles\n");
            return -1;
        }
    }

    SetConsoleCtrlHandler(cw_stop_ctrl_handler, TRUE);

    wprintf(L"Loading Clamav DB...\n");

    if ((ret = cl_init(CL_INIT_DEFAULT)))
    {
        printf("Can't initialize libclamav: %s\n", cl_strerror(ret));
        return -1;
    }

    if (!(engine = cl_engine_new()))
    {
        printf("Can't initialize antivirus engine\n");
        return -1;
    }

#ifndef DAILY_ONLY
    ret = cl_load("main.cvd", engine, &no, CL_DB_STDOPT);
    if (ret || !engine || !no)
    {
        printf("Error loading main db...\n");
        return 1;
    }
#endif

    if ((ret = cl_load("daily.cvd", engine, &no, CL_DB_STDOPT)))
    {
        printf("Error loading daily.cvd...\n");
        return -1;
    }

    printf("Known viruses: %d\n", no);

    if ((ret = cl_engine_compile(engine)) != 0)
    {
        printf("Database initialization error: %s\n", cl_strerror(ret));
        cl_engine_free(engine);
    }

    if (cycles)
    {
        printf("Warm up cycle\n");
        benchmark();
        printf("Warm up done... starting..." PADDING_48);

        for (runs = 1; runs <= cycles; runs++)
            speed += benchmark();
    }
    else
    {
        cycles = 1;
        speed = benchmark();
    }

    cl_engine_free(engine);

    printf("Average speed: %4.3f mb/s" PADDING_50, (speed * BMTOMS / cycles));
    return 0;
}

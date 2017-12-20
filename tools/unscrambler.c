/*
 * Clamav Native Windows Port: Mini Dump files unscrambler
 *
 * Copyright (c) 2006-2008 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(int argc, char *argv[])
{
    HANDLE hFile, hMapFile;
    LPBYTE lpMapAddress;
    size_t i = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage %s scrambled.dmp\n", argv[0]);
        return -1;
    }

    hFile = CreateFileA(argv[1], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE )
    {
        fprintf(stderr, "CreateFile() failed error code %d\n", GetLastError());
        return -1;
    }

    hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, "ClamWinDumper");

    if (!hMapFile)
    {
        fprintf(stderr, "CreateFileMapping() failed error code %d\n", GetLastError());
        CloseHandle(hFile);
        return -1;
    }

    lpMapAddress = (LPBYTE) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (!lpMapAddress)
    {
        fprintf(stderr, "MapViewOfFile() failed error code %d\n", GetLastError());
        CloseHandle(hFile);
        CloseHandle(hMapFile);
        return -1;
    }

    if (memcmp(lpMapAddress, "gngz", 4))
        fprintf(stderr, "Invalid signature, maybe already unscrambled?\n");
    else
    {
        for (i = 0; i < GetFileSize(hFile, NULL); i++)
            lpMapAddress[i] ^= 42;
        FlushViewOfFile(lpMapAddress, 0);
        fprintf(stderr, "%s unscrambed\n", argv[1]);
    }

    UnmapViewOfFile(lpMapAddress);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
    return 0;
}

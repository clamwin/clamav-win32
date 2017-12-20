/*
 * Clamav Native Windows Port: stat() replacement
 * plain stat doesn't support unc paths, also it returns strange results
 * when called on invalid paths like c:\com7.ppd
 *
 * Copyright (c) 2005-2008 Gianluigi Tiesi <sherpya@netfarm.it>
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

#undef fstat
#undef stat
int cw_stat(const char *path, struct stat *buf)
{
    int res = 0;
    int fd = -1;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION fi;

    /* set device to the drive letter for cross-fs option
       network path device is always 0, there is no easy
       way to map a network path to 32bit integer */
    dev_t dev = PATH_ISNET(path) ? 0 : tolower(PATH_PLAIN(path)[0]) - 'a';

    memset(buf, 0, sizeof(struct stat));
    if ((hFile = CreateFileA(path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL)) == INVALID_HANDLE_VALUE)
    {
        /* Windows will fail to open the file with this flag if it's locked */
        WIN32_FIND_DATA fdata;
        DWORD err = GetLastError();

        /* Short circuit */
        if ((err == ERROR_FILE_NOT_FOUND) || (err == ERROR_PATH_NOT_FOUND))
        {
             cw_leerrno();
             return -1;
        }

        /* Win9x is not able to get a directory HANDLE with CreateFileA */
        if (isWin9x() && (err == ERROR_ACCESS_DENIED)) return stat(path, buf);

        if ((hFile = FindFirstFileA(path, &fdata)) == INVALID_HANDLE_VALUE)
        {
            cw_leerrno();
            return -1;
        }

        FindClose(hFile);
        buf->st_mode = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _S_IFDIR : _S_IFREG;
        buf->st_size = fdata.nFileSizeLow;
        buf->st_dev = buf->st_rdev = dev;

        return 0;
    }

    memset(&fi, 0, sizeof(fi));
    if (!GetFileInformationByHandle(hFile, &fi))
    {
        CloseHandle(hFile);
        return -1;
    }

    if ((fd = _open_osfhandle((intptr_t) hFile, _O_RDONLY)) < 0)
        return -1;

    res = fstat(fd, buf);
    _close(fd);

    buf->st_dev = buf->st_rdev = dev;
    if (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        buf->st_mode = (buf->st_mode & ~_S_IFMT) | _S_IFDIR;
    return res;
}

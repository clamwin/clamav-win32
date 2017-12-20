/*
 * Clamav Native Windows Port: libltdl "emulation"
 *
 * Copyright (c) 2008 Gianluigi Tiesi <sherpya@netfarm.it>
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

#ifndef __LTDL_H_
#define __LTDL_H_

#define SEARCH_LIBDIR ""

typedef HMODULE lt_dlhandle;

typedef struct {
  char *filename;
  char *name;
} lt_dlinfo;

#define lt_dlinit() (0)
#define lt_dladdsearchdir(x) (0)
#define lt_dlgetsearchpath() "Standard Windows Paths"
#define lt_dlgetinfo(x) (NULL)
#define lt_dlerror() cw_strerror(cw_leerrno())
#define lt_dlsym GetProcAddress

static inline lt_dlhandle lt_dlopen(const char *filename)
{
    UINT olderr = SetErrorMode(SEM_FAILCRITICALERRORS);
    HMODULE lib = LoadLibraryA(filename);
    SetErrorMode(olderr);
    return lib;
}

#endif /* __LTDL_H_ */

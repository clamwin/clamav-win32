/*
 * main() wrapper to handle startup code
 *
 * Copyright (c) 2008-2025 Gianluigi Tiesi <sherpya@gmail.com>
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
#include "crashdump.h"

/* Disable crt globbing, it's broken */
#ifdef __MINGW32__
int _CRT_glob = 0;
#endif

extern int cw_main(int argc, char *argv[]);

#undef main
int main(int argc, char* argv[])
{
#if defined(_MSC_VER) && !defined(_DEBUG) /* Avoid bypassing calls to Debugger */
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
#endif

    if (_setmode(_fileno(stdin), O_BINARY) == -1)
    {
        perror("_setmode");
        abort();
    }

#ifndef _WIN64
    disablefsredir();
#endif
    return cw_main(argc, argv);
}

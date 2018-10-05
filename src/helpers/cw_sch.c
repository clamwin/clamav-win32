/*
 * Clamav Native Windows Port: default stop control handler
 *
 * Copyright (c) 2005-2018 Gianluigi Tiesi <sherpya@netfarm.it>
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

/* default stop control handler */
BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType)
{
    if (CtrlType == CTRL_C_EVENT)
    {
        SetConsoleCtrlHandler(cw_stop_ctrl_handler, FALSE);
        fprintf(stderr, "Control+C pressed, aborting...\n");
        exit(0);
    }
    return TRUE;
}

/*
 * executable modules analyzer
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
#include <exeScanner.h>

int main(int argc, char *argv[])
{
    int packed = 0;
    if (argc != 2)
    {
        printf("Usage: %s filename\n", argv[0]);
        return -1;
    }

    SetLastError(ERROR_SUCCESS); /* A bit ugly but who cares here? */
    packed = is_packed(argv[1]);
    if (GetLastError() == ERROR_SUCCESS)
        printf("exeScanner: %s -> %s\n", argv[1], (packed ? "Packed" : "Not Packed"));
    return 0;
}

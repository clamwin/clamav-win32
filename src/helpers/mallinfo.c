/*
 * Clamav Native Windows Port: mallinfo for win32
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

#include <windows.h>
#include <psapi.h>

#include "platform.h"

struct mallinfo mallinfo(void)
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    MEMORYSTATUSEX ms;
    _HEAPINFO hinfo;
    struct mallinfo info;
    int numBlocks = 0;
    int freeBlocks = 0;
    size_t freeSize = 0;
    size_t totalHeapSize = 0;

    memset(&info, 0, sizeof(struct mallinfo));
    ms.dwLength = sizeof(MEMORYSTATUSEX);

    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        /* WorkingSetSize is a good approximation for heap memory */
        info.arena = pmc.WorkingSetSize;

        /* Space used from memory mapped files */
        info.hblkhd = pmc.PrivateUsage;
    }

    /* Get global memory status */
    if (GlobalMemoryStatusEx(&ms))
    {
        /*
         * Critical: We set usmblks to 0 since it gets added to uordblks in the
         * calculation. In the Unix mallinfo, usmblks is not actually the maximum
         * available memory but rather the "maximum total allocated space".
         */
        info.usmblks = 0;

        /* Available virtual memory that could be allocated */
        info.fordblks = ms.ullAvailVirtual;
    }

    /* Scan the heap to count blocks */
    hinfo._pentry = NULL;
    while (_heapwalk(&hinfo) == _HEAPOK)
    {
        numBlocks++;
        if (hinfo._useflag == _FREEENTRY)
        {
            freeBlocks++;
            freeSize += hinfo._size;
        }
        totalHeapSize += hinfo._size;
    }

    /* Set values based on heap walk */
    info.ordblks = freeBlocks;
    info.fsmblks = freeSize; /* Free memory in smaller blocks */

    /*
     * Set uordblks to committed memory minus free space.
     * This ensures mem_used calculation will be accurate.
     */
    info.uordblks = pmc.PrivateUsage - freeSize;

    /* Calculate potentially releasable memory */
    info.keepcost = freeSize > 0 ? freeSize : 0;

    /* For fields that don't have a direct Windows equivalent */
    info.smblks = 0; /* Windows doesn't have fastbins */
    info.hblks = 1;  /* Assume at least one mmap region */

    return info;
}

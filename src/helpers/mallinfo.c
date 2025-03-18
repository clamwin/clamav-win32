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

#include "platform.h"

#include <windows.h>
#include <psapi.h>

struct mallinfo mallinfo(void)
{
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS_EX pmc;
    struct mallinfo info;
    int freeRegions = 0;

    memset(&info, 0, sizeof(struct mallinfo));

#if _WIN32_WINNT >= _WIN32_WINNT_WINXP
    /* Get process memory information for basic metrics */
    memset(&pmc, 0, sizeof(pmc));
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
    {
        /* Private working set (reasonable approximation for heap) */
        info.arena = pmc.WorkingSetSize;

        /* Total private usage */
        info.hblkhd = pmc.PrivateUsage;
    }
#endif

    /* Get heap metrics more directly */
    DWORD dwHeapCount = GetProcessHeaps(0, NULL);
    HANDLE *pHeaps = (HANDLE *)malloc(dwHeapCount * sizeof(HANDLE));

    if (pHeaps)
    {
        size_t freeHeapSize = 0;
        GetProcessHeaps(dwHeapCount, pHeaps);

        /* Directly query each heap for size information */
        for (DWORD i = 0; i < dwHeapCount; i++)
        {
            PROCESS_HEAP_ENTRY entry;
            memset(&entry, 0, sizeof(entry));

            if (HeapLock(pHeaps[i]))
            {
                while (HeapWalk(pHeaps[i], &entry))
                {
                    if (entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
                    {
                        /* Uncommitted range */
                        continue;
                    }

                    if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
                    {
                        /* Used block */
                    }
                    else
                    {
                        /* Free block */
                        freeHeapSize += entry.cbData;
                        freeRegions++;
                    }
                }
                HeapUnlock(pHeaps[i]);
            }
        }

        /* Update values with more accurate heap information */
        if (freeHeapSize > 0)
        {
            info.fordblks = freeHeapSize;
            info.fsmblks = freeHeapSize;
            info.keepcost = freeHeapSize;
        }

        free(pHeaps);
    }

    /* Set the remaining fields based on our data */
    info.ordblks = freeRegions;
    info.smblks = 0;                                  /* Windows doesn't have fastbins */
    info.hblks = dwHeapCount;                         /* Number of heaps */
    info.usmblks = 0;                                 /* Set to 0 as it gets added to uordblks */
    info.uordblks = pmc.PrivateUsage - info.fordblks; /* Total allocated minus free */

    return info;
}

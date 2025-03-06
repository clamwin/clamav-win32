/*
 * Windows XP Compatibility Layer
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define InitOnceBeginInitialize NO_InitOnceBeginInitialize
#define InitOnceComplete NO_InitOnceComplete
#include "winxp_compat.h"
#undef InitOnceBeginInitialize
#undef InitOnceComplete

#include <ntstatus.h>
#include <psapi.h>

// Define our fallback INIT_ONCE structure for one-time initialization.
// It must be zero-initialized (e.g. as a static/global variable).
// state (Ptr) // 0 = not initialized, 1 = initializing, 2 = initialized

// Fallback implementation of InitOnceBeginInitialize for Windows XP.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - reserved, must be 0
//   lpPending - output flag that indicates whether the calling thread should perform initialization (TRUE)
//   lpContext - reserved, must be NULL
// Returns TRUE on success, FALSE on error (with an appropriate error code set)
WINBOOL WINAPI InitOnceBeginInitialize(PINIT_ONCE pInitOnce, DWORD dwFlags, PBOOL lpPending, LPVOID *lpContext)
{
    TRACE(L"InitOnceBeginInitialize(0x%p, 0x%08x, 0x%p, 0x%p)\n",
          pInitOnce,
          dwFlags,
          lpPending,
          lpContext);

    // Validate parameters.
    //  lpContext must be NULL and dwFlags must be 0.
    if (pInitOnce == NULL || lpPending == NULL || lpContext != NULL || dwFlags != 0)
    {
        TRACE(L"InitOnceBeginInitialize -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // If initialization is already complete, indicate no pending initialization.
    if (pInitOnce->Ptr == (LPVOID)2)
    {
        *lpPending = FALSE;
        return TRUE;
    }

    // Attempt to mark the INIT_ONCE structure as "initializing".
    // If the current state is 0 (not initialized), atomically set it to 1.
    if (InterlockedCompareExchangePointer(&pInitOnce->Ptr, (LPVOID)1, 0) == (LPVOID)0)
    {
        // The current thread is responsible for performing the initialization.
        *lpPending = TRUE;
        return TRUE;
    }
    else
    {
        // Another thread is performing initialization.
        // Spin-wait until the state becomes 2 (initialized).
        while (pInitOnce->Ptr != (LPVOID)2)
            Sleep(0); // Yield execution to other threads.

        *lpPending = FALSE;
        return TRUE;
    }
}

// Fallback implementation of InitOnceComplete for Windows XP.
// This function should be called by the thread that performed the initialization to mark it as complete.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - reserved, must be 0
//   lpContext - reserved, must be NULL
// Returns TRUE on success, or FALSE if an invalid parameter is provided.
WINBOOL WINAPI InitOnceComplete(PINIT_ONCE pInitOnce, DWORD dwFlags, LPVOID lpContext)
{
    TRACE("InitOnceComplete(0x%p, 0x%08x, 0x%p)\n",
          pInitOnce,
          dwFlags,
          lpContext);

    // Validate parameters: pInitOnce must not be NULL, lpContext must be NULL, and dwFlags must be 0.
    if (pInitOnce == NULL || lpContext != NULL || dwFlags != 0)
    {
        TRACE(L"InitOnceComplete -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Mark the initialization as complete by setting the state to 2.
    InterlockedExchangePointer(&pInitOnce->Ptr, (PVOID)2);

    return TRUE;
}

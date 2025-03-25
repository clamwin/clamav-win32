/*
 * Legacy Windows Compatibility Layer
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

#include "legacy.h"

// Define states for our INIT_ONCE structure
// We use pointer values to represent states:
// 0 = not initialized
// 1 = initializing
// 2 = initialized successfully
// 3 = initialization failed

// Fallback implementation of InitOnceBeginInitialize for Windows XP.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - reserved, must be 0
//   lpPending - output flag that indicates whether the calling thread should perform initialization (TRUE)
//   lpContext - reserved, must be NULL
// Returns TRUE on success, FALSE on error (with an appropriate error code set)
WINBOOL WINAPI InitOnceBeginInitialize(PINIT_ONCE pInitOnce, DWORD dwFlags, PBOOL lpPending, LPVOID *lpContext)
{
    TRACE("InitOnceBeginInitialize(0x%p, 0x%08lx, 0x%p, 0x%p)\n",
          pInitOnce,
          dwFlags,
          lpPending,
          lpContext);

    // Validate parameters.
    //  lpContext must be NULL and dwFlags must be 0.
    if (pInitOnce == NULL || lpPending == NULL || lpContext != NULL || dwFlags != 0)
    {
        TRACE("InitOnceBeginInitialize -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // If initialization is already complete, indicate no pending initialization.
    if (pInitOnce->Ptr == (LPVOID)2)
    {
        *lpPending = FALSE;
        return TRUE;
    }

    // If initialization previously failed, we'll retry
    // This resets the state to allow another initialization attempt
    if (pInitOnce->Ptr == (LPVOID)3)
    {
        // Reset the state to "not initialized" to allow retrying
        if (InterlockedCompareExchangePointer(&pInitOnce->Ptr, (LPVOID)0, (LPVOID)3) == (LPVOID)3)
        {
            // Successfully reset, continue with initialization attempt
        }
        else
        {
            // Another thread already reset it or is initializing now
            // Fall through to the standard wait case below
        }
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
        // Spin-wait until the state becomes 2 (initialized successfully) or 3 (failed).
        for (;;)
        {
            PVOID state = pInitOnce->Ptr;

            if (state == (LPVOID)2)
            {
                // Initialization completed successfully
                *lpPending = FALSE;
                return TRUE;
            }
            else if (state == (LPVOID)3)
            {
                // Initialization failed previously
                SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
                return FALSE;
            }
            else if (state == (LPVOID)1)
            {
                // Still initializing, yield execution
                Sleep(0);
                continue;
            }
            else
            {
                // Unexpected state (shouldn't happen)
                SetLastError(ERROR_INVALID_STATE);
                return FALSE;
            }
        }
    }
}

// Fallback implementation of InitOnceComplete for Windows XP.
// This function should be called by the thread that performed the initialization to mark it as complete.
// Parameters:
//   pInitOnce - pointer to our INIT_ONCE_FALLBACK structure (must be zero-initialized)
//   dwFlags   - supports INIT_ONCE_INIT_FAILED to indicate initialization failure
//   lpContext - reserved, must be NULL
// Returns TRUE on success, or FALSE if an invalid parameter is provided.
WINBOOL WINAPI InitOnceComplete(PINIT_ONCE pInitOnce, DWORD dwFlags, LPVOID lpContext)
{
    TRACE("InitOnceComplete(0x%p, 0x%08lx, 0x%p)\n", pInitOnce, dwFlags, lpContext);

    // Basic parameter validation
    if (pInitOnce == NULL || lpContext != NULL || (dwFlags & ~INIT_ONCE_INIT_FAILED) != 0)
    {
        TRACE("InitOnceComplete -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Ensure we're only marking something that's in the "initializing" state
    if (pInitOnce->Ptr != (LPVOID)1)
    {
        TRACE("InitOnceComplete -> ERROR_INVALID_STATE\n");
        SetLastError(ERROR_INVALID_STATE);
        return FALSE;
    }

    if (dwFlags & INIT_ONCE_INIT_FAILED)
    {
        // Mark initialization as failed (state 3)
        InterlockedExchangePointer(&pInitOnce->Ptr, (PVOID)3);
    }
    else
    {
        // Mark initialization as succeeded (state 2)
        InterlockedExchangePointer(&pInitOnce->Ptr, (PVOID)2);
    }

    return TRUE;
}

// InitOnceInitialize - Initializes an INIT_ONCE structure.
// Parameters:
//   pInitOnce - pointer to the INIT_ONCE structure to initialize
// Returns: Always returns TRUE
void WINAPI InitOnceInitialize(PINIT_ONCE pInitOnce)
{
    TRACE("InitOnceInitialize(0x%p)\n", pInitOnce);

    // Initialize the INIT_ONCE structure by setting its Ptr to NULL (state 0)
    pInitOnce->Ptr = NULL;
}

// Function pointer type for the InitOnce callback
typedef BOOL(WINAPI *PINIT_ONCE_FN)(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *Context);

// InitOnceExecuteOnce - Executes the given initialization routine exactly once.
// Parameters:
//   pInitOnce - pointer to INIT_ONCE structure
//   pInitFn   - pointer to initialization function to be called
//   Parameter - parameter to pass to the initialization function
//   Context   - pointer to receive context value from initialization function
// Returns: TRUE if initialization succeeded, FALSE otherwise
WINBOOL WINAPI InitOnceExecuteOnce(PINIT_ONCE pInitOnce, PINIT_ONCE_FN pInitFn, PVOID Parameter, PVOID *Context)
{
    BOOL fPending = FALSE;
    PVOID lpContext = NULL;

    TRACE("InitOnceExecuteOnce(0x%p, 0x%p, 0x%p, 0x%p)\n",
          pInitOnce,
          pInitFn,
          Parameter,
          Context);

    // Parameter validation
    if (pInitOnce == NULL || pInitFn == NULL)
    {
        TRACE("InitOnceExecuteOnce -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Begin initialization phase - determine if this thread should perform initialization
    if (!InitOnceBeginInitialize(pInitOnce, 0, &fPending, &lpContext))
    {
        // InitOnceBeginInitialize already set the error code
        TRACE("InitOnceExecuteOnce -> InitOnceBeginInitialize failed\n");
        return FALSE;
    }

    // If we're not pending (another thread did the initialization), we're done
    if (!fPending)
    {
        TRACE("InitOnceExecuteOnce -> Already initialized\n");
        return TRUE;
    }

    // We're responsible for initialization, call the provided initialization function
    BOOL bResult = pInitFn(pInitOnce, Parameter, Context);

    // Complete the initialization process with appropriate flags
    if (!InitOnceComplete(pInitOnce, bResult ? 0 : INIT_ONCE_INIT_FAILED, NULL))
    {
        // InitOnceComplete already set the error code
        TRACE("InitOnceExecuteOnce -> InitOnceComplete failed\n");
        return FALSE;
    }

    return bResult;
}

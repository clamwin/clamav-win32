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

typedef enum _INIT_ONCE_STATE
{
    INIT = 0,
    INPROGRESS = 1,
    INIT_DONE = 2,
    INPROGRESS_ASYNC = 3
} INIT_ONCE_STATE;

#define INIT_ONCE_MASK 3

WINBOOL WINAPI InitOnceBeginInitialize(PINIT_ONCE pInitOnce, DWORD dwFlags, PBOOL lpPending, LPVOID *lpContext)
{
    TRACE("InitOnceBeginInitialize(0x%p, 0x%08lx, 0x%p, 0x%p)\n",
          pInitOnce,
          dwFlags,
          lpPending,
          lpContext);

    if (dwFlags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        if (dwFlags & RTL_RUN_ONCE_ASYNC)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ULONG_PTR val = (ULONG_PTR)pInitOnce->Ptr;

        if ((val & INIT_ONCE_MASK) != INIT_DONE)
        {
            RtlNtStatusToDosError(STATUS_UNSUCCESSFUL);
            return FALSE;
        }

        if (lpContext)
            *lpContext = (LPVOID)(val & ~INIT_ONCE_MASK);

        *lpPending = FALSE;
        return TRUE;
    }

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)pInitOnce->Ptr;

        switch (val & INIT_ONCE_MASK)
        {
        case INIT:
            LPVOID newval = (dwFlags & RTL_RUN_ONCE_ASYNC) ? (LPVOID)INPROGRESS_ASYNC : (LPVOID)INPROGRESS;
            if (!InterlockedCompareExchangePointer(&pInitOnce->Ptr, newval, 0))
            {
                *lpPending = TRUE;
                return TRUE;
            }
            break;

        case INPROGRESS:
            if (dwFlags & RTL_RUN_ONCE_ASYNC)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            next = val & ~INIT_ONCE_MASK;

            if (InterlockedCompareExchangePointer(&pInitOnce->Ptr, (LPVOID)((ULONG_PTR)&next | INPROGRESS),
                                                  (LPVOID)val) == (LPVOID)val)
            {
                Sleep(0);
                continue;
            }
            break;

        case INIT_DONE:
            *lpPending = FALSE;
            if (lpContext)
                *lpContext = (LPVOID)(val & ~INIT_ONCE_MASK);
            return TRUE;

        case INPROGRESS_ASYNC:
            if (!(dwFlags & RTL_RUN_ONCE_ASYNC))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            *lpPending = TRUE;
            return TRUE;
        }
    }
}

WINBOOL WINAPI InitOnceComplete(PINIT_ONCE pInitOnce, DWORD dwFlags, LPVOID lpContext)
{
    TRACE("InitOnceComplete(0x%p, 0x%08lx, 0x%p)\n", pInitOnce, dwFlags, lpContext);

    if ((ULONG_PTR)lpContext & INIT_ONCE_MASK)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwFlags & INIT_ONCE_INIT_FAILED)
    {
        if (lpContext || (dwFlags & RTL_RUN_ONCE_ASYNC))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    else
        lpContext = (LPVOID)((ULONG_PTR)lpContext | INIT_DONE);

    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)pInitOnce->Ptr;

        switch (val & INIT_ONCE_MASK)
        {
        case INPROGRESS:
            if (InterlockedCompareExchangePointer(&pInitOnce->Ptr,
                                                  lpContext, (LPVOID)val) != (LPVOID)val)
                break;

            val &= ~INIT_ONCE_MASK;

            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                Sleep(0);
                val = next;
            }

            return TRUE;
        case INPROGRESS_ASYNC:
            if (!(dwFlags & RTL_RUN_ONCE_ASYNC))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            if (InterlockedCompareExchangePointer(&pInitOnce->Ptr, lpContext, (LPVOID)val) != (LPVOID)val)
                break;

            return TRUE;

        default:
            RtlNtStatusToDosError(STATUS_UNSUCCESSFUL);
            return FALSE;
        }
    }
}

void WINAPI InitOnceInitialize(PINIT_ONCE pInitOnce)
{
    TRACE("InitOnceInitialize(0x%p)\n", pInitOnce);
    pInitOnce->Ptr = NULL;
}

typedef BOOL(WINAPI *PINIT_ONCE_FN)(
    PINIT_ONCE InitOnce,
    PVOID Parameter,
    PVOID *Context);

WINBOOL WINAPI InitOnceExecuteOnce(PINIT_ONCE pInitOnce, PINIT_ONCE_FN pInitFn, PVOID Parameter, PVOID *lpContext)
{
    BOOL fPending = FALSE;

    TRACE("InitOnceExecuteOnce(0x%p, 0x%p, 0x%p, 0x%p)\n",
          pInitOnce,
          pInitFn,
          Parameter,
          lpContext);

    // Begin initialization phase - determine if this thread should perform initialization
    if (!InitOnceBeginInitialize(pInitOnce, 0, &fPending, lpContext))
    {
        TRACE("InitOnceExecuteOnce -> InitOnceBeginInitialize failed\n");
        return FALSE;
    }

    // If we're not pending (another thread did the initialization), we're done
    if (!fPending)
    {
        TRACE("InitOnceExecuteOnce -> Already initialized\n");
        return TRUE;
    }

    BOOL bResult = pInitFn(pInitOnce, Parameter, lpContext);

    // Complete the initialization process with appropriate flags
    if (!InitOnceComplete(pInitOnce, bResult ? 0 : INIT_ONCE_INIT_FAILED, lpContext ? *lpContext : NULL))
    {
        TRACE("InitOnceExecuteOnce -> InitOnceComplete failed\n");
        return FALSE;
    }

    return bResult;
}

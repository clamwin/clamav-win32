/*
 * Windows XP Compatibility Layer - SRW Lock Functions
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


#define AcquireSRWLockExclusive NO_AcquireSRWLockExclusive
#define ReleaseSRWLockExclusive NO_ReleaseSRWLockExclusive
#define AcquireSRWLockShared NO_AcquireSRWLockShared
#define ReleaseSRWLockShared NO_ReleaseSRWLockShared
#define InitializeSRWLock NO_InitializeSRWLock
#define TryAcquireSRWLockExclusive NO_TryAcquireSRWLockExclusive
#define TryAcquireSRWLockShared NO_TryAcquireSRWLockShared
#include "winxp_compat.h"
#undef AcquireSRWLockExclusive
#undef ReleaseSRWLockExclusive
#undef AcquireSRWLockShared
#undef ReleaseSRWLockShared
#undef InitializeSRWLock
#undef TryAcquireSRWLockExclusive
#undef TryAcquireSRWLockShared

#include <ntstatus.h>

/*
 * SRW Lock Implementation for Windows XP
 *
 * SRW Locks are implemented using a single LONG that encodes:
 * - The lock state (exclusive/shared/free)
 * - The count of shared acquisitions
 * - Bits to indicate waiters
 *
 * Bit layout in the RTL_SRWLOCK.Ptr:
 * 31 30     16 15        0
 * |E |  XXXX  |  Shared  |
 *
 * Where:
 * - Bit 31 (E): Set when the lock is held exclusively
 * - Bits 16-30: Flags and waiters information
 * - Bits 0-15: Count of shared acquisitions
 */

// Bit flags for the SRW lock
#define SRWLOCK_MASK_EXCLUSIVE 0x80000000         // Lock is held exclusively
#define SRWLOCK_MASK_WAITERS 0x40000000           // Waiters are present
#define SRWLOCK_MASK_SHARED_COUNT 0x0000FFFF      // Mask for the shared count
#define SRWLOCK_MASK_EXCLUSIVE_WAITING 0x20000000 // Waiters for exclusive access

VOID WINAPI InitializeSRWLock(PSRWLOCK SRWLock)
{
    TRACE(L"InitializeSRWLock(0x%p)\n", SRWLock);

    SRWLock->Ptr = NULL;
}

VOID WINAPI AcquireSRWLockExclusive(PSRWLOCK SRWLock)
{
    TRACE(L"AcquireSRWLockExclusive(0x%p)\n", SRWLock);

    for (;;)
    {
        LONG_PTR current = (LONG_PTR)SRWLock->Ptr;

        // If the lock is free (not exclusive and no shared holders)
        if (current == 0)
        {
            // Try to acquire it exclusively
            if (!InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)(SRWLOCK_MASK_EXCLUSIVE), NULL))
            {
                // Successfully acquired
                return;
            }
        }
        else
        {
            // Lock is already owned - mark ourselves as waiting for exclusive access
            if (!(current & SRWLOCK_MASK_EXCLUSIVE_WAITING))
            {
                InterlockedCompareExchangePointer(&SRWLock->Ptr,
                                                  (PVOID)(current | SRWLOCK_MASK_EXCLUSIVE_WAITING),
                                                  (PVOID)current);
            }

            // Wait using a backoff strategy to reduce contention
            for (DWORD i = 0; i < 1000; i++)
            {
                // Quick check if lock became free
                if ((LONG_PTR)SRWLock->Ptr == 0)
                    break;

                if (i < 32)
                    YieldProcessor(); // Spin for a while on multi-core systems
                else
                    Sleep(0); // After spinning, yield to other threads
            }
        }
    }
}

// Release an SRW lock held in exclusive mode
VOID WINAPI ReleaseSRWLockExclusive(PSRWLOCK SRWLock)
{
    TRACE(L"ReleaseSRWLockExclusive(0x%p)\n", SRWLock);

    // Simply clear the exclusive bit and any waiting bits
    // This fully releases the lock for the next acquirer
    InterlockedExchangePointer(&SRWLock->Ptr, NULL);
}

// Acquire an SRW lock in shared mode
VOID WINAPI AcquireSRWLockShared(PSRWLOCK SRWLock)
{
    TRACE(L"AcquireSRWLockShared(0x%p)\n", SRWLock);

    for (;;)
    {
        LONG_PTR current = (LONG_PTR)SRWLock->Ptr;

        // If the lock is not held exclusively
        if (!(current & SRWLOCK_MASK_EXCLUSIVE))
        {
            LONG_PTR newValue;
            LONG_PTR sharedCount = current & SRWLOCK_MASK_SHARED_COUNT;

            // Check for overflow
            if (sharedCount >= SRWLOCK_MASK_SHARED_COUNT)
            {
                // Too many shared locks - very rare case
                TRACE(L"AcquireSRWLockShared -> Too many shared locks\n");
                Sleep(1); // Wait a bit and retry
                continue;
            }

            // Increment shared count
            newValue = current + 1;

            if (InterlockedCompareExchangePointer(&SRWLock->Ptr,
                                                  (PVOID)newValue,
                                                  (PVOID)current) == (PVOID)current)
            {
                // Successfully acquired in shared mode
                return;
            }
        }
        else
        {
            // Lock is held exclusively, mark as waiting
            if (!(current & SRWLOCK_MASK_WAITERS))
            {
                InterlockedCompareExchangePointer(&SRWLock->Ptr,
                                                  (PVOID)(current | SRWLOCK_MASK_WAITERS),
                                                  (PVOID)current);
            }

            // Wait using a backoff strategy
            for (DWORD i = 0; i < 1000; i++)
            {
                // Check if lock is no longer exclusive
                if (!((LONG_PTR)SRWLock->Ptr & SRWLOCK_MASK_EXCLUSIVE))
                    break;

                if (i < 32)
                    YieldProcessor();
                else
                    Sleep(0);
            }
        }
    }
}

// Release an SRW lock held in shared mode
VOID WINAPI ReleaseSRWLockShared(PSRWLOCK SRWLock)
{
    TRACE(L"ReleaseSRWLockShared(0x%p)\n", SRWLock);

    for (;;)
    {
        LONG_PTR current = (LONG_PTR)SRWLock->Ptr;
        LONG_PTR newValue;

        // Ensure we're actually in shared mode
        if (current & SRWLOCK_MASK_EXCLUSIVE)
        {
            TRACE(L"ReleaseSRWLockShared -> Lock not in shared mode\n");
            return;
        }

        // Decrement the shared count
        newValue = current - 1;

        // If this is the last shared holder, clear any waiting bits
        if ((newValue & SRWLOCK_MASK_SHARED_COUNT) == 0)
            newValue = 0;

        if (InterlockedCompareExchangePointer(&SRWLock->Ptr,
                                              (PVOID)newValue,
                                              (PVOID)current) == (PVOID)current)
            return;
    }
}

// Try to acquire an SRW lock in exclusive mode
BOOLEAN WINAPI TryAcquireSRWLockExclusive(PSRWLOCK SRWLock)
{
    TRACE(L"TryAcquireSRWLockExclusive(0x%p)\n", SRWLock);

    // Only succeed if the lock is completely free
    if (!InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)SRWLOCK_MASK_EXCLUSIVE, NULL))
        return TRUE;

    return FALSE;
}

// Try to acquire an SRW lock in shared mode
BOOLEAN WINAPI TryAcquireSRWLockShared(PSRWLOCK SRWLock)
{
    TRACE(L"TryAcquireSRWLockShared(0x%p)\n", SRWLock);

    for (;;)
    {
        LONG_PTR current = (LONG_PTR)SRWLock->Ptr;

        // If the lock is held exclusively, fail immediately
        if (current & SRWLOCK_MASK_EXCLUSIVE)
            return FALSE;

        LONG_PTR sharedCount = current & SRWLOCK_MASK_SHARED_COUNT;

        // Check for overflow
        if (sharedCount >= SRWLOCK_MASK_SHARED_COUNT)
            return FALSE;

        // Try to increment the shared count
        if (InterlockedCompareExchangePointer(&SRWLock->Ptr,
                                              (PVOID)(current + 1),
                                              (PVOID)current) == (PVOID)current)
            return TRUE;
    }
}

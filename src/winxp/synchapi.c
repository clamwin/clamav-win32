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

#include "winxp_compat.h"

typedef struct _ADDRESS_WAIT_ENTRY
{
    volatile void *Address;
    HANDLE Event;
    struct _ADDRESS_WAIT_ENTRY *Next;
} ADDRESS_WAIT_ENTRY;

// Global synchronization
CRITICAL_SECTION g_AddressWaitLock;
ADDRESS_WAIT_ENTRY *g_AddressWaitList = NULL;

// Initialize the critical section (called on first use).
__attribute__((constructor)) static void InitializeAddressWait()
{
    TRACE(L"InitializeAddressWait\n");
    InitializeCriticalSection(&g_AddressWaitLock);
}

__attribute__((destructor)) static void CleanupAddressWait()
{
    DeleteCriticalSection(&g_AddressWaitLock);
}

/**
 * WaitOnAddress - Waits for the value at the specified address to change
 *
 * @param Address - Pointer to the memory address to monitor
 * @param CompareAddress - Pointer to the memory containing the comparison value
 * @param AddressSize - Size of the memory to compare (1, 2, 4, or 8 bytes)
 * @param dwMilliseconds - Timeout in milliseconds
 *
 * @return TRUE if the wait succeeded, FALSE if timeout or error
 */
BOOL WINAPI WaitOnAddress(volatile void *Address, void *CompareAddress, SIZE_T AddressSize, DWORD dwMilliseconds)
{
    TRACE(L"WaitOnAddress(0x%p, 0x%p, %d, %d)\n", Address, CompareAddress, AddressSize, dwMilliseconds);

    // Validate address size
    if (AddressSize != 1 && AddressSize != 2 && AddressSize != 4 && AddressSize != 8)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Create wait event
    HANDLE waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (waitEvent == NULL)
        return FALSE;

    // Create wait entry
    ADDRESS_WAIT_ENTRY *entry = (ADDRESS_WAIT_ENTRY *)malloc(sizeof(ADDRESS_WAIT_ENTRY));
    if (entry == NULL)
    {
        CloseHandle(waitEvent);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    entry->Address = Address;
    entry->Event = waitEvent;

    // Add to wait list
    EnterCriticalSection(&g_AddressWaitLock);

    // Check if the value has already changed
    BOOL valueMatches = FALSE;
    switch (AddressSize)
    {
    case 1:
        valueMatches = (*(volatile BYTE *)Address == *(BYTE *)CompareAddress);
        break;
    case 2:
        valueMatches = (*(volatile SHORT *)Address == *(SHORT *)CompareAddress);
        break;
    case 4:
        valueMatches = (*(volatile LONG *)Address == *(LONG *)CompareAddress);
        break;
    case 8:
        valueMatches = (*(volatile LONG64 *)Address == *(LONG64 *)CompareAddress);
        break;
    }

    BOOL result = TRUE;

    if (valueMatches)
    {
        // Value still matches, add to wait list
        entry->Next = g_AddressWaitList;
        g_AddressWaitList = entry;
        LeaveCriticalSection(&g_AddressWaitLock);

        // Wait for the event
        DWORD waitResult = WaitForSingleObject(waitEvent, dwMilliseconds);

        // Remove from wait list if timeout
        if (waitResult == WAIT_TIMEOUT)
        {
            EnterCriticalSection(&g_AddressWaitLock);
            // Search for our entry to remove it
            ADDRESS_WAIT_ENTRY **current = &g_AddressWaitList;
            while (*current != NULL)
            {
                if (*current == entry)
                {
                    *current = entry->Next;
                    break;
                }
                current = &((*current)->Next);
            }
            LeaveCriticalSection(&g_AddressWaitLock);
            result = FALSE;
        }
    }
    else
    {
        // Value has already changed, no need to wait
        LeaveCriticalSection(&g_AddressWaitLock);
    }

    // Cleanup
    CloseHandle(waitEvent);
    free(entry);

    return result;
}

/**
 * WakeByAddressAll - Wakes all threads waiting on the specified address
 *
 * @param Address - Pointer to the memory address to wake waiters on
 */
void WINAPI WakeByAddressAll(void *Address)
{
    TRACE(L"WakeByAddressAll(0x%p)\n", Address);

    EnterCriticalSection(&g_AddressWaitLock);

    // Find all entries for this address and signal them
    ADDRESS_WAIT_ENTRY **current = &g_AddressWaitList;
    while (*current != NULL)
    {
        ADDRESS_WAIT_ENTRY *entry = *current;

        if (entry->Address == Address)
        {
            // Signal this thread
            SetEvent(entry->Event);

            // Remove from list
            *current = entry->Next;

            // Note: The waiting thread is responsible for freeing its entry
        }
        else
        {
            // Move to next entry
            current = &(entry->Next);
        }
    }

    LeaveCriticalSection(&g_AddressWaitLock);
}

/**
 * WakeByAddressSingle - Wakes a single thread waiting on the specified address
 *
 * @param Address - Pointer to the memory address to wake a waiter on
 */
void WINAPI WakeByAddressSingle(void *Address)
{
    TRACE(L"WakeByAddressSingle(0x%p)\n", Address);

    EnterCriticalSection(&g_AddressWaitLock);

    // Find first entry for this address and signal it
    ADDRESS_WAIT_ENTRY **current = &g_AddressWaitList;
    while (*current != NULL)
    {
        ADDRESS_WAIT_ENTRY *entry = *current;

        if (entry->Address == Address)
        {
            // Signal this thread
            SetEvent(entry->Event);

            // Remove from list
            *current = entry->Next;

            // We only wake one thread, so break
            break;
        }
        else
        {
            // Move to next entry
            current = &(entry->Next);
        }
    }

    LeaveCriticalSection(&g_AddressWaitLock);
}

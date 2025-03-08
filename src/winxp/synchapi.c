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

#define SignalObjectAndWait NO_SignalObjectAndWait
#define InitializeConditionVariable NO_InitializeConditionVariable
#define WakeConditionVariable NO_WakeConditionVariable
#define WakeAllConditionVariable NO_WakeAllConditionVariable
#define SleepConditionVariableCS NO_SleepConditionVariableCS
#define SleepConditionVariableSRW NO_SleepConditionVariableSRW
#include "winxp_compat.h"
#undef SignalObjectAndWait
#undef InitializeConditionVariable
#undef WakeConditionVariable
#undef WakeAllConditionVariable
#undef SleepConditionVariableCS
#undef SleepConditionVariableSRW

typedef struct _ADDRESS_WAIT_ENTRY
{
    volatile void *Address;
    HANDLE Event;
    struct _ADDRESS_WAIT_ENTRY *Next;
} ADDRESS_WAIT_ENTRY;

typedef struct _CONDITION_VARIABLE_WAIT_ENTRY
{
    HANDLE WaitEvent;
    struct _CONDITION_VARIABLE_WAIT_ENTRY *Next;
} CONDITION_VARIABLE_WAIT_ENTRY;

// Global synchronization
CRITICAL_SECTION g_AddressWaitLock;
CRITICAL_SECTION g_ConditionVariableLock;
ADDRESS_WAIT_ENTRY *g_AddressWaitList = NULL;

__attribute__((constructor)) static void synchapi_ctor()
{
    TRACE(L"synchapi_ctor\n");
    InitializeCriticalSection(&g_AddressWaitLock);
    InitializeCriticalSection(&g_ConditionVariableLock);
}

__attribute__((destructor)) static void synchapi_dtor()
{
    TRACE(L"synchapi_dtor\n");
    DeleteCriticalSection(&g_AddressWaitLock);
    DeleteCriticalSection(&g_ConditionVariableLock);
}

/**
 * InitializeConditionVariable - Initializes a condition variable
 *
 * @param ConditionVariable - Pointer to the condition variable
 */
void WINAPI InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    TRACE(L"InitializeConditionVariable(0x%p)\n", ConditionVariable);

    ConditionVariable->Ptr = NULL;
}

static HANDLE GetConditionEvent(PCONDITION_VARIABLE ConditionVariable)
{
    HANDLE hEvent = (HANDLE)ConditionVariable->Ptr;

    if (hEvent)
        return hEvent;

    // Create an auto-reset event
    HANDLE newEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!newEvent)
    {
        TRACE(L"GetConditionEvent -> Failed to create event\n");
        return NULL;
    }

    // Try to set it atomically - if we lose the race, close our event
    if (InterlockedCompareExchangePointer(&ConditionVariable->Ptr, newEvent, NULL))
    {
        CloseHandle(newEvent);
        hEvent = (HANDLE)ConditionVariable->Ptr;
    }
    else
        hEvent = newEvent;

    return hEvent;
}

/**
 * @brief Signals an object and waits on another object
 *
 * This function atomically signals one object and waits on another object.
 * This is useful in synchronization scenarios where you need to signal one
 * thread and immediately wait for a response or another condition.
 *
 * @param hObjectToSignal Handle to the object to signal
 * @param hObjectToWaitOn Handle to the object to wait on
 * @param dwMilliseconds Maximum time to wait in milliseconds, or INFINITE
 * @param bAlertable TRUE if the wait is alertable, FALSE otherwise
 * @return DWORD The wait result: WAIT_OBJECT_0, WAIT_TIMEOUT, WAIT_ABANDONED, etc.
 */
DWORD WINAPI SignalObjectAndWait(
    HANDLE hObjectToSignal,
    HANDLE hObjectToWaitOn,
    DWORD dwMilliseconds,
    BOOL bAlertable)
{
    if (!hObjectToSignal || !hObjectToWaitOn)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }

    // Signal the first object
    if (!SetEvent(hObjectToSignal))
        return WAIT_FAILED;

    // Wait on the second object
    return WaitForSingleObjectEx(hObjectToWaitOn, dwMilliseconds, bAlertable);
}

/**
 * WakeConditionVariable - Wakes a single thread waiting on the specified condition variable
 *
 * @param ConditionVariable - Pointer to the condition variable
 */
void WINAPI WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    TRACE(L"WakeConditionVariable(0x%p)\n", ConditionVariable);

    if (!ConditionVariable)
    {
        TRACE(L"WakeConditionVariable -> Invalid parameter\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    EnterCriticalSection(&g_ConditionVariableLock);

    // Get the first waiter from the Ptr field
    CONDITION_VARIABLE_WAIT_ENTRY *waiter = (CONDITION_VARIABLE_WAIT_ENTRY *)ConditionVariable->Ptr;

    if (waiter)
    {
        // Remove from list
        ConditionVariable->Ptr = waiter->Next;

        // Signal the event
        SetEvent(waiter->WaitEvent);
    }

    LeaveCriticalSection(&g_ConditionVariableLock);
}

/**
 * SleepConditionVariableCS - Puts the current thread to sleep until the condition variable is signaled
 *
 * @param ConditionVariable - Pointer to the condition variable
 * @param CriticalSection - Critical section associated with the condition variable
 * @param dwMilliseconds - Timeout in milliseconds
 *
 * @return TRUE if the wait succeeded, FALSE if timeout or error
 */
BOOL WINAPI SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable, PCRITICAL_SECTION CriticalSection, DWORD dwMilliseconds)
{
    TRACE(L"SleepConditionVariableCS(0x%p, 0x%p, %d)\n", ConditionVariable, CriticalSection, dwMilliseconds);

    // Create wait event
    HANDLE waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!waitEvent)
        return FALSE;

    // Create wait entry
    CONDITION_VARIABLE_WAIT_ENTRY *entry = (CONDITION_VARIABLE_WAIT_ENTRY *)malloc(sizeof(CONDITION_VARIABLE_WAIT_ENTRY));
    if (!entry)
    {
        CloseHandle(waitEvent);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    entry->WaitEvent = waitEvent;

    // Add to the wait list (protected by global lock)
    EnterCriticalSection(&g_ConditionVariableLock);
    entry->Next = (CONDITION_VARIABLE_WAIT_ENTRY *)ConditionVariable->Ptr;
    ConditionVariable->Ptr = entry;
    LeaveCriticalSection(&g_ConditionVariableLock);

    // Release the critical section and wait
    LeaveCriticalSection(CriticalSection);

    // Wait for the event
    DWORD waitResult = WaitForSingleObject(waitEvent, dwMilliseconds);
    BOOL result = (waitResult == WAIT_OBJECT_0);

    // If timeout occurred, we need to remove ourselves from the wait list
    if (waitResult == WAIT_TIMEOUT)
    {
        EnterCriticalSection(&g_ConditionVariableLock);

        // Search for our entry to remove it
        CONDITION_VARIABLE_WAIT_ENTRY **current = (CONDITION_VARIABLE_WAIT_ENTRY **)&ConditionVariable->Ptr;
        while (*current)
        {
            if (*current == entry)
            {
                *current = entry->Next;
                break;
            }
            current = &((*current)->Next);
        }

        LeaveCriticalSection(&g_ConditionVariableLock);
    }

    // Reacquire the critical section before returning (as per API contract)
    EnterCriticalSection(CriticalSection);

    // Cleanup
    CloseHandle(waitEvent);
    free(entry);

    if (!result)
        SetLastError(ERROR_TIMEOUT);

    return result;
}

/**
 * SleepConditionVariableSRW - Puts the current thread to sleep until the condition variable is signaled (SRW version)
 *
 * @param ConditionVariable - Pointer to the condition variable
 * @param SRWLock - SRW lock associated with the condition variable
 * @param dwMilliseconds - Timeout in milliseconds
 * @param Flags - Flags (can be CONDITION_VARIABLE_LOCKMODE_SHARED)
 *
 * @return TRUE if the wait succeeded, FALSE if timeout or error
 */
BOOL WINAPI SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK SRWLock, DWORD dwMilliseconds, ULONG Flags)
{
    TRACE(L"SleepConditionVariableSRW(0x%p, 0x%p, %d, 0x%x)\n", ConditionVariable, SRWLock, dwMilliseconds, Flags);

    if (!ConditionVariable || !SRWLock)
    {
        TRACE(L"SleepConditionVariableSRW -> ERROR_INVALID_PARAMETER\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Create wait event
    HANDLE waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!waitEvent)
        return FALSE;

    // Create wait entry
    CONDITION_VARIABLE_WAIT_ENTRY *entry = (CONDITION_VARIABLE_WAIT_ENTRY *)malloc(sizeof(CONDITION_VARIABLE_WAIT_ENTRY));
    if (!entry)
    {
        CloseHandle(waitEvent);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    entry->WaitEvent = waitEvent;

    // Add to the wait list (protected by global lock)
    EnterCriticalSection(&g_ConditionVariableLock);
    entry->Next = (CONDITION_VARIABLE_WAIT_ENTRY *)ConditionVariable->Ptr;
    ConditionVariable->Ptr = entry;
    LeaveCriticalSection(&g_ConditionVariableLock);

    // Release the SRW lock and wait
    if (Flags & CONDITION_VARIABLE_LOCKMODE_SHARED)
        ReleaseSRWLockShared(SRWLock);
    else
        ReleaseSRWLockExclusive(SRWLock);

    // Wait for the event
    DWORD waitResult = WaitForSingleObject(waitEvent, dwMilliseconds);
    BOOL result = (waitResult == WAIT_OBJECT_0);

    // If timeout occurred, we need to remove ourselves from the wait list
    if (waitResult == WAIT_TIMEOUT)
    {
        EnterCriticalSection(&g_ConditionVariableLock);

        // Search for our entry to remove it
        CONDITION_VARIABLE_WAIT_ENTRY **current = (CONDITION_VARIABLE_WAIT_ENTRY **)&ConditionVariable->Ptr;
        while (*current)
        {
            if (*current == entry)
            {
                *current = entry->Next;
                break;
            }
            current = &((*current)->Next);
        }

        LeaveCriticalSection(&g_ConditionVariableLock);
    }

    // Reacquire the SRW lock before returning (as per API contract)
    if (Flags & CONDITION_VARIABLE_LOCKMODE_SHARED)
        AcquireSRWLockShared(SRWLock);
    else
        AcquireSRWLockExclusive(SRWLock);

    // Cleanup
    CloseHandle(waitEvent);
    free(entry);

    if (!result)
        SetLastError(ERROR_TIMEOUT);

    return result;
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
    if (!waitEvent)
        return FALSE;

    // Create wait entry
    ADDRESS_WAIT_ENTRY *entry = (ADDRESS_WAIT_ENTRY *)malloc(sizeof(ADDRESS_WAIT_ENTRY));
    if (!entry)
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
            while (*current)
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
 * WakeAllConditionVariable - Wakes all threads waiting on the specified condition variable
 *
 * @param ConditionVariable - Pointer to the condition variable
 */
void WINAPI WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    TRACE(L"WakeAllConditionVariable(0x%p)\n", ConditionVariable);

    if (!ConditionVariable)
    {
        TRACE(L"WakeConditionVariable -> Invalid parameter\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    EnterCriticalSection(&g_ConditionVariableLock);

    // Get all waiters
    CONDITION_VARIABLE_WAIT_ENTRY *waiter = (CONDITION_VARIABLE_WAIT_ENTRY *)ConditionVariable->Ptr;

    // Clear the list
    ConditionVariable->Ptr = NULL;

    // Wake all waiters
    while (waiter)
    {
        CONDITION_VARIABLE_WAIT_ENTRY *next = waiter->Next;
        SetEvent(waiter->WaitEvent);
        waiter = next;
    }

    LeaveCriticalSection(&g_ConditionVariableLock);
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
    while (*current)
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
    while (*current)
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

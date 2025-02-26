
#include "winxp_compat.h"

// --- Fallback WaitOnAddress / WakeByAddress Implementation ---

// Each waiting thread inserts a Waiter node into a global list.
typedef struct Waiter
{
    void *address; // The address on which the thread is waiting.
    HANDLE event;  // An event the thread waits on.
    BOOL signaled; // Flag to indicate if this waiter was signaled.
    struct Waiter *next;
} Waiter;

// Global list of waiters and a critical section to protect it.
static CRITICAL_SECTION g_waiters_cs;
static BOOL g_waiters_cs_initialized = FALSE;
static Waiter *g_waiters = NULL;

// Initialize the critical section (called on first use).
static void InitWaitersCriticalSection(void)
{
    TRACE(L"InitWaitersCriticalSection\n");

    if (!g_waiters_cs_initialized)
    {
        InitializeCriticalSection(&g_waiters_cs);
        g_waiters_cs_initialized = TRUE;
    }
}

// WaitOnAddress fallback.
// Parameters:
//   Address: pointer to the memory to monitor.
//   CompareAddress: pointer to a buffer holding the expected value.
//   AddressSize: size (in bytes) of the data to compare.
//   dwMilliseconds: timeout in milliseconds.
WINBASEAPI BOOL WINAPI WaitOnAddress(
    volatile VOID *Address,
    PVOID CompareAddress,
    SIZE_T AddressSize,
    DWORD dwMilliseconds)
{
    TRACE(L"WaitOnAddress(0x%p, 0x%p, %d, %d)\n", Address, CompareAddress, AddressSize, dwMilliseconds);

    InitWaitersCriticalSection();

    // Quick check: if the value at Address is already different, return immediately.
    if (memcmp((const void *)Address, CompareAddress, AddressSize) != 0)
        return TRUE;

    // Create an event for this waiter.
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent)
        return FALSE;

    // Allocate and initialize a waiter node.
    Waiter *waiter = (Waiter *)malloc(sizeof(Waiter));
    if (!waiter)
    {
        CloseHandle(hEvent);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    waiter->address = (void *)Address;
    waiter->event = hEvent;
    waiter->signaled = FALSE;
    waiter->next = NULL;

    // Insert the waiter into the global list.
    EnterCriticalSection(&g_waiters_cs);
    waiter->next = g_waiters;
    g_waiters = waiter;
    LeaveCriticalSection(&g_waiters_cs);

    // Wait on the event.
    DWORD dwWait = WaitForSingleObject(hEvent, dwMilliseconds);
    BOOL ret = (dwWait == WAIT_OBJECT_0);

    // Remove the waiter from the global list.
    EnterCriticalSection(&g_waiters_cs);
    {
        Waiter **pp = &g_waiters;
        while (*pp)
        {
            if (*pp == waiter)
            {
                *pp = waiter->next;
                break;
            }
            pp = &((*pp)->next);
        }
    }
    LeaveCriticalSection(&g_waiters_cs);

    CloseHandle(hEvent);
    free(waiter);
    return ret;
}

// WakeByAddressSingle fallback.
// Wakes one thread waiting on the specified Address.
WINBASEAPI VOID WINAPI WakeByAddressSingle(PVOID Address)
{
    TRACE(L"WakeByAddressSingle(0x%p)\n", Address);

    InitWaitersCriticalSection();

    EnterCriticalSection(&g_waiters_cs);
    {
        Waiter *curr = g_waiters;
        while (curr)
        {
            if (curr->address == Address && !curr->signaled)
            {
                curr->signaled = TRUE;
                SetEvent(curr->event);
                break; // Only wake one waiter.
            }
            curr = curr->next;
        }
    }
    LeaveCriticalSection(&g_waiters_cs);
}

// WakeByAddressAll fallback.
// Wakes all threads waiting on the specified Address.
WINBASEAPI VOID WINAPI WakeByAddressAll(PVOID Address)
{
    TRACE(L"WakeByAddressAll(0x%p)\n", Address);

    InitWaitersCriticalSection();

    EnterCriticalSection(&g_waiters_cs);
    {
        Waiter *curr = g_waiters;
        while (curr)
        {
            if (curr->address == Address && !curr->signaled)
            {
                curr->signaled = TRUE;
                SetEvent(curr->event);
            }
            curr = curr->next;
        }
    }
    LeaveCriticalSection(&g_waiters_cs);
}

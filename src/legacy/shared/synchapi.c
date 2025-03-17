/*
 * Windows Legacy Compatibility Layer - Process synchronisation
 *
 * Copyright (c) 2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * Copyright 1996, 1997, 1998 Marcus Meissner
 * Copyright 1997, 1998, 1999 Alexandre Julliard
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2003 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "legacy.h"
#include "list.h"

#include <synchapi.h>

struct futex_entry
{
    struct list entry;
    const void *address;
    HANDLE event;
};

struct futex_queue
{
    struct list queue;
    LONG lock;
};

static struct futex_queue futex_queues[256];

static struct futex_queue *get_futex_queue(const void *addr)
{
    ULONG_PTR val = (ULONG_PTR)addr;
    return &futex_queues[(val >> 4) % ARRAYSIZE(futex_queues)];
}

static void spin_lock(LONG *lock)
{
    while (InterlockedCompareExchange(lock, -1, 0))
        YieldProcessor();
}

static void spin_unlock(LONG *lock)
{
    InterlockedExchange(lock, 0);
}

static BOOL compare_addr(const void *addr, const void *cmp, SIZE_T size)
{
    switch (size)
    {
    case 1:
        return (*(const UCHAR *)addr == *(const UCHAR *)cmp);
    case 2:
        return (*(const USHORT *)addr == *(const USHORT *)cmp);
    case 4:
        return (*(const ULONG *)addr == *(const ULONG *)cmp);
    case 8:
        return (*(const ULONG64 *)addr == *(const ULONG64 *)cmp);
    }

    return FALSE;
}

/**
 * WakeByAddressSingle - Wakes a single thread waiting on the specified address
 *
 * @param Address - Pointer to the memory address to wake a waiter on
 */
void WINAPI WakeByAddressSingle(void *Address)
{
    TRACE("WakeByAddressSingle(0x%p)\n", Address);

    if (!Address)
        return;

    struct futex_queue *queue = get_futex_queue(Address);
    struct futex_entry *entry;

    spin_lock(&queue->lock);

    if (!queue->queue.next)
        list_init(&queue->queue);

    HANDLE event = NULL;
    LIST_FOR_EACH_ENTRY(entry, &queue->queue, struct futex_entry, entry)
    {
        if (entry->address == Address)
        {
            event = entry->event;

            // Remove from list
            entry->address = NULL;
            list_remove(&entry->entry);

            // We only wake one thread, so break
            break;
        }
    }

    spin_unlock(&queue->lock);

    // Signal this thread
    if (event)
        SetEvent(event);
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
    TRACE("WaitOnAddress(0x%p, 0x%p, %ld, %d)\n", Address, CompareAddress, AddressSize, dwMilliseconds);

    // Validate address size
    if (AddressSize != 1 && AddressSize != 2 && AddressSize != 4 && AddressSize != 8)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    struct futex_queue *queue = get_futex_queue((const void *)Address);
    struct futex_entry entry;

    spin_lock(&queue->lock);

    /* Do the comparison inside of the spinlock, to reduce spurious wakeups. */
    if (!compare_addr((const void *)Address, CompareAddress, AddressSize))
    {
        spin_unlock(&queue->lock);
        return TRUE;
    }

    // Create wait event
    if (!(entry.event = CreateEvent(NULL, FALSE, FALSE, NULL)))
    {
        TRACE("WaitOnAddress: CreateEvent() failed (%ld)\n", GetLastError());
        spin_unlock(&queue->lock);
        return FALSE;
    }

    entry.address = (const void *)Address;

    if (!queue->queue.next)
        list_init(&queue->queue);
    list_add_tail(&queue->queue, &entry.entry);

    spin_unlock(&queue->lock);

    // Wait for the event
    DWORD waitResult = WaitForSingleObject(entry.event, dwMilliseconds);
    CloseHandle(entry.event);

    if (entry.address)
    {
        spin_lock(&queue->lock);
        if (entry.address)
            list_remove(&entry.entry);
        spin_unlock(&queue->lock);
    }

    return (waitResult != WAIT_TIMEOUT);
}

/**
 * WakeByAddressAll - Wakes all threads waiting on the specified address
 *
 * @param Address - Pointer to the memory address to wake waiters on
 */
void WINAPI WakeByAddressAll(void *Address)
{
    TRACE("WakeByAddressAll(0x%p)\n", Address);

    if (!Address)
        return;

    struct futex_queue *queue = get_futex_queue(Address);
    struct futex_entry *entry, *next;

    spin_lock(&queue->lock);

    if (!queue->queue.next)
        list_init(&queue->queue);

    HANDLE events[256];
    int count = 0;

    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &queue->queue, struct futex_entry, entry)
    {
        if (entry->address == Address)
        {
            entry->address = NULL;
            list_remove(&entry->entry);
            /* Try to buffer wakes, so that we don't make a system call while
             * holding a spinlock. */
            if (count < ARRAYSIZE(events))
                events[count++] = entry->event;
            else
                SetEvent(entry->event);
        }
    }

    spin_unlock(&queue->lock);

    for (int i = 0; i < count; ++i)
        SetEvent(events[i]);
}

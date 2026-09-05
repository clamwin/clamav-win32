/*
 * Windows Legacy Compatibility Layer - fiber local storage
 *
 * Copyright (c) 2026 Gianluigi Tiesi <sherpya@gmail.com>
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

#include <errno.h>
#include <pthread.h>

/*
 * Rust uses FLS only to arrange for its TLS destructor runner to be called at
 * thread exit.  winpthreads already has the required loader TLS callback, so a
 * pthread key gives us the same lifetime on systems predating native FLS.
 *
 * The wrapper is necessary on x86: PFLS_CALLBACK_FUNCTION is stdcall while a
 * pthread key destructor is cdecl.
 */
struct fls_key;

struct fls_value
{
    struct fls_key *owner;
    struct fls_value *next;
    void *value;
};

struct fls_key
{
    struct fls_key *next;
    struct fls_value *values;
    pthread_key_t key;
    PFLS_CALLBACK_FUNCTION callback;
};

static struct fls_key *fls_keys;
static LONG fls_lock;

static void fls_spin_lock(void)
{
    while (InterlockedCompareExchange(&fls_lock, 1, 0))
        YieldProcessor();
}

static void fls_spin_unlock(void)
{
    InterlockedExchange(&fls_lock, 0);
}

static struct fls_key *fls_find_key(DWORD index)
{
    struct fls_key *key;

    for (key = fls_keys; key; key = key->next)
    {
        if (key->key == index)
            return key;
    }

    return NULL;
}

static void fls_unlink_value(struct fls_value *value)
{
    struct fls_value **cursor = &value->owner->values;

    while (*cursor && *cursor != value)
        cursor = &(*cursor)->next;
    if (*cursor)
        *cursor = value->next;
}

static void fls_destroy_value(void *opaque)
{
    struct fls_value *value = opaque;
    PFLS_CALLBACK_FUNCTION callback;
    void *data;

    if (!value)
        return;

    fls_spin_lock();
    fls_unlink_value(value);
    callback = value->owner->callback;
    data = value->value;
    fls_spin_unlock();

    HeapFree(GetProcessHeap(), 0, value);
    if (callback && data)
        callback(data);
}

DWORD WINAPI FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback)
{
    struct fls_key *key;
    int result;

    TRACE("FlsAlloc(%p)\n", lpCallback);

    key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*key));
    if (!key)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FLS_OUT_OF_INDEXES;
    }

    result = pthread_key_create(&key->key, fls_destroy_value);
    if (result)
    {
        HeapFree(GetProcessHeap(), 0, key);
        SetLastError(result == ENOMEM ? ERROR_NOT_ENOUGH_MEMORY : ERROR_INVALID_PARAMETER);
        return FLS_OUT_OF_INDEXES;
    }

    key->callback = lpCallback;
    fls_spin_lock();
    key->next = fls_keys;
    fls_keys = key;
    fls_spin_unlock();

    return (DWORD)key->key;
}

BOOL WINAPI FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData)
{
    struct fls_value *value;
    struct fls_key *key;
    BOOL allocated = FALSE;
    int result;

    TRACE("FlsSetValue(%lu, %p)\n", dwFlsIndex, lpFlsData);

    fls_spin_lock();
    key = fls_find_key(dwFlsIndex);
    fls_spin_unlock();
    if (!key)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    value = pthread_getspecific(key->key);
    if (!lpFlsData)
    {
        if (!value)
            return TRUE;

        result = pthread_setspecific(key->key, NULL);
        if (result)
        {
            SetLastError(result == ENOMEM ? ERROR_NOT_ENOUGH_MEMORY : ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        fls_spin_lock();
        fls_unlink_value(value);
        fls_spin_unlock();
        HeapFree(GetProcessHeap(), 0, value);
        return TRUE;
    }

    if (!value)
    {
        value = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*value));
        if (!value)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        value->owner = key;
        value->value = lpFlsData;

        fls_spin_lock();
        value->next = key->values;
        key->values = value;
        fls_spin_unlock();
        allocated = TRUE;
    }
    else
    {
        value->value = lpFlsData;
    }

    result = pthread_setspecific(key->key, value);
    if (result)
    {
        if (allocated)
        {
            fls_spin_lock();
            fls_unlink_value(value);
            fls_spin_unlock();
            HeapFree(GetProcessHeap(), 0, value);
        }
        SetLastError(result == ENOMEM ? ERROR_NOT_ENOUGH_MEMORY : ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI FlsFree(DWORD dwFlsIndex)
{
    struct fls_value *values;
    struct fls_value *value;
    struct fls_key **cursor;
    struct fls_key *key;
    int result;

    TRACE("FlsFree(%lu)\n", dwFlsIndex);

    fls_spin_lock();
    cursor = &fls_keys;
    while (*cursor && (*cursor)->key != dwFlsIndex)
        cursor = &(*cursor)->next;
    key = *cursor;
    if (key)
        *cursor = key->next;
    fls_spin_unlock();

    if (!key)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    result = pthread_key_delete(key->key);
    if (result)
    {
        fls_spin_lock();
        key->next = fls_keys;
        fls_keys = key;
        fls_spin_unlock();
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* FlsFree runs the callback for every non-null value. */
    values = key->values;
    while (values)
    {
        value = values;
        values = value->next;
        if (key->callback && value->value)
            key->callback(value->value);
        HeapFree(GetProcessHeap(), 0, value);
    }

    HeapFree(GetProcessHeap(), 0, key);
    return TRUE;
}

BOOL WINAPI IsThreadAFiber(void)
{
    PVOID fiber_data;

    /* FiberData is NULL for a regular thread on NT4, Windows 2000 and XP. */
#if defined(__GNUC__) && defined(__i386__)
    __asm__ volatile("movl %%fs:0x10, %0" : "=r"(fiber_data));
#elif defined(__GNUC__) && defined(__x86_64__)
    __asm__ volatile("movq %%gs:0x20, %0" : "=r"(fiber_data));
#else
    fiber_data = ((PNT_TIB)NtCurrentTeb())->FiberData;
#endif
    return fiber_data != NULL;
}

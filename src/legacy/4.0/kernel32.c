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

#ifndef _WIN64
#include "legacy.h"
#include "dynload.h"
#include "initializer.h"

#include <assert.h>
#include <process.h>

typedef struct _WAIT_CONTEXT
{
    HANDLE hObject;
    HANDLE hCancelEvent;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    DWORD dwMilliseconds;
    HANDLE hThread;
} WAIT_CONTEXT, *PWAIT_CONTEXT;

static unsigned int __stdcall WaitThreadProc(void *pArg)
{
    PWAIT_CONTEXT ctx = pArg;
    HANDLE handles[2] = {ctx->hObject, ctx->hCancelEvent};

    TRACE("WaitThreadProc: waiting on handles: hObject=0x%p, hCancelEvent=0x%p, timeout=%lu\n",
          ctx->hObject, ctx->hCancelEvent, ctx->dwMilliseconds);

    DWORD dwResult = WaitForMultipleObjects(2, handles, FALSE, ctx->dwMilliseconds);

    if (dwResult == WAIT_OBJECT_0)
        ctx->Callback(ctx->Context, FALSE);
    else if (dwResult == WAIT_TIMEOUT)
        ctx->Callback(ctx->Context, TRUE);

    TRACE("WaitThreadProc: done (result=%ld)\n", dwResult);
    _endthreadex(0);
    return 0;
}

BOOL WINAPI RegisterWaitForSingleObject_compat(PHANDLE phNewWaitObject,
                                               HANDLE hObject,
                                               WAITORTIMERCALLBACK Callback,
                                               PVOID Context,
                                               ULONG dwMilliseconds,
                                               ULONG dwFlags)
{
    // TRACE("RegisterWaitForSingleObject\n");

    if (!hObject || hObject == NtCurrentProcess() || hObject == NtCurrentThread() || !Callback)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((dwFlags & WT_EXECUTEONLYONCE) == 0)
    {
        fprintf(stderr, "[legacy] Unsupported RegisterWaitForSingleObject without WT_EXECUTEONLYONCE\n");
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    PWAIT_CONTEXT ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WAIT_CONTEXT));
    if (!ctx)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!(ctx->hCancelEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        HeapFree(GetProcessHeap(), 0, ctx);
        return FALSE;
    }

    TRACE("RegisterWaitForSingleObject: Created hCancelEvent=0x%p, hObject=0x%p\n", ctx->hCancelEvent, hObject);

    ctx->hObject = hObject;
    ctx->Callback = Callback;
    ctx->Context = Context;
    ctx->dwMilliseconds = dwMilliseconds;

    unsigned int threadId;
    if (!(ctx->hThread = (HANDLE)_beginthreadex(NULL, 0, WaitThreadProc, ctx, 0, &threadId)))
    {
        fprintf(stderr, "RegisterWaitForSingleObject: CreateThread() failed with %ld\n", GetLastError());
        CloseHandle(ctx->hCancelEvent);
        HeapFree(GetProcessHeap(), 0, ctx);
        return FALSE;
    }

    if (phNewWaitObject)
        *phNewWaitObject = ctx;

    return TRUE;
}

BOOL WINAPI UnregisterWaitEx_compat(HANDLE hWaitObject, HANDLE hCompletionEvent)
{
    TRACE("UnregisterWaitEx(0x%p, 0x%p)\n", hWaitObject, hCompletionEvent);

    if (!hWaitObject)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hCompletionEvent == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "[legacy] Unsupported UnregisterWaitEx with INVALID_HANDLE_VALUE\n");
        return FALSE;
    }

    PWAIT_CONTEXT ctx = hWaitObject;
    assert(ctx->hCancelEvent);

    TRACE("UnregisterWaitEx: About to signal cancel event 0x%p\n", ctx->hCancelEvent);
    if (!SetEvent(ctx->hCancelEvent))
        fprintf(stderr, "UnregisterWaitEx: SetEvent failed with error %ld\n", GetLastError());

    TRACE("UnregisterWaitEx: Waiting for thread 0x%p\n", ctx->hThread);
#ifdef LEGACY_TRACE
    DWORD waitRes =
#endif
        WaitForSingleObject(ctx->hThread, INFINITE);
    TRACE("UnregisterWaitEx: WaitForSingleObject returned %ld\n", waitRes);

    CloseHandle(ctx->hCancelEvent);
    CloseHandle(ctx->hThread);

    if (hCompletionEvent)
    {
        TRACE("UnregisterWaitEx: SetEvent(%p)\n", hCompletionEvent);
        SetEvent(hCompletionEvent);
    }

    HeapFree(GetProcessHeap(), 0, ctx);
    return TRUE;
}

BOOL WINAPI UnregisterWait_compat(HANDLE hWaitObject)
{
    TRACE("UnregisterWait(0x%p)\n", hWaitObject);
    return UnregisterWaitEx_compat(hWaitObject, NULL);
}

/* windows 2k has this function, but whatever... */
BOOL WINAPI SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    TRACE("SetFilePointerEx(0x%p, %lld, %p, %ld)\n", hFile, liDistanceToMove.QuadPart, lpNewFilePointer, dwMoveMethod);

    liDistanceToMove.LowPart = SetFilePointer(hFile, liDistanceToMove.LowPart, &liDistanceToMove.HighPart, dwMoveMethod);
    if (liDistanceToMove.LowPart == INVALID_SET_FILE_POINTER)
        return FALSE;

    if (lpNewFilePointer)
        lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;

    return TRUE;
}

#ifdef _UNICODE
DWORD WINAPI GetProcessId(HANDLE Process)
{
    PROCESS_BASIC_INFORMATION pbi;
    NTSTATUS status = NtQueryInformationProcess(Process,
                                                ProcessBasicInformation,
                                                &pbi,
                                                sizeof(PROCESS_BASIC_INFORMATION),
                                                NULL);

    if (!NT_SUCCESS(status))
    {
        TRACE("GetProcessId: NtQueryInformationProcess failed with 0x%08lx\n", status);
        SetLastError(RtlNtStatusToDosError(status));
        return 0;
    }

    return (DWORD)pbi.UniqueProcessId;
}
#else
DWORD WINAPI GetProcessId(HANDLE Process)
{
    TRACE("GetProcessId(0x%p)\n", Process);
    fprintf(stderr, "GetProcessId is not supported!\n");
    return 0;
}

NTSTATUS NTAPI NtOpenFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions)
{
    fprintf(stderr, "NtOpenFile is not supported!\n");
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS NTAPI NtReadFile(
    HANDLE FileHandle,
    HANDLE Event,
    PIO_APC_ROUTINE ApcRoutine,
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER ByteOffset,
    PULONG Key)
{
    DWORD bytesRead = 0;
    BOOL result;

    // TRACE("NtReadFile(0x%p, 0x%p): %lu\n", FileHandle, Event, Length);

    if (Event || ByteOffset)
    {
        fprintf(stderr, "NtReadFile() unsupported Event or ByteOffset\n");
        return STATUS_NOT_SUPPORTED;
    }

    result = ReadFile(FileHandle, Buffer, Length, &bytesRead, NULL);

    if (result)
    {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = bytesRead;
    }
    else
    {
        IoStatusBlock->Status = GetLastError();
        IoStatusBlock->Information = 0;
    }

    return IoStatusBlock->Status;
}

NTSTATUS NTAPI NtWriteFile(
    HANDLE FileHandle,
    HANDLE Event,
    PIO_APC_ROUTINE ApcRoutine,
    PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER ByteOffset,
    PULONG Key)
{
    DWORD bytesWritten = 0;
    BOOL result;

    // TRACE("NtWriteFile(0x%p): %lu\n", FileHandle, Length);

    if (Event || ByteOffset)
    {
        fprintf(stderr, "NtWriteFile() unsupported args\n");
        return STATUS_NOT_SUPPORTED;
    }

    result = WriteFile(FileHandle, Buffer, Length, &bytesWritten, NULL);

    if (result)
    {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = bytesWritten;
    }
    else
    {
        IoStatusBlock->Status = GetLastError();
        IoStatusBlock->Information = 0;
    }

    return IoStatusBlock->Status;
}
#endif // _UNICODE

imp_MultiByteToWideChar pMultiByteToWideChar = NULL;
imp_RegisterWaitForSingleObject pRegisterWaitForSingleObject = RegisterWaitForSingleObject_compat;
imp_UnregisterWait pUnregisterWait = UnregisterWait_compat;
imp_UnregisterWaitEx pUnregisterWaitEx = UnregisterWaitEx_compat;

imp_CreateHardLinkW pCreateHardLinkW = NULL;
imp_CreateToolhelp32Snapshot pCreateToolhelp32Snapshot = NULL;
imp_Process32FirstW pProcess32FirstW = NULL;
imp_Process32NextW pProcess32NextW = NULL;
imp_Module32FirstW pModule32FirstW = NULL;
imp_Module32NextW pModule32NextW = NULL;
imp_AddVectoredExceptionHandler pAddVectoredExceptionHandler = NULL;

// windows 98/nt do not like MB_ERR_INVALID_CHARS and loop rust message function
int WINAPI MultiByteToWideChar_wrapper(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
    return pMultiByteToWideChar(CodePage, dwFlags & ~MB_ERR_INVALID_CHARS, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

INITIALIZER(init_kernel32_4_0)
{
    TRACE("Init @ " __FILE__ "\n");
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32"));
    if (!kernel32) // meh
        return;

    IMPORT_FUNCTION(kernel32, MultiByteToWideChar);
    IMPORT_FUNCTION(kernel32, RegisterWaitForSingleObject);
    IMPORT_FUNCTION(kernel32, UnregisterWait);
    IMPORT_FUNCTION(kernel32, UnregisterWaitEx);

    IMPORT_FUNCTION(kernel32, AddVectoredExceptionHandler);
    IMPORT_FUNCTION(kernel32, CreateHardLinkW);
    IMPORT_FUNCTION(kernel32, CreateToolhelp32Snapshot);
    IMPORT_FUNCTION(kernel32, Process32FirstW);
    IMPORT_FUNCTION(kernel32, Process32NextW);
    IMPORT_FUNCTION(kernel32, Module32FirstW);
    IMPORT_FUNCTION(kernel32, Module32NextW);
}
#endif /* _WIN64 */

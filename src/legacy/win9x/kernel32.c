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

#define STRSAFE_NO_DEPRECATE
#include "legacy.h"

#include <psapi.h>
#include <tlhelp32.h>
#include <ntstatus.h>

PVOID WINAPI AddVectoredExceptionHandler(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler)
{
    TRACE("AddVectoredExceptionHandler: STATUS_NOT_SUPPORTED\n");
    return NULL;
}

BOOL WINAPI CreateHardLinkW(LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TRACE("CreateHardLinkW: STATUS_NOT_SUPPORTED\n");
    return FALSE;
}

BOOL WINAPI SetFileInformationByHandle(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    TRACE("SetFileInformationByHandle(0x%p, %d, 0x%p, %ld)\n", hFile, FileInformationClass, lpFileInformation, dwBufferSize);
    return FALSE;
}

DWORD WINAPI GetProcessId(HANDLE Process)
{
    TRACE("GetProcessId(0x%p)\n", Process);
    return 0;
}

BOOL WINAPI Module32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    TRACE("Module32FirstW: STATUS_NOT_SUPPORTED\n");
    return FALSE;
}

BOOL WINAPI Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    TRACE("Module32NextW: STATUS_NOT_SUPPORTED\n");
    return FALSE;
}

typedef struct _cbdata_t
{
    HANDLE hObject;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG dwMilliseconds;
    ULONG dwFlags;
    HANDLE hStop;
} cbdata_t;

typedef struct _tdata_t
{
    HANDLE hThread;
    DWORD dwTid;
    cbdata_t *cbdata;
} tdata_t;

static DWORD WINAPI WaitThread(LPVOID lpParam)
{
    TRACE("WaitThread\n");

    cbdata_t *cbdata = (cbdata_t *)lpParam;
    DWORD result;
    HANDLE wEvents[2] = {cbdata->hObject, cbdata->hStop};

    result = WaitForMultipleObjects(2, wEvents, FALSE, cbdata->dwMilliseconds);

    if (result == WAIT_OBJECT_0)
        cbdata->Callback(cbdata->Context, FALSE);
    else if (result == WAIT_TIMEOUT)
        cbdata->Callback(cbdata->Context, TRUE);

    return 0;
}

BOOL WINAPI RegisterWaitForSingleObject_win9x(PHANDLE phNewWaitObject,
                                              HANDLE hObject,
                                              WAITORTIMERCALLBACK Callback,
                                              PVOID Context,
                                              ULONG dwMilliseconds,
                                              ULONG dwFlags)
{
    TRACE("RegisterWaitForSingleObject\n");

    tdata_t *tdata = calloc(1, sizeof(tdata_t));
    tdata->cbdata = calloc(1, sizeof(cbdata_t));
    tdata->cbdata->hObject = hObject;
    tdata->cbdata->Callback = Callback;
    tdata->cbdata->Context = Context;
    tdata->cbdata->dwMilliseconds = dwMilliseconds;
    tdata->cbdata->dwFlags = dwFlags;
    tdata->cbdata->hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    tdata->hThread = CreateThread(NULL, 0, WaitThread, (LPVOID)tdata->cbdata, 0, &tdata->dwTid);
    *phNewWaitObject = (HANDLE)tdata;
    return TRUE;
}

BOOL WINAPI UnregisterWaitEx_win9x(HANDLE WaitHandle, HANDLE CompletionEvent)
{
    TRACE("UnregisterWaitEx\n");
    tdata_t *tdata = (tdata_t *)WaitHandle;

    SetEvent(tdata->cbdata->hStop);

    if (WaitForSingleObject(tdata->hThread, 15000) != WAIT_OBJECT_0)
    {
        fprintf(stderr, "[legacy] Warning Thread %ld still alive, killing it\n", tdata->dwTid);
        TerminateThread(tdata->hThread, 0);
    }

    CloseHandle(tdata->cbdata->hStop);
    CloseHandle(tdata->hThread);

    if (CompletionEvent)
        SetEvent(CompletionEvent);

    free(tdata->cbdata);
    free(tdata);

    return TRUE;
}

BOOL WINAPI UnregisterWait_win9x(HANDLE WaitHandle)
{
    TRACE("UnregisterWait\n");
    return UnregisterWaitEx_win9x(WaitHandle, NULL);
}

BOOL WINAPI SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
    TRACE("SetFilePointerEx\n");
    liDistanceToMove.LowPart = SetFilePointer(hFile, liDistanceToMove.LowPart, &liDistanceToMove.HighPart, dwMoveMethod);
    if (liDistanceToMove.LowPart == INVALID_SET_FILE_POINTER)
        return FALSE;

    if (lpNewFilePointer)
        lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;

    return TRUE;
}

NTSTATUS NTAPI NtOpenFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions)
{
    fprintf(stderr, "NtOpenFile: STATUS_NOT_SUPPORTED\n");
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
    OVERLAPPED overlapped = {0};
    DWORD bytesRead = 0;
    BOOL result;

    // TRACE("NtReadFile\n");

    if (ByteOffset != NULL)
    {
        overlapped.Offset = ByteOffset->LowPart;
        overlapped.OffsetHigh = ByteOffset->HighPart;
    }

    if (Event != NULL)
        overlapped.hEvent = Event;

    result = ReadFile(FileHandle, Buffer, Length, &bytesRead, &overlapped);

    if (!result)
    {
        DWORD lastError = GetLastError();

        if (lastError == ERROR_IO_PENDING)
        {
            if (WaitForSingleObject(overlapped.hEvent != NULL ? overlapped.hEvent : FileHandle, INFINITE) == WAIT_OBJECT_0)
            {
                GetOverlappedResult(FileHandle, &overlapped, &bytesRead, TRUE);
                IoStatusBlock->Status = STATUS_SUCCESS;
                IoStatusBlock->Information = bytesRead;
                return STATUS_SUCCESS;
            }
        }

        IoStatusBlock->Status = lastError;
        IoStatusBlock->Information = 0;
        return lastError;
    }

    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = bytesRead;

    return STATUS_SUCCESS;
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
    OVERLAPPED overlapped = {0};
    DWORD bytesWritten = 0;
    BOOL result;

    // TRACE("NtWriteFile\n");

    if (ByteOffset != NULL)
    {
        overlapped.Offset = ByteOffset->LowPart;
        overlapped.OffsetHigh = ByteOffset->HighPart;
    }

    if (Event != NULL)
        overlapped.hEvent = Event;

    result = WriteFile(FileHandle, Buffer, Length, &bytesWritten, &overlapped);

    if (!result)
    {
        DWORD lastError = GetLastError();

        if (lastError == ERROR_IO_PENDING)
        {
            if (WaitForSingleObject(overlapped.hEvent != NULL ? overlapped.hEvent : FileHandle, INFINITE) == WAIT_OBJECT_0)
            {
                GetOverlappedResult(FileHandle, &overlapped, &bytesWritten, TRUE);
                IoStatusBlock->Status = STATUS_SUCCESS;
                IoStatusBlock->Information = bytesWritten;
                return STATUS_SUCCESS;
            }
        }

        IoStatusBlock->Status = lastError;
        IoStatusBlock->Information = 0;
        return lastError;
    }

    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = bytesWritten;

    return STATUS_SUCCESS;
}

DWORD WINAPI GetFinalPathNameByHandleW(
    HANDLE hFile,
    LPWSTR lpszFilePath,
    DWORD cchFilePath,
    DWORD dwFlags)
{
    TRACE("GetFinalPathNameByHandleW: STATUS_NOT_SUPPORTED\n");
    SetLastError(STATUS_NOT_SUPPORTED);
    return 0;
}

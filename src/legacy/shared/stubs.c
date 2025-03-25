/*
 * Legacy Windows Compatibility Layer: stubs
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

BOOLEAN APIENTRY CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
    TRACE("CreateSymbolicLinkW(%ls, %ls, %ld)\n", lpSymlinkFileName, lpTargetFileName, dwFlags);

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// Fallback implementation of SetThreadStackGuarantee for Windows XP.
// Since XP does not support changing the thread stack guarantee, this stub simply returns TRUE.
// The value pointed to by StackSizeInBytes is left unchanged.
WINBOOL WINAPI SetThreadStackGuarantee(PULONG StackSizeInBytes)
{
    TRACE("SetThreadStackGuarantee(%p)\n", StackSizeInBytes);

    if (StackSizeInBytes == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // On Windows XP, this API is not supported. Do nothing.
    return TRUE;
}

// Define a minimal dummy attribute list structure.
// This structure is only used as a placeholder because Windows XP does not support extended attributes.
typedef struct _PROC_THREAD_ATTRIBUTE_LIST
{
    SIZE_T cbSize;
    // No attribute storage is provided in this fallback.
} PROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

// Fallback implementation of InitializeProcThreadAttributeList for XP.
// If lpAttributeList is NULL, the required size is returned via lpSize and the function fails
// (as per the normal pattern). Otherwise, the dummy attribute list is initialized.
WINBOOL WINAPI InitializeProcThreadAttributeList(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwAttributeCount, // Ignored in this fallback.
    DWORD dwFlags,          // Must be zero.
    PSIZE_T lpSize)
{
    TRACE("InitializeProcThreadAttributeList(0x%p, %ld, %ld, %p)\n",
          lpAttributeList, dwAttributeCount, dwFlags, lpSize);

    if (lpSize == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Our dummy attribute list requires only a fixed size.
    SIZE_T requiredSize = sizeof(PROC_THREAD_ATTRIBUTE_LIST);

    // If caller is requesting the required size, return it.
    if (lpAttributeList == NULL)
    {
        *lpSize = requiredSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Verify that the provided buffer is large enough.
    if (*lpSize < requiredSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialize the dummy attribute list.
    lpAttributeList->cbSize = requiredSize;

    // Ignore dwAttributeCount and dwFlags (since no attributes are supported).
    return TRUE;
}

// Fallback implementation of DeleteProcThreadAttributeList for XP.
// In this dummy implementation, no resources are allocated, so this function simply clears the structure.
VOID WINAPI DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList)
{
    TRACE("DeleteProcThreadAttributeList(0x%p)\n", lpAttributeList);

    if (lpAttributeList)
    {
        // Clear the structure.
        lpAttributeList->cbSize = 0;
    }
}

// Fallback implementation of UpdateProcThreadAttribute for XP.
// Since extended process/thread attributes are not supported on XP,
// this function always fails and sets the error to ERROR_NOT_SUPPORTED.
WINBOOL WINAPI UpdateProcThreadAttribute(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwFlags,
    DWORD_PTR Attribute,
    PVOID lpValue,
    SIZE_T cbSize,
    PVOID lpPreviousValue,
    PSIZE_T lpReturnSize)
{
    TRACE("UpdateProcThreadAttribute(0x%p, %ld, 0x%lx, 0x%p, %ld, 0x%p, 0x%p)\n",
          lpAttributeList,
          dwFlags,
          Attribute,
          lpValue,
          cbSize,
          lpPreviousValue,
          lpReturnSize);

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

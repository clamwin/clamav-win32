#define CreateSymbolicLinkW NO_CreateSymbolicLinkW
#define CreateWaitableTimerExW NO_CreateWaitableTimerExW
#define InitializeProcThreadAttributeList NO_InitializeProcThreadAttributeList
#define DeleteProcThreadAttributeList NO_DeleteProcThreadAttributeList
#define UpdateProcThreadAttribute NO_UpdateProcThreadAttribute
#include "winxp_compat.h"
#undef CreateSymbolicLinkW
#undef CreateWaitableTimerExW
#undef InitializeProcThreadAttributeList
#undef DeleteProcThreadAttributeList
#undef UpdateProcThreadAttribute

BOOLEAN CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
    TRACE(L"CreateSymbolicLinkW(%ls, %ls, %d)\n", lpSymlinkFileName, lpTargetFileName, dwFlags);

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

HANDLE CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES lpTimerAttributes, LPCWSTR lpTimerName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    TRACE(L"CreateWaitableTimerExW(0x%p, %ls, %d, %d, %d)\n",
          lpTimerAttributes, lpTimerName, dwFlags, dwDesiredAccess);

    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

// Fallback implementation of SetThreadStackGuarantee for Windows XP.
// Since XP does not support changing the thread stack guarantee, this stub simply returns TRUE.
// The value pointed to by StackSizeInBytes is left unchanged.
WINBASEAPI WINBOOL WINAPI SetThreadStackGuarantee(PULONG StackSizeInBytes)
{
    TRACE(L"SetThreadStackGuarantee(%d)\n", StackSizeInBytes);

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
    TRACE(L"InitializeProcThreadAttributeList(0x%p, %d, %d, %d)\n",
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
    TRACE(L"DeleteProcThreadAttributeList(0x%p)\n", lpAttributeList);

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
    TRACE(L"UpdateProcThreadAttribute(0x%p, %d, 0x%p, 0x%p, %d, 0x%p, 0x%p)\n",
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

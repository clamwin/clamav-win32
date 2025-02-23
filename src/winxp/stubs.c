#define _WIN32_WINNT 0x0501
#define _SYNCHAPI_H_

#include <windows.h>

BOOLEAN APIENTRY CreateSymbolicLinkW(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

HANDLE WINAPI CreateWaitableTimerExW(LPSECURITY_ATTRIBUTES lpTimerAttributes, LPCWSTR lpTimerName, DWORD dwFlags, DWORD dwDesiredAccess)
{
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
}

// Fallback implementation of SetThreadStackGuarantee for Windows XP.
// Since XP does not support changing the thread stack guarantee, this stub simply returns TRUE.
// The value pointed to by StackSizeInBytes is left unchanged.
WINBASEAPI WINBOOL WINAPI SetThreadStackGuarantee(PULONG StackSizeInBytes)
{
    if (StackSizeInBytes == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    // On Windows XP, this API is not supported. Do nothing.
    return TRUE;
}

// Define a minimal dummy attribute list structure.
// This structure is only used as a placeholder because Windows XP does not support extended attributes.
typedef struct _PROC_THREAD_ATTRIBUTE_LIST {
    SIZE_T cbSize;
    // No attribute storage is provided in this fallback.
} PROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

// Fallback implementation of InitializeProcThreadAttributeList for XP.
// If lpAttributeList is NULL, the required size is returned via lpSize and the function fails
// (as per the normal pattern). Otherwise, the dummy attribute list is initialized.
WINBOOL WINAPI InitializeProcThreadAttributeList(
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    DWORD dwAttributeCount,   // Ignored in this fallback.
    DWORD dwFlags,            // Must be zero.
    PSIZE_T lpSize)
{
    if (lpSize == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Our dummy attribute list requires only a fixed size.
    SIZE_T requiredSize = sizeof(PROC_THREAD_ATTRIBUTE_LIST);

    // If caller is requesting the required size, return it.
    if (lpAttributeList == NULL) {
        *lpSize = requiredSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Verify that the provided buffer is large enough.
    if (*lpSize < requiredSize) {
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
    if (lpAttributeList) {
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
    UNREFERENCED_PARAMETER(lpAttributeList);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(Attribute);
    UNREFERENCED_PARAMETER(lpValue);
    UNREFERENCED_PARAMETER(cbSize);
    UNREFERENCED_PARAMETER(lpPreviousValue);
    UNREFERENCED_PARAMETER(lpReturnSize);

    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

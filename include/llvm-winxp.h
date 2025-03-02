#ifndef LLVM_WINXP_H
#define LLVM_WINXP_H

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

#define GetLargePageMinimum() 0
#define EnumProcessModulesEx(p, m, cb, n, f) EnumProcessModules(p, m, cb, n)

static inline DWORD GetFinalPathNameByHandleW_winxp(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    std::cerr << "GetFinalPathNameByHandleW called" << std::endl;
    abort();
}
#define GetFinalPathNameByHandleW GetFinalPathNameByHandleW_winxp

static inline BOOL SetFileInformationByHandle_winxp(HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
    std::cerr << "SetFileInformationByHandle called" << std::endl;
    abort();
}
#define SetFileInformationByHandle SetFileInformationByHandle_winxp

static inline long RegGetValueW_winxp(HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
    std::cerr << "RegGetValueW called" << std::endl;
    abort();
}
#define RegGetValueW RegGetValueW_winxp

// Fallback implementation of GetLogicalProcessorInformationEx for Windows XP.
// Only supports RelationProcessorCore and RelationNumaNode.
static inline BOOL GetLogicalProcessorInformationEx_winxp(
    LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
    PDWORD ReturnedLength)
{
    if (ReturnedLength == NULL)
        return FALSE;

    // Only support the two relationships we intend to convert.
    if (Relationship != RelationProcessorCore && Relationship != RelationNumaNode)
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    // First, use the legacy API to retrieve processor information.
    DWORD legacySize = 0;
    BOOL res = GetLogicalProcessorInformation(NULL, &legacySize);

    if (!res && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;

    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION legacyInfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(legacySize);
    if (legacyInfo == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!GetLogicalProcessorInformation(legacyInfo, &legacySize))
    {
        free(legacyInfo);
        return FALSE;
    }

    DWORD count = legacySize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    // First pass: calculate required size.
    DWORD requiredSize = 0;
    for (DWORD i = 0; i < count; i++)
    {
        if (legacyInfo[i].Relationship == Relationship)
        {
            requiredSize += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        }
    }

    *ReturnedLength = requiredSize;
    if (Buffer == NULL)
    {
        free(legacyInfo);
        return TRUE;
    }

    if (requiredSize > *ReturnedLength)
    {
        free(legacyInfo);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Second pass: fill the output buffer.
    BYTE *pCurrent = (BYTE *)Buffer;
    for (DWORD i = 0; i < count; i++)
    {
        if (legacyInfo[i].Relationship == Relationship)
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pEntry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)pCurrent;
            pEntry->Relationship = legacyInfo[i].Relationship;
            pEntry->Size = sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
            if (Relationship == RelationProcessorCore)
                pEntry->Processor.Flags = legacyInfo[i].ProcessorCore.Flags;
            else if (Relationship == RelationNumaNode)
                pEntry->NumaNode.NodeNumber = legacyInfo[i].NumaNode.NodeNumber;
            pCurrent += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX);
        }
    }

    free(legacyInfo);
    return TRUE;
}
#define GetLogicalProcessorInformationEx GetLogicalProcessorInformationEx_winxp

static inline BOOL GetProcessGroupAffinity_winxp(HANDLE hProcess, PUSHORT GroupCount, PUSHORT GroupArray)
{
    // Validate parameters.
    if (GroupCount == NULL || GroupArray == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // On Windows XP, processor groups do not exist.
    // We'll indicate that there is exactly one group, with index 0.
    *GroupCount = 1;
    GroupArray[0] = 0;
    return TRUE;
}
#define GetProcessGroupAffinity GetProcessGroupAffinity_winxp

// Fallback implementation of GetThreadGroupAffinity for Windows XP.
// Since processor groups are not supported on XP, we assume every thread is in group 0
// and we use the process affinity mask (obtained via GetProcessAffinityMask) as the thread’s mask.
static inline BOOL GetThreadGroupAffinity_winxp(HANDLE hThread, PGROUP_AFFINITY GroupAffinity)
{
    if (GroupAffinity == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // On XP there is no per-thread affinity retrieval via groups.
    // We'll use the process affinity mask as a substitute.
    DWORD_PTR processAffinity = 0, systemAffinity = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity))
    {
        return FALSE;
    }

    GroupAffinity->Mask = processAffinity;
    GroupAffinity->Group = 0;
    GroupAffinity->Reserved[0] = GroupAffinity->Reserved[1] = GroupAffinity->Reserved[2] = 0;
    return TRUE;
}
#define GetThreadGroupAffinity GetThreadGroupAffinity_winxp

// Fallback implementation for Windows XP.
// Since Windows XP does not support processor groups, we simply ignore the
// Group field in the provided GROUP_AFFINITY structure and use SetThreadAffinityMask.
static inline BOOL SetThreadGroupAffinity_winxp(
    HANDLE hThread,
    const GROUP_AFFINITY *GroupAffinity,
    PGROUP_AFFINITY PreviousGroupAffinity)
{
    if (hThread == NULL || GroupAffinity == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // On Windows XP there are no processor groups.
    // Use the provided mask and call SetThreadAffinityMask.
    DWORD_PTR oldAffinity = SetThreadAffinityMask(hThread, GroupAffinity->Mask);
    if (oldAffinity == 0)
        return FALSE;

    // If the caller requested the previous affinity, fill it in.
    if (PreviousGroupAffinity != NULL)
    {
        PreviousGroupAffinity->Mask = oldAffinity;
        PreviousGroupAffinity->Group = 0;
        PreviousGroupAffinity->Reserved[0] = 0;
        PreviousGroupAffinity->Reserved[1] = 0;
        PreviousGroupAffinity->Reserved[2] = 0;
    }
    return TRUE;
}
#define SetThreadGroupAffinity SetThreadGroupAffinity_winxp


// Fallback implementation of GetThreadId.
static inline DWORD GetThreadId_winxp(HANDLE hThread)
{
    // If we're asking for the current thread, use GetCurrentThreadId.
    if (hThread == GetCurrentThread())
        return GetCurrentThreadId();

    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status = NtQueryInformationThread(hThread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
    if (status != STATUS_SUCCESS)
        return 0;

    // The UniqueThread member of ClientId holds the thread id.
    return (DWORD)(ULONG_PTR)tbi.ClientId.UniqueThread;
}
#define GetThreadId GetThreadId_winxp
#endif

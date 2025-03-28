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

#include "legacy.h"

// https://github.com/zeroclear/xpext/blob/master/xpext_ver4/k32_file.cpp#L445
// https://stackoverflow.com/questions/65170/how-to-get-name-associated-with-open-handle/5286888#5286888
#ifdef _UNICODE
static BOOL BasepGetObjectNTName(HANDLE Handle, wchar_t **lpName)
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION NtObjectName = NULL;
    ULONG size = sizeof(OBJECT_NAME_INFORMATION) + (MAX_PATH * sizeof(wchar_t));
    ULONG ReturnLength;

    do
    {
        if (NtObjectName)
            RtlFreeHeap(GetProcessHeap(), 0, NtObjectName);

        if (!(NtObjectName = RtlAllocateHeap(GetProcessHeap(), 0, size)))
        {
            SetLastError(ERROR_NO_SYSTEM_RESOURCES);
            return FALSE;
        }

        // Get object name information (full NT path)
        status = NtQueryObject(Handle, ObjectNameInformation, NtObjectName, size, &ReturnLength);
        size = ReturnLength;
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (!NT_SUCCESS(status))
    {
        TRACE("BasepGetObjectNTName: NtQueryObject failed (0x%08x)\n", status);
        if (NtObjectName)
            RtlFreeHeap(GetProcessHeap(), 0, NtObjectName);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    int Length = NtObjectName->Name.Length;
    TRACE("Name=%.*ls\n", (int)(NtObjectName->Name.Length / sizeof(wchar_t)), NtObjectName->Name.Buffer);

    memmove(NtObjectName, NtObjectName->Name.Buffer, Length);
    *lpName = (wchar_t *)NtObjectName;
    (*lpName)[(Length / sizeof(wchar_t))] = L'\0';

    TRACE("BasepGetObjectNTName -> %ls\n", *lpName);

    return TRUE;
}

static BOOL BasepGetFileNameInformation(HANDLE Handle, FILE_INFORMATION_CLASS FileInformationClass, wchar_t **lpName)
{
    NTSTATUS status;
    PFILE_NAME_INFORMATION fileName = NULL;
    ULONG size = sizeof(FILE_NAME_INFORMATION) + (MAX_PATH * sizeof(wchar_t));
    IO_STATUS_BLOCK IoStatusBlock;

    TRACE("BasepGetFileNameInformation Class %d\n", FileInformationClass);

    do
    {
        if (fileName)
            RtlFreeHeap(GetProcessHeap(), 0, fileName);

        if (!(fileName = RtlAllocateHeap(GetProcessHeap(), 0, size)))
        {
            SetLastError(ERROR_NO_SYSTEM_RESOURCES);
            return FALSE;
        }

        status = NtQueryInformationFile(Handle, &IoStatusBlock, fileName, size, FileInformationClass);
        size = fileName->FileNameLength + sizeof(FILE_NAME_INFORMATION);
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (!NT_SUCCESS(status))
    {
        TRACE("GetFinalPathNameByHandleW->NtQueryInformationFile failed (0x%08x)\n", status);
        if (fileName)
            RtlFreeHeap(GetProcessHeap(), 0, fileName);
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    SIZE_T Length = fileName->FileNameLength;
    TRACE("Name=%.*ls\n", (int)(Length / sizeof(wchar_t)), fileName->FileName);

    memmove(fileName, fileName->FileName, Length);
    *lpName = (wchar_t *)fileName;
    (*lpName)[(Length / sizeof(wchar_t))] = L'\0';
    return TRUE;
}

// https://stackoverflow.com/questions/3012828/using-ioctl-mountmgr-query-points
static BOOL BasepGetVolumeGUIDFromNTName(const wchar_t *Src, wchar_t **VolumeName)
{
    TRACE("BasepGetVolumeGUIDFromNTName [%ls]\n", Src);

    HANDLE hDevice = CreateFile(
        MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        TRACE("BasepGetVolumeGUIDFromNTName: CreateFile failed on MountPoint Manager %ld\n", GetLastError());
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    USHORT DeviceNameLength = (USHORT)(wcslen(Src) * sizeof(wchar_t));
    DWORD nInBufferSize = DeviceNameLength + sizeof(MOUNTMGR_MOUNT_POINT);

    PMOUNTMGR_MOUNT_POINT MountPoint = RtlAllocateHeap(GetProcessHeap(), 0, nInBufferSize);
    if (!MountPoint)
    {
        CloseHandle(hDevice);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    memset(MountPoint, 0, sizeof(MOUNTMGR_MOUNT_POINT));
    MountPoint->DeviceNameLength = DeviceNameLength;
    MountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);

    memcpy((LPBYTE)MountPoint + MountPoint->DeviceNameOffset, Src, DeviceNameLength);
    DWORD nOutBufferSize = sizeof(MOUNTMGR_MOUNT_POINTS) + 10 * (40 + sizeof(MOUNTMGR_MOUNT_POINT));

    PMOUNTMGR_MOUNT_POINTS MountPoints = NULL;
    DWORD bytesReturned;
    BOOL success = FALSE;

    do
    {
        if (MountPoints)
            RtlFreeHeap(GetProcessHeap(), FALSE, MountPoints);

        MountPoints = RtlAllocateHeap(GetProcessHeap(), 0, nOutBufferSize);
        if (!MountPoints)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        success = DeviceIoControl(hDevice,
                                  IOCTL_MOUNTMGR_QUERY_POINTS,
                                  MountPoint, nInBufferSize,
                                  MountPoints, nOutBufferSize,
                                  &bytesReturned,
                                  NULL);

        nOutBufferSize = MountPoints->Size + sizeof(MOUNTMGR_MOUNT_POINT);
    } while (GetLastError() == ERROR_MORE_DATA);

    CloseHandle(hDevice);

    if (!success)
    {
        RtlFreeHeap(GetProcessHeap(), 0, MountPoint);
        RtlFreeHeap(GetProcessHeap(), 0, MountPoints);
        TRACE("DeviceIoControl(IOCTL_MOUNTMGR_QUERY_POINTS) failed with %ld\n", GetLastError());
        return 0;
    }

    BOOL result = FALSE;

    do
    {
        if (!MountPoints->NumberOfMountPoints)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            break;
        }

        LPBYTE Base = (LPBYTE)MountPoints;

        for (ULONG i = 0; i < MountPoints->NumberOfMountPoints; i++)
        {
            PMOUNTMGR_MOUNT_POINT Current = &MountPoints->MountPoints[i];
            wchar_t *SymbolicLinkName = (wchar_t *)(Base + Current->SymbolicLinkNameOffset);
            TRACE("Device=%.*ls\n", (int)(Current->SymbolicLinkNameLength / sizeof(wchar_t)),
                  SymbolicLinkName);

            if (!MOUNTMGR_IS_VOLUME_NAME2(SymbolicLinkName, Current->SymbolicLinkNameLength))
                continue;

            *VolumeName = RtlAllocateHeap(GetProcessHeap(), 0, Current->SymbolicLinkNameLength + sizeof(wchar_t));
            if (!*VolumeName)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }

            memcpy(*VolumeName, SymbolicLinkName, Current->SymbolicLinkNameLength);
            (*VolumeName)[(Current->SymbolicLinkNameLength / sizeof(wchar_t))] = L'\0';
            (*VolumeName)[1] = L'\\';
            result = TRUE;
        }

        if (!result)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            break;
        }
    } while (0);

    RtlFreeHeap(GetProcessHeap(), 0, MountPoint);

    if (MountPoints)
        RtlFreeHeap(GetProcessHeap(), 0, MountPoints);

    return result;
}

static inline BOOL UncPath(wchar_t **VolumeName)
{
    if ((*VolumeName = RtlAllocateHeap(GetProcessHeap(), 0, 8 * sizeof(wchar_t))))
    {
        StringCchCopyW(*VolumeName, 8, L"\\\\?\\UNC");
        return TRUE;
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
}

static BOOL BasepGetVolumeDosLetterNameFromNTNameAndFileName(const wchar_t *Src, wchar_t *fileName, wchar_t **VolumeName)
{
    TRACE("BasepGetVolumeDosLetterNameFromNTNameAndFileName [%ls] [%ls]\n", Src, fileName);

    // Get list of logical drives
    wchar_t szLogicalDrives[MAX_PATH] = {0};
    DWORD driveCount = GetLogicalDriveStringsW(MAX_PATH - 1, szLogicalDrives);

    if (!driveCount || driveCount >= MAX_PATH)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        TRACE("GetLogicalDriveStringsW failed or returned too many drives (%ld)\n", GetLastError());
        return FALSE;
    }

    // Try to match device path to a drive letter
    wchar_t targetDevice[MAX_PATH + 1];
    wchar_t *drive = szLogicalDrives;
    size_t deviceNameLen = wcslen(Src);

    while (*drive)
    {
        BOOL bFound = FALSE, bRemote = FALSE;
        wchar_t driveLetter[3] = {drive[0], drive[1], L'\0'};

        TRACE("QueryDosDeviceW: %ls\n", driveLetter);

        if (QueryDosDeviceW(driveLetter, targetDevice, MAX_PATH))
        {
            TRACE("%ls is {%ls}\n", driveLetter, targetDevice);
            // TRACE("targePath: %ls\n", targetPath);

            TRACE("wcsncmp(\"%ls\", \"%ls\", %zu)\n", Src, targetDevice, deviceNameLen);
            // Found matching drive
            if (wcsncmp(Src, targetDevice, deviceNameLen) == 0)
            {
                // Handle network path
                if (wcsstr(targetDevice, L"\\Device\\LanmanRedirector\\;"))
                {
                    /* \Device\LanmanRedirector\;C:0000000000000000\Complete Path\To File.ext */
                    if ((wcslen(targetDevice) > 28) && (targetDevice[26] == drive[0]) && (targetDevice[27] == ':'))
                    {
                        wchar_t *path = wcschr(&targetDevice[28], L'\\');
                        if (path)
                        {
                            // same network host, different share
                            if (wcsncmp(path, fileName, wcslen(path)) == 0)
                            {
                                TRACE("Matched LanmanRedirector path: %ls\n", path);
                                bFound = bRemote = TRUE;
                            }
                        }
                    }
                }
                else
                {
                    // Other network providers (e.g. VBoxMiniRdr)
                    // \Device\VBoxMiniRdr\;Z:\VBoxSvr\shared
                    UINT driveType = GetDriveTypeW(driveLetter);
                    TRACE("Matched: %ls -> %ls [%d]\n", driveLetter, targetDevice, driveType);
                    bFound = TRUE;
                    bRemote = driveType == DRIVE_REMOTE;
                }

                if (bFound)
                {
                    // remote and mapped
                    if (bRemote)
                        return UncPath(VolumeName);

                    if ((*VolumeName = RtlAllocateHeap(GetProcessHeap(), 0, 7 * sizeof(wchar_t))))
                    {
                        StringCchCopyW(*VolumeName, 7, L"\\\\?\\X:");
                        (*VolumeName)[4] = drive[0];
                        return TRUE;
                    }
                    else
                    {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        return FALSE;
                    }
                    break;
                }
            }
        }
        drive += wcslen(drive) + 1;
    }

    // assume unmapped remote path
    return UncPath(VolumeName);
}

static BOOL BasepGetVolumeDosLetterNameFromNTName(const wchar_t *Src, wchar_t **VolumeName)
{
    TRACE("BasepGetVolumeDosLetterNameFromNTName [%ls]\n", Src);

    if (wcsnicmp(Src, L"\\Device\\Mup", 11) == 0)
        return UncPath(VolumeName);

    HANDLE hDevice = CreateFile(
        MOUNTMGR_DOS_DEVICE_NAME,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        TRACE("BasepGetVolumeDosLetterNameFromNTName: CreateFile failed on MountPoint Manager %ld\n", GetLastError());
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    DWORD Length = (DWORD)(wcslen(Src) * sizeof(wchar_t));
    DWORD nInBufferSize = Length + sizeof(MOUNTMGR_TARGET_NAME);
    PMOUNTMGR_TARGET_NAME TargetName = RtlAllocateHeap(GetProcessHeap(), 0, nInBufferSize);

    if (!TargetName)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    DWORD bytesReturned;

    TargetName->DeviceNameLength = (USHORT)Length;
    memcpy(TargetName->DeviceName, Src, Length);

    const wchar_t Prefix[] = L"\\\\?\\";
    const SIZE_T PrefixChars = wcslen(Prefix);
    const SIZE_T PrefixLength = PrefixChars * sizeof(wchar_t);

    LPBYTE Buffer = NULL;
    PFILE_NAME_INFORMATION NameInfo = NULL;
    SIZE_T nOutBufferSize = PrefixLength + sizeof(FILE_NAME_INFORMATION) + (MAX_PATH * sizeof(wchar_t));

    BOOL success = FALSE;

    do
    {
        if (Buffer)
            RtlFreeHeap(GetProcessHeap(), 0, Buffer);

        if (!(Buffer = RtlAllocateHeap(GetProcessHeap(), 0, nOutBufferSize)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        success = DeviceIoControl(hDevice,
                                  IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                  TargetName,
                                  nInBufferSize,
                                  Buffer + PrefixLength,
                                  (DWORD)(nOutBufferSize - PrefixLength),
                                  &bytesReturned,
                                  NULL);

        NameInfo = (PFILE_NAME_INFORMATION)(Buffer + PrefixLength);
        nOutBufferSize = NameInfo->FileNameLength + PrefixLength + sizeof(FILE_NAME_INFORMATION);
    } while (GetLastError() == ERROR_MORE_DATA);

    CloseHandle(hDevice);
    RtlFreeHeap(GetProcessHeap(), 0, TargetName);

    if (!success)
    {
        if (Buffer)
            RtlFreeHeap(GetProcessHeap(), 0, Buffer);

        // Windows 2000 and XP on network paths
#ifdef LEGACY_TRACE
        DWORD lastError = GetLastError();
        if ((lastError != ERROR_INVALID_FUNCTION) && (lastError != ERROR_NOT_SUPPORTED))
            TRACE("DeviceIoControl(IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH) failed with %ld\n", lastError);
#endif
        return FALSE;
    }

    if (MOUNTMGR_IS_VOLUME_NAME2(NameInfo->FileName, NameInfo->FileNameLength))
    {
        RtlFreeHeap(GetProcessHeap(), 0, Buffer);
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    else
    {
        *VolumeName = (wchar_t *)Buffer;
        SIZE_T Length = NameInfo->FileNameLength;
        memcpy(*VolumeName, Prefix, PrefixLength);
        memmove(Buffer + 8, NameInfo->FileName, Length);
        (*VolumeName)[(Length / sizeof(wchar_t)) + PrefixChars] = L'\0';
        TRACE("BasepGetVolumeDosLetterNameFromNTName -> [%ls]\n", *VolumeName);
        return TRUE;
    }
}

typedef enum _VOLUME_NAME_TYPE
{
    VOLUME_NAME_TYPE_INVALID = 0,
    VOLUME_NAME_TYPE_GUID = 1,
    VOLUME_NAME_TYPE_NT = 2,
    VOLUME_NAME_TYPE_DOS = 3
} VOLUME_NAME_TYPE;

/*
 * GetFinalPathNameByHandleW - Retrieves the final path for the specified file
 *
 * @param hFile       - Handle to a file or directory
 * @param lpszFilePath - Buffer to receive the path
 * @param cchFilePath - Size of the buffer in characters
 * @param dwFlags     - Format of the returned path (currently only VOLUME_NAME_DOS supported)
 *
 * @return Number of characters in the final path (excluding terminator), or 0 on failure
 */
HOTFUNC DWORD WINAPI
GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    TRACE("GetFinalPathNameByHandleW(0x%p, 0x%p, %ld, %ld)\n", hFile, lpszFilePath, cchFilePath, dwFlags);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        TRACE("GetFinalPathNameByHandleW: -> INVALID_HANDLE_VALUE\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (dwFlags & ~(FILE_NAME_OPENED | VOLUME_NAME_GUID | VOLUME_NAME_NONE | VOLUME_NAME_NT))
    {
        TRACE("GetFinalPathNameByHandleW: Unknown dwFlags: %lx\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    int flagCount = 0;
    VOLUME_NAME_TYPE volumeNameType = 0;

    if (dwFlags & VOLUME_NAME_GUID)
    {
        volumeNameType = VOLUME_NAME_TYPE_GUID;
        flagCount++;
    }

    if (dwFlags & VOLUME_NAME_NT)
    {
        volumeNameType = VOLUME_NAME_TYPE_NT;
        flagCount++;
    }

    if (dwFlags & VOLUME_NAME_NONE)
    {
        volumeNameType = VOLUME_NAME_TYPE_DOS;
        flagCount++;
    }

    if (flagCount > 1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (flagCount == 0)
        volumeNameType = VOLUME_NAME_TYPE_DOS;

    wchar_t *NtObjectName = NULL, *fileName = NULL;

    if (!BasepGetObjectNTName(hFile, &NtObjectName))
        return 0;

    if (!BasepGetFileNameInformation(hFile, FileNameInformation, &fileName))
    {
        RtlFreeHeap(GetProcessHeap(), 0, NtObjectName);
        return 0;
    }

    if (*fileName != L'\\')
    {
        TRACE("BasepGetFileNameInformation: NameInfo does not start with \\: [%ls]", fileName);
        RtlFreeHeap(GetProcessHeap(), 0, NtObjectName);
        RtlFreeHeap(GetProcessHeap(), 0, fileName);
        SetLastError(ERROR_ACCESS_DENIED);
        return 0;
    }

    if (wcslen(fileName) >= wcslen(NtObjectName))
    {
        // WTF?
        SetLastError(ERROR_BAD_PATHNAME);
    }

    NtObjectName[wcslen(NtObjectName) - wcslen(fileName)] = L'\0';

    wchar_t *VolumeName = NULL;
    SIZE_T result = 0;

    TRACE("volumeNameType=%d\n", volumeNameType);

    switch (volumeNameType)
    {
    case VOLUME_NAME_TYPE_GUID:
        if (!BasepGetVolumeGUIDFromNTName(NtObjectName, &VolumeName))
        {
            if (GetLastError() == ERROR_INVALID_FUNCTION)
                SetLastError(ERROR_PATH_NOT_FOUND);
            goto cleanup;
        }
        break;
    case VOLUME_NAME_TYPE_NT:
        VolumeName = NtObjectName;
        NtObjectName = NULL;
        break;
    case VOLUME_NAME_TYPE_DOS:
        if (!BasepGetVolumeDosLetterNameFromNTName(NtObjectName, &VolumeName))
        {
            DWORD lastError = GetLastError();
            if ((lastError != ERROR_INVALID_FUNCTION) && (lastError != ERROR_NOT_SUPPORTED))
                goto cleanup;

            if (!BasepGetVolumeDosLetterNameFromNTNameAndFileName(NtObjectName, fileName, &VolumeName))
                goto cleanup;
        }
        break;
    default:
        VolumeName = NULL;
        break;
    }

    if ((dwFlags && FILE_NAME_OPENED) == 0)
    {
        wchar_t *normalizedName = NULL, *Dest = NULL;
        if (BasepGetFileNameInformation(hFile, FileNormalizedNameInformation, &normalizedName))
        {
            RtlFreeHeap(GetProcessHeap(), 0, fileName);
            fileName = normalizedName;
        }
        else
        {
            DWORD le = GetLastError();
            if ((le != ERROR_INVALID_PARAMETER) && (le != ERROR_INVALID_LEVEL) && (le != ERROR_NOT_SUPPORTED))
                goto cleanup;

            wchar_t *Source = NULL;

            if ((volumeNameType != VOLUME_NAME_TYPE_NT) && volumeNameType)
                Source = VolumeName;
            else
            {
                wchar_t *SrcName = NtObjectName ? NtObjectName : VolumeName;

                if (!BasepGetVolumeDosLetterNameFromNTName(SrcName, &Dest) && (GetLastError() == ERROR_NOT_ENOUGH_MEMORY))
                    goto cleanup;

                if (!Dest && !BasepGetVolumeGUIDFromNTName(SrcName, &Dest))
                    goto cleanup;

                Source = Dest ? Dest : VolumeName;
            }

            SIZE_T len = (wcslen(fileName) + wcslen(Source) + 1) * sizeof(wchar_t);
            normalizedName = RtlAllocateHeap(GetProcessHeap(), 0, len);
            if (!normalizedName)
            {
                if (Dest)
                    RtlFreeHeap(GetProcessHeap(), 0, Dest);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto cleanup;
            }

            SIZE_T Length = len / sizeof(wchar_t);
            StringCchCopyW(normalizedName, Length, Source);
            StringCchCatW(normalizedName, Length, fileName);

            TRACE("GetLongPathNameW(\"%ls\", \"%ls\", %zu)\n", normalizedName, normalizedName, Length);
            SIZE_T LongPathLength = GetLongPathNameW(normalizedName, normalizedName, (DWORD)Length);
            if (!LongPathLength)
            {
                TRACE("GetLongPathNameW returned 0\n");
                RtlFreeHeap(GetProcessHeap(), 0, normalizedName);
                if (Dest)
                    RtlFreeHeap(GetProcessHeap(), 0, Dest);
                goto cleanup;
            }
            TRACE("GetLongPathNameW ->[%ls]\n", normalizedName);

            if (LongPathLength >= Length)
            {
                if (Dest)
                {
                    SIZE_T diff = LongPathLength - wcslen(Dest);
                    SIZE_T destlen = VolumeName ? wcslen(VolumeName) : 0;
                    LongPathLength = destlen + diff;
                    RtlFreeHeap(GetProcessHeap(), 0, Dest);
                }
                result = LongPathLength + 1;
                RtlFreeHeap(GetProcessHeap(), 0, normalizedName);
                SetLastError(ERROR_SUCCESS);
                goto cleanup;
            }

            Source = Dest ? Dest : VolumeName;
            SIZE_T SrcLen = wcslen(Source);
            len = (wcslen(normalizedName) - SrcLen + 1) * sizeof(wchar_t);
            memmove(normalizedName, &normalizedName[SrcLen], len);
            if (Dest)
                RtlFreeHeap(GetProcessHeap(), 0, Dest);
        }
    }

    SIZE_T Length = VolumeName ? wcslen(VolumeName) : 0;
    result = Length + wcslen(fileName);

    if (result + 1 <= cchFilePath)
    {
        *lpszFilePath = L'\0';
        if (VolumeName)
            StringCchCopyW(lpszFilePath, cchFilePath, VolumeName);
        StringCchCatW(lpszFilePath, cchFilePath, fileName);
    }
    else
    {
        result++;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

cleanup:
    if (NtObjectName)
        RtlFreeHeap(GetProcessHeap(), 0, NtObjectName);
    if (fileName)
        RtlFreeHeap(GetProcessHeap(), 0, fileName);
    if (VolumeName)
        RtlFreeHeap(GetProcessHeap(), 0, VolumeName);

    TRACE("GetFinalPathNameByHandleW -> [%ls](%d)\n", result ? lpszFilePath : NULL, (DWORD)result);
    return (DWORD)result;
}

#else
DWORD WINAPI GetFinalPathNameByHandleW(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags)
{
    TRACE("GetFinalPathNameByHandleW(0x%p, 0x%p, %ld, %ld)\n", hFile, lpszFilePath, cchFilePath, dwFlags);
    fprintf(stderr, "GetFinalPathNameByHandleW: ERROR_CALL_NOT_IMPLEMENTED\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
#endif // _UNICODE

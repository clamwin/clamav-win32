#ifndef _WIN64
.section .rdata,"dr"

// kernel32
.global __imp__LoadLibraryW@4
.global __imp__GetEnvironmentVariableW@12
.global __imp__FindFirstFileW@8
.global __imp__FindFirstFileExW@24
.global __imp__FindNextFileW@8
.global __imp__FormatMessageW@28
.global __imp__SetFileAttributesW@8
.global __imp__SetCurrentDirectoryW@4
.global __imp__RemoveDirectoryW@4
.global __imp__MoveFileW@8
.global __imp__GetSystemDirectoryW@8
.global __imp__GetShortPathNameW@12
.global __imp__GetLongPathNameW@12
.global __imp__GetFullPathNameW@16
.global __imp__GetModuleFileNameW@12
.global __imp__GetModuleHandleW@4
.global __imp__GetFileAttributesW@4
.global __imp__GetDriveTypeW@4
.global __imp__GetDiskFreeSpaceExW@16
.global __imp__GetCurrentDirectoryW@8
.global __imp__ExpandEnvironmentStringsW@12
.global __imp__DeleteFileW@4
.global __imp__CreateFileW@28
.global __imp__CreateEventW@16
.global __imp__CreateDirectoryW@8
.global __imp__CompareStringW@24
.global __imp__CreateSemaphoreW@16
.global __imp__GetVersionExW@4
.global __imp__lstrlenW@4

__imp__LoadLibraryW@4:
    .long _LoadLibraryW@4
__imp__GetEnvironmentVariableW@12:
    .long _GetEnvironmentVariableW@12
__imp__FindFirstFileW@8:
    .long _FindFirstFileW@8
__imp__FindFirstFileExW@24:
    .long _FindFirstFileExW@24
__imp__FindNextFileW@8:
    .long _FindNextFileW@8
__imp__FormatMessageW@28:
    .long _FormatMessageW@28
__imp__SetFileAttributesW@8:
    .long _SetFileAttributesW@8
__imp__SetCurrentDirectoryW@4:
    .long _SetCurrentDirectoryW@4
__imp__RemoveDirectoryW@4:
    .long _RemoveDirectoryW@4
__imp__MoveFileW@8:
    .long _MoveFileW@8
__imp__GetSystemDirectoryW@8:
    .long _GetSystemDirectoryW@8
__imp__GetShortPathNameW@12:
    .long _GetShortPathNameW@12
__imp__GetLongPathNameW@12:
    .long _GetLongPathNameW@12
__imp__GetFullPathNameW@16:
    .long _GetFullPathNameW@16
__imp__GetModuleFileNameW@12:
    .long _GetModuleFileNameW@12
__imp__GetModuleHandleW@4:
    .long _GetModuleHandleW@4
__imp__GetFileAttributesW@4:
    .long _GetFileAttributesW@4
__imp__GetDriveTypeW@4:
    .long _GetDriveTypeW@4
__imp__GetDiskFreeSpaceExW@16:
    .long _GetDiskFreeSpaceExW@16
__imp__GetCurrentDirectoryW@8:
    .long _GetCurrentDirectoryW@8
__imp__ExpandEnvironmentStringsW@12:
    .long _ExpandEnvironmentStringsW@12
__imp__DeleteFileW@4:
    .long _DeleteFileW@4
__imp__CreateFileW@28:
    .long _CreateFileW@28
__imp__CreateEventW@16:
    .long _CreateEventW@16
__imp__CreateDirectoryW@8:
    .long _CreateDirectoryW@8
__imp__CompareStringW@24:
    .long _CompareStringW@24
__imp__CreateSemaphoreW@16:
    .long _CreateSemaphoreW@16
__imp__GetVersionExW@4:
    .long _GetVersionExW@4
__imp__lstrlenW@4:
    .long _lstrlenW@4

// advapi32
.global __imp__CryptAcquireContextW@20
.global __imp__RegQueryValueExW@24
.global __imp__RegOpenKeyExW@20

__imp__CryptAcquireContextW@20:
    .long _CryptAcquireContextW@20
__imp__RegQueryValueExW@24:
    .long _RegQueryValueExW@24
__imp__RegOpenKeyExW@20:
    .long _RegOpenKeyExW@20

// user32
.global __imp__MessageBoxW@16
.global __imp__CharLowerW@4
.global __imp__CharUpperW@4

__imp__MessageBoxW@16:
    .long _MessageBoxW@16
__imp__CharLowerW@4:
    .long _CharLowerW@4
__imp__CharUpperW@4:
    .long _CharUpperW@4

// shell32
.global __imp__SHGetPathFromIDListW@8

__imp__SHGetPathFromIDListW@8:
    .long _SHGetPathFromIDListW@8

#endif // _WIN64

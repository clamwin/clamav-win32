#ifdef _WIN64
.global GetFinalPathNameByHandleW
.global __imp_GetFinalPathNameByHandleW
.global SetFileInformationByHandle
.global __imp_SetFileInformationByHandle

.align 8
__imp_GetFinalPathNameByHandleW:
    .quad GetFinalPathNameByHandleW

__imp_SetFileInformationByHandle:
    .quad SetFileInformationByHandle

#else
.global __imp__GetFinalPathNameByHandleW@16
.global __imp__SetFileInformationByHandle@16
.global __imp__RegGetValueW@28
.global _SHGetKnownFolderPath@16

.align 4
__imp__GetFinalPathNameByHandleW@16:
    .long _GetFileInformationByHandleEx

_GetFileInformationByHandleEx:
    .long _GetFileInformationByHandleEx

__imp__SetFileInformationByHandle@16:
    .long _SetFileInformationByHandle

_SetFileInformationByHandle:
    .long _SetFileInformationByHandle

__imp__RegGetValueW@28:
    .long _RegGetValueW@28

_SHGetKnownFolderPath:
    .long _SHGetKnownFolderPath

_SHGetKnownFolderPath@16:
    .long _SHGetKnownFolderPath
#endif

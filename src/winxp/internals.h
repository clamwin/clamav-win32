#ifndef _DDK_H_
#define _DDK_H_

#ifdef __GNUC__
#include <ntdef.h>
#include <winternl.h>
#include <ddk/mountmgr.h>
#else
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _REPARSE_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG  Flags;
			WCHAR  PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR  PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	} _;
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;

/*
typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

*/

#define MOUNTMGRCONTROLTYPE ((ULONG) 'm')

#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH \
  CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTMGR_TARGET_NAME {
	USHORT DeviceNameLength;
	WCHAR DeviceName[1];
} MOUNTMGR_TARGET_NAME, * PMOUNTMGR_TARGET_NAME;

typedef struct _MOUNTMGR_VOLUME_PATHS {
	ULONG MultiSzLength;
	WCHAR MultiSz[1];
} MOUNTMGR_VOLUME_PATHS, * PMOUNTMGR_VOLUME_PATHS;

typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, * PFILE_NAME_INFORMATION;

#define ObjectNameInformation 1
#define FileBasicInformation 4
#define FileNameInformation 9
#define FileDispositionInformation 13

typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, * PFILE_BASIC_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
	BOOLEAN DoDeleteFile;
} FILE_DISPOSITION_INFORMATION, * PFILE_DISPOSITION_INFORMATION;

__kernel_entry NTSYSCALLAPI NTSTATUS NtQueryInformationFile(
	HANDLE                 FileHandle,
	PIO_STATUS_BLOCK       IoStatusBlock,
	PVOID                  FileInformation,
	ULONG                  Length,
	FILE_INFORMATION_CLASS FileInformationClass
);

__kernel_entry NTSYSCALLAPI NTSTATUS NtSetInformationFile(
	HANDLE                 FileHandle,
	PIO_STATUS_BLOCK       IoStatusBlock,
	PVOID                  FileInformation,
	ULONG                  Length,
	FILE_INFORMATION_CLASS FileInformationClass
);

#endif

#endif // _DDK_H_

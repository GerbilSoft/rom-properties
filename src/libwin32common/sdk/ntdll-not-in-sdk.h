/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ntdll-not-in-sdk.h: NTDLL definitions not in the Windows SDK.           *
 ***************************************************************************/

#pragma once

// NTDLL functions not declared in winternl.h.
#include "../RpWin32_sdk.h"
#include <winternl.h>

extern "C" {

#ifndef _MSC_VER
// MinGW might not have __kernel_entry defined.
#  define __kernel_entry
#endif

// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwdeletefile
__kernel_entry NTSTATUS NTAPI NtDeleteFile(
	POBJECT_ATTRIBUTES ObjectAttributes
);

// NOTE: winternl.h does define FILE_INFORMATION_CLASS,
// but the official Windows SDK only has FileDirectoryInformation...
// MinGW-w64 *does* have the other entries.
// (TODO: Which MinGW-w64 version added it?)
#ifdef __MINGW32__
#  define FILE_INFORMATION_CLASS_NT FILE_INFORMATION_CLASS
#else /* !__MINGW32__ */
typedef enum _FILE_INFORMATION_CLASS_NT {
	FileDirectoryInformation_NT = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileObjectIdInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileQuotaInformation,
	FileReparsePointInformation,
	FileNetworkOpenInformation,
	FileAttributeTagInformation,
	FileTrackingInformation,
	FileIdBothDirectoryInformation,
	FileIdFullDirectoryInformation,
	FileValidDataLengthInformation,
	FileShortNameInformation,
	FileIoCompletionNotificationInformation,
	FileIoStatusBlockRangeInformation,
	FileIoPriorityHintInformation,
	FileSfioReserveInformation,
	FileSfioVolumeInformation,
	FileHardLinkInformation,
	FileProcessIdsUsingFileInformation,
	FileNormalizedNameInformation,
	FileNetworkPhysicalNameInformation,
	FileIdGlobalTxDirectoryInformation,
	FileIsRemoteDeviceInformation,
	FileUnusedInformation,
	FileNumaNodeInformation,
	FileStandardLinkInformation,
	FileRemoteProtocolInformation,
	FileRenameInformationBypassAccessCheck,
	FileLinkInformationBypassAccessCheck,
	FileVolumeNameInformation,
	FileIdInformation,
	FileIdExtdDirectoryInformation,
	FileReplaceCompletionInformation,
	FileHardLinkFullIdInformation,
	FileIdExtdBothDirectoryInformation,
	FileDispositionInformationEx,
	FileRenameInformationEx,
	FileRenameInformationExBypassAccessCheck,
	FileDesiredStorageClassInformation,
	FileStatInformation,
	FileMemoryPartitionInformation,
	FileStatLxInformation,
	FileCaseSensitiveInformation,
	FileLinkInformationEx,
	FileLinkInformationExBypassAccessCheck,
	FileStorageReserveIdInformation,
	FileCaseSensitiveInformationForceAccessCheck,
	FileKnownFolderInformation,
	FileMaximumInformation
} FILE_INFORMATION_CLASS_NT, *PFILE_INFORMATION_CLASS_NT;
#endif /* __MINGW32__ */

#ifndef __MINGW32__
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_directory_information
typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	WCHAR         FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
#endif /* !__MINGW32__ */

// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntquerydirectoryfile
__kernel_entry NTSTATUS NTAPI NtQueryDirectoryFile(
	HANDLE                 FileHandle,
	HANDLE                 Event,
	PIO_APC_ROUTINE        ApcRoutine,
	PVOID                  ApcContext,
	PIO_STATUS_BLOCK       IoStatusBlock,
	PVOID                  FileInformation,
	ULONG                  Length,
	FILE_INFORMATION_CLASS FileInformationClass,
	BOOLEAN                ReturnSingleEntry,
	PUNICODE_STRING        FileName,
	BOOLEAN                RestartScan
);

#ifndef __MINGW32__
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_file_basic_information
typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
#endif /* !__MINGW32__ */

// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationfile
__kernel_entry NTSTATUS NTAPI NtQueryInformationFile(
	HANDLE                    FileHandle,
	PIO_STATUS_BLOCK          IoStatusBlock,
	PVOID                     FileInformation,
	ULONG                     Length,
	FILE_INFORMATION_CLASS_NT FileInformationClass
);

// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntsetinformationfile
__kernel_entry NTSTATUS NTAPI NtSetInformationFile(
	HANDLE                    FileHandle,
	PIO_STATUS_BLOCK          IoStatusBlock,
	PVOID                     FileInformation,
	ULONG                     Length,
	FILE_INFORMATION_CLASS_NT FileInformationClass
);

__kernel_entry NTSTATUS NTAPI NtWriteFile(
	HANDLE           FileHandle,
	HANDLE           Event,
	PIO_APC_ROUTINE  ApcRoutine,
	PVOID            ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID            Buffer,
	ULONG            Length,
	PLARGE_INTEGER   ByteOffset,
	PULONG           Key
);

}

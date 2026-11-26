/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 * (Win32 implementation)                                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RecursiveScan.hpp"
#include "FileSystem.hpp"

#include "common.h"
#include "d_type.h"

// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/NtDllDlopen.hpp"

#include <ntstatus.h>
#include <winternl.h>	// for NT API
#include "libwin32common/sdk/ntdll-not-in-sdk.h"

// FILE_SHARE_VALID_FLAGS isn't defined in the Win32 SDK for some reason...
// (MinGW-w64 has it in ntdef.h, winnt.h, and ddk/wdm.h.)
#ifndef FILE_SHARE_VALID_FLAGS
#  define FILE_SHARE_VALID_FLAGS (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#endif /* !FILE_SHARE_VALID_FLAGS */

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
#include <memory>
using std::pair;
using std::tstring;
using std::unique_ptr;

// Uninitialized vector class
#include "uvector.h"

// RecursiveScan isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_RecursiveScan_ForceLinkage;
	unsigned char RP_LibRpFile_RecursiveScan_ForceLinkage;
}

namespace LibRpFile {

static LibWin32Common::NtDllDlopen ntDllDlopen;

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, *.jxl, and "version.txt".
 *
 * Win32 non-NT implementation: Uses FindFirstFile() and FindNextFile().
 *
 * @param state		[in/out] Current scan state (use zero-init struct on first call)
 * @param path		[in] Path to scan
 * @param mode		[in] Scan mode
 * @param user_data	[in,opt] User data for the callback function
 * @param callback	[in,opt] Callback function (at the end to make it easier to use lambdas)
 * @return RecursiveScanRet_t
 */
static int recursiveScan_int_nonNT(RecursiveScanState_t &state,
	const TCHAR *path, RecursiveScanMode mode,
	void *user_data, RecursiveScanCallback_t callback)
{
	tstring findFilter(path);
	findFilter += _T("\\*");

	WIN32_FIND_DATA findFileData;
	HANDLE hFindFile = FindFirstFile(findFilter.c_str(), &findFileData);
	if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) {
		// Error finding files.
		return -1;
	}

	tstring fullpath(path);
	fullpath += _T('\\');
	const size_t fullpath_base_len = fullpath.size();

	do {
		// Skip "." and "..".
		if (findFileData.cFileName[0] == _T('.') &&
			(findFileData.cFileName[1] == _T('\0') ||
			 (findFileData.cFileName[1] == _T('.') && findFileData.cFileName[2] == _T('\0'))))
		{
			continue;
		}

		// Check if this is a regular file.
		const uint8_t d_type = FileSystem::win32_attrs_to_d_type(findFileData.dwFileAttributes);
		switch (d_type) {
			default:
				// Not supported.
				FindClose(hFindFile);
				return RSE_FOUND_UNEXPECTED_FILE_TYPE;

			case DT_REG:
				// Regular file: Supported.
				state.fileCount++;
				break;

			case DT_DIR:
				// Directory: Supported.
				state.dirCount++;
				break;

			case DT_LNK:
				// Symbolic link.
				// NOT SUPPORTED; there should not be symbolic links
				// in the cache directories!
				FindClose(hFindFile);
				return RSE_FOUND_SYMBOLIC_LINK;
		}

		// Check the filename to see if we should delete it.
		if (d_type == DT_REG) {
			// Thumbs.db files can be deleted.
			if (!_tcsicmp(findFileData.cFileName, _T("Thumbs.db"))) {
				goto isok;
			}

			// Check the extension.
			size_t len = _tcslen(findFileData.cFileName);
			if (len <= 4) {
				// Filename is too short. This is bad.
				FindClose(hFindFile);
				return RSE_FOUND_UNEXPECTED_FILE_EXT;
			}

			const TCHAR *pExt = &findFileData.cFileName[len-4];
			if (_tcsicmp(pExt, _T(".png")) != 0 &&
			    _tcsicmp(pExt, _T(".jpg")) != 0 &&
			    _tcsicmp(pExt, _T(".jxl")) != 0 &&
			    _tcsicmp(findFileData.cFileName, _T("version.txt")) != 0)
			{
				// Extension is not valid.
				FindClose(hFindFile);
				return RSE_FOUND_UNEXPECTED_FILE_EXT;
			}

			// All checks pass.
		}

	isok:
		fullpath.resize(fullpath_base_len);
		fullpath += findFileData.cFileName;

		switch (d_type) {
			default:
				assert(!"Should not happen...");
				break;

			case DT_REG:
				if (mode == RecursiveScanMode::ScanAndDelete) {
					// Delete this file.

					// Check if the file has the legacy read-only attribute set.
					BOOL bRet = TRUE;
					const DWORD attrs = GetFileAttributes(fullpath.c_str());
					if (unlikely(attrs == INVALID_FILE_ATTRIBUTES)) {
						// GetFileAttributes() failed...
						bRet = FALSE;
					} else if (attrs & FILE_ATTRIBUTE_READONLY) {
						// Read-only is set. Remove it.
						bRet = SetFileAttributes(fullpath.c_str(), (attrs & ~FILE_ATTRIBUTE_READONLY));
					}
					if (bRet) {
						bRet = DeleteFile(fullpath.c_str());
					}

					if (!bRet) {
						// Error clearing the read-only attribute
						// and/or deleting the file.
						state.fileErrs++;
					}
				}
				break;

			case DT_DIR:
				// Recursively scan this directory.
				int ret = recursiveScan(state, fullpath.c_str(), mode, user_data, callback);
				if (ret != 0) {
					// recursiveScan() failed. Return immediately.
					FindClose(hFindFile);
					return ret;
				}

				if (mode == RecursiveScanMode::ScanAndDelete) {
					// Remove this directory.
					BOOL bRet = RemoveDirectory(fullpath.c_str());
					if (!bRet) {
						// Error removing the directory...
						state.dirErrs++;
					}
				}
				break;
		}

		if (callback) {
			callback(state, user_data);
		}
	} while (FindNextFile(hFindFile, &findFileData));
	FindClose(hFindFile);

	return 0;
}

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, *.jxl, and "version.txt".
 *
 * Win32 NT implementation: Uses NtQueryDirectoryFile().
 *
 * @param state		[in/out] Current scan state (use zero-init struct on first call)
 * @param hDir		[in] Directory handle to scan
 * @param mode		[in] Scan mode
 * @param user_data	[in,opt] User data for the callback function
 * @param callback	[in,opt] Callback function (at the end to make it easier to use lambdas)
 * @return RecursiveScanRet_t
 */
static int recursiveScan_int_NT(RecursiveScanState_t &state,
	HANDLE hDir, RecursiveScanMode mode,
	void *user_data, RecursiveScanCallback_t callback)
{
	// Assuming hDir has already had its attributes checked to
	// ensure it is a directory and not a reparse point.

	// Directory entry buffer (64 KB initial starting size; 4 MB maximum)
	static constexpr size_t BUF_SIZE_INIT = 64U * 1024U;
	static constexpr size_t BUF_SIZE_MAX = 4096U * 1024U;
	rp::uvector<uint8_t> buf;
	buf.resize(BUF_SIZE_INIT);

	IO_STATUS_BLOCK iosb;
	bool restartScan = true;

	while (true) {
		NTSTATUS status = ntDllDlopen.pfn_NtQueryDirectoryFile(
			hDir,				// FileHandle
			nullptr,			// Event
			nullptr,			// ApcRoutine
			nullptr,			// ApcContext
			&iosb,				// IoStatusBlock
			buf.data(),			// FileInformation
			static_cast<ULONG>(buf.size()),	// Length
			FileDirectoryInformation,	// FileInformationClass
			FALSE,				// ReturnSingleEntry
			nullptr,			// FileName
			restartScan);			// RestartScan

		if (status == STATUS_NO_MORE_FILES) {
			// Finished scanning files.
			break;
		} else if (status == STATUS_BUFFER_OVERFLOW ||
			   status == STATUS_INFO_LENGTH_MISMATCH)
		{
			// Need to increase the buffer size and restart the scan.
			// (Should only happen on the first run...)
			if (buf.size() >= BUF_SIZE_MAX) {
				// Buffer size is too big!
				return -ENOMEM;
			}
			buf.resize(buf.size() * 2);
			restartScan = true;
			continue;
		} else if (!NT_SUCCESS(status)) {
			// NtQueryDirectoryFile() failed.
			// TODO: NTSTATUS_to_POSIX()?
			return -EIO;
		}

		// Scan should not be restarted on the next iteration.
		restartScan = false;

		// Process the entries in the buffer.
		// NOTE: fdi->FileNameLength is in *bytes*, not TCHARs!
		ULONG curEntryOffset = 0;
		const FILE_DIRECTORY_INFORMATION *fdi = reinterpret_cast<const FILE_DIRECTORY_INFORMATION*>(buf.data());
		while (true) {
			// Convert to TCHAR length to make things easier to work with.
			assert(fdi->FileNameLength % 2 == 0);
			const ULONG tfnamelen = fdi->FileNameLength / sizeof(TCHAR);

			// Skip "." and "..".
			if ((tfnamelen == 1 && fdi->FileName[0] == _T('.')) ||
			    (tfnamelen == 2 && !memcmp(fdi->FileName, _T(".."), 2*sizeof(TCHAR))))
			{
				if (fdi->NextEntryOffset == 0) {
					// No more entries in the buffer.
					break;
				}
				curEntryOffset += fdi->NextEntryOffset;
				if (curEntryOffset + sizeof(FILE_DIRECTORY_INFORMATION) > buf.size()) {
					// Next entry is out of bounds?
					break;
				}
				fdi = reinterpret_cast<const FILE_DIRECTORY_INFORMATION*>(&buf[curEntryOffset]);
				continue;
			}

			// Check if this is a regular file.
			const uint8_t d_type = FileSystem::win32_attrs_to_d_type(fdi->FileAttributes);
			switch (d_type) {
				default:
					// Not supported.
					return RSE_FOUND_UNEXPECTED_FILE_TYPE;

				case DT_REG:
					// Regular file: Supported.
					state.fileCount++;
					break;

				case DT_DIR:
					// Directory: Supported.
					state.dirCount++;
					break;

				case DT_LNK:
					// Symbolic link.
					// NOT SUPPORTED; there should not be symbolic links
					// in the cache directories!
					return RSE_FOUND_SYMBOLIC_LINK;
			}

			// Check the filename to see if we should delete it.
			if (d_type == DT_REG) {
				// Thumbs.db files can be deleted.
				if (tfnamelen == 9 && !_tcsnicmp(fdi->FileName, _T("Thumbs.db"), 9)) {
					goto isok;
				}

				// Check the extension.
				if (tfnamelen <= 4) {
					// Filename is too short. This is bad.
					return RSE_FOUND_UNEXPECTED_FILE_EXT;
				}

				// NOTE: The last filename in a batch of entries might not be
				// NULL-terminated, so we have to use _tcsnicmp() here.
				const TCHAR *pExt = &fdi->FileName[tfnamelen-4];
				if (_tcsnicmp(pExt, _T(".png"), 4) != 0 &&
				    _tcsnicmp(pExt, _T(".jpg"), 4) != 0 &&
				    _tcsnicmp(pExt, _T(".jxl"), 4) != 0 &&
				    (tfnamelen != 11 || _tcsnicmp(fdi->FileName, _T("version.txt"), 11) != 0))
				{
					// Extension is not valid.
					return RSE_FOUND_UNEXPECTED_FILE_EXT;
				}

				// All checks pass.
			}

		isok:
			UNICODE_STRING us_path;
			us_path.Length = static_cast<USHORT>(fdi->FileNameLength);
			us_path.MaximumLength = us_path.Length;
			us_path.Buffer = const_cast<PWSTR>(&fdi->FileName[0]);

			OBJECT_ATTRIBUTES oa;
			InitializeObjectAttributes(&oa, &us_path, 0, hDir, nullptr);

			switch (d_type) {
				default:
					assert(!"Should not happen...");
					break;

				case DT_REG:
					if (mode == RecursiveScanMode::ScanAndDelete) {
						// Delete this file.
						// Check if the file has the legacy read-only attribute set.
						status = STATUS_SUCCESS;
						if (fdi->FileAttributes & FILE_ATTRIBUTE_READONLY) {
							// Read-only is set. Remove it.
							// NOTE: We don't need to explicitly specify OBJ_CASE_INSENSITIVE here
							// because we're using the filename directly from the directory.
							HANDLE hFile = nullptr;
							status = ntDllDlopen.pfn_NtOpenFile(
								&hFile,		// FileHandle
								FILE_WRITE_ATTRIBUTES,	// DesiredAccess
								&oa,			// ObjectAttributes
								&iosb,			// IoStatusBlock
								FILE_SHARE_VALID_FLAGS,	// ShareAccess [TODO]
								FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT);		// OpenOptions
							if (NT_SUCCESS(status)) {
								FILE_BASIC_INFORMATION fbi = {{}, {}, {}, {}, fdi->FileAttributes & ~FILE_ATTRIBUTE_READONLY};
								status = ntDllDlopen.pfn_NtSetInformationFile(
									hFile,	// FileHandle
									&iosb,	// IoStatusBlock
									&fbi,	// FileInformation
									static_cast<ULONG>(sizeof(fbi)),	// Length
									FileBasicInformation);	// FileInformationClass
							}
						}
						if (NT_SUCCESS(status)) {
							status = ntDllDlopen.pfn_NtDeleteFile(&oa);
						}

						if (!NT_SUCCESS(status)) {
							// Error clearing the read-only attribute
							// and/or deleting the file.
							state.fileErrs++;
						}
					}
					break;

				case DT_DIR: {
					// Recursively scan this directory.
					HANDLE hSubDir = nullptr;
					status = ntDllDlopen.pfn_NtOpenFile(
						&hSubDir,		// FileHandle
						FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE | DELETE, // DesiredAccess
						&oa,			// ObjectAttributes
						&iosb,			// IoStatusBlock
						FILE_SHARE_VALID_FLAGS,	// ShareAccess [TODO]
						FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT);		// OpenOptions
					if (!NT_SUCCESS(status)) {
						// Failed to open the subdirectory...
						// TODO: NTSTATUS_to_POSIX()?
						return -EIO;
					}

					// Verify that this is a regular directory and not a reparse point.
					FILE_BASIC_INFORMATION fbi;
					status = ntDllDlopen.pfn_NtQueryInformationFile(
						hSubDir,		// FileHandle
						&iosb,		// IoStatusBlock
						&fbi,		// FileInformation
						sizeof(fbi),	// Length
						FileBasicInformation);
					if (!NT_SUCCESS(status)) {
						// Failed to get attributes.
						// TODO: NTSTATUS_to_POSIX()?
						ntDllDlopen.pfn_NtClose(hDir);
						return -EIO;
					}

					if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
						// It's a reparse point! Not supported!
						return RSE_FOUND_SYMBOLIC_LINK;
					} else if (!(fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						// Not a directory...
						return -ENOTDIR;
					}

					// Start the recursive scan.
					// NOTE: Unlike the POSIX version, recursiveScan_int_NT will *not*
					// close hDir before returning, so we have to close it ourselves.
					int ret = recursiveScan_int_NT(state, hSubDir, mode, user_data, callback);
					if (ret != 0) {
						// recursiveScan() failed. Return immediately.
						ntDllDlopen.pfn_NtClose(hSubDir);
						return ret;
					}

					if (mode == RecursiveScanMode::ScanAndDelete) {
						// Remove this directory.
						status = ntDllDlopen.pfn_NtDeleteFile(&oa);
						if (!NT_SUCCESS(status)) {
							// Error removing the directory...
							state.dirErrs++;
						}
					}
					ntDllDlopen.pfn_NtClose(hSubDir);
					break;
				}
			}

			if (callback) {
				callback(state, user_data);
			}

			// Next entry.
			if (fdi->NextEntryOffset == 0) {
				// No more entries in the buffer.
				break;
			}
			curEntryOffset += fdi->NextEntryOffset;
			if (curEntryOffset + sizeof(FILE_DIRECTORY_INFORMATION) > buf.size()) {
				// Next entry is out of bounds?
				break;
			}
			fdi = reinterpret_cast<const FILE_DIRECTORY_INFORMATION*>(&buf[curEntryOffset]);
		}
	}

	return 0;
}

/**
 * Is this Windows NT?
 * @return True if NT; false if not.
 */
static inline bool isWindowsNT(void)
{
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4996)
#endif /* _MSC_VER */
	// Get the OS version information.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* _MSC_VER */

	return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, *.jxl, and "version.txt".
 *
 * Win32 implementation: Uses FindFirstFile() and FindNextFile().
 *
 * @param state		[in/out] Current scan state (use zero-init struct on first call)
 * @param path		[in] Path to scan
 * @param mode		[in] Scan mode
 * @param user_data	[in,opt] User data for the callback function
 * @param callback	[in,opt] Callback function (at the end to make it easier to use lambdas)
 * @return RecursiveScanRet_t
 */
int recursiveScan(RecursiveScanState_t &state,
	const TCHAR *path, RecursiveScanMode mode,
	void *user_data, RecursiveScanCallback_t callback)
{
	// TODO: init once?
	if (ntDllDlopen.init() != 0) {
		// NTDLL could not be initialized...
		if (!isWindowsNT()) {
			// Not Windows NT: try the path-based fallback.
			// (On NT, NTDLL.DLL is *always* available, so if it's
			// not initializing for some reason, that's an error.)
			return recursiveScan_int_nonNT(state, path, mode, user_data, callback);
		}
		return -ENOTSUP;
	}

	// NtOpenFile() requires "\\??\\", whereas
	// Win32 extended paths use "\\\\?\\".
	// We'll need to copy the path and adjust it.
	tstring ts_path;
	if (!_tcsncmp(path, _T("\\\\?\\"), 4)) {
		// path starts with "\\\\?\\".
		// Copy and adjust.
		ts_path = path;
		ts_path[1] = _T('?');
	} else {
		// FIXME: If it's an absolute path, prepend "\\??\\".
		// Otherwise, we need to adjust it...
		return -EINVAL;
	}

	// Open the directory using NT API.
	// NOTE: Length is in *bytes*, not characters.
	UNICODE_STRING us_path;
	us_path.Length = static_cast<USHORT>(ts_path.size() * sizeof(TCHAR));
	us_path.MaximumLength = us_path.Length + sizeof(TCHAR);
	us_path.Buffer = const_cast<PWSTR>(ts_path.c_str());

	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, &us_path, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

	IO_STATUS_BLOCK iosb;
	HANDLE hDir = nullptr;
	NTSTATUS status = ntDllDlopen.pfn_NtOpenFile(
		&hDir,			// FileHandle
		FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY | FILE_TRAVERSE, // DesiredAccess
		&oa,			// ObjectAttributes
		&iosb,			// IoStatusBlock
		FILE_SHARE_VALID_FLAGS,	// ShareAccess [TODO]
		FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT);		// OpenOptions
	if (!NT_SUCCESS(status)) {
		// Failed to open the directory...
		// TODO: NTSTATUS_to_POSIX()?
		return -EIO;
	}

	// Verify that this is a regular directory and not a reparse point.
	FILE_BASIC_INFORMATION fbi;
	status = ntDllDlopen.pfn_NtQueryInformationFile(
		hDir,		// FileHandle
		&iosb,		// IoStatusBlock
		&fbi,		// FileInformation
		sizeof(fbi),	// Length
		FileBasicInformation);
	if (!NT_SUCCESS(status)) {
		// Failed to get attributes.
		// TODO: NTSTATUS_to_POSIX()?
		ntDllDlopen.pfn_NtClose(hDir);
		return -EIO;
	}

	if (fbi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		// It's a reparse point! Not supported!
		return RSE_FOUND_SYMBOLIC_LINK;
	} else if (!(fbi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		// Not a directory...
		return -ENOTDIR;
	}

	// Start the recursive scan.
	// NOTE: Unlike the POSIX version, recursiveScan_int_NT will *not*
	// close hDir before returning, so we have to close it ourselves.
	int ret = recursiveScan_int_NT(state, hDir, mode, user_data, callback);
	ntDllDlopen.pfn_NtClose(hDir);
	return ret;
}

}

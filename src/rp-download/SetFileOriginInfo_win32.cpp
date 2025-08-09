/******************************************************************************
 * ROM Properties Page shell extension. (rp-download)                         *
 * SetFileOriginInfo_win32.cpp: setFileOriginInfo() function. (Win32 version) *
 *                                                                            *
 * Copyright (c) 2016-2025 by David Korth.                                    *
 * SPDX-License-Identifier: GPL-2.0-or-later                                  *
 ******************************************************************************/

#include "stdafx.h"
#include "SetFileOriginInfo.hpp"

#ifndef _WIN32
#  error SetFileOriginInfo_posix.cpp is for Windows systems, not POSIX.
#endif /* !_WIN32 */

// libwin32common
#include "libwin32common/userdirs.hpp"
#include "libwin32common/w32err.hpp"
#include "libwin32common/w32time.h"

// librptext
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"

// C includes
#include <sys/utime.h>

// C++ STL classes
using std::string;
using std::tstring;

#ifdef UNICODE
// NTDLL function calls
#include <io.h>
#include <winternl.h>

// FIXME: RTL_CONSTANT_STRING macro is in ntdef.h,
// but #include'ing it breaks things.
// This version is from MinGW-w64 12.0.0.
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), const_cast<PWSTR>(s) }

// NTDLL functions not declared in winternl.h.
extern "C" {

#ifndef _MSC_VER
// MinGW might not have __kernel_entry defined.
#  define __kernel_entry
#endif

__kernel_entry NTSYSCALLAPI NTSTATUS NtWriteFile(
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

typedef const OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;
__kernel_entry NTSYSCALLAPI NTSTATUS NtDeleteFile(
	PCOBJECT_ATTRIBUTES ObjectAttributes
);

typedef __typeof__(NtCreateFile) *pfn_NtCreateFile_t;
typedef __typeof__(NtWriteFile)  *pfn_NtWriteFile_t;
typedef __typeof__(NtClose)      *pfn_NtClose_t;
typedef __typeof__(NtDeleteFile) *pfn_NtDeleteFile_t;

}
#endif /* UNICODE */

namespace RpDownload {

/**
 * Get the storeFileOriginInfo setting from rom-properties.conf.
 *
 * Default value is true.
 *
 * @return storeFileOriginInfo setting.
 */
static bool getStoreFileOriginInfo(void)
{
	static constexpr bool default_value = true;
	DWORD dwRet;
	TCHAR szValue[64];

	// Get the config filename.
	// NOTE: Not cached, since rp-download downloads one file per run.
	// NOTE: This is sitll readable even when running as Low integrity.
	tstring conf_filename = U82T_s(LibWin32Common::getConfigDirectory());
	if (conf_filename.empty()) {
		// Empty filename...
		return default_value;
	}
	// Add a trailing slash if necessary.
	if (conf_filename.at(conf_filename.size()-1) != DIR_SEP_CHR) {
		conf_filename += DIR_SEP_CHR;
	}
	conf_filename += _T("rom-properties\\rom-properties.conf");

	dwRet = GetPrivateProfileString(_T("Downloads"), _T("StoreFileOriginInfo"),
		NULL, szValue, _countof(szValue), conf_filename.c_str());

	if ((dwRet == 5 && !_tcsicmp(szValue, _T("false"))) ||
	    (dwRet == 1 && szValue[0] == _T('0')))
	{
		// Disabled.
		return false;
	}

	// Other value. Assume enabled.
	return true;
}

/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param file Open file (Must be writable)
 * @param url Origin URL
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int setFileOriginInfo(FILE *file, const TCHAR *url, time_t mtime)
{
	// NOTE: Even if one of the xattr functions fails, we'll
	// continue with others and setting mtime. The first error
	// will be returned at the end of the function.
	int err = 0;

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#  error 32-bit time_t is not supported. Get a newer compiler.
#endif

	// We need the Win32 file handle to use Win32 API instead of MSVCRT functions.
	HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(fileno(file)));

#ifdef UNICODE
	// Check if storeFileOriginInfo is enabled.
	const bool storeFileOriginInfo = getStoreFileOriginInfo();
	if (storeFileOriginInfo) do {
		// Create an ADS named "Zone.Identifier".
		// References:
		// - https://cqureacademy.com/blog/alternate-data-streams
		// - https://stackoverflow.com/questions/46141321/open-alternate-data-stream-ads-from-file-handle-or-file-id
		// - https://stackoverflow.com/a/46141949

		// NOTE: ntdll.lib isn't present in all build environments.
		// Use GetModuleHandle() and GetProcAddress() instead.
		HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
		if (!hNtDll) {
			// No NTDLL.DLL? Maybe this is Win9x...
			break;
		}

#define NTDLL_LOAD(fn) pfn_##fn##_t pfn_##fn = reinterpret_cast<pfn_##fn##_t>(GetProcAddress(hNtDll, #fn))
		NTDLL_LOAD(NtCreateFile);
		NTDLL_LOAD(NtWriteFile);
		NTDLL_LOAD(NtClose);
		NTDLL_LOAD(NtDeleteFile);

		if (!pfn_NtCreateFile || !pfn_NtWriteFile ||
		    !pfn_NtClose || !pfn_NtDeleteFile)
		{
			// At least one function pointer is missing.
			break;
		}

		IO_STATUS_BLOCK iosb;
		UNICODE_STRING ObjectName = RTL_CONSTANT_STRING(L":Zone.Identifier");

		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, &ObjectName, 0, hFile, nullptr);

		HANDLE hAds = nullptr;
		NTSTATUS status = pfn_NtCreateFile(
			&hAds,			// FileHandle
			FILE_GENERIC_WRITE,	// DesiredAccess
			&oa,			// ObjectAttributes
			&iosb,			// IoStatusBlock
			nullptr,		// AllocationSize
			FILE_ATTRIBUTE_NORMAL,	// FileAttributes
			FILE_SHARE_READ,	// ShareAccess
			FILE_OVERWRITE_IF,	// CreateDisposition
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,	// CreateOptions
			nullptr,		// EaBuffer
			0);			// EaLength
		if (status < 0) {
			// Error opening the ADS.
			// TODO: Convert NTSTATUS to POSIX?
			/*err = w32err_to_posix(GetLastError());
			if (err == 0) {
				err = EIO;
			}*/
			err = EIO;
			break;
		}

		// Write a zone identifier.
		// NOTE: Assuming UTF-8 encoding.
		// FIXME: Chromium has some shenanigans for Windows 10.
		// Reference: https://github.com/chromium/chromium/blob/55f44515cd0b9e7739b434d1c62f4b7e321cd530/components/services/quarantine/quarantine_win.cc
		static constexpr char zoneID_hdr[] = "[ZoneTransfer]\r\nZoneID=3\r\nHostUrl=";
		string s_zoneID;
		s_zoneID.reserve(sizeof(zoneID_hdr) + _tcslen(url) + 2 + 16);
		s_zoneID = zoneID_hdr;
		s_zoneID += T2U8(url);
		s_zoneID += "\r\n";

		status = pfn_NtWriteFile(
			hAds,					// FileHandle
			nullptr,				// Event
			nullptr,				// ApcRoutine
			nullptr,				// ApcContext
			&iosb,					// IoStatusBlock
			s_zoneID.data(),			// Buffer
			static_cast<DWORD>(s_zoneID.size()),	// Length
			nullptr,				// ByteOffset
			nullptr);				// Key
		pfn_NtClose(hAds);

		if (status < 0) {
			// Error writing the stream data.
			// Delete the stream.
			pfn_NtDeleteFile(&oa);

			// TODO: Convert NTSTATUS to Win32?
			err = -EIO;
		}
	} while (0);
#endif /* UNICODE */

	if (mtime >= 0) {
		// Flush the file before setting the times to ensure
		// that MSVCRT doesn't write anything afterwards.
		::fflush(file);

		// SetFileTime() requires FILETIME format.
		// NOTE: We only need to adjust mtime, not atime.
		FILETIME ft_mtime;
		UnixTimeToFileTime(mtime, &ft_mtime);

		if (!SetFileTime(hFile, nullptr, nullptr, &ft_mtime)) {
			// Error setting the file time.
			err = w32err_to_posix(GetLastError());
			if (err == 0) {
				err = EIO;
			}
		}
	}

	return -err;
}

} //namespace RpDownload

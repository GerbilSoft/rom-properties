/******************************************************************************
 * ROM Properties Page shell extension. (rp-download)                         *
 * SetFileOriginInfo_win32.cpp: setFileOriginInfo() function. (Win32 version) *
 *                                                                            *
 * Copyright (c) 2016-2023 by David Korth.                                    *
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
	static const bool default_value = true;
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
 * @param file Open file. (Must be writable.)
 * @param filename Filename. [FIXME: Make it so we don't need this on Windows.]
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int setFileOriginInfo(FILE *file, const TCHAR *filename, const TCHAR *url, time_t mtime)
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

	// Check if storeFileOriginInfo is enabled.
	const bool storeFileOriginInfo = getStoreFileOriginInfo();
	if (storeFileOriginInfo) do {
		// Create an ADS named "Zone.Identifier".
		// References:
		// - https://cqureacademy.com/blog/alternate-data-streams
		// - https://stackoverflow.com/questions/46141321/open-alternate-data-stream-ads-from-file-handle-or-file-id
		// - https://stackoverflow.com/a/46141949
		// FIXME: NtCreateFile() seems to have issues, and we end up
		// getting STATUS_INVALID_PARAMETER (0xC000000D).
		// We'll use a regular CreateFile() call for now.
		tstring tfilename = filename;
		tfilename += _T(":Zone.Identifier");
		HANDLE hAds = CreateFile(
			tfilename.c_str(),	// lpFileName
			GENERIC_WRITE,		// dwDesiredAccess
			FILE_SHARE_READ,	// dwShareMode
			nullptr,		// lpSecurityAttributes
			CREATE_ALWAYS,		// dwCreationDisposition
			FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes
			nullptr);		// hTemplateFile
		if (!hAds || hAds == INVALID_HANDLE_VALUE) {
			// Error opening the ADS.
			err = w32err_to_posix(GetLastError());
			if (err == 0) {
				err = EIO;
			}
			break;
		}

		// Write a zone identifier.
		// NOTE: Assuming UTF-8 encoding.
		// FIXME: Chromium has some shenanigans for Windows 10.
		// Reference: https://github.com/chromium/chromium/blob/55f44515cd0b9e7739b434d1c62f4b7e321cd530/components/services/quarantine/quarantine_win.cc
		static const char zoneID_hdr[] = "[ZoneTransfer]\r\nZoneID=3\r\nHostUrl=";
		string s_zoneID;
		s_zoneID.reserve(sizeof(zoneID_hdr) + _tcslen(url) + 2 + 16);
		s_zoneID = zoneID_hdr;
		s_zoneID += T2U8(url);
		s_zoneID += "\r\n";

		DWORD dwBytesWritten = 0;
		BOOL bRet = WriteFile(hAds, s_zoneID.data(), static_cast<DWORD>(s_zoneID.size()),
			&dwBytesWritten, nullptr);
		if ((!bRet || dwBytesWritten != static_cast<DWORD>(s_zoneID.size())) && err == 0) {
			// Some error occurred...
			err = w32err_to_posix(GetLastError());
			if (err == 0) {
				err = EIO;
			}
		}

		CloseHandle(hAds);
	} while (0);

	if (mtime >= 0) {
		// TODO: 100ns timestamp precision for access time?
		struct _utimbuf utimbuf;
		utimbuf.actime = time(nullptr);
		utimbuf.modtime = mtime;

		// Flush the file before setting the times to ensure
		// that MSVCRT doesn't write anything afterwards.
		::fflush(file);

		// Set the mtime.
		int ret = _futime(fileno(file), &utimbuf);
		if (ret != 0 && err == 0) {
			err = errno;
			if (err == 0) {
				err = EIO;
			}
		}
	}

	return -err;
}

} //namespace RpDownload

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpFile_win32.cpp: Standard file object. (Win32 implementation)          *
 * setOriginInfo() function.                                               *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "../RpFile.hpp"
#include "../RpFile_p.hpp"

// Config class (for storeFileOriginInfo)
#include "../../config/Config.hpp"

// librpbase
#include "TextFuncs_wchar.hpp"

// libwin32common
#include "libwin32common/w32err.h"
#include "libwin32common/w32time.h"

// C includes.
#include <fcntl.h>

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::wstring;

namespace LibRpBase {

/** File properties (NON-VIRTUAL) **/

/**
 * Set the file origin info.
 * This uses xattrs on Linux and ADS on Windows.
 * @param url Origin URL.
 * @param mtime If >= 0, this value is set as the mtime.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpFile::setOriginInfo(const std::string &url, time_t mtime)
{
	RP_D(RpFile);

	// NOTE: Even if one of the xattr functions fails, we'll
	// continue with others and setting mtime. The first error
	// will be returned at the end of the function.
	int err = 0;

	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
# error 32-bit time_t is not supported. Get a newer compiler.
#endif

	// NOTE: This will force a configuration timestamp check.
	const Config *const config = Config::instance();
	const bool storeFileOriginInfo = config->storeFileOriginInfo();
	if (storeFileOriginInfo) {
		// Create an ADS named "Zone.Identifier".
		// References:
		// - https://cqureacademy.com/blog/alternate-data-streams
		// - https://stackoverflow.com/questions/46141321/open-alternate-data-stream-ads-from-file-handle-or-file-id
		// - https://stackoverflow.com/a/46141949
		// FIXME: NtCreateFile() seems to have issues, and we end up
		// getting STATUS_INVALID_PARAMETER (0xC000000D).
		// We'll use a regular CreateFile() call for now.
		tstring tfilename = U82T_s(d->filename);
		tfilename += _T(":Zone.Identifier");
		HANDLE hAds = CreateFile(
			tfilename.c_str(),	// lpFileName
			GENERIC_WRITE,		// dwDesiredAccess
			FILE_SHARE_READ,	// dwShareMode
			nullptr,		// lpSecurityAttributes
			CREATE_ALWAYS,		// dwCreationDisposition
			FILE_ATTRIBUTE_NORMAL,	// dwFlagsAndAttributes
			nullptr);		// hTemplateFile
		if (hAds && hAds != INVALID_HANDLE_VALUE) {
			// Write a zone identifier.
			// NOTE: Assuming UTF-8 encoding.
			// FIXME: Chromium has some shenanigans for Windows 10.
			// Reference: https://github.com/chromium/chromium/blob/55f44515cd0b9e7739b434d1c62f4b7e321cd530/components/services/quarantine/quarantine_win.cc
			static const char zoneID_hdr[] = "[ZoneTransfer]\r\nZoneID=3\r\nHostUrl=";
			std::string s_zoneID;
			s_zoneID.reserve(sizeof(zoneID_hdr) + url.size() + 2);
			s_zoneID = zoneID_hdr;
			s_zoneID += url;
			s_zoneID += "\r\n";
			DWORD dwBytesWritten = 0;
			BOOL bRet = WriteFile(hAds, s_zoneID.data(),
				static_cast<DWORD>(s_zoneID.size()),
				&dwBytesWritten, nullptr);
			if ((!bRet || dwBytesWritten != static_cast<DWORD>(s_zoneID.size())) && err != 0) {
				// Some error occurred...
				m_lastError = w32err_to_posix(GetLastError());
				err = m_lastError;
			}
			CloseHandle(hAds);
		} else {
			// Error opening the ADS.
			if (err != 0) {
				m_lastError = w32err_to_posix(GetLastError());
				err = m_lastError;
			}
		}
	}

	if (mtime >= 0) {
		// Convert to FILETIME.
		FILETIME ft_mtime;
		UnixTimeToFileTime(mtime, &ft_mtime);

		// Flush the file before setting the times to ensure
		// that Windows doesn't write anything afterwards.
		FlushFileBuffers(d->file);

		// Set the mtime.
		BOOL bRet = SetFileTime(d->file, nullptr, nullptr, &ft_mtime);
		if (!bRet && err != 0) {
			m_lastError = w32err_to_posix(GetLastError());
			err = m_lastError;
		}
	}

	return -err;
}

}

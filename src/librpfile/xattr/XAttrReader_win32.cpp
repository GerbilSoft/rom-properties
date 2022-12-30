/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_win32.cpp: Extended Attribute reader (Windows version)      *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

// Windows SDK
#include "libwin32common/RpWin32_sdk.h"

// librpbase
#include "librpbase/TextFuncs_wchar.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;

// XAttrReader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern uint8_t RP_LibRpFile_XAttrReader_impl_ForceLinkage;
	uint8_t RP_LibRpFile_XAttrReader_impl_ForceLinkage;
}

// ADS functions (Windows Vista and later)
typedef HANDLE (WINAPI *PFNFINDFIRSTSTREAMW)(_In_ LPCWSTR lpFileName, _In_ STREAM_INFO_LEVELS InfoLevel, _Out_ LPVOID lpFindStreamData, DWORD dwFlags);
typedef HANDLE (WINAPI *PFNFINDNEXTSTREAMW)(_In_ HANDLE hFindStream, _Out_ LPVOID lpFindStreamData);

namespace LibRpFile {

/** XAttrReaderPrivate **/

XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
	: filename(U82W_c(filename))
	, lastError(0)
	, hasLinuxAttributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, linuxAttributes(0)
	, dosAttributes(0)
{
	init();
}

XAttrReaderPrivate::XAttrReaderPrivate(const wchar_t *filename)
	: filename(filename)
	, lastError(0)
	, hasLinuxAttributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, linuxAttributes(0)
	, dosAttributes(0)
{
	init();
}

/**
 * Initialize attributes.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::init(void)
{
	// NOTE: While there is a GetFileInformationByHandle() function,
	// there's no easy way to get alternate data streams using a
	// handle from the file, so we'll just use the filename.

	// Load the attributes.
	loadLinuxAttrs();
	loadDosAttrs();
	loadGenericXattrs();
	return 0;
}

/**
 * Load Linux attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadLinuxAttrs(void)
{
	// FIXME: WSL support?
	linuxAttributes = 0;
	hasLinuxAttributes = false;
	return -ENOTSUP;
}

/**
 * Load MS-DOS attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadDosAttrs(void)
{
	dosAttributes = GetFileAttributes(filename.c_str());
	hasDosAttributes = (dosAttributes != INVALID_FILE_ATTRIBUTES);
	return (hasDosAttributes ? 0 : -ENOTSUP);
}

/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs(void)
{
	genericXAttrs.clear();

	// Windows Vista: Use FindFirstStream().
	// TODO: Implement an XP version using BackupRead().
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32"));
	assert(hKernel32 != nullptr);
	if (!hKernel32)
		return -ENOMEM;

	PFNFINDFIRSTSTREAMW pfnFindFirstStreamW = (PFNFINDFIRSTSTREAMW)GetProcAddress(hKernel32, "FindFirstStreamW");
	PFNFINDNEXTSTREAMW pfnFindNextStreamW = (PFNFINDNEXTSTREAMW)GetProcAddress(hKernel32, "FindNextStreamW");
	if (pfnFindFirstStreamW && pfnFindNextStreamW) {
		// We have FindFirstStreamW().
		WIN32_FIND_STREAM_DATA fsd;
		HANDLE hADS = pfnFindFirstStreamW(filename.c_str(), FindStreamInfoStandard, &fsd, 0);
		if (!hADS || hADS == INVALID_HANDLE_VALUE) {
			// FindFirstStream() failed.
			return -ENOENT;
		}

		do {
			if (!_tcscmp(fsd.cStreamName, _T("::$DATA"))) {
				// Primary data stream. Ignore it.
				continue;
			}

			// TODO: Read the stream data. (or at least up to 256 bytes/chars)
			// For now, just set the name.
			// TODO: Remove ":$DATA" from the attribute name?
			genericXAttrs.emplace(T2U8(fsd.cStreamName), "dummy");
		} while (pfnFindNextStreamW(hADS, &fsd));

		FindClose(hADS);
	} else {
		// We don't have FindFirstStreamW().
		// TODO: BackupRead().
		return -ENOTSUP;
	}

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}

}

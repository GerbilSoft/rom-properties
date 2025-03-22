/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * XAttrReader_win32.cpp: Extended Attribute reader (Windows version)      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "XAttrReader.hpp"
#include "XAttrReader_p.hpp"

// Windows SDK
#include "libwin32common/RpWin32_sdk.h"

// librptext
#include "librptext/wchar.hpp"

// C++ STL classes
using std::string;
using std::tstring;
using std::unique_ptr;

// XAttrReader isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
	unsigned char RP_LibRpFile_XAttrReader_impl_ForceLinkage;
}

// ADS functions (Windows Vista and later)
typedef HANDLE (WINAPI *pfnFindFirstStreamW_t)(_In_ LPCWSTR lpFileName, _In_ STREAM_INFO_LEVELS InfoLevel, _Out_ LPVOID lpFindStreamData, DWORD dwFlags);
typedef HANDLE (WINAPI *pfnFindNextStreamW_t)(_In_ HANDLE hFindStream, _Out_ LPVOID lpFindStreamData);

namespace LibRpFile {

// Valid MS-DOS attributes
static constexpr unsigned int VALID_DOS_ATTRIBUTES_FAT = \
	FILE_ATTRIBUTE_READONLY | \
	FILE_ATTRIBUTE_HIDDEN | \
	FILE_ATTRIBUTE_SYSTEM | \
	FILE_ATTRIBUTE_ARCHIVE;

/** XAttrReaderPrivate **/

#ifdef _UNICODE
XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
	: XAttrReaderPrivate(U82T_c(filename))
{}

XAttrReaderPrivate::XAttrReaderPrivate(const wchar_t *filename)
#else /* !_UNICODE */
XAttrReaderPrivate::XAttrReaderPrivate(const char *filename)
#endif /* _UNICODE */
	: filename(filename)
	, lastError(0)
	, hasExt2Attributes(false)
	, hasXfsAttributes(false)
	, hasDosAttributes(false)
	, hasGenericXAttrs(false)
	, ext2Attributes(0)
	, xfsXFlags(0)
	, xfsProjectId(0)
	, dosAttributes(0)
	, validDosAttributes(0)
{
	// NOTE: While there is a GetFileInformationByHandle() function,
	// there's no easy way to get alternate data streams using a
	// handle from the file, so we'll just use the filename.

	// Load the attributes.
	loadExt2Attrs();
	loadXfsAttrs();
	loadDosAttrs();
	loadGenericXattrs();
}

/**
 * Load Ext2 attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadExt2Attrs(void)
{
	// FIXME: WSL support?
	ext2Attributes = 0;
	hasExt2Attributes = false;
	return -ENOTSUP;
}

/**
 * Load XFS attributes, if available.
 * @param fd File descriptor of the open file
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadXfsAttrs(void)
{
	// FIXME: WSL support?
	xfsXFlags = 0;
	xfsProjectId = 0;
	hasXfsAttributes = false;
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
	if (!hasDosAttributes) {
		// No MS-DOS attributes?
		validDosAttributes = 0;
		return -ENOTSUP;
	}

	// NOTE: Assuming generic FAT attributes if unable to determine the actual file system.
	validDosAttributes = VALID_DOS_ATTRIBUTES_FAT;

	// Get the volume path name.
	TCHAR volumePathName[MAX_PATH];
	if (!GetVolumePathName(filename.c_str(), volumePathName, _countof(volumePathName))) {
		// Unable to get the volume path name.
		return 0;
	}

	// Get the volume information.
	DWORD fileSystemFlags = 0;
	if (!GetVolumeInformation(
		volumePathName,		// lpRootPathName
		nullptr,		// lpVolumeNameBuffer
		0,			// nVolumeNameSize
		nullptr,		// lpVolumeSerialNumber
		nullptr,		// lpMaximumComponentLength
		&fileSystemFlags,	// lpFileSystemFlags
		nullptr,		// lpFileSystemNameBuffer
		0))			// nFileSystemNameSize
	{
		// Failed to get volume information.
		return 0;
	}

	// Check the file system flags.
	if (fileSystemFlags & FILE_FILE_COMPRESSION) {
		// Compression is supported.
		validDosAttributes |= FILE_ATTRIBUTE_COMPRESSED;
	}
	if (fileSystemFlags & FILE_SUPPORTS_ENCRYPTION) {
		// Encryption is supported.
		validDosAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
	}

	// Valid MS-DOS attributes obtained.
	return 0;
}

#ifdef UNICODE
/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * FindFirstStreamW() version; requires Windows Vista or later.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs_FindFirstStreamW(void)
{
	// Windows Vista: Use FindFirstStream().
	// Windows XP: Use BackupRead(). [TODO]
	HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != nullptr);
	if (!hKernel32) {
		return -ENOMEM;
	}

	pfnFindFirstStreamW_t pfnFindFirstStreamW = reinterpret_cast<pfnFindFirstStreamW_t>(
		GetProcAddress(hKernel32, "FindFirstStreamW"));
	pfnFindNextStreamW_t pfnFindNextStreamW = reinterpret_cast<pfnFindNextStreamW_t>(
		GetProcAddress(hKernel32, "FindNextStreamW"));
	if (!pfnFindFirstStreamW || !pfnFindNextStreamW) {
		// Unable to retrieve the procedure addresses.
		return -ENOTSUP;
	}

	// We have FindFirstStreamW().
	WIN32_FIND_STREAM_DATA fsd;
	HANDLE hFindADS = pfnFindFirstStreamW(filename.c_str(), FindStreamInfoStandard, &fsd, 0);
	if (!hFindADS || hFindADS == INVALID_HANDLE_VALUE) {
		// FindFirstStream() failed.
		return -ENOENT;
	}

	tstring ads_filename = filename;
	union {
		uint8_t u8[257 * sizeof(TCHAR)];
		char ch[257];
		wchar_t wch[257];
		unsigned int zero_data;
	} ads_data;
	bool is_unicode = false;

	do {
		// We're only allowing $DATA streams.
		const size_t streamName_len = _tcslen(fsd.cStreamName);
		if (streamName_len < 7) {
			// Stream name is too small.
			continue;
		}
		if (_tcscmp(&fsd.cStreamName[streamName_len - 6], L":$DATA") != 0) {
			// Not a $DATA stream.
			continue;
		}
		// Remove ":$DATA" from the stream name.
		fsd.cStreamName[streamName_len - 6] = L'\0';

		// If the stream name is ":", it's the primary data stream.
		// This doesn't count as an extended attribute.
		if (!_tcscmp(fsd.cStreamName, _T(":"))) {
			// Primary data stream. Ignore it.
			continue;
		}

		// Read up to 257 TCHARs from the alternate data stream.
		// If we get 257, truncate it to 253 and add "...".
		ads_filename.resize(filename.size());
		ads_filename += fsd.cStreamName;
		HANDLE hStream = CreateFile(ads_filename.c_str(),
			GENERIC_READ, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hStream && hStream != INVALID_HANDLE_VALUE) {
			// Read up to 257 TCHARs.
			DWORD bytesRead;
			if (ReadFile(hStream, ads_data.u8, sizeof(ads_data.u8), &bytesRead, nullptr) &&
			    bytesRead > 0 && bytesRead <= sizeof(ads_data.u8))
			{
				// Data read. Check if it's likely to be Unicode or not.
				if (IsTextUnicode(ads_data.u8, bytesRead, nullptr)) {
					// It's likely Unicode.
					is_unicode = true;
					// Truncate at TCHAR[253] if it's >= 257 TCHARs.
					if (bytesRead >= 257 * sizeof(TCHAR)) {
						ads_data.wch[253] = L'.';
						ads_data.wch[254] = L'.';
						ads_data.wch[255] = L'.';
						ads_data.wch[256] = L'\0';
					} else {
						// Ensure the string is NULL-terminated.
						// TODO: Make sure it's a multiple of 2 bytes.
						ads_data.wch[bytesRead/sizeof(TCHAR)] = L'\0';
					}
				} else {
					// It's likely *not* Unicode.
					is_unicode = false;
					// Truncate at char[253] if it's >= 257 chars.
					if (bytesRead >= 257 * sizeof(char)) {
						ads_data.ch[253] = '.';
						ads_data.ch[254] = '.';
						ads_data.ch[255] = '.';
						ads_data.ch[256] = '\0';
					} else {
						// Ensure the string is NULL-terminated.
						ads_data.ch[bytesRead/sizeof(char)] = '\0';
					}
				}
			} else {
				// Unable to read data from the stream.
				ads_data.zero_data = 0;
			}
			CloseHandle(hStream);
		} else {
			// Unable to open the data stream.
			ads_data.zero_data = 0;
		}

		// The leading ':' and trailing ":$DATA" will be removed
		// from the attribute name.
		// FIXME: Currently allowing blank stream names. Is this valid?
		string s_name, s_value;
		if (fsd.cStreamName[0] != L'\0') {
			s_name.assign(W2U8(&fsd.cStreamName[1]));
		}
		if (ads_data.zero_data != 0) {
			s_value.assign(is_unicode ? W2U8(ads_data.wch) : A2U8(ads_data.ch));
		}
		genericXAttrs.emplace(std::move(s_name), std::move(s_value));
	} while (pfnFindNextStreamW(hFindADS, &fsd));

	FindClose(hFindADS);

	// Extended attributes retrieved.
	hasGenericXAttrs = true;
	return 0;
}
#endif /* UNICODE */

/**
 * Load generic xattrs, if available.
 * (POSIX xattr on Linux; ADS on Windows)
 * BackupRead() version.
 * Internal fd (filename on Windows) must be set.
 * @return 0 on success; negative POSIX error code on error.
 */
int XAttrReaderPrivate::loadGenericXattrs_BackupRead(void)
{
	// TODO: Implement this.
	return -ENOSYS;
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

#ifdef UNICODE
	// Try FindFirstStreamW() first.
	int ret = loadGenericXattrs_FindFirstStreamW();
	if (ret != -ENOTSUP) {
		// Succeeded, or an error unrelated to
		// FindFirstStreamW() not being available.
		return ret;
	}
#endif /* UNICODE */

	// Try BackupRead().
	return loadGenericXattrs_BackupRead();
}

} // namespace LibRpFile

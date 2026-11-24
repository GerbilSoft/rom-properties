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

// d_type compatibility values
#include "d_type.h"

// C++ STL classes
using std::forward_list;
using std::pair;
using std::tstring;

// libwin32common
#include "libwin32common/RpWin32_sdk.h"

// RecursiveScan isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpFile_RecursiveScan_ForceLinkage;
	unsigned char RP_LibRpFile_RecursiveScan_ForceLinkage;
}

namespace LibRpFile {

/**
 * Recursively scan a directory for cache files to delete.
 * This finds *.png, *.jpg, *.jxl, and "version.txt".
 *
 * Win32 implementation: Uses FindFirstFile() and FindNextFile().
 *
 * @param path	[in] Path to scan.
 * @param rlist	[in/out] Return list for filenames and file types. (d_type)
 * @return 0 on success; non-zero on error.
 */
int recursiveScan(const TCHAR *path, forward_list<pair<tstring, uint8_t> > &rlist)
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
			case DT_DIR:
				// Supported.
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

		// Add the filename and d_type.
		rlist.emplace_front(fullpath, d_type);

		// If this is a directory, recursively scan it.
		// This is done *after* adding the directory because forward_list
		// enumerates items in reverse order.
		if (d_type == DT_DIR) {
			// Recursively scan it.
			recursiveScan(fullpath.c_str(), rlist);
		}
	} while (FindNextFile(hFindFile, &findFileData));
	FindClose(hFindFile);

	return 0;
}

}

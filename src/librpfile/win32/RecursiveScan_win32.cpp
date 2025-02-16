/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 * (Win32 implementation)                                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RecursiveScan.hpp"
#include "FileSystem.hpp"

// d_type compatibility values
#include "d_type.h"

// C++ STL classes
using std::forward_list;
using std::pair;
using std::tstring;

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

	do {
		// Skip "." and "..".
		if (findFileData.cFileName[0] == _T('.') &&
			(findFileData.cFileName[1] == _T('\0') ||
			 (findFileData.cFileName[1] == _T('.') && findFileData.cFileName[2] == _T('\0'))))
		{
			continue;
		}

		// Make sure we should delete this file.
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// Thumbs.db files can be deleted.
			if (!_tcsicmp(findFileData.cFileName, _T("Thumbs.db"))) {
				goto isok;
			}

			// Check the extension.
			size_t len = _tcslen(findFileData.cFileName);
			if (len <= 4) {
				// Filename is too short. This is bad.
				FindClose(hFindFile);
				return -EIO;
			}

			const TCHAR *pExt = &findFileData.cFileName[len-4];
			if (_tcsicmp(pExt, _T(".png")) != 0 &&
			    _tcsicmp(pExt, _T(".jpg")) != 0 &&
			    _tcsicmp(pExt, _T(".jxl")) != 0 &&
			    _tcsicmp(findFileData.cFileName, _T("version.txt")) != 0)
			{
				// Extension is not valid.
				FindClose(hFindFile);
				return -EIO;
			}

			// All checks pass.
		}

	isok:
		tstring fullFileName(path);
		fullFileName += _T('\\');
		fullFileName += findFileData.cFileName;

		// Add the filename and d_type.
		const auto &elem = rlist.emplace_front(std::move(fullFileName),
			FileSystem::win32_attrs_to_d_type(findFileData.dwFileAttributes));

		// If this is a directory, recursively scan it.
		// This is done *after* adding the directory because forward_list
		// enumerates items in reverse order.
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recursively scan it.
			recursiveScan(elem.first.c_str(), rlist);
		}
	} while (FindNextFile(hFindFile, &findFileData));
	FindClose(hFindFile);

	return 0;
}

}

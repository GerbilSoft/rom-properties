/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 * (POSIX implementation)                                                  *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RecursiveScan.hpp"
#include "FileSystem.hpp"

// d_type compatibility values
#include "d_type.h"

// C includes
#include <sys/types.h>
#include <dirent.h>

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
 * POSIX implementation: Uses readdir().
 *
 * @param path	[in] Path to scan.
 * @param rlist	[in/out] Return list for filenames and file types. (d_type)
 * @return 0 on success; non-zero on error.
 */
int recursiveScan(const TCHAR *path, forward_list<pair<tstring, uint8_t> > &rlist)
{
	DIR *const pdir = opendir(path);
	if (!pdir) {
		// Error opening the directory.
		return -errno;
	}

	// Search for files and directories.
	struct dirent *dirent;
	while ((dirent = readdir(pdir)) != nullptr) {
		// Skip "." and "..".
		if (dirent->d_name[0] == '.' &&
		    (dirent->d_name[1] == '\0' ||
		     (dirent->d_name[1] == '.' && dirent->d_name[2] == '\0')))
		{
			continue;
		}

		string fullpath(path);
		fullpath += '/';
		fullpath += dirent->d_name;

		// Check if this is a regular file.
		uint8_t d_type = dirent->d_type;
		switch (d_type) {
			default:
				// Not supported.
				// TODO: Better error message.
				closedir(pdir);
				return -EIO;

			case DT_REG:
			case DT_DIR:
				// Supported.
				break;

			case DT_LNK:
				// Symbolic link. Dereference it and check again.
				d_type = FileSystem::get_file_d_type(fullpath.c_str(), true);
				switch (d_type) {
					default:
						// Not supported.
						// TODO: Better error message.
						closedir(pdir);
						return -EIO;

					case DT_REG:
					case DT_DIR:
						// Supported.
						break;

					case DT_UNKNOWN:
						// This is probably a dangling symlink.
						// Delete it anyway.
						break;
				}
				break;

			case DT_UNKNOWN:
				// Unknown. Use stat().
				d_type = FileSystem::get_file_d_type(fullpath.c_str(), false);
				switch (d_type) {
					default:
						// Not supported.
						// TODO: Better error message.
						closedir(pdir);
						return -EIO;

					case DT_REG:
					case DT_DIR:
						// Supported.
						break;

					case DT_LNK:
						// Symbolic link. Dereference it and check again.
						d_type = FileSystem::get_file_d_type(fullpath.c_str(), true);
						switch (d_type) {
							default:
								// Not supported.
								// TODO: Better error message.
								closedir(pdir);
								return -EIO;

							case DT_REG:
							case DT_DIR:
								// Supported.
								break;

							case DT_UNKNOWN:
								// This is probably a dangling symlink.
								// Delete it anyway.
								break;
						}
						break;
				}
				break;
		}

		// Check the filename to see if we should delete it.
		if (d_type == DT_REG || d_type == DT_UNKNOWN) {
			// Thumbs.db files can be deleted.
			if (!strcasecmp(dirent->d_name, "Thumbs.db")) {
				goto isok;
			}

			// Check the extension.
			const size_t len = strlen(dirent->d_name);
			if (len <= 4) {
				// Filename is too short. This is bad.
				closedir(pdir);
				return -EIO;
			}

			const char *pExt = &dirent->d_name[len-4];
			if (strcasecmp(pExt, ".png") != 0 &&
			    strcasecmp(pExt, ".jpg") != 0 &&
			    strcasecmp(pExt, ".jxl") != 0 &&
			    strcasecmp(dirent->d_name, "version.txt") != 0)
			{
				// Extension is not valid.
				closedir(pdir);
				return -EIO;
			}

			// All checks pass.
		}

	isok:
		// Add the filename and file type.
		rlist.emplace_front(fullpath, d_type);

		// If this is a directory, recursively scan it.
		// This is done *after* adding the directory because forward_list
		// enumerates items in reverse order.
		if (d_type == DT_DIR) {
			// Recursively scan the directory.
			recursiveScan(fullpath.c_str(), rlist);
		}
	};
	closedir(pdir);

	return 0;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RecursiveScan.cpp: Recursively scan for cache files to delete.          *
 * (POSIX implementation)                                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librpfile.h"
#include "RecursiveScan.hpp"
#include "FileSystem.hpp"

// d_type compatibility values
#include "d_type.h"

// Other rom-properties libraries
#include "librptext/strfuncs.h"

// C includes
#include <dirent.h>
#include <fcntl.h>	// AT_FDCWD, O_NOFOLLOW, AT_REMOVEDIR
#include <sys/types.h>
#include <unistd.h>

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ STL classes
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
 * This finds *.png, *.jpg, and "version.txt".
 *
 * POSIX implementation: Uses readdir().
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
	// Open the directory, with these flags:
	// - O_RDONLY: Read-only
	// - O_NOFOLLOW: Don't follow symlinks on the last path component
	// - O_DIRECTORY: Fail if it's not a directory
	// - O_CLOEXEC: Close on exec()
	int dfd = open(path, O_RDONLY | O_NOFOLLOW | O_DIRECTORY | O_CLOEXEC);
	if (dfd < 0) {
		// Error opening the directory.
		int err = -errno;
		if (err == -ELOOP) {
			// Unexpected symlink!
			err = RSE_FOUND_SYMBOLIC_LINK;
		}
		return err;
	}

	// Open the directory as a DIR*.
	// NOTE: dfd will be owned by the DIR* and will
	// be closed when closedir() is called.
	DIR *const pdir = fdopendir(dfd);
	if (!pdir) {
		// Error opening the directory.
		close(dfd);
		return -errno;
	}

	string fullpath(path);
	fullpath += '/';
	const size_t fullpath_base_len = fullpath.size();

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

		fullpath.resize(fullpath_base_len);
		fullpath += dirent->d_name;

		// Check if this is a regular file.
		uint8_t d_type = dirent->d_type;
		switch (d_type) {
			default:
				// Not supported.
				closedir(pdir);
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
				closedir(pdir);
				return RSE_FOUND_SYMBOLIC_LINK;

			case DT_UNKNOWN:
				// Unknown. Use lstat().
				d_type = FileSystem::get_file_d_type(fullpath.c_str(), true);
				switch (d_type) {
					default:
						// Not supported.
						closedir(pdir);
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
						closedir(pdir);
						return RSE_FOUND_SYMBOLIC_LINK;
				}
				break;
		}

		// Check the filename to see if we should delete it.
		if (d_type == DT_REG) {
			// Thumbs.db files can be deleted.
			if (!rp_ascii_strcasecmp(dirent->d_name, "Thumbs.db")) {
				goto isok;
			}

			// Check the extension.
			const size_t len = strlen(dirent->d_name);
			if (len <= 4) {
				// Filename is too short. This is bad.
				closedir(pdir);
				return RSE_FOUND_UNEXPECTED_FILE_EXT;
			}

			const char *pExt = &dirent->d_name[len-4];
			if (rp_ascii_strcasecmp(pExt, ".png") != 0 &&
			    rp_ascii_strcasecmp(pExt, ".jpg") != 0 &&
			    rp_ascii_strcasecmp(pExt, ".jxl") != 0 &&
			    rp_ascii_strcasecmp(dirent->d_name, "version.txt") != 0)
			{
				// Extension is not valid.
				closedir(pdir);
				return RSE_FOUND_UNEXPECTED_FILE_EXT;
			}

			// All checks pass.
		}

	isok:
		switch (d_type) {
			default:
				assert(!"Should not happen...");
				break;

			case DT_REG:
				if (mode == RecursiveScanMode::ScanAndDelete) {
					// Delete this file.
					int ret = unlinkat(dfd, dirent->d_name, 0);
					if (ret != 0) {
						// Error deleting the file...
						state.fileErrs++;
					}
				}
				break;

			case DT_DIR:
				// Recursively scan this directory.
				int ret = recursiveScan(state, fullpath.c_str(), mode, user_data, callback);
				if (ret != 0) {
					// recursiveScan() failed. Return immediately.
					closedir(pdir);
					return ret;
				}
				if (mode == RecursiveScanMode::ScanAndDelete) {
					// Remove this directory.
					int ret = unlinkat(dfd, dirent->d_name, AT_REMOVEDIR);
					if (ret != 0) {
						// Error removing the directory...
						state.dirErrs++;
					}
				}
				break;
		}

		if (callback) {
			callback(state, user_data);
		}
	};

	closedir(pdir);
	return 0;
}

}

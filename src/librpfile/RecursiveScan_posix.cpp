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
#include <fcntl.h>	// AT_FDCWD, O_NOFOLLOW
#include <sys/stat.h>	// stat(), statx()
#include <sys/types.h>

// C includes (C++ namespace)
#include <cerrno>
#include <cstring>

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
 * @return 0 on success; negative POSIX error code or positive RecursiveScanError on error.
 */
int recursiveScan(const TCHAR *path, forward_list<pair<tstring, uint8_t> > &rlist)
{
	// Open the directory, but make sure we don't follow
	// a symbolic link on the last path component.
	int dfd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
	if (dfd < 0) {
		// Error opening the directory.
		return -errno;
	}

	// Verify that this is, in fact, a directory.
	uint16_t mode = 0;
#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(dfd, "", AT_EMPTY_PATH, STATX_TYPE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_TYPE)) {
		// statx() failed and/or did not return the file type.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	mode = sbx.stx_mode;
#else /* !HAVE_STATX */
	struct stat sb;
	if (fstat(dfd, &sb) != 0) {
		// stat() failed.
		int ret = -errno;
		return (ret != 0 ? ret : -EIO);
	}
	mode = sb.st_mode;
#endif /* HAVE_STATX */
	if (!S_ISDIR(mode)) {
		// Not a directory.
		close(dfd);
		return -ENOTDIR;
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
			case DT_DIR:
				// Supported.
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
					case DT_DIR:
						// Supported.
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

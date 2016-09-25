/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.cpp: FST printer.                                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "FstPrint.hpp"

// libromdata
#include "TextFuncs.hpp"
#include "disc/IFst.hpp"
using LibRomData::rp_string;

// C includes.
#include <stdint.h>

// C++ includes.
#include <sstream>
#include <vector>
using std::endl;
using std::ostream;
using std::vector;

namespace LibRomData {

struct fst_stats {
	unsigned int dirs;
	unsigned int files;

	fst_stats() : dirs(0), files(0) { }
};

/**
 * Print an FST to an ostream.
 * @param fst		[in]FST to print.
 * @param os		[in] ostream.
 * @param path		[in] Directory path.
 * @param level		[in] Current directory level. (0 == root)
 * @param tree_lines	[in/out] Levels with tree lines.
 * @param stats		[out] Statistics.
 * @return 0 on success; negative POSIX error code on error.
 */
static int fstPrint(IFst *fst, ostream &os, const rp_string &path, int level, vector<uint8_t> &tree_lines, fst_stats &stats)
{
	// Open the root path in the FST.
	IFst::Dir *dirp = fst->opendir(path);
	if (!dirp) {
		// Error opening the directory.
		// TODO: ENOENT, EIO, or what?
		return -EIO;
	}

	// Print the tree lines for the directory name.
	if (level == 0) {
		// Root directory.
		os << path << endl;
	} else {
		for (int i = level-1; i > 0; i--) {
			// Print the tree lines.
			os << "\xE2\x94\x9C   ";
		}

		// Final tree line for the directory entry.
		if (tree_lines[level-1]) {
			// More files are present in the previous directory.
			os << "\xE2\x94\x9C";
		} else {
			// No more files are present in the previous directory.
			os << "\xE2\x94\x94";
		}
		os << "\xE2\x94\x80\xE2\x94\x80 ";

		// Find the last slash.
		size_t slash_pos = path.find_last_of(_RP_CHR('/'));
		if ((slash_pos == 0 && path.size() > 1) && slash_pos != rp_string::npos) {
			// Found the last slash.
			os << path.substr(slash_pos + 1);
		} else {
			// Root directory.
			os << path;
		}

		os << endl;
	}

	// Read the directory entries.
	IFst::DirEnt *dirent = fst->readdir(dirp);
	while (dirent != nullptr) {
		// Print the tree lines.
		for (int i = 0; i < level; i++)
		{
			if (tree_lines[i]) {
				// Directory tree exists for this segment.
				os << "\xE2\x94\x82   ";
			} else {
				// No directory tree for this segment.
				os << "    ";
			}
		}

		// NOTE: The next DirEnt is obtained in one of the
		// following if/else branches. We can't obtain more
		// than one DirEnt at a time, since the DirEnt is
		// actually stored within the Dir object.
		if (dirent->type == DT_DIR) {
			// Subdirectory.
			stats.dirs++;

			rp_string subdir = path;
			if (path.empty() || (path[path.size()-1] != _RP_CHR('/'))) {
				// Append a trailing slash.
				subdir += _RP_CHR('/');
			}
			subdir += dirent->name;

			// Check if any more entries are present.
			dirent = fst->readdir(dirp);
			tree_lines.push_back(dirent ? 1 : 0);

			// Print the subdirectory.
			int ret = fstPrint(fst, os, subdir, level+1, tree_lines, stats);
			if (ret != 0) {
				// ERROR
				return ret;
			}

			// Remove the extra tree line.
			tree_lines.resize(tree_lines.size()-1);
		} else {
			// File.
			stats.files++;

			// Save the filename. [TODO: other attributes]
			rp_string name = dirent->name;

			// Check if any more entries are present.
			dirent = fst->readdir(dirp);
			if (dirent) {
				// More files are present in the current directory.
				os << "\xE2\x94\x9C";
			} else {
				// No more files are present in the current directory.
				os << "\xE2\x94\x94";
			}
			os << "\xE2\x94\x80\xE2\x94\x80 ";

			// Print the filename.
			// TODO: Attributes.
			os << name << endl;
		}
	}

	// Directory printed.
	return 0;
}

/**
 * Print an FST to an ostream.
 * @param fst FST to print.
 * @param os ostream.
 * @return 0 on success; negative POSIX error code on error.
 */
int fstPrint(IFst *fst, ostream &os)
{
	if (!fst) {
		// Invalid parameters.
		return -EINVAL;
	}

	std::vector<uint8_t> tree_lines;
	tree_lines.reserve(16);

	fst_stats stats;

	int ret = fstPrint(fst, os, _RP("/"), 0, tree_lines, stats);
	if (ret != 0) {
		return ret;
	}

	os << endl <<
		stats.dirs << ' ' << (stats.dirs == 1 ? "directory" : "directories") << ", " <<
		stats.files << ' ' << (stats.files == 1 ? "file" : "files") << endl;
	return 0;
}

};

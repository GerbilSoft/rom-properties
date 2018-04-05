/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.cpp: FST printer.                                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "FstPrint.hpp"

// librpbase
#include "librpbase/TextFuncs.hpp"
#include "librpbase/disc/IFst.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes.
#include <stdint.h>

#if defined(_MSC_VER) && _MSC_VER < 1700
// MSVC 2012 added inttypes.h.
// Older versions don't have it.
#define PRIu64 "I64u"
#define PRIX64 "I64X"
#else
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

// C++ includes.
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
using std::ostream;
using std::ostringstream;
using std::setw;
using std::string;
using std::vector;

namespace LibRomData {

struct FstFileCount {
	unsigned int dirs;
	unsigned int files;
};

/**
 * Print an FST to an ostream.
 * @param fst		[in]FST to print.
 * @param os		[in] ostream.
 * @param path		[in] Directory path.
 * @param level		[in] Current directory level. (0 == root)
 * @param tree_lines	[in/out] Levels with tree lines.
 * @param fc		[out] File count.
 * @return 0 on success; negative POSIX error code on error.
 */
static int fstPrint(IFst *fst, ostream &os, const string &path,
	int level, vector<uint8_t> &tree_lines, FstFileCount &fc)
{
	// Open the root path in the FST.
	IFst::Dir *dirp = fst->opendir(path);
	if (!dirp) {
		// Error opening the directory.
		// TODO: ENOENT, EIO, or what?
		return -EIO;
	}

	// NOTE: The directory name is printed by the caller,
	// except for the root directory.
	if (level == 0) {
		// Root directory.
		os << path << '\n';
	}

	// Read the directory entries.
	IFst::DirEnt *dirent = fst->readdir(dirp);
	while (dirent != nullptr) {
		if (!dirent->name || dirent->name[0] == 0) {
			// Empty name...
			continue;
		}

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
			fc.dirs++;

			string name = dirent->name;
			string subdir = path;
			if (path.empty() || (path[path.size()-1] != '/')) {
				// Append a trailing slash.
				subdir += '/';
			}
			subdir += name;

			// Check if any more entries are present.
			dirent = fst->readdir(dirp);
			tree_lines.push_back(dirent ? 1 : 0);

			// Tree line for the directory entry.
			if (dirent) {
				// More files are present in the current directory.
				os << "\xE2\x94\x9C";
			} else {
				// No more files are present in the current directory.
				os << "\xE2\x94\x94";
			}
			os << "\xE2\x94\x80\xE2\x94\x80 ";

			// Print the subdirectory name.
			os << name << '\n';

			// Print the subdirectory.
			int ret = fstPrint(fst, os, subdir, level+1, tree_lines, fc);
			if (ret != 0) {
				// ERROR
				return ret;
			}

			// Remove the extra tree line.
			tree_lines.resize(tree_lines.size()-1);
		} else {
			// File.
			fc.files++;

			// Save the filename.
			string name = dirent->name;

			// Tree + name length.
			// - Tree is 4 characters per level.
			// - Attrs should start at column 40.
			// TODO: Handle full-width and non-BMP Unicode characters correctly.
			const int tree_name_length = ((level+1)*4) + 1 +
					(int)utf8_to_utf16(name).size();
			int attr_spaces;
			if (tree_name_length < 40) {
				// Pad it to 40 columns.
				attr_spaces = 40 - tree_name_length;
			} else {
				// Use the next closest multiple of 4.
				attr_spaces = 4 - (tree_name_length % 4);
			}

			// Print the attributes. (address, size)
			char attrs[48];
			snprintf(attrs, sizeof(attrs), "[addr:0x%08" PRIX64 ", size:%" PRIu64 "]",
				 dirent->offset, dirent->size);

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

			// Print the filename and attributes.
			os << name << setw(attr_spaces) << ' ' << setw(0) << attrs << '\n';
		}
	}

	// Directory printed.
	fst->closedir(dirp);
	return 0;
}

/**
 * Print an FST to an ostream.
 * @param fst	[in] FST to print.
 * @param os	[in,out] ostream.
 *
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

	FstFileCount fc = {0, 0};
	int ret = fstPrint(fst, os, "/", 0, tree_lines, fc);
	if (ret != 0) {
		return ret;
	}

	// Print the file count.
	// NOTE: Formatting numbers using ostringstream() because
	// MSVC's printf() doesn't support thousands separators.
	// TODO: CMake checks?
	ostringstream dircount, filecount;
	dircount << fc.dirs;
	filecount << fc.files;

	os << '\n' <<
		// tr: Parameter is a number; it's formatted elsewhere.
		rp_sprintf(NC_("FstPrint", "%s directory", "%s directories", fc.dirs), dircount.str().c_str()) << ", " <<
		// tr: Parameter is a number; it's formatted elsewhere.
		rp_sprintf(NC_("FstPrint", "%s file", "%s files", fc.files), filecount.str().c_str()) << '\n';

	os.flush();
	return 0;
}

};

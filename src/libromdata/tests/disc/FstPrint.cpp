/***************************************************************************
 * ROM Properties Page shell extension. (libromdata/tests)                 *
 * FstPrint.cpp: FST printer.                                              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FstPrint.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpbase/disc/IFst.hpp"
#include "librptext/conversion.hpp"
using namespace LibRpBase;
using namespace LibRpText;

// C includes (C++ namespace)
#include <cerrno>
#include <cstdint>
#include <cstdio>

// C++ includes
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
using std::ostream;
using std::setw;
using std::string;
using std::vector;

// libfmt
#include "rp-libfmt.h"

namespace LibRomData {

struct FstFileCount {
	unsigned int dirs;
	unsigned int files;
};

/**
 * Print an FST to an ostream.
 * @param fst		[in] FST to print
 * @param os		[in] ostream
 * @param path		[in] Directory path
 * @param level		[in] Current directory level (0 == root)
 * @param tree_lines	[in/out] Levels with tree lines
 * @param fc		[out] File count
 * @param pt		[in] If true, print partition numbers
 * @return 0 on success; negative POSIX error code on error.
 */
static int fstPrint(IFst *fst, ostream &os, const string &path,
	int level, vector<uint8_t> &tree_lines, FstFileCount &fc,
	bool pt)
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
	const IFst::DirEnt *dirent = fst->readdir(dirp);
	while (dirent != nullptr) {
		if (!dirent->name || dirent->name[0] == 0) {
			// Empty name...
			continue;
		}

		// Print the tree lines.
		for (int i = 0; i < level; i++) {
			if (tree_lines[i]) {
				// Directory tree exists for this segment.
				os << "\xE2\x94\x82   ";
			} else {
				// No directory tree for this segment.
				os << "    ";
			}
		}

		const string name = dirent->name;
		// Sanity check: Filenames cannot contain '/'.
		if (name.find('/') != string::npos) {
			// ERROR
			// TODO: Print an error message?
			fst->closedir(dirp);
			return -EIO;
		}

		// NOTE: The next DirEnt is obtained in one of the
		// following if/else branches. We can't obtain more
		// than one DirEnt at a time, since the DirEnt is
		// actually stored within the Dir object.
		if (dirent->type == DT_DIR) {
			// Subdirectory
			fc.dirs++;

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
			int ret = fstPrint(fst, os, subdir, level+1, tree_lines, fc, pt);
			if (ret != 0) {
				// ERROR
				// TODO: Print an error message?
				fst->closedir(dirp);
				return ret;
			}

			// Remove the extra tree line.
			tree_lines.resize(tree_lines.size()-1);
		} else {
			// File
			fc.files++;

			// Tree + name length.
			// - Tree is 4 characters per level.
			// - Attrs should start at column 40.
			// TODO: Handle full-width and non-BMP Unicode characters correctly.
			const int tree_name_length = ((level+1)*4) + 1 +
					static_cast<int>(utf8_to_utf16(name).size());
			int attr_spaces;
			if (tree_name_length < 40) {
				// Pad it to 40 columns.
				attr_spaces = 40 - tree_name_length;
			} else {
				// Use the next closest multiple of 4.
				attr_spaces = 4 - (tree_name_length % 4);
			}

			// Print the attributes. (address, size)
			string s_attrs;
			if (pt) {
				s_attrs = fmt::format(FSTR("[pt:0x{:0>2x}, addr:0x{:0>8X}, size:{:d}]"),
					dirent->ptnum, static_cast<uint64_t>(dirent->offset), dirent->size);
			} else {
				s_attrs = fmt::format(FSTR("[addr:0x{:0>8X}, size:{:d}]"),
					static_cast<uint64_t>(dirent->offset), dirent->size);
			}

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
			os << name << setw(attr_spaces) << ' ' << setw(0) << s_attrs << '\n';
		}
	}

	// Directory printed.
	fst->closedir(dirp);
	return 0;
}

/**
 * Print an FST to an ostream.
 * @param fst	[in] FST to print
 * @param os	[in,out] ostream
 * @param pt	[in] If true, print partition numbers
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int fstPrint(IFst *fst, ostream &os, bool pt)
{
	if (!fst) {
		// Invalid parameters.
		return -EINVAL;
	}

	vector<uint8_t> tree_lines;
	tree_lines.reserve(16);

	FstFileCount fc = {0, 0};
	int ret = fstPrint(fst, os, "/", 0, tree_lines, fc, pt);
	if (ret != 0) {
		return ret;
	}

	os << '\n' <<
		// tr: {:Ld} == number of directories processed
		fmt::format(FRUN(NC_("FstPrint", "{:Ld} directory", "{:Ld} directories", fc.dirs)), fc.dirs) << ", " <<
		// tr: {:Ld} == number of files processed
		fmt::format(FRUN(NC_("FstPrint", "{:Ld} file", "{:Ld} files", fc.files)), fc.files) << '\n';

	os.flush();
	return 0;
}

};

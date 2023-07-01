/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RelatedFile.hpp"
#include "FileSystem.hpp"
#include "RpFile.hpp"

// C++ STL classes.
using std::string;

namespace LibRpFile { namespace FileSystem {

/**
 * Attempt to open a related file. (read-only)
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename.
 * @param basename	[in] New basename.
 * @param ext		[in] New extension, including leading dot.
 * @return IRpFile*, or nullptr if not found.
 */
IRpFile *openRelatedFile(const char *filename, const char *basename, const char *ext)
{
	assert(filename != nullptr);
	assert(ext != nullptr);
	if (!filename || !ext) {
		return nullptr;
	}

	// Get the directory portion of the filename.
	string s_dir = filename;
	const size_t slash_pos = s_dir.find_last_of(DIR_SEP_CHR);
	if (slash_pos != string::npos) {
		s_dir.resize(slash_pos+1);
	} else {
		// No directory. Probably a filename in the
		// current directory, e.g. when using `rpcli`.
		s_dir.clear();
	}

	// Get the base name.
	string s_basename;
	if (basename) {
		s_basename = basename;
	} else {
		s_basename = &filename[slash_pos+1];

		// Check for any dots.
		const size_t dot_pos = s_basename.find_last_of('.');
		if (dot_pos != string::npos) {
			// Remove the extension.
			s_basename.resize(dot_pos);
		}
	}

	// NOTE: Windows 10 1709 supports per-directory case-sensitivity on NTFS,
	// and Linux 5.2 supports per-directory case-insensitivity on EXT4.
	// Hence, we should check for both uppercase and lowercase extensions
	// on all platforms.

	// Check for uppercase extensions first.
	string s_ext = ext;
	std::transform(s_ext.begin(), s_ext.end(), s_ext.begin(),
		[](unsigned char c) noexcept -> char { return std::toupper(c); });

	// Attempt to open the related file.
	string rel_filename = s_dir + s_basename + s_ext;
	IRpFile *test_file = new RpFile(rel_filename, RpFile::FM_OPEN_READ);
	if (!test_file->isOpen()) {
		// Error opening the related file.
		test_file->unref();

		// Try again with a lowercase extension.
		std::transform(s_ext.begin(), s_ext.end(), s_ext.begin(),
			[](unsigned char c) noexcept -> char { return std::tolower(c); });
		rel_filename.replace(rel_filename.size() - s_ext.size(), s_ext.size(), s_ext);
		test_file = new RpFile(rel_filename, RpFile::FM_OPEN_READ);
		if (!test_file->isOpen()) {
			// Still can't open the related file.
			UNREF_AND_NULL_NOCHK(test_file);
		}
	}

	if (!test_file && FileSystem::is_symlink(filename)) {
		// Could not open the related file, but the
		// primary file is a symlink. Dereference the
		// symlink and check the original directory.
		const string deref_filename = FileSystem::resolve_symlink(filename);
		if (!deref_filename.empty()) {
			test_file = openRelatedFile(deref_filename.c_str(), basename, ext);
		}
	}
	return test_file;
}

} }

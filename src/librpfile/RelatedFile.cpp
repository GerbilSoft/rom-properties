/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RelatedFile.hpp"
#include "FileSystem.hpp"

#include "IRpFile.hpp"
#include "RpFile.hpp"

// C++ STL classes
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibRpFile { namespace FileSystem {

/**
 * Attempt to open a related file. (read-only)
 * RAW POINTER VERSION; use with caution.
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
IRpFile *openRelatedFile_rawptr(const char *filename, const char *basename, const char *ext)
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
		[](char c) noexcept -> char { return std::toupper(c); });

	// Attempt to open the related file.
	string s_rel_filename = s_dir + s_basename + s_ext;
	IRpFile *test_file = new RpFile(s_rel_filename, RpFile::FM_OPEN_READ);
	if (!test_file->isOpen()) {
		// Error opening the related file.
		delete test_file;

		// Try again with a lowercase extension.
		std::transform(s_ext.begin(), s_ext.end(), s_ext.begin(),
			[](char c) noexcept -> char { return std::tolower(c); });
		s_rel_filename.replace(s_rel_filename.size() - s_ext.size(), s_ext.size(), s_ext);
		test_file = new RpFile(s_rel_filename, RpFile::FM_OPEN_READ);
		if (!test_file->isOpen()) {
			// Still can't open the related file.
			delete test_file;
			test_file = nullptr;
		}
	}

	if (!test_file && FileSystem::is_symlink(filename)) {
		// Could not open the related file, but the
		// primary file is a symlink. Dereference the
		// symlink and check the original directory.
		const string deref_filename = FileSystem::resolve_symlink(filename);
		if (!deref_filename.empty()) {
			return openRelatedFile_rawptr(deref_filename.c_str(), basename, ext);
		}
	}

	return test_file;
}

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
 * @return IRpFilePtr, or nullptr if not found.
 */
IRpFilePtr openRelatedFile(const char *filename, const char *basename, const char *ext)
{
	return IRpFilePtr(openRelatedFile_rawptr(filename, basename, ext));
}

#ifdef _WIN32
/**
 * Attempt to open a related file. (read-only)
 * RAW POINTER VERSION; use with caution.
 *
 * Related files are located in the same directory as the
 * primary file, but may have a different filename and/or
 * file extension.
 *
 * If the primary file is a symlink, the related file may
 * be located in the original file's directory.
 *
 * @param filename	[in] Primary filename. (UTF-16)
 * @param basename	[in,opt] New basename. (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-16)
 * @return IRpFile*, or nullptr if not found.
 */
IRpFile *openRelatedFile_rawptr(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW)
{
	assert(filenameW != nullptr);
	assert(extW != nullptr);
	if (!filenameW || !extW) {
		return nullptr;
	}

	// Get the directory portion of the filename.
	wstring ws_dir = filenameW;
	const size_t slash_pos = ws_dir.find_last_of(DIR_SEP_CHR);
	if (slash_pos != string::npos) {
		ws_dir.resize(slash_pos+1);
	} else {
		// No directory. Probably a filename in the
		// current directory, e.g. when using `rpcli`.
		ws_dir.clear();
	}

	// Get the base name.
	wstring ws_basename;
	if (basenameW) {
		ws_basename = basenameW;
	} else {
		ws_basename = &filenameW[slash_pos+1];

		// Check for any dots.
		const size_t dot_pos = ws_basename.find_last_of(L'.');
		if (dot_pos != string::npos) {
			// Remove the extension.
			ws_basename.resize(dot_pos);
		}
	}

	// NOTE: Windows 10 1709 supports per-directory case-sensitivity on NTFS,
	// and Linux 5.2 supports per-directory case-insensitivity on EXT4.
	// Hence, we should check for both uppercase and lowercase extensions
	// on all platforms.

	// Check for uppercase extensions first.
	wstring ws_ext = extW;
	std::transform(ws_ext.begin(), ws_ext.end(), ws_ext.begin(),
		[](wchar_t c) noexcept -> wchar_t { return std::towupper(c); });

	// Attempt to open the related file.
	wstring ws_rel_filename = ws_dir + ws_basename + ws_ext;
	IRpFile *test_file = new RpFile(ws_rel_filename, RpFile::FM_OPEN_READ);
	if (!test_file->isOpen()) {
		// Error opening the related file.
		delete test_file;

		// Try again with a lowercase extension.
		std::transform(ws_ext.begin(), ws_ext.end(), ws_ext.begin(),
			[](wchar_t c) noexcept -> wchar_t { return std::towlower(c); });
		ws_rel_filename.replace(ws_rel_filename.size() - ws_ext.size(), ws_ext.size(), ws_ext);
		test_file = new RpFile(ws_rel_filename, RpFile::FM_OPEN_READ);
		if (!test_file->isOpen()) {
			// Still can't open the related file.
			delete test_file;
			test_file = nullptr;
		}
	}

	if (!test_file && FileSystem::is_symlink(filenameW)) {
		// Could not open the related file, but the
		// primary file is a symlink. Dereference the
		// symlink and check the original directory.
		const wstring deref_filename = FileSystem::resolve_symlink(filenameW);
		if (!deref_filename.empty()) {
			return openRelatedFile_rawptr(deref_filename.c_str(), basenameW, extW);
		}
	}

	return test_file;
}

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
 * @param filename	[in] Primary filename. (UTF-16)
 * @param basename	[in,opt] New basename. (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot. (UTF-16)
 * @return IRpFilePtr, or nullptr if not found.
 */
IRpFilePtr openRelatedFile(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW)
{
	return IRpFilePtr(openRelatedFile_rawptr(filenameW, basenameW, extW));
}
#endif /* _WIN32 */

} }

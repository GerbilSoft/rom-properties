/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RelatedFile.hpp: Open a related file.                                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RelatedFile.hpp"
#include "FileSystem.hpp"

#include "IRpFile.hpp"
#include "RpFile.hpp"

#include "force_inline.h"

// C includes (C++ namespace)
#include <cassert>
#ifdef _WIN32
#  include <cwctype>
#endif /* _WIN32 */

// C++ STL classes
#include <algorithm>
using std::string;
using std::unique_ptr;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibRpFile { namespace FileSystem {

namespace {

static RP_FORCEINLINE char toupper_wrapper(char c)
{
	return std::toupper(c);
}

static RP_FORCEINLINE char tolower_wrapper(char c)
{
	return std::tolower(c);
}

#if defined(_WIN32) && defined(_UNICODE)
static RP_FORCEINLINE wchar_t toupper_wrapper(wchar_t c)
{
	return std::towupper(c);
}

static RP_FORCEINLINE wchar_t tolower_wrapper(wchar_t c)
{
	return std::towlower(c);
}
#endif /* _WIN32 && _UNICODE */

}

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
 * @tparam CharType	Character type (char for UTF-8, or wchar_t for UTF-16)
 * @param filename	[in] Primary filename (CharType)
 * @param basename	[in,opt] New basename (CharType) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot (CharType)
 * @return IRpFile*, or nullptr if not found.
 */
template<typename CharType>
IRpFile *T_openRelatedFile_rawptr(const CharType *filename, const CharType *basename, const CharType *ext)
{
	using ct_string = std::basic_string<CharType>;

	assert(filename != nullptr);
	assert(ext != nullptr);
	if (!filename || !ext) {
		return nullptr;
	}

	// Get the directory portion of the filename.
	ct_string s_dir = filename;
	const size_t slash_pos = s_dir.find_last_of(DIR_SEP_CHR);
	if (slash_pos != string::npos) {
		s_dir.resize(slash_pos+1);
	} else {
		// No directory. Probably a filename in the
		// current directory, e.g. when using `rpcli`.
		s_dir.clear();
	}

	// Get the base name.
	ct_string s_basename;
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
	ct_string s_ext = ext;
	std::transform(s_ext.begin(), s_ext.end(), s_ext.begin(),
		[](CharType c) noexcept -> CharType { return toupper_wrapper(c); });

	// Attempt to open the related file.
	ct_string s_rel_filename = s_dir + s_basename + s_ext;
	unique_ptr<IRpFile> test_file(new RpFile(s_rel_filename, RpFile::FM_OPEN_READ));
	if (!test_file->isOpen()) {
		// Error opening the related file.

		// Try again with a lowercase extension.
		std::transform(s_ext.begin(), s_ext.end(), s_ext.begin(),
			[](CharType c) noexcept -> CharType { return tolower_wrapper(c); });
		s_rel_filename.replace(s_rel_filename.size() - s_ext.size(), s_ext.size(), s_ext);
		test_file.reset(new RpFile(s_rel_filename, RpFile::FM_OPEN_READ));
		if (!test_file->isOpen()) {
			// Still can't open the related file.
			test_file.reset();
		}
	}

	if (!test_file && FileSystem::is_symlink(filename)) {
		// Could not open the related file, but the
		// primary file is a symlink. Dereference the
		// symlink and check the original directory.
		const ct_string deref_filename = FileSystem::resolve_symlink(filename);
		if (!deref_filename.empty()) {
			return openRelatedFile_rawptr(deref_filename.c_str(), basename, ext);
		}
	}

	return test_file.release();
}

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
 * @param filename	[in] Primary filename (UTF-8)
 * @param basename	[in,opt] New basename (UTF-8) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot (UTF-8)
 * @return IRpFile*, or nullptr if not found.
 */
IRpFile *openRelatedFile_rawptr(const char *filename, const char *basename, const char *ext)
{
	return T_openRelatedFile_rawptr(filename, basename, ext);
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
 * @param filename	[in] Primary filename (UTF-8)
 * @param basename	[in,opt] New basename (UTF-8) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot (UTF-8)
 * @return IRpFilePtr, or nullptr if not found.
 */
IRpFilePtr openRelatedFile(const char *filename, const char *basename, const char *ext)
{
	return IRpFilePtr(T_openRelatedFile_rawptr(filename, basename, ext));
}

#if defined(_WIN32) && defined(_UNICODE)
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
 * @param filename	[in] Primary filename (UTF-16)
 * @param basename	[in,opt] New basename (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot (UTF-16)
 * @return IRpFile*, or nullptr if not found.
 */
IRpFile *openRelatedFile_rawptr(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW)
{
	return T_openRelatedFile_rawptr(filenameW, basenameW, extW);
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
 * @param filename	[in] Primary filename (UTF-16)
 * @param basename	[in,opt] New basename (UTF-16) If nullptr, uses the existing basename.
 * @param ext		[in] New extension, including leading dot (UTF-16)
 * @return IRpFilePtr, or nullptr if not found.
 */
IRpFilePtr openRelatedFile(const wchar_t *filenameW, const wchar_t *basenameW, const wchar_t *extW)
{
	return IRpFilePtr(T_openRelatedFile_rawptr(filenameW, basenameW, extW));
}
#endif /* _WIN32 && _UNICODE */

} }

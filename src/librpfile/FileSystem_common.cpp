/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * FileSystem_common.cpp: File system functions. (Common functions)        *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FileSystem.hpp"

// OS-specific userdirs
#ifdef _WIN32
#  include "libwin32common/userdirs.hpp"
#  define OS_NAMESPACE LibWin32Common
#else
#  include "libunixcommon/userdirs.hpp"
#  define OS_NAMESPACE LibUnixCommon
#endif

// libcachecommon
#include "libcachecommon/CacheDir.hpp"

// C++ STL classes
#include <mutex>
using std::string;

namespace LibRpFile { namespace FileSystem {

/** Configuration directories. **/
// std::call_once() control variable.
static std::once_flag cfgdir_once_flag;
// User's configuration directory.
static string config_dir;

/**
 * Initialize the configuration directory paths.
 * Called by std::call_once().
 */
static void initConfigDirectories(void)
{
	// Uses LibUnixCommon or LibWin32Common, depending on platform.

	// Config directory.
	config_dir = OS_NAMESPACE::getConfigDirectory();
	if (!config_dir.empty()) {
		// Add a trailing slash if necessary.
		if (config_dir.at(config_dir.size()-1) != DIR_SEP_CHR)
			config_dir += DIR_SEP_CHR;
		// Append "rom-properties".
		config_dir += "rom-properties";
	}

	// Directories have been initialized.
}

/**
 * Get the user's cache directory.
 * This is usually one of the following:
 * - Windows XP: %APPDATA%\Local Settings\rom-properties\cache
 * - Windows Vista: %LOCALAPPDATA%\rom-properties\cache
 * - Linux: ~/.cache/rom-properties
 *
 * @return User's rom-properties cache directory, or empty string on error.
 */
const string &getCacheDirectory(void)
{
	// TODO: Handle errors.
	return LibCacheCommon::getCacheDirectory();
}

/**
 * Get the user's rom-properties configuration directory.
 * This is usually one of the following:
 * - Windows: %APPDATA%\rom-properties
 * - Linux: ~/.config/rom-properties
 *
 * @return User's rom-properties configuration directory, or empty string on error.
 */
const string &getConfigDirectory(void)
{
	// TODO: Handle errors.
	std::call_once(cfgdir_once_flag, initConfigDirectories);
	return config_dir;
}

/**
 * Get the file extension from a filename or pathname.
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename
 * @return File extension, including the leading dot; nullptr if no extension.
 */
const char *file_ext(const char *filename)
{
	if (unlikely(!filename))
		return nullptr;

	const char *const dotpos = strrchr(filename, '.');
	const char *const slashpos = strrchr(filename, DIR_SEP_CHR);
	if (!dotpos || dotpos[1] == '\0' ||
	    (slashpos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		return nullptr;
	}

	// Return the file extension. (pointer to within the filename)
	return dotpos;
}

#ifdef _WIN32
/**
 * Get the file extension from a filename or pathname. (wchar_t version)
 * NOTE: Returned value points into the specified filename.
 * @param filename Filename
 * @return File extension, including the leading dot; nullptr if no extension.
 */
const wchar_t *file_ext(const wchar_t *filename)
{
	if (unlikely(!filename))
		return nullptr;

	const wchar_t *const dotpos = wcsrchr(filename, '.');
	const wchar_t *const slashpos = wcsrchr(filename, DIR_SEP_CHR);
	if (!dotpos || dotpos[1] == L'\0' ||
	    (slashpos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		return nullptr;
	}

	// Return the file extension. (pointer to within the filename)
	return dotpos;
}
#endif /* _WIN32 */

/**
 * Replace the file extension from a filename.
 * @param filename	[in] Filename.
 * @param ext		[in] New extension.
 * @return Filename, with replaced extension.
 */
std::string replace_ext(const char *filename, const char *ext)
{
	if (!filename || filename[0] == '\0') {
		// No filename...
		return {};
	}

	string s_ret = filename;
	const size_t dotpos = s_ret.find_last_of('.');
	const size_t slashpos = s_ret.find_last_of(DIR_SEP_CHR);
	if (dotpos == string::npos ||
	    dotpos >= s_ret.size()-1 ||
	    (slashpos != string::npos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		// Add the extension.
		s_ret += ext;
		return s_ret;
	}

	// Replace the file extension.
	s_ret.resize(dotpos);
	if (ext && ext[0] != '\0') {
		s_ret += ext;
	}
	return s_ret;
}

} }

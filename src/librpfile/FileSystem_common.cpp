/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * FileSystem_common.cpp: File system functions. (Common functions)        *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "FileSystem.hpp"

// OS-specific userdirs
#ifdef _WIN32
# include "libwin32common/userdirs.hpp"
# define OS_NAMESPACE LibWin32Common
#else
# include "libunixcommon/userdirs.hpp"
# define OS_NAMESPACE LibUnixCommon
#endif

// librpthreads
#include "librpthreads/pthread_once.h"

// libcachecommon
#include "libcachecommon/CacheDir.hpp"

// C++ STL classes.
using std::string;

namespace LibRpBase { namespace FileSystem {

/** Configuration directories. **/
// pthread_once() control variable.
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
// User's configuration directory.
static string config_dir;

/**
 * Initialize the configuration directory paths.
 * Called by pthread_once().
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
	pthread_once(&once_control, initConfigDirectories);
	return config_dir;
}

/**
 * Get the file extension from a filename or pathname.
 * @param filename Filename.
 * @return File extension, including the leading dot. (pointer to within the filename) [nullptr if no extension]
 */
const char *file_ext(const string &filename)
{
	size_t dotpos = filename.find_last_of('.');
	size_t slashpos = filename.find_last_of(DIR_SEP_CHR);
	if (dotpos == string::npos ||
	    dotpos >= filename.size()-1 ||
	    (slashpos != string::npos && dotpos <= slashpos))
	{
		// Invalid or missing file extension.
		return nullptr;
	}

	// Return the file extension. (pointer to within the filename)
	return &filename[dotpos];
}

} }

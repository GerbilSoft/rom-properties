/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs.cpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.libunixcommon.h"

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// C includes
#include <fcntl.h>	// AT_FDCWD
#include <pwd.h>	// getpwuid_r()
#include <sys/stat.h>	// stat(), statx(), S_ISDIR()
#include <stdlib.h>
#include <unistd.h>

// C includes (C++ namespace)
#include <cassert>

// C++ includes
#include <memory>
#include <string>
using std::string;
using std::unique_ptr;

namespace LibUnixCommon {

/**
 * Check if a directory is writable.
 * @param path Directory path.
 * @return True if this is a writable directory; false if not.
 */
bool isWritableDirectory(const char *path)
{
#ifdef HAVE_STATX
	struct statx sbx;
	int ret = statx(AT_FDCWD, path, 0, STATX_TYPE, &sbx);
	if (ret != 0 || !(sbx.stx_mask & STATX_TYPE)) {
		// statx() failed.
		return false;
	} else if (!S_ISDIR(sbx.stx_mode)) {
		// Not a directory.
		return false;
	}
#else /* !HAVE_STATX */
	struct stat sb;
	int ret = stat(path, &sb);
	if (ret != 0) {
		// stat() failed.
		return false;
	} else if (!S_ISDIR(sb.st_mode)) {
		// Not a directory.
		return false;
	}
#endif /* HAVE_STATX */

	// This is a directory.
	// Return true if it's writable.
	return !access(path, R_OK|W_OK);
}

/**
 * Remove trailing slashes.
 * @param path Path to remove trailing slashes from.
 */
static inline void removeTrailingSlashes(string &path)
{
	while (!path.empty() && path[path.size()-1] == '/') {
		path.resize(path.size()-1);
	}
}

/**
 * Get the user's home directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's home directory (without trailing slash), or empty string on error.
 */
string getHomeDirectory(void)
{
	string home_dir;

	const char *const home_env = getenv("HOME");
	if (home_env && home_env[0] != 0) {
		// HOME variable is set.
		// Make sure the directory is writable.
		if (isWritableDirectory(home_env)) {
			// $HOME is writable.
			home_dir = home_env;
			// Remove trailing slashes.
			removeTrailingSlashes(home_dir);
			// If the path was "/", this will result in an empty directory.
			if (!home_dir.empty()) {
				return home_dir;
			}
		}
	}

	// HOME variable is not set or the directory is not writable.
	// Check the user's pwent.
	// TODO: Can pwd_result be nullptr?
	// TODO: Check for ENOMEM?
	const char *pw_dir = nullptr;
#if defined(HAVE_GETPWUID_R) && defined(_SC_GETPW_R_SIZE_MAX)
#  define GETPW_BUF_SIZE 16384
	unique_ptr<char[]> buf(new char[GETPW_BUF_SIZE]);
	struct passwd pwd;
	struct passwd *pwd_result;
	int ret = getpwuid_r(getuid(), &pwd, buf.get(), GETPW_BUF_SIZE, &pwd_result);
	if (ret != 0 || !pwd_result) {
		// getpwuid_r() failed.
		return string();
	}
	pw_dir = pwd_result->pw_dir;
#elif defined(HAVE_GETPWUID)
	// NOTE: Non-reentrant version...
	struct passwd *pwd = getpwuid(getuid());
	if (!pwd) {
		// getpwuid() failed.
		return string();
	}
	pw_dir = pwd->pw_dir;
#else
#  error No supported getpwuid() function.
#endif

	if (!pw_dir || pw_dir[0] == 0) {
		// Empty home directory...
		return string();
	}

	// Make sure the directory is writable.
	if (isWritableDirectory(pw_dir)) {
		// Directory is writable.
		// $HOME is writable.
		home_dir = pw_dir;
		// Remove trailing slashes.
		removeTrailingSlashes(home_dir);
		// If the path was "/", this will result in an empty directory.
		if (!home_dir.empty()) {
			return home_dir;
		}
	}

	// Unable to get the user's home directory...
	assert(!"Unable to get the user's home directory.");
	return string();
}

/**
 * Get an XDG directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @param xdgvar XDG variable name.
 * @param relpath Default path relative to the user's home directory (without leading slash).
 * @param mode Mode for mkdir() if the directory doesn't exist.
 *
 * @return XDG directory (without trailing slash), or empty string on error.
 */
static string getXDGDirectory(const char *xdgvar, const char *relpath, int mode)
{
	assert(xdgvar != nullptr);
	assert(relpath != nullptr);
	assert(relpath[0] != '/');

	string xdg_dir;

	// Check the XDG variable first.
	const char *const xdg_env = getenv(xdgvar);
	if (xdg_env && xdg_env[0] == '/') {
		// If the directory doesn't exist, create it.
		if (access(xdg_env, F_OK) != 0) {
			mkdir(xdg_env, mode);
		}

		// Make sure this is a writable directory.
		if (isWritableDirectory(xdg_env)) {
			// This is a writable directory.
			xdg_dir = xdg_env;
			// Remove trailing slashes.
			removeTrailingSlashes(xdg_dir);
			// If the path was "/", this will result in an empty directory.
			if (!xdg_dir.empty()) {
				return xdg_dir;
			}
		}
	}

	// Get the user's home directory.
	xdg_dir = getHomeDirectory();
	if (xdg_dir.empty()) {
		// No home directory...
		return string();
	}

	xdg_dir += '/';
	xdg_dir += relpath;

	// If the directory doesn't exist, create it.
	if (access(xdg_dir.c_str(), F_OK) != 0) {
		mkdir(xdg_dir.c_str(), mode);
	}
	return xdg_dir;
}

// TODO: Parameters indicating if directories should be created or not.
// We shouldn't create directories if we're just going to be reading
// files that should already be present within the directories.
// This will cause an ABI break, so wait until libromdata.so.4.

/**
 * Get the user's cache directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
string getCacheDirectory(void)
{
	return getXDGDirectory("XDG_CACHE_HOME", ".cache", 0700);
}

/**
 * Get the user's configuration directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's configuration directory (without trailing slash), or empty string on error.
 */
string getConfigDirectory(void)
{
	return getXDGDirectory("XDG_CONFIG_HOME", ".config", 0777);
}

}

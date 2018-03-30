/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs.cpp: Find user directories.                                    *
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

#include "config.libunixcommon.h"

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// C includes.
#include <pwd.h>	/* getpwuid_r() */
#include <sys/stat.h>	/* stat(), S_ISDIR */
#include <stdlib.h>
#include <unistd.h>

// C++ includes.
#include <string>
using std::string;

namespace LibUnixCommon {

/**
 * Check if a directory is writable.
 * @param path Directory path.
 * @return True if this is a writable directory; false if not.
 */
static inline bool isWritableDirectory(const char *path)
{
	struct stat sb;
	int ret = stat(path, &sb);
	if (ret == 0 && S_ISDIR(sb.st_mode)) {
		// This is a directory.
		if (!access(path, R_OK|W_OK)) {
			// Directory is writable.
			return true;
		}
	}

	// Not a writable directory.
	return false;
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
std::string getHomeDirectory(void)
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
#if defined(HAVE_GETPWUID_R)
	char buf[2048];
	struct passwd pwd;
	struct passwd *pwd_result;
	int ret = getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &pwd_result);
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
# error No supported getpwuid() function.
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
	return string();
}

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
	string cache_dir;

	// Check $XDG_CACHE_HOME first.
	const char *const xdg_cache_home_env = getenv("XDG_CACHE_HOME");
	if (xdg_cache_home_env && xdg_cache_home_env[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(xdg_cache_home_env)) {
			// $XDG_CACHE_HOME is a writable directory.
			cache_dir = xdg_cache_home_env;
			// Remove trailing slashes.
			removeTrailingSlashes(cache_dir);
			// If the path was "/", this will result in an empty directory.
			if (!cache_dir.empty()) {
				return cache_dir;
			}
		}
	}

	// Get the user's home directory.
	cache_dir = getHomeDirectory();
	if (cache_dir.empty()) {
		// No home directory...
		return string();
	}

	cache_dir += "/.cache";
	return cache_dir;
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
	string config_dir;

	// Check $XDG_CONFIG_HOME first.
	const char *const xdg_config_home_env = getenv("XDG_CONFIG_HOME");
	if (xdg_config_home_env && xdg_config_home_env[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(xdg_config_home_env)) {
			// $XDG_CACHE_HOME is a writable directory.
			config_dir = xdg_config_home_env;
			// Remove trailing slashes.
			removeTrailingSlashes(config_dir);
			// If the path was "/", this will result in an empty directory.
			if (!config_dir.empty()) {
				return config_dir;
			}
		}
	}

	// Get the user's home directory.
	config_dir = getHomeDirectory();
	if (config_dir.empty()) {
		// No home directory...
		return string();
	}

	config_dir += "/.config";
	return config_dir;
}

}

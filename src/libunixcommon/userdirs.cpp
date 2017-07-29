/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs.cpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// C includes.
#include <unistd.h>
// C includes. (stat, getpwuid_r)
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

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
	const char *const home = getenv("HOME");
	if (home && home[0] != 0) {
		// HOME variable is set.
		return string(home);
	}

	// HOME variable is not set.
	// Check the user's pwent.
	// TODO: Can pwd_result be nullptr?
	// TODO: Check for ENOMEM?
	// TODO: Support getpwuid() if the system doesn't support getpwuid_r()?
	char buf[2048];
	struct passwd pwd;
	struct passwd *pwd_result;
	int ret = getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &pwd_result);
	if (ret != 0 || !pwd_result) {
		// getpwuid_r() failed.
		return string();
	}

	if (pwd_result->pw_dir[0] == 0) {
		// Empty home directory...
		return string();
	}

	return string(pwd_result->pw_dir);
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

	// Fall back to $HOME/.cache/.
	const char *const home_env = getenv("HOME");
	if (home_env && home_env[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(home_env)) {
			// $HOME is a writable directory.
			cache_dir = home_env;
			// Remove trailing slashes.
			removeTrailingSlashes(cache_dir);
			// If the path was "/", this will result in an empty directory.
			if (!cache_dir.empty()) {
				cache_dir += "/.cache";
				return cache_dir;
			}
		}
	}

	// $HOME isn't valid. Use getpwuid_r().
	char buf[2048];
	struct passwd pwd;
	struct passwd *pwd_result;
	errno = 0;
	int ret = getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &pwd_result);
	if (ret != 0 || !pwd_result) {
		// getpwuid_r() failed.
		return string();
	}

	if (pwd_result->pw_dir[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(pwd_result->pw_dir)) {
			// $HOME is a writable directory.
			cache_dir = pwd_result->pw_dir;
			// Remove trailing slashes.
			removeTrailingSlashes(cache_dir);
			// If the path was "/", this will result in an empty directory.
			if (!cache_dir.empty()) {
				cache_dir += "/.cache";
				return cache_dir;
			}
		}
	}

	// Unable to determine the XDG cache directory.
	return string();
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

	// TODO: $XDG_CONFIG_DIRS

	// Fall back to $HOME/.config/.
	const char *const home_env = getenv("HOME");
	if (home_env && home_env[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(home_env)) {
			// $HOME is a writable directory.
			config_dir = home_env;
			// Remove trailing slashes.
			removeTrailingSlashes(config_dir);
			// If the path was "/", this will result in an empty directory.
			if (!config_dir.empty()) {
				config_dir += "/.config";
				return config_dir;
			}
		}
	}

	// $HOME isn't valid. Use getpwuid_r().
	char buf[2048];
	struct passwd pwd;
	struct passwd *pwd_result;
	errno = 0;
	int ret = getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &pwd_result);
	if (ret != 0 || !pwd_result) {
		// getpwuid_r() failed.
		return string();
	}

	if (pwd_result->pw_dir[0] == '/') {
		// Make sure this is a writable directory.
		if (isWritableDirectory(pwd_result->pw_dir)) {
			// $HOME is a writable directory.
			config_dir = pwd_result->pw_dir;
			// Remove trailing slashes.
			removeTrailingSlashes(config_dir);
			// If the path was "/", this will result in an empty directory.
			if (!config_dir.empty()) {
				config_dir += "/.config";
				return config_dir;
			}
		}
	}

	// Unable to determine the XDG configuration directory.
	return string();
}

}

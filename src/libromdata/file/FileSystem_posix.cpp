/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * FileSystem_posix.cpp: File system functions. (POSIX implementation)     *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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

#include "FileSystem.hpp"

// libromdata
#include "TextFuncs.hpp"

// C includes.
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <utime.h>

// C includes. (C++ namespace)
#include <cstring>
#include <ctime>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// Atomic function macros.
// TODO: C++11 atomic support; test all of this.
#if defined(__clang__)
# if 0 && (__has_feature(c_atomic) || __has_extension(c_atomic))
   /* Clang: Use prefixed C11-style atomics. */
   /* FIXME: Not working... (clang-3.9.0 complains that it's not declared.) */
#  define ATOMIC_ADD_FETCH(ptr, val) __c11_atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
#  define ATOMIC_OR_FETCH(ptr, val) __c11_atomic_or_fetch(ptr, val, __ATOMIC_SEQ_CST)
# else
   /* Use Itanium-style atomics. */
#  define ATOMIC_ADD_FETCH(ptr, val) __sync_add_and_fetch(ptr, val)
#  define ATOMIC_OR_FETCH(ptr, val) __sync_or_and_fetch(ptr, val)
# endif
#elif defined(__GNUC__)
# if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
   /* gcc-4.7: Use prefixed C11-style atomics. */
#  define ATOMIC_ADD_FETCH(ptr, val) __atomic_add_fetch(ptr, val, __ATOMIC_SEQ_CST)
#  define ATOMIC_OR_FETCH(ptr, val) __atomic_or_fetch(ptr, val, __ATOMIC_SEQ_CST)
# else
   /* gcc-4.6 and earlier: Use Itanium-style atomics. */
#  define ATOMIC_ADD_FETCH(ptr, val) __sync_add_and_fetch(ptr, val)
#  define ATOMIC_OR_FETCH(ptr, val) __sync_or_and_fetch(ptr, val)
# endif
#endif

namespace LibRomData { namespace FileSystem {

// Configuration directories.
static int init_counter = -1;
static volatile int dir_is_init = 0;
// User's cache directory.
static rp_string cache_dir;
// User's configuration directory.
static rp_string config_dir;

/**
 * Recursively mkdir() subdirectories.
 *
 * The last element in the path will be ignored, so if
 * the entire pathname is a directory, a trailing slash
 * must be included.
 *
 * NOTE: Only native separators ('\\' on Windows, '/' on everything else)
 * are supported by this function.
 *
 * @param path Path to recursively mkdir. (last component is ignored)
 * @return 0 on success; negative POSIX error code on error.
 */
int rmkdir(const rp_string &path)
{
	// Linux (and most other systems) use UTF-8 natively.
	string path8 = rp_string_to_utf8(path);

	// Find all slashes and ensure the directory component exists.
	size_t slash_pos = path8.find(DIR_SEP_CHR, 0);
	if (slash_pos == 0) {
		// Root is always present.
		slash_pos = path8.find(DIR_SEP_CHR, 1);
	}

	while (slash_pos != string::npos) {
		// Temporarily NULL out this slash.
		path8[slash_pos] = 0;

		// Attempt to create this directory.
		if (::mkdir(path8.c_str(), 0777) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		path8[slash_pos] = DIR_SEP_CHR;
		slash_pos++;

		// Find the next slash.
		slash_pos = path8.find(DIR_SEP_CHR, slash_pos);
	}

	// rmkdir() succeeded.
	return 0;
}

/**
 * Does a file exist?
 * @param pathname Pathname.
 * @param mode Mode.
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const rp_string &pathname, int mode)
{
#if defined(RP_UTF16)
	string pathname8 = rp_string_to_utf8(pathname);
	return ::access(pathname8.c_str(), mode);
#elif defined(RP_UTF8)
	return ::access(pathname.c_str(), mode);
#endif
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const rp_string &filename)
{
	struct stat buf;
#if defined(RP_UTF16)
	string filename8 = rp_string_to_utf8(filename);
	int ret = stat(filename8.c_str(), &buf);
#elif defined(RP_UTF8)
	int ret = stat(filename.c_str(), &buf);
#endif

	if (ret != 0) {
		// stat() failed.
		ret = -errno;
		if (ret == 0) {
			// Something happened...
			ret = -EINVAL;
		}

		return ret;
	}

	// Return the file size.
	return buf.st_size;
}

/**
 * Initialize the configuration directory paths.
 */
static void initConfigDirectories(void)
{
	// How this works:
	// - init_counter is initially -1.
	// - Incrementing it returns 0; this means that the
	//   directories have not been initialized yet.
	// - dir_is_init is set when initializing.
	// - If the counter wraps around, the directories won't be
	//   reinitialized because dir_is_init will be set.

	if (ATOMIC_ADD_FETCH(&init_counter, 1) != 0) {
		// Function has already been called.
		// Wait for directories to be initialized.
		while (ATOMIC_OR_FETCH(&dir_is_init, 0) == 0) {
			// TODO: Timeout counter?
			usleep(0);
		}
		return;
	}

	/** Home directory. **/
	// NOTE: The home directory is NOT cached.
	// It's only used to determine the other directories.

	rp_string home_dir;
	const char *const home = getenv("HOME");
	if (home && home[0] != 0) {
		// HOME variable is set.
		home_dir = utf8_to_rp_string(home);
	} else {
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
			return;
		}

		if (pwd_result->pw_dir[0] == 0) {
			// Empty home directory...
			return;
		}

		home_dir = utf8_to_rp_string(pwd_result->pw_dir, strlen(pwd_result->pw_dir));
	}
	if (home_dir.empty()) {
		// Unable to get the home directory...
		return;
	}

	/** Cache directory. **/
	// TODO: Check XDG variables.

	// Unix/Linux: Cache directory is ~/.cache/rom-properties/.
	// TODO: Mac OS X.
	cache_dir = home_dir;
	// Add a trailing slash if necessary.
	if (cache_dir.at(cache_dir.size()-1) != '/')
		cache_dir += _RP_CHR('/');
	// Append ".cache/rom-properties".
	cache_dir += _RP(".cache/rom-properties");

	/** Configuration directory. **/
	// TODO: Check XDG variables.

	// Unix/Linux: Cache directory is ~/.config/rom-properties/.
	// TODO: Mac OS X.
	config_dir = home_dir;
	// Add a trailing slash if necessary.
	if (config_dir.at(config_dir.size()-1) != '/')
		config_dir += _RP_CHR('/');
	// Append ".config/rom-properties".
	config_dir += _RP(".config/rom-properties");

	// Directories have been initialized.
	dir_is_init = 1;
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
const rp_string &getCacheDirectory(void)
{
	// NOTE: It's safe to check dir_is_init here, since it's
	// only ever set to 1 by our code.
	if (!dir_is_init) {
		initConfigDirectories();
	}
	return cache_dir;
}

/**
 * Get the user's rom-properties configuration directory.
 * This is usually one of the following:
 * - Windows: %APPDATA%\rom-properties
 * - Linux: ~/.config/rom-properties
 *
 * @return User's rom-properties configuration directory, or empty string on error.
 */
const rp_string &getConfigDirectory(void)
{
	// NOTE: It's safe to check dir_is_init here, since it's
	// only ever set to 1 by our code.
	if (!dir_is_init) {
		initConfigDirectories();
	}
	return config_dir;
}

/**
 * Set the modification timestamp of a file.
 * @param filename Filename.
 * @param mtime Modification time.
 * @return 0 on success; negative POSIX error code on error.
 */
int set_mtime(const rp_string &filename, time_t mtime)
{
	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
	struct utimbuf utbuf;
	utbuf.actime = time(nullptr);
	utbuf.modtime = mtime;
	int ret = utime(rp_string_to_utf8(filename).c_str(), &utbuf);

	return (ret == 0 ? 0 : -errno);
}

/**
 * Get the modification timestamp of a file.
 * @param filename Filename.
 * @param pMtime Buffer for the modification timestamp.
 * @return 0 on success; negative POSIX error code on error.
 */
int get_mtime(const rp_string &filename, time_t *pMtime)
{
	if (!pMtime)
		return -EINVAL;

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
	struct stat buf;
	int ret = stat(rp_string_to_utf8(filename).c_str(), &buf);
	if (ret == 0) {
		// stat() buffer retrieved.
		*pMtime = buf.st_mtime;
	}

	return (ret == 0 ? 0 : -errno);
}

/**
 * Delete a file.
 * @param filename Filename.
 * @return 0 on success; negative POSIX error code on error.
 */
int delete_file(const rp_char *filename)
{
	if (!filename || filename[0] == 0)
		return -EINVAL;

	int ret = unlink(rp_string_to_utf8(filename).c_str());
	if (ret != 0) {
		// Error deleting the file.
		ret = -errno;
	}

	return ret;
}

} }

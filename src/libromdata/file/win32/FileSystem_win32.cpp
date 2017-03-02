/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * FileSystem_posix.cpp: File system functions. (Win32 implementation)     *
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

#include "../FileSystem.hpp"

// libromdata
#include "TextFuncs.hpp"

// C includes.
#include <sys/utime.h>

// C includes. (C++ namespace)
#include <cstring>
#include <ctime>
#include <cwctype>

// C++ includes.
#include <string>
using std::string;
using std::u16string;
using std::wstring;

// Windows includes.
#include "../../RpWin32.hpp"
#include <shlobj.h>
#include <direct.h>

namespace LibRomData { namespace FileSystem {

// Configuration directories.
static LONG init_counter = -1;
static volatile LONG dir_is_init = 0;
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
	// Windows uses UTF-16 natively, so handle as UTF-16.
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else
#error Win32 must have a 16-bit wchar_t.
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif

	wstring path16 = RP2W_s(path);
	if (path16.size() == 3) {
		// 3 characters. Root directory is always present.
		return 0;
	} else if (path16.size() < 3) {
		// Less than 3 characters. Path isn't valid.
		return -EINVAL;
	}

	// Find all backslashes and ensure the directory component exists.
	// (Skip the drive letter and root backslash.)
	size_t slash_pos = 4;
	while ((slash_pos = path16.find((char16_t)DIR_SEP_CHR, slash_pos)) != string::npos) {
		// Temporarily NULL out this slash.
		path16[slash_pos] = 0;

		// Attempt to create this directory.
		if (::_wmkdir(path16.c_str()) != 0) {
			// Could not create the directory.
			// If it exists already, that's fine.
			// Otherwise, something went wrong.
			if (errno != EEXIST) {
				// Something went wrong.
				return -errno;
			}
		}

		// Put the slash back in.
		path16[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
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
	// Windows doesn't recognize X_OK.
	mode &= ~X_OK;
	return ::_waccess(RP2W_s(pathname), mode);
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const rp_string &filename)
{
	struct _stati64 buf;
	int ret = _wstati64(RP2W_s(filename), &buf);

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
	if (InterlockedIncrement(&init_counter) != 0 || dir_is_init) {
		// Function has already been called.
		// Wait for directories to be initialized.
		while (InterlockedExchangeAdd(&dir_is_init, 0) == 0) {
			// TODO: Timeout counter?
			Sleep(0);
		}
		return;
	}

	wchar_t path[MAX_PATH];
	HRESULT hr;

	/** Cache directory. **/

	// Windows: Get CSIDL_LOCAL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Local Settings\Application Data
	// - Windows Vista: C:\Users\username\AppData\Local
	hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		cache_dir = W2RP_c(path);
		if (!cache_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (cache_dir.at(cache_dir.size()-1) != _RP_CHR('\\')) {
				cache_dir += _RP_CHR('\\');
			}

			// Append "rom-properties\\cache".
			cache_dir += _RP("rom-properties\\cache");
		}
	}

	/** Configuration directory. **/

	// Windows: Get CSIDL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Application Data
	// - Windows Vista: C:\Users\username\AppData\Roaming
	hr = SHGetFolderPath(nullptr, CSIDL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		config_dir = W2RP_c(path);
		if (!config_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (config_dir.at(config_dir.size()-1) != _RP_CHR('\\')) {
				config_dir += _RP_CHR('\\');
			}

			// Append "rom-properties".
			config_dir += _RP("rom-properties");
		}
	}

	// Directories have been initialized.
	dir_is_init = true;
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
#if _USE_32BIT_TIME_T
#error 32-bit time_t is not supported. Get a newer compiler.
#endif
	struct __utimbuf64 utbuf;
	utbuf.actime = _time64(nullptr);
	utbuf.modtime = mtime;
	int ret = _wutime64(RP2W_s(filename), &utbuf);

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
	if (!pMtime) {
		return -EINVAL;
	}

	// FIXME: time_t is 32-bit on 32-bit Linux.
	// TODO: Add a static_warning() macro?
	// - http://stackoverflow.com/questions/8936063/does-there-exist-a-static-warning
#if _USE_32BIT_TIME_T
#error 32-bit time_t is not supported. Get a newer compiler.
#endif
	// Use GetFileTime() instead of _stati64().
	HANDLE hFile = CreateFile(RP2W_s(filename),
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile) {
		// Error opening the file.
		return -w32err_to_posix(GetLastError());
	}

	FILETIME mtime;
	BOOL bRet = GetFileTime(hFile, nullptr, nullptr, &mtime);
	CloseHandle(hFile);
	if (!bRet) {
		// Error getting the file time.
		return -w32err_to_posix(GetLastError());
	}

	// Convert to Unix timestamp.
	*pMtime = FileTimeToUnixTime(&mtime);
	return 0;
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
	int ret = 0;

	// If this is an absolute path, make sure it starts with
	// "\\?\" in order to support filenames longer than MAX_PATH.
	wstring filenameW;
	if (iswascii(filename[0]) && iswalpha(filename[0]) &&
	    filename[1] == _RP_CHR(':') && filename[2] == _RP_CHR('\\'))
	{
		// Absolute path. Prepend "\\?\" to the path.
		filenameW = L"\\\\?\\";
		filenameW += RP2W_c(filename);
	} else {
		// Not an absolute path, or "\\?\" is already
		// prepended. Use it as-is.
		filenameW = RP2W_c(filename);
	}

	BOOL bRet = DeleteFile(filenameW.c_str());
	if (!bRet) {
		// Error deleting file.
		ret = -w32err_to_posix(GetLastError());
	}

	return ret;
}

} }

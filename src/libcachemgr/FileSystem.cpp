/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * FileSystem.cpp: File system functions.                                  *
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
#include "libromdata/TextFuncs.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#include "libromdata/RpWin32.hpp"
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

// C++ includes.
#include <string>
using std::string;
using std::u16string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibCacheMgr {
namespace FileSystem {

// User's cache directory.
static LibRomData::rp_string cache_dir;

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
int rmkdir(const LibRomData::rp_string &path)
{
	// TODO: Combine the two code paths using a templated function?

#ifdef _WIN32
	// Windows uses UTF-16 natively, so handle as UTF-16.
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else
#error Win32 must have a 16-bit wchar_t.
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif

	wstring path16 = RP2W_s(path);

	// Find all backslashes and ensure the directory component exists.
	// (Skip the drive letter and root backslash.)
	size_t slash_pos = path16.find((char16_t)DIR_SEP_CHR, 4);

	do {
		// Temporarily NULL out this slash.
		path16[slash_pos] = 0;

		// Does the directory exist?
		if (!::_waccess(path16.c_str(), F_OK)) {
			// Directory component exists.
			// Put the slash back in.
			path16[slash_pos] = DIR_SEP_CHR;
			slash_pos++;
			continue;
		}

		// Attempt to create this directory.
		if (::_wmkdir(path16.c_str()) != 0) {
			// Error creating the directory.
			return -errno;
		}

		// Put the slash back in.
		path16[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
	} while ((slash_pos = path16.find((char16_t)DIR_SEP_CHR, slash_pos)) != string::npos);

#else /* !_WIN32 */
	// Linux (and most other systems) use UTF-8 natively.
	string path8 = LibRomData::rp_string_to_utf8(path);

	// Find all slashes and ensure the directory component exists.
	size_t slash_pos = path8.find(DIR_SEP_CHR, 0);
	if (slash_pos == 0) {
		// Root is always present.
		slash_pos = path8.find(DIR_SEP_CHR, 1);
	}

	do {
		// Temporarily NULL out this slash.
		path8[slash_pos] = 0;

		// Does the directory exist?
		if (!::access(path8.c_str(), F_OK)) {
			// Directory component exists.
			// Put the slash back in.
			path8[slash_pos] = DIR_SEP_CHR;
			slash_pos++;
			continue;
		}

		// Attempt to create this directory.
		int ret = mkdir(path8.c_str(), 0777);
		if (ret != 0) {
			// Error creating the directory.
			return -errno;
		}

		// Put the slash back in.
		path8[slash_pos] = DIR_SEP_CHR;
		slash_pos++;
	} while ((slash_pos = path8.find(DIR_SEP_CHR, slash_pos)) != string::npos);
#endif

	// rmkdir() succeeded.
	return 0;
}

/**
 * Does a file exist?
 * @param pathname Pathname.
 * @param mode Mode.
 * @return 0 if the file exists with the specified mode; non-zero if not.
 */
int access(const LibRomData::rp_string &pathname, int mode)
{
#ifdef _WIN32
	// Windows doesn't recognize X_OK.
	mode &= ~X_OK;
	return ::_waccess(RP2W_s(pathname), mode);

#else /* !_WIN32 */

#if defined(RP_UTF16)
	string pathname8 = LibRomData::rp_string_to_utf8(pathname);
	return ::access(pathname8.c_str(), mode);
#elif defined(RP_UTF8)
	return ::access(pathname.c_str(), mode);
#endif

#endif /* _WIN32 */

	// Should not get here...
	return -1;
}

/**
 * Get a file's size.
 * @param filename Filename.
 * @return Size on success; -1 on error.
 */
int64_t filesize(const LibRomData::rp_string &filename)
{
	int ret = -1;

#ifdef _WIN32
	struct _stati64 buf;
	ret = _wstati64(RP2W_s(filename), &buf);
#else /* !_WIN32 */

	struct stat buf;
#if defined(RP_UTF16)
	string filename8 = LibRomData::rp_string_to_utf8(filename);
	ret = stat(filename8.c_str(), &buf);
#elif defined(RP_UTF8)
	ret = stat(filename.c_str(), &buf);
#endif

#endif /* _WIN32 */

	if (ret != 0) {
		// stat() failed.
		ret = -errno;
		if (ret == 0) {
			// Something happened...
			ret = -EINVAL;
		}

		return ret;
	}

	// Return the filesize.
	return buf.st_size;
}

/**
 * Get the user's cache directory for ROM Properties.
 * This is usually one of the following:
 * - WinXP: %APPDATA%\Local Settings\rom-properties\cache
 * - WinVista: %LOCALAPPDATA%\rom-properties\cache
 * - Linux: ~/.cache/rom-properties
 *
 * @return Cache directory, or empty string on error.
 */
const LibRomData::rp_string &getCacheDirectory(void)
{
	if (!cache_dir.empty()) {
		// We already got the cache directory.
		return cache_dir;
	}

#ifdef _WIN32
	// Windows: Get CSIDL_LOCAL_APPDATA.
	// XP: C:\Documents and Settings\username\Local Settings\Application Data
	// Vista+: C:\Users\username\AppData\Local
	wchar_t path[MAX_PATH];
	HRESULT hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr != S_OK)
		return cache_dir;

	cache_dir = LibRomData::utf16_to_rp_string(reinterpret_cast<const char16_t*>(path), wcslen(path));
	if (cache_dir.empty())
		return cache_dir;

	// Add a trailing backslash if necessary.
	if (cache_dir.at(cache_dir.size()-1) != _RP_CHR('\\'))
		cache_dir += _RP_CHR('\\');

	// Append "rom-properties\\cache".
	cache_dir += _RP("rom-properties\\cache");
#else
	// Linux: Cache directory is ~/.cache/rom-properties/.
	// TODO: Mac OS X.
	const char *home = getenv("HOME");
	string path;
	if (home && home[0] != 0) {
		// HOME variable is set.
		path = home;
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
			return cache_dir;
		}

		if (pwd_result->pw_dir[0] == 0)
			return cache_dir;

		path = string(pwd_result->pw_dir);
	}

	// Add a trailing slash if necessary.
	if (path.at(path.size()-1) != '/')
		path += '/';

	// Append ".cache/rom-properties".
	path += ".cache/rom-properties";

	// Convert to rp_string.
	cache_dir = LibRomData::utf8_to_rp_string(path);
#endif

	return cache_dir;
}

} }

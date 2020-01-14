/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * userdirs.cpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;

// Windows SDK.
#include "RpWin32_sdk.h"
#include <shlobj.h>

namespace LibWin32Common {

/**
 * Internal T2U8() function.
 * @param wcs TCHAR string.
 * @return UTF-8 C++ string.
 */
#ifdef UNICODE
static inline string T2U8(const TCHAR *wcs)
{
	string s_ret;

	// NOTE: cbMbs includes the NULL terminator.
	int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 1) {
		return s_ret;
	}
	cbMbs--;
 
	char *mbs = static_cast<char*>(malloc(cbMbs));
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, mbs, cbMbs, nullptr, nullptr);
	s_ret.assign(mbs, cbMbs);
	free(mbs);
	return s_ret;
}
#else /* !UNICODE */
// TODO: Convert ANSI to UTF-8?
# define T2U8(mbs) (mbs)
#endif /* UNICODE */

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
	TCHAR path[MAX_PATH];
	HRESULT hr;

	// Windows: Get CSIDL_PROFILE.
	// - Windows XP: C:\Documents and Settings\username
	// - Windows Vista: C:\Users\username
	hr = SHGetFolderPath(nullptr, CSIDL_PROFILE,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		home_dir = T2U8(path);
		if (!home_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (home_dir.at(home_dir.size()-1) != '\\') {
				home_dir += '\\';
			}
		}
	}

	return home_dir;
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
	TCHAR path[MAX_PATH];
	HRESULT hr;

	// Windows: Get CSIDL_LOCAL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Local Settings\Application Data
	// - Windows Vista: C:\Users\username\AppData\Local
	hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		cache_dir = T2U8(path);
		if (!cache_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (cache_dir.at(cache_dir.size()-1) != '\\') {
				cache_dir += '\\';
			}
		}
	}

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
	TCHAR path[MAX_PATH];
	HRESULT hr;

	// Windows: Get CSIDL_APPDATA.
	// - Windows XP: C:\Documents and Settings\username\Application Data
	// - Windows Vista: C:\Users\username\AppData\Roaming
	hr = SHGetFolderPath(nullptr, CSIDL_APPDATA,
		nullptr, SHGFP_TYPE_CURRENT, path);
	if (hr == S_OK) {
		config_dir = T2U8(path);
		if (!config_dir.empty()) {
			// Add a trailing backslash if necessary.
			if (config_dir.at(config_dir.size()-1) != '\\') {
				config_dir += '\\';
			}
		}
	}

	return config_dir;
}

}

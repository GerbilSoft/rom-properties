/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * userdirs.cpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "userdirs.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes
#include <string>
using std::string;

// Windows SDK
#include "RpWin32_sdk.h"
#include <shlobj.h>

// librptext
#include "librptext/conversion.hpp"
#include "librptext/wchar.hpp"

namespace LibWin32Common {

/**
 * Get a CSIDL path using SHGetFolderPath().
 * @param csidl CSIDL.
 * @return Path (without trailing slash), or empty string on error.
 */
static string getCSIDLPath(int csidl)
{
	string s_path;
	TCHAR path[MAX_PATH];

	HRESULT hr = SHGetFolderPath(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, path);
	if (SUCCEEDED(hr)) {
		s_path = T2U8(path);
		if (!s_path.empty()) {
			// Remove the trailing backslash if necessary.
			if (s_path.at(s_path.size()-1) == '\\') {
				s_path.resize(s_path.size()-1);
			}
		}
	}

	return s_path;
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
	return getCSIDLPath(CSIDL_PROFILE);
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
	TCHAR szPath[MAX_PATH];

	// shell32.dll might be delay-loaded to avoid a gdi32.dll penalty.
	// Call SHGetFolderPath() with invalid parameters to load it into
	// memory before using GetModuleHandle().
	SHGetFolderPath(nullptr, 0, nullptr, 0, szPath);

	// Windows: Get FOLDERID_LocalAppDataLow. (WinXP and earlier: CSIDL_LOCAL_APPDATA)
	// LocalLow is preferred because it allows rp-download to run as a
	// low-integrity process on Windows Vista and later.
	// - Windows XP: C:\Documents and Settings\username\Local Settings\Application Data
	// - Windows Vista: C:\Users\username\AppData\LocalLow
	HMODULE hShell32_dll = GetModuleHandle(_T("shell32.dll"));
	assert(hShell32_dll != nullptr);
	if (!hShell32_dll) {
		// Not possible. Both SHGetFolderPath() and
		// SHGetKnownFolderPath() are in shell32.dll.
		return cache_dir;
	}

	// Check for SHGetKnownFolderPath. (Windows Vista and later)
	typedef HRESULT (WINAPI *PFNSHGETKNOWNFOLDERPATH)(
		_In_ REFKNOWNFOLDERID rfid,
		_In_ DWORD /* KNOWN_FOLDER_FLAG */ dwFlags,
		_In_opt_ HANDLE hToken,
		_Outptr_ PWSTR *ppszPath);
	PFNSHGETKNOWNFOLDERPATH pfnSHGetKnownFolderPath =
		(PFNSHGETKNOWNFOLDERPATH)GetProcAddress(hShell32_dll, "SHGetKnownFolderPath");
	if (pfnSHGetKnownFolderPath) {
		// We have SHGetKnownFolderPath. (NOTE: Unicode only!)
		PWSTR pszPath = nullptr;	// free with CoTaskMemFree()
		HRESULT hr = pfnSHGetKnownFolderPath(FOLDERID_LocalAppDataLow,
			SHGFP_TYPE_CURRENT, nullptr, &pszPath);
		if (SUCCEEDED(hr) && pszPath != nullptr) {
			// Path obtained.
			cache_dir = W2U8(pszPath);
		}
		CoTaskMemFree(pszPath);
		pszPath = nullptr;

		if (cache_dir.empty()) {
			// SHGetKnownFolderPath(FOLDERID_LocalAppDataLow) failed.
			// Try again with FOLDERID_LocalAppData.
			// NOTE: This might cause problems if rp-download is running
			// with a low integrity level.
			hr = pfnSHGetKnownFolderPath(FOLDERID_LocalAppData,
				SHGFP_TYPE_CURRENT, nullptr, &pszPath);
			if (SUCCEEDED(hr) && pszPath != nullptr) {
				// Path obtained.
				cache_dir = W2U8(pszPath);
			}
			CoTaskMemFree(pszPath);
			pszPath = nullptr;
		}
	}

	if (cache_dir.empty()) {
		// SHGetKnownFolderPath() either isn't available
		// or failed. Fall back to SHGetFolderPath().
		szPath[0] = _T('\0');
		HRESULT hr = SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA,
			nullptr, SHGFP_TYPE_CURRENT, szPath);
		if (SUCCEEDED(hr)) {
			cache_dir = T2U8(szPath);
		}
	}

	// We're done trying to obtain the cache directory.
	if (!cache_dir.empty()) {
		// Remove the trailing backslash if necessary.
		if (cache_dir.at(cache_dir.size()-1) == '\\') {
			cache_dir.resize(cache_dir.size()-1);
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
	return getCSIDLPath(CSIDL_APPDATA);
}

} //namespace LibWin32Common

/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * rp-download.cpp: Standalone cache downloader.                           *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

// C includes. (C++ namespace)
#include <cstdio>
#include <cstring>
#include <stdlib.h>

// C++ includes.
#include <string>
#ifdef _WIN32
using std::wstring;
# define tstring wstring
#else
using std::string;
# define tstring string
#endif /* _WIN32 */

#ifdef _WIN32
// libwin32common
#include "libwin32common/RpWin32_sdk.h"
#include "libwin32common/secoptions.h"
#endif /* _WIN32 */

// libcachecommon
#include "libcachecommon/CacheKeys.hpp"

// librpbase
#include "librpbase/common.h"

// TODO: tcharx.h?
#ifndef _WIN32
# define TCHAR char
# define _T(x) (x)
# define _tmain main
# define _tcschr strchr
# define _tcsncmp strncmp
# define _ftprintf fprintf
# define _tprintf printf
# define _sntprintf snprintf
#endif

#ifndef _countof
# define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

/**
 * rp-download: Download an image from a supported online database.
 * @param cache_key Cache key, e.g. "ds/cover/US/ADAE"
 * @return 0 on success; non-zero on error.
 *
 * TODO:
 * - More error codes based on the error.
 * - Verbose logging. (-v, --verbose)
 */
int RP_C_API _tmain(int argc, TCHAR *argv[])
{
	// Create a downloader based on OS:
	// - Linux: CurlDownloader
	// - Windows: UrlmonDownloader

	// Syntax: rp-download cache_key
	// Example: rp-download ds/coverM/US/ADAE

	// If http_proxy or https_proxy are set, they will be used
	// by the downloader code if supported.

#ifdef _WIN32
	// Set Win32 security options.
	secoptions_init();
#endif /* _WIN32 */

	if (argc < 2) {
		// TODO: Add a verbose option to print messages.
		// Normally, the only output is a return value.
		_ftprintf(stderr, _T("Syntax: %s cache_key\n"), argv[0]);
		return EXIT_FAILURE;
	}

	// Check the cache key prefix. The prefix indicates the system
	// and identifies the online database used.
	// [key] indicates the cache key without the prefix.
	// - wii:    https://art.gametdb.com/wii/[key]
	// - wiiu:   https://art.gametdb.com/wiiu/[key]
	// - 3ds:    https://art.gametdb.com/3ds/[key]
	// - ds:     https://art.gametdb.com/3ds/[key]
	// - amiibo: https://amiibo.life/[key]/image
	const TCHAR *const cache_key = argv[1];
	const TCHAR *slash_pos = _tcschr(cache_key, _T('/'));
	if (slash_pos == nullptr) {
		// No slash. Not a valid cache key.
		return EXIT_FAILURE;
	} else if (slash_pos[1] == '\0') {
		// Slash is the last character.
		// Not a valid cache key.
		return EXIT_FAILURE;
	} else if (slash_pos == cache_key) {
		// Slash is the first character.
		// Not a valid cache key.
		return EXIT_FAILURE;
	}

	const ptrdiff_t prefix_len = (slash_pos - cache_key);
	if (prefix_len <= 0) {
		// Empty prefix.
		return EXIT_FAILURE;
	}

	// Determine the full URL based on the cache key.
	TCHAR full_url[256];
	const TCHAR *ext = _T(".png");	// default to PNG format
	if (prefix_len == 3 && !_tcsncmp(cache_key, _T("wii"), 3)) {
		// Wii
		// All supported images are in PNG format.
		ext = _T(".png");
		_sntprintf(full_url, _countof(full_url),
			_T("https://art.gametdb.com/%s%s"), cache_key, ext);
	} else if ((prefix_len == 5 && !_tcsncmp(cache_key, _T("wiiu"), 4)) ||
	           (prefix_len == 3 && !_tcsncmp(cache_key, _T("3ds"), 3)) ||
		       (prefix_len == 2 && !_tcsncmp(cache_key, _T("ds"), 2)))
	{
		// Wii U, Nintendo 3DS, Nintendo DS
		// "cover" and "coverfull" are in JPEG format.
		// All others are in PNG format.
		if (!_tcsncmp(slash_pos, _T("/cover/"), 7) ||
		    !_tcsncmp(slash_pos, _T("/coverfull/"), 11))
		{
			// JPEG format.
			ext = _T(".jpg");
		}
		_sntprintf(full_url, _countof(full_url),
			_T("https://art.gametdb.com/%s%s"), cache_key, ext);
	} else if (prefix_len == 6 && !_tcsncmp(cache_key, _T("amiibo"), 6)) {
		// amiibo.
		// All files are in PNG format.
		_sntprintf(full_url, _countof(full_url),
			_T("https://amiibo.life/nfc/%s/image"), slash_pos+1);
	} else {
		// Prefix is not supported.
		return EXIT_FAILURE;
	}

	_tprintf(_T("URL: %s\n"), full_url);

	// Get the cache filename.
	tstring cache_filename = LibCacheCommon::getCacheFilename(cache_key);
	if (cache_filename.empty()) {
		// Invalid cache filename.
		return EXIT_FAILURE;
	}
	cache_filename += ext;
	_tprintf(_T("Cache Filename: %s\n"), cache_filename.c_str());

	// Success.
	return EXIT_SUCCESS;
}

/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon)                   *
 * CacheKeys.cpp: Cache key handling functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.libcachecommon.h"
#include "CacheKeys.hpp"
#include "CacheDir.hpp"

// C includes (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdint.h>

// C++ STL classes
#include <array>
#include <string>
using std::array;
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

// OS-specific directory separator.
#ifdef _WIN32
#  include "libwin32common/RpWin32_sdk.h"
#  define DIR_SEP_CHR '\\'
#  define DIR_SEP_WCHR L'\\'
#else /* !_WIN32 */
#  define DIR_SEP_CHR '/'
#  include <unistd.h>	/* for R_OK */
#endif /* _WIN32 */

namespace LibCacheCommon {

// Don't allow control characters, invalid FAT32 characters, or dots.
// '/' is allowed for cache hierarchy. (Converted to '\\' on Windows.)
// '.' is allowed for file extensions.
// (NOTE: '/' and '.' are allowed for extensions and cache hierarchy.)
// Reference: https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
// Values:
// - 0: Not allowed (converted to '_')
// - 1: Allowed
// - 2: Dot
// - 3: Slash
// - 4: Backslash or colon (error)
static constexpr array<uint8_t, 0x80> valid_ascii_tbl = {{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x00
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10
	1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 3, // 0x20 (", *, ., /)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0, 1, 0, 0, // 0x30 (:, <, >, ?)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, // 0x50 (\\)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, // 0x80 (|)
}};

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8, NULL-terminated.)
 * @return 0 on success; -EINVAL if pCacheKey is invalid.
 */
int filterCacheKey(char *pCacheKey)
{
	bool foundSlash = true;
	int dotCount = 0;

	// Quick check: Ensure the cache key is not empty and
	// that it doesn't start with a path separator.
	if (!pCacheKey || pCacheKey[0] == '\0' ||
	    pCacheKey[0] == '/' || pCacheKey[0] == '\\')
	{
		// Cache key is either empty or starts with
		// a path separator.
		return -EINVAL;
	}

	for (char *p = pCacheKey; *p != '\0'; p++) {
		// See valid_ascii_table for a description of valid characters.
		const uint8_t chr = (uint8_t)*p;
		if (chr & 0x80) {
			// Start of UTF-8 sequence.
			// Verify that the sequence is valid.
			// NOTE: Checking for 0x80 first because most cache keys
			// will be ASCII, not UTF-8.

			// TODO: Remove extra bytes?
			// NOTE: NULL check isn't needed, since these tests will all
			// fail if a NULL byte is encountered.
			if (((chr  & 0xE0) == 0xC0) &&
			    ((p[1] & 0xC0) == 0x80))
			{
				// Two-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr2 =
					((chr  & 0x1F) <<  6) |
					 (p[1] & 0x3F);
				if (uchr2 < 0x80) {
					// Overlong sequence. Not allowed.
					*p = '_';
					continue;
				}

				// Sequence is not overlong.
				p++;
				continue;
			}
			else if (((chr  & 0xF0) == 0xE0) &&
				 ((p[1] & 0xC0) == 0x80) &&
				 ((p[2] & 0xC0) == 0x80))
			{
				// Three-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr3 =
					((chr  & 0x0F) << 12) |
					((p[1] & 0x3F) <<  6) |
					 (p[2] & 0x3F);
				if (uchr3 < 0x800) {
					// Overlong sequence. Not allowed.
					*p = '_';
					continue;
				}

				// Sequence is not overlong.
				p += 2;
				continue;
			}
			else if (((chr  & 0xF8) == 0xF0) &&
				 ((p[1] & 0xC0) == 0x80) &&
				 ((p[2] & 0xC0) == 0x80) &&
				 ((p[3] & 0xC0) == 0x80))
			{
				// Four-byte sequence.
				// Verify that it is not overlong.
				const unsigned int uchr4 =
					((chr  & 0x07) << 18) |
					((p[1] & 0x3F) << 12) |
					((p[2] & 0x3F) <<  6) |
					 (p[3] & 0x3F);
				if (uchr4 < 0x10000) {
					// Overlong sequence. Not allowed.
					*p = '_';
					continue;
				}

				// Sequence is not overlong.
				p += 3;
				continue;
			}

			// Invalid UTF-8 sequence.
			*p = '_';
			continue;
		}

		switch (valid_ascii_tbl[chr] & 7) {
			case 0:
			default:
				// Invalid character.
				*p = '_';
				foundSlash = false;
				break;

			case 1:
				// Valid character.
				foundSlash = false;
				break;

			case 2:
				// Dot.
				// Check for "../" (or ".." at the end of the cache key).
				if (foundSlash) {
					dotCount++;
					if (dotCount >= 2) {
						// Invalid cache key.
						return -EINVAL;
					}
				}
				break;

			case 3:
				// Slash.
#ifdef _WIN32
				// Convert to backslash on Windows.
				*p = '\\';
#endif /* _WIN32 */
				foundSlash = true;
				dotCount = 0;
				break;

			case 4:
				// Backslash or colon.
				// Not allowed at all.
				return -EINVAL;
		}
	}

	// Cache key has been filtered.
	return 0;
}

#ifdef RP_WIS16
/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-16, NULL-terminated.)
 * @return 0 on success; -EINVAL if pCacheKey is invalid.
 */
int filterCacheKey(wchar_t *pCacheKey)
{
	static_assert(sizeof(wchar_t) == 2, "sizeof(wchar_t) IS NOT 2!");
	bool foundSlash = true;
	int dotCount = 0;

	// Quick check: Ensure the cache key is not empty and
	// that it doesn't start with a path separator.
	if (!pCacheKey || pCacheKey[0] == L'\0' ||
	    pCacheKey[0] == L'/' || pCacheKey[0] == L'\\')
	{
		// Cache key is either empty or starts with
		// a path separator.
		return -EINVAL;
	}

	for (wchar_t *p = pCacheKey; *p != L'\0'; p++) {
		// See valid_ascii_table for a description of valid characters.
		const wchar_t chr = *p;

		// Surrogate pair check
		const wchar_t schk = (chr & 0xDC00);
		if (schk == 0xD800) {
			// High surrogate: Start of UTF-16 surrogate pair.
			// Verify that the next code point is a low surrogate.
			if ((p[1] & 0xDC00) != 0xDC00) {
				// Not a low surrogate.
				// This is invalid.
				*p = L'_';
				continue;
			}

			// This is a low surrogate.
			p++;
			continue;
		} else if (schk == 0xDC00) {
			// Unpaired low surrogate.
			// This is invalid.
			*p = L'_';
			continue;
		}

		// Not a surrogate pair character.
		// Check for invalid ASCII characters.
		if (chr < 0x80) {
			switch (valid_ascii_tbl[chr] & 7) {
				case 0:
				default:
					// Invalid character.
					*p = L'_';
					foundSlash = false;
					break;

				case 1:
					// Valid character.
					foundSlash = false;
					break;

				case 2:
					// Dot.
					// Check for "../" (or ".." at the end of the cache key).
					if (foundSlash) {
						dotCount++;
						if (dotCount >= 2) {
							// Invalid cache key.
							return -EINVAL;
						}
					}
					break;

				case 3:
					// Slash.
#ifdef _WIN32
					// Convert to backslash on Windows.
					*p = L'\\';
#endif /* _WIN32 */
					foundSlash = true;
					dotCount = 0;
					break;

				case 4:
					// Backslash or colon.
					// Not allowed at all.
					return -EINVAL;
			}
		}
	}

	// Cache key has been filtered.
	return 0;
}
#endif /* RP_WIS16 */

/**
 * Combine a cache key with the cache directory to get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-8, NULL-terminated.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
string getCacheFilename(const char *pCacheKey)
{
	assert(pCacheKey != nullptr);
	assert(pCacheKey[0] != '\0');
	if (!pCacheKey || pCacheKey[0] == '\0') {
		// No cache key...
		return {};
	}

	// Filter the cache key.
	string filteredCacheKey = pCacheKey;
	int ret = filterCacheKey(filteredCacheKey);
	if (ret != 0) {
		// Invalid cache key.
		return {};
	}

	// Cache filename in the user's directory.
	string cacheFilename_user;

	// Make sure the cache directory is initialized.
	// NOTE: May be empty if the cache directory isn't
	// accessible, e.g. when running under bubblewrap.
	const string &cache_dir = getCacheDirectory();
	if (!cache_dir.empty()) {
		// Get the cache filename.
		// This is the cache directory plus the cache key.
		cacheFilename_user = cache_dir;
		if (cacheFilename_user.at(cacheFilename_user.size()-1) != DIR_SEP_CHR) {
			cacheFilename_user += DIR_SEP_CHR;
		}
		cacheFilename_user += filteredCacheKey;
	}

#ifdef DIR_INSTALL_CACHE
	// If the requested file is in the system-wide cache directory,
	// but is not in the user's cache directory, use the system-wide
	// version. This is useful in cases where the thumbnailer cannot
	// download files, e.g. bubblewrap.
	string cacheFilename_sys = DIR_INSTALL_CACHE;
	if (cacheFilename_sys.at(cacheFilename_sys.size()-1) != DIR_SEP_CHR) {
		cacheFilename_sys += DIR_SEP_CHR;
	}
	cacheFilename_sys += filteredCacheKey;

	if (!access(cacheFilename_sys.c_str(), R_OK)) {
		// File is in the system-wide cache.
		if (!cacheFilename_user.empty() && !access(cacheFilename_user.c_str(), R_OK)) {
			// File is also in the user's cache.
			// User version overrides the system version.
			return cacheFilename_user;
		}
		// File is not in the user's cache.
		return cacheFilename_sys;
	}
#endif /* DIR_INSTALL_CACHE */

	return cacheFilename_user;
}

#ifdef _WIN32
/**
 * Internal U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ string.
 */
static inline wstring U82W(const string &mbs)
{
	wstring s_wcs;

	const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs.c_str(), static_cast<int>(mbs.size()), nullptr, 0);
	if (cchWcs <= 0) {
		return s_wcs;
	}

	s_wcs.resize(cchWcs);
	MultiByteToWideChar(CP_UTF8, 0, mbs.c_str(), static_cast<int>(mbs.size()), &s_wcs[0], cchWcs);
	return s_wcs;
}

/**
 * Combine a cache key with the cache directory to get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-16.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
wstring getCacheFilename(const wchar_t *pCacheKey)
{
	wstring cacheFilename_user;
	assert(pCacheKey != nullptr);
	assert(pCacheKey[0] != L'\0');
	if (!pCacheKey || pCacheKey[0] == L'\0') {
		// No cache key...
		return cacheFilename_user;
	}

	// Make sure the cache directory is initialized.
	const string &cache_dir = getCacheDirectory();
	if (cache_dir.empty()) {
		// Unable to get the cache directory.
		return cacheFilename_user;
	}

	// Filter the cache key.
	wstring filteredCacheKey = pCacheKey;
	int ret = filterCacheKey(filteredCacheKey);
	if (ret != 0) {
		// Invalid cache key.
		return cacheFilename_user;
	}

	// Get the cache filename.
	// This is the cache directory plus the cache key.
	cacheFilename_user = U82W(cache_dir);
	if (cacheFilename_user.at(cacheFilename_user.size()-1) != DIR_SEP_WCHR) {
		cacheFilename_user += DIR_SEP_WCHR;
	}
	cacheFilename_user += filteredCacheKey;
	return cacheFilename_user;
}
#endif /* _WIN32 */

/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
string urlencode(const char *url)
{
	string s_ret;
	s_ret.reserve(strlen(url)+8);

	assert(url != nullptr);
	for (; *url != '\0'; url++) {
		const uint8_t chr = static_cast<uint8_t>(*url);
		if (chr & 0x80) {
			// UTF-8 code sequence.
			char buf[8];
			snprintf(buf, sizeof(buf), "%%%02X", chr);
			s_ret += buf;
		} else {
			switch (*url) {
				case ' ':
					s_ret += "%20";
					break;
				case '#':
					s_ret += "%23";
					break;
				case '%':
					s_ret += "%25";
					break;
				case '^':
					s_ret += "%5E";
					break;
				default:
					s_ret += *url;
					break;
			}
		}
	}

	return s_ret;
}

#ifdef _WIN32
/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
wstring urlencode(const wchar_t *url)
{
	wstring ws_ret;
	ws_ret.reserve(wcslen(url)+8);

	assert(url != nullptr);
	for (; *url != L'\0'; url++) {
		switch (*url) {
			case L' ':
				ws_ret += L"%20";
				break;
			case L'#':
				ws_ret += L"%23";
				break;
			case L'%':
				ws_ret += L"%25";
				break;
			case L'^':
				ws_ret += L"%5E";
				break;
			default:
				ws_ret += *url;
				break;
		}
	}

	return ws_ret;
}
#endif /* _WIN32 */

} //namespace LibCacheCommon

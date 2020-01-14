/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * cache_common.cpp: Common caching functions.                               *
 * Shared between libcachemgr and rp-download.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "cache_common.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <stdint.h>

// C++ STL classes.
using std::string;

// librpthreads
#include "librpthreads/pthread_once.h"

// OS-specific userdirs
#ifdef _WIN32
# include "libwin32common/userdirs.hpp"
# define OS_NAMESPACE LibWin32Common
# define DIR_SEP_CHR '\\'
#else
# include "libunixcommon/userdirs.hpp"
# define OS_NAMESPACE LibUnixCommon
# define DIR_SEP_CHR '/'
#endif

namespace LibCacheCommon {

/** Configuration directories. **/
// pthread_once() control variable.
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
// User's cache directory.
static string cache_dir;

/**
 * Initialize the cache directory.
 * Called by pthread_once().
 */
static void initCacheDirectory(void)
{
	// Uses LibUnixCommon or LibWin32Common, depending on platform.

	cache_dir = OS_NAMESPACE::getCacheDirectory();
	if (!cache_dir.empty()) {
		// Add a trailing slash if necessary.
		if (cache_dir.at(cache_dir.size()-1) != DIR_SEP_CHR)
			cache_dir += DIR_SEP_CHR;
#ifdef _WIN32
		// Append "rom-properties\\cache".
		cache_dir += "rom-properties\\cache";
#else /* !_WIN32 */
		// Append "rom-properties".
		cache_dir += "rom-properties";
#endif /* _WIN32 */
	}
}

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8.)
 * @return 0 on success; -EINVAL if pCacheKey is invalid.
 */
int filterCacheKey(string &cacheKey)
{
	bool foundSlash = true;
	int dotCount = 0;

	// Quick check: Ensure the cache key is not empty and
	// that it doesn't start with a path separator.
	if (cacheKey.empty() || cacheKey[0] == '/' || cacheKey[0] == '\\') {
		// Cache key is either empty or starts with
		// a path separator.
		return -EINVAL;
	}

	for (auto p = cacheKey.begin(); p != cacheKey.end(); ++p) {
		// Don't allow control characters, invalid FAT32 characters, or dots.
		// '/' is allowed for cache hierarchy. (Converted to '\\' on Windows.)
		// '.' is allowed for file extensions.
		// (NOTE: '/' and '.' are allowed for extensions and cache hierarchy.)
		// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
		// Values:
		// - 0: Not allowed (converted to '_')
		// - 1: Allowed
		// - 2: Dot
		// - 3: Slash
		// - 4: Backslash or colon (error)
		static const uint8_t valid_ascii_tbl[0x80] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x00
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
			1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 3, // 0x20 (", *, ., /)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0, 1, 0, 0,	// 0x30 (:, <, >, ?)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, // 0x50 (\\)
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, // 0x80 (|)
		};
		uint8_t chr = (uint8_t)*p;
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

/**
 * Combine a cache key with the cache directory to get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-8.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
std::string getCacheFilename(const std::string &cacheKey)
{
	string cacheFilename;
	assert(!cacheKey.empty());
	if (cacheKey.empty()) {
		// No cache key...
		return cacheFilename;
	}

	// Make sure the cache directory is initialized.
	pthread_once(&once_control, initCacheDirectory);
	assert(!cache_dir.empty());
	if (cache_dir.empty()) {
		// Unable to get the cache directory.
		return cacheFilename;
	}

	// Filter the cache key.
	string filteredCacheKey = cacheKey;
	int ret = filterCacheKey(filteredCacheKey);
	if (ret != 0) {
		// Invalid cache key.
		return cacheFilename;
	}

	// Get the cache filename.
	// This is the cache directory plus the cache key.
	cacheFilename = cache_dir;
	if (cacheFilename.at(cacheFilename.size()-1) != DIR_SEP_CHR) {
		cacheFilename += DIR_SEP_CHR;
	}

	// Append the filtered cache key.
	cacheFilename += filteredCacheKey;
	return cacheFilename;
}

}

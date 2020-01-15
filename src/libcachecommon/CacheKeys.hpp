/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon)                   *
 * CacheKeys.cpp: Cache key handling functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHECOMMON_CACHEKEYS_HPP__
#define __ROMPROPERTIES_LIBCACHECOMMON_CACHEKEYS_HPP__

// C++ includes.
#include <string>

namespace LibCacheCommon {

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8, NULL-terminated.)
 * @return 0 on success; negative POSIX error code on error.
 */
int filterCacheKey(char *pCacheKey);

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8.)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int filterCacheKey(std::string &cacheKey)
{
	// Ensure cacheKey is NULL-terminated by calling c_str().
	cacheKey.c_str();
	return filterCacheKey(&cacheKey[0]);
}

#ifdef _WIN32
/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-16, NULL-terminated.)
 * @return 0 on success; negative POSIX error code on error.
 */
int filterCacheKey(wchar_t *pCacheKey);

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8.)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int filterCacheKey(std::wstring &cacheKey)
{
	// Ensure cacheKey is NULL-terminated by calling c_str().
	cacheKey.c_str();
	return filterCacheKey(&cacheKey[0]);
}
#endif /* _WIN32 */

/**
 * Get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-8, NULL-terminated.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
std::string getCacheFilename(const char *pCacheKey);

/**
 * Get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-8.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
static inline std::string getCacheFilename(const std::string &cacheKey)
{
	return getCacheFilename(cacheKey.c_str());
}

#ifdef _WIN32
/**
 * Get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-16, NULL-terminated.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
std::wstring getCacheFilename(const wchar_t *pCacheKey);

/**
 * Get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-16.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
static inline std::wstring getCacheFilename(const std::wstring &cacheKey)
{
	return getCacheFilename(cacheKey.c_str());
}
#endif /* _WIN32 */

}

#endif /* __ROMPROPERTIES_LIBCACHECOMMON_CACHEKEYS_HPP__ */

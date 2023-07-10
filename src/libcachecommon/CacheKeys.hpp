/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon)                   *
 * CacheKeys.cpp: Cache key handling functions.                            *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "dll-macros.h"			// for RP_LIBROMDATA_PUBLIC
#include "librptext/config.librptext.h"	// for RP_WIS16

// C++ includes.
#include <string>

namespace LibCacheCommon {

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param cacheKey Cache key. (Must be UTF-8, NULL-terminated.)
 * @return 0 on success; negative POSIX error code on error.
 */
RP_LIBROMDATA_PUBLIC
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

#ifdef RP_WIS16
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
 * @param cacheKey Cache key. (Must be UTF-16.)
 * @return 0 on success; negative POSIX error code on error.
 */
static inline int filterCacheKey(std::wstring &cacheKey)
{
	// Ensure cacheKey is NULL-terminated by calling c_str().
	cacheKey.c_str();
	return filterCacheKey(&cacheKey[0]);
}
#endif /* RP_WIS16 */

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

/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
std::string urlencode(const char *url);

/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
static inline std::string urlencode(const std::string &url)
{
	return urlencode(url.c_str());
}

#ifdef _WIN32
/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
std::wstring urlencode(const wchar_t *url);

/**
 * urlencode a URL component.
 * This only encodes essential characters, e.g. ' ' and '%'.
 * @param url URL component.
 * @return urlencoded URL component.
 */
static inline std::wstring urlencode(const std::wstring &url)
{
	return urlencode(url.c_str());
}
#endif /* _WIN32 */

}

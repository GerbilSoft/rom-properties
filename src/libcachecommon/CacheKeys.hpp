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
 * @param cacheKey Cache key. (Must be UTF-8.)
 * @return 0 on success; negative POSIX error code on error.
 */
int filterCacheKey(std::string &cacheKey);

/**
 * Get a cache filename.
 * @param cacheKey Cache key. (Must be UTF-8.) (Will be filtered using filterCacheKey().)
 * @return Cache filename, or empty string on error.
 */
std::string getCacheFilename(const std::string &cacheKey);

}

#endif /* __ROMPROPERTIES_LIBCACHECOMMON_CACHEKEYS_HPP__ */

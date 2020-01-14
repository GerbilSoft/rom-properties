/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * cache_common.hpp: Common caching functions.                             *
 * Shared between libcachemgr and rp-download.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_HPP__
#define __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_HPP__

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

}

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_HPP__ */

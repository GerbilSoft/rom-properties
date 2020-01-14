/***************************************************************************
 * ROM Properties Page shell extension. (libcachemgr)                      *
 * cache_common.h: Common caching functions.                               *
 * Shared between libcachemgr and rp-download.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_H__
#define __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Filter invalid characters from a cache key.
 * This updates the cache key in place.
 * @param pCacheKey Cache key. (Must be UTF-8, NULL-terminated.)
 * @return 0 on success; negative POSIX error code on error.
 */
int filterCacheKey(char *pCacheKey);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBCACHEMGR_CACHE_COMMON_H__ */

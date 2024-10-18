/***************************************************************************
 * ROM Properties Page shell extension. (rp-download)                      *
 * CacheKeyVerify.hpp: Cache key verifier.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes
#include <string>

// TCHAR
#include "tcharx.h"

namespace RpDownload {

enum class CacheKeyError {
	OK = 0,
	Invalid = 1,
	PrefixNotSupported = 2,
};

/**
 * Verify a cache key and convert it to a source URL.
 * @param outURL	[out] tstring for the source URL
 * @param check_newer	[out] If true, always check, but only download if newer (for [sys])
 * @param cache_key	[in] Cache key
 * @return CacheKeyError
 */
CacheKeyError verifyCacheKey(std::string &outURL, bool &check_newer, const TCHAR *cache_key);

} // namespace RpDownload

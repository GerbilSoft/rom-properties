/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon)                   *
 * CacheDir.hpp: Cache directory handler.                                  *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes.
#include <string>

namespace LibCacheCommon {

/**
 * Get the cache directory.
 *
 * NOTE: May return an empty string if the cache directory
 * isn't accessible, e.g. when running under bubblewrap.
 *
 * @return Cache directory, or empty string on error.
 */
const std::string &getCacheDirectory(void);

} //namespace LibCacheCommon

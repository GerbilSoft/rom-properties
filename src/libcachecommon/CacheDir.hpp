/***************************************************************************
 * ROM Properties Page shell extension. (libcachecommon)                   *
 * CacheDir.hpp: Cache directory handler.                                  *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBCACHECOMMON_CACHEDIR_HPP__
#define __ROMPROPERTIES_LIBCACHECOMMON_CACHEDIR_HPP__

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

}

#endif /* __ROMPROPERTIES_LIBCACHECOMMON_CACHEDIR_HPP__ */

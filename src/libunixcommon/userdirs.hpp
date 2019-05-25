/***************************************************************************
 * ROM Properties Page shell extension. (libunixcommon)                    *
 * userdirs.hpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBUNIXCOMMON_USERDIRS_HPP__
#define __ROMPROPERTIES_LIBUNIXCOMMON_USERDIRS_HPP__

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.

// C++ includes.
#include <string>

namespace LibUnixCommon {

/**
 * Get the user's home directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
std::string getHomeDirectory(void);

/**
 * Get the user's cache directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
std::string getCacheDirectory(void);

/**
 * Get the user's configuration directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's configuration directory (without trailing slash), or empty string on error.
 */
std::string getConfigDirectory(void);

}

#endif /* __ROMPROPERTIES_LIBUNIXCOMMON_UNIXDIRS_HPP__ */

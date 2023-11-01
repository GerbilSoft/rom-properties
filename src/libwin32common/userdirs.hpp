/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * userdirs.hpp: Find user directories.                                    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: All functions return 8-bit strings.
// This is usually encoded as UTF-8.
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C++ includes.
#include <string>

namespace LibWin32Common {

/**
 * Get the user's home directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
std::string getHomeDirectory(void);

/**
 * Get the user's cache directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's cache directory (without trailing slash), or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
std::string getCacheDirectory(void);

/**
 * Get the user's configuration directory.
 *
 * NOTE: This function does NOT cache the directory name.
 * Callers should cache it locally.
 *
 * @return User's configuration directory (without trailing slash), or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
std::string getConfigDirectory(void);

} //namespace LibWin32Common

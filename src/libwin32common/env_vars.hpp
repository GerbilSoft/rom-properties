/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * env_vars.hpp: Environment variable functions.                           *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_ENV_VARS_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_ENV_VARS_HPP__

// C++ includes.
#include <string>

// Windows SDK.
#include "RpWin32_sdk.h"

namespace LibWin32Common {

/**
 * Find the specified file in the system PATH.
 * @param szAppName File (usually an application name)
 * @return Full path, or empty string if not found.
 */
std::tstring findInPath(LPCTSTR szAppName);

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_ENV_VARS_HPP__ */

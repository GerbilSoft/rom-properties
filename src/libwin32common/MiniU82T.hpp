/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.hpp: Mini-U82T                                                 *
 * Use this in projects where librptext can't be used for some reason.     *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RpWin32_sdk.h"
#include "tcharx.h"

// C++ STL classes
#include <memory>
#include <string>

namespace LibWin32Common {

std::wstring U82W_int(const char *mbs);
#define U82W_c(str) (U82W_int(str).c_str())
#define U82W_s(str) (U82W_int(str.c_str()).c_str())

#ifdef _UNICODE
#  define U82T_c(str) (U82W_int(str).c_str())
#  define U82T_s(str) (U82W_int(str.c_str()).c_str())
#else /* _UNICODE */
static inline const char *U82T_c(const char *str)
{
	return str;
}
static inline const char *U82T_s(const std::string &str)
{
	return str.c_str();
}
#endif /* _UNICODE */

std::string W2U8(const wchar_t *wcs);
std::string W2U8(const std::wstring wstr);

std::string A2U8(const char *mbs);

}

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
#  define U82T_c(str) (str)
#  define U82T_s(str) (str.c_str())
#endif /* _UNICODE */

std::string W2U8(const wchar_t *wcs, int len = -1);
static inline std::string W2U8(const std::wstring &wstr)
{
	return W2U8(wstr.data(), static_cast<int>(wstr.size()));
}

std::string A2U8(const char *mbs, int len = -1);
static inline std::string A2U8(const std::string &astr)
{
	return A2U8(astr.data(), static_cast<int>(astr.size()));
}

#ifdef _UNICODE
static inline std::string T2U8(const std::wstring &wstr)
{
	return W2U8(wstr.data(), static_cast<int>(wstr.size()));
}
#else /* !_UNICODE */
static inline std::string T2U8(const std::string &str)
{
	return A2U8(str.data(), static_cast<int>(str.size()));
}
#endif /* _UNICODE */

}

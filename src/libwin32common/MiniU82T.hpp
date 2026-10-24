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
#include <string_view>

namespace LibWin32Common {

std::wstring U82W_int(std::string_view mbs);
#define U82W(str) (U82W_int(str).c_str())

#ifdef _UNICODE
#  define U82T(str) (U82W_int(str).c_str())
#else /* _UNICODE */
#  define U82T(str) (str)
#endif /* _UNICODE */

std::string W2U8(std::wstring_view wcs);
std::string A2U8(std::string_view mbs);

#ifdef _UNICODE
static inline std::string T2U8(std::wstring_view tcs)
{
	return W2U8(tcs);
}
#else /* !_UNICODE */
static inline std::string T2U8(std::string_view tcs)
{
	return A2U8(tcs);
}
#endif /* _UNICODE */

}

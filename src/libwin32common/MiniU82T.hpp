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

std::wstring U82W_str(std::string_view mbs);
#define U82W(mbs) (U82W_str(mbs).c_str())

static inline std::string U82A_str(std::string_view mbs)
{
	// FIXME: Actually convert from ANSI?
	return std::string(mbs);
}
#define U82A(mbs) (U82A_str(mbs).c_str())

#ifdef _UNICODE
#  define U82T_str(mbs) U82W_str(mbs)
#  define U82T(mbs)     U82W(mbs)
#else /* _UNICODE */
#  define U82T_str(mbs) U82A_str(mbs)
#  define U82T(mbs)     U82A(mbs)
#endif /* _UNICODE */

std::string W2U8_str(std::wstring_view wcs);
#define W2U8(wcs) (W2U8_str(wcs).c_str())

std::string A2U8_str(std::string_view mbs);
#define A2U8(mbs) (A2U8_str(mbs).c_str())

#ifdef _UNICODE
#  define T2U8_str(mbs) W2U8_str(mbs)
#  define T2U8(mbs)     W2U8(mbs)
#else /* _UNICODE */
#  define T2U8_str(mbs) A2U8_str(mbs)
#  define T2U8(mbs)     A2U8(mbs)
#endif /* _UNICODE */

}

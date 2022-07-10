/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.cpp: Minimal U82T()/T2U8() functions.                          *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_MINIU82T_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_MINIU82T_HPP__

// C++ includes.
#include <string>

// Windows SDK.
#include "RpWin32_sdk.h"
#include "tcharx.h"

namespace LibWin32Common {

/**
 * Mini T2U8() function.
 * @param wcs TCHAR string.
 * @return UTF-8 C++ string.
 */
#ifdef UNICODE
std::u8string T2U8_c(const TCHAR *wcs);
#else /* !UNICODE */
static inline std::u8string T2U8_c(const TCHAR *mbs)
{
	// TODO: Convert ANSI to UTF-8?
	return reinterpret_cast<const char8_t*>(mbs);
}
#endif /* UNICODE */
static inline std::u8string T2U8_s(const std::tstring &mbs)
{
	return T2U8_c(mbs.c_str());
}

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ wstring.
 */
std::wstring U82W_c(const char8_t *mbs);
static inline std::wstring U82W_s(const std::u8string &mbs)
{
	return U82W_c(mbs.c_str());
}

/**
 * Mini U82T() function.
 * @param mbs UTF-8 string.
 * @return TCHAR C++ string.
 */
static inline std::tstring U82T_c(const char8_t *mbs)
{
#ifdef UNICODE
	return U82W_c(mbs);
#else /* !UNICODE */
	// TODO: Convert UTF-8 to ANSI?
	return mbs;
#endif /* UNICODE */
}
static inline std::tstring U82T_s(const std::u8string &mbs)
{
	return U82T_c(mbs.c_str());
}

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_MINIU82T_HPP__ */

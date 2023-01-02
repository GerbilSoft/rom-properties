/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.cpp: Minimal U82T()/T2U8() functions.                          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// C++ includes.
#include <string>

// Windows SDK.
#include "RpWin32_sdk.h"
#include "tcharx.h"

namespace LibWin32Common {

/**
 * Mini W2U8() function.
 * @param wcs WCHAR string
 * @return UTF-8 C++ string
 */
std::string W2U8(const wchar_t *wcs);
static inline std::string W2U8(const std::wstring &wcs)
{
	return W2U8(wcs.c_str());
}

/**
 * Mini T2U8() function.
 * @param wcs TCHAR string
 * @return UTF-8 C++ string
 */
static inline std::string T2U8(const TCHAR *tcs)
{
#ifdef UNICODE
	return W2U8(tcs);
#else /* !UNICODE */
	// TODO: Convert ANSI to UTF-8?
	return tcs;
#endif /* UNICODE */
}
static inline std::string T2U8(const std::tstring &tcs)
{
	return T2U8(tcs.c_str());
}

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string
 * @return UTF-16 C++ wstring
 */
std::wstring U82W(const char *mbs);
static inline std::wstring U82W(const std::string &mbs)
{
	return U82W(mbs.c_str());
}

/**
 * Mini U82T() function.
 * @param mbs UTF-8 string
 * @return TCHAR C++ string
 */
static inline std::tstring U82T(const char *mbs)
{
#ifdef UNICODE
	return U82W(mbs);
#else /* !UNICODE */
	// TODO: Convert UTF-8 to ANSI?
	return mbs;
#endif /* UNICODE */
}
static inline std::tstring U82T(const std::string &mbs)
{
	return U82T(mbs.c_str());
}

}

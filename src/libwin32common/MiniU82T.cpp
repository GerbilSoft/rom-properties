/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.cpp: Minimal U82T()/T2U8() functions.                          *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "MiniU82T.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::u8string;
using std::wstring;

namespace LibWin32Common {

/**
 * Mini T2U8() function.
 * @param wcs TCHAR string.
 * @return UTF-8 C++ string.
 */
#ifdef UNICODE
u8string T2U8_c(const TCHAR *wcs)
{
	u8string s_ret;

	// NOTE: cbMbs includes the NULL terminator.
	int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 1) {
		return s_ret;
	}
	cbMbs--;
 
	char8_t *const mbs = new char8_t[cbMbs];
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, reinterpret_cast<LPSTR>(mbs), cbMbs, nullptr, nullptr);
	s_ret.assign(mbs, cbMbs);
	delete[] mbs;
	return s_ret;
}
#endif /* UNICODE */

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ wstring.
 */
wstring U82W_c(const char8_t *mbs)
{
	wstring ws_ret;

	// NOTE: cchWcs includes the NULL terminator.
	int cchWcs = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(mbs), -1, nullptr, 0);
	if (cchWcs <= 1) {
		return ws_ret;
	}
	cchWcs--;
 
	wchar_t *const wcs = new wchar_t[cchWcs];
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<LPCCH>(mbs), -1, wcs, cchWcs);
	ws_ret.assign(wcs, cchWcs);
	delete[] wcs;
	return ws_ret;
}

}

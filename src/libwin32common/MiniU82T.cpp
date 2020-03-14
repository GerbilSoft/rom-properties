/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.cpp: Minimal U82T()/T2U8() functions.                          *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "MiniU82T.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::string;
using std::wstring;

namespace LibWin32Common {

/**
 * Mini T2U8() function.
 * @param wcs TCHAR string.
 * @return UTF-8 C++ string.
 */
#ifdef UNICODE
string T2U8_c(const TCHAR *wcs)
{
	string s_ret;

	// NOTE: cbMbs includes the NULL terminator.
	int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 1) {
		return s_ret;
	}
	cbMbs--;
 
	char *mbs = static_cast<char*>(malloc(cbMbs * sizeof(char)));
	assert(mbs != nullptr);
	if (!mbs) {
		return s_ret;
	}
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, mbs, cbMbs, nullptr, nullptr);
	s_ret.assign(mbs, cbMbs);
	free(mbs);
	return s_ret;
}
#endif /* UNICODE */

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ wstring.
 */
wstring U82W_c(const char *mbs)
{
	tstring ts_ret;

	// NOTE: cchWcs includes the NULL terminator.
	int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 1) {
		return ts_ret;
	}
	cchWcs--;
 
	wchar_t *wcs = static_cast<wchar_t*>(malloc(cchWcs * sizeof(wchar_t)));
	assert(wcs != nullptr);
	if (!wcs) {
		return ts_ret;
	}
	MultiByteToWideChar(CP_UTF8, 0, mbs, -1, wcs, cchWcs);
	ts_ret.assign(wcs, cchWcs);
	free(wcs);
	return ts_ret;
}

}

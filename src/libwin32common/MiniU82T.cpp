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
using std::string;
using std::wstring;

namespace LibWin32Common {

/**
 * Mini W2U8() function.
 * @param wcs WCHAR string
 * @return UTF-8 C++ string
 */
string W2U8(const wchar_t *wcs)
{
	string s_mbs;

	// NOTE: cbMbs includes the NULL terminator.
	int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 1) {
		return s_mbs;
	}
	cbMbs--;
 
	s_mbs.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, &s_mbs[0], cbMbs, nullptr, nullptr);
	return s_mbs;
}

/**
 * Mini U82W() function.
 * @param mbs UTF-8 string.
 * @return UTF-16 C++ wstring.
 */
wstring U82W(const char *mbs)
{
	wstring s_wcs;

	// NOTE: cchWcs includes the NULL terminator.
	int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 1) {
		return s_wcs;
	}
	cchWcs--;
 
	s_wcs.resize(cchWcs);
	MultiByteToWideChar(CP_UTF8, 0, mbs, -1, &s_wcs[0], cchWcs);
	return s_wcs;
}

}

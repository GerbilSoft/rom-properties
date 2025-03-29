/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.hpp: Mini-U82T                                                 *
 * Use this in projects where librptext can't be used for some reason.     *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "MiniU82T.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::wstring;

namespace LibWin32Common {

wstring U82W_int(const char *mbs)
{
	const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0) {
		return {};
	}

	wstring wstr;
	wstr.resize(cchWcs);
	MultiByteToWideChar(CP_UTF8, 0, mbs, -1, &wstr[0], cchWcs);
	return wstr;
}

string W2U8(const wchar_t *wcs, int len)
{
	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, len, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return {};
	}

	string str;
	str.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wcs, len, &str[0], cbMbs, nullptr, nullptr);
	if (len < 0 && str[cbMbs-1] == '\0') {
		str.resize(cbMbs-1);
	}
	return str;
}

string A2U8(const char *mbs, int len)
{
	int cchWcs = MultiByteToWideChar(CP_ACP, 0, mbs, len, nullptr, 0);
	if (cchWcs <= 0) {
		return {};
	}

	unique_ptr<wchar_t[]> wcs(new wchar_t[cchWcs]);
	MultiByteToWideChar(CP_ACP, 0, mbs, -1, wcs.get(), cchWcs);
	if (len < 0 && wcs[cchWcs-1] == L'\0') {
		cchWcs--;
	}

	const int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wcs.get(), cchWcs, nullptr, 0, nullptr, nullptr);
	if (cbUtf8 <= 0) {
		return {};
	}

	string u8str;
	u8str.resize(cbUtf8);
	WideCharToMultiByte(CP_UTF8, 0, wcs.get(), cchWcs, &u8str[0], cbUtf8, nullptr, nullptr);
	return u8str;
}

}

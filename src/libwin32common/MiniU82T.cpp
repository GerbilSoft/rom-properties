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

string W2U8(const wchar_t *wcs)
{
	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return {};
	}

	string str;
	str.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, &str[0], cbMbs, nullptr, nullptr);
	return str;
}

string W2U8(const wstring wstr)
{
	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return {};
	}

	string str;
	str.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], cbMbs, nullptr, nullptr);
	return str;
}

string A2U8(const char *mbs)
{
	const int cchWcs = MultiByteToWideChar(CP_ACP, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0) {
		return {};
	}

	unique_ptr<wchar_t[]> wcs(new wchar_t[cchWcs]);
	MultiByteToWideChar(CP_ACP, 0, mbs, -1, wcs.get(), cchWcs);

	const int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wcs.get(), -1, nullptr, 0, nullptr, nullptr);
	if (cbUtf8 <= 0) {
		return {};
	}

	string u8str;
	u8str.resize(cbUtf8);
	WideCharToMultiByte(CP_UTF8, 0, wcs.get(), -1, &u8str[0], cbUtf8, nullptr, nullptr);
	return u8str;
}

}

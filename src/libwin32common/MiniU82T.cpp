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
using std::string_view;
using std::unique_ptr;
using std::wstring;
using std::wstring_view;

namespace LibWin32Common {

wstring U82W_int(string_view mbs)
{
	wstring wcs;

	const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs.data(), static_cast<int>(mbs.size()), nullptr, 0);
	if (cchWcs <= 0) {
		return wcs;
	}

	wcs.resize(cchWcs);
	MultiByteToWideChar(CP_UTF8, 0, mbs.data(), static_cast<int>(mbs.size()), &wcs[0], cchWcs);
	return wcs;
}

string W2U8(wstring_view wcs)
{
	string mbs;

	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs.data(), static_cast<int>(wcs.size()), nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return mbs;
	}

	mbs.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wcs.data(), static_cast<int>(wcs.size()), &mbs[0], cbMbs, nullptr, nullptr);
	return mbs;
}

string A2U8(string_view mbs)
{
	string u8str;

	int cchWcs = MultiByteToWideChar(CP_ACP, 0, mbs.data(), static_cast<int>(mbs.size()), nullptr, 0);
	if (cchWcs <= 0) {
		return u8str;
	}

	unique_ptr<wchar_t[]> wcs(new wchar_t[cchWcs]);
	MultiByteToWideChar(CP_ACP, 0, mbs.data(), static_cast<int>(mbs.size()), wcs.get(), cchWcs);

	const int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wcs.get(), cchWcs, nullptr, 0, nullptr, nullptr);
	if (cbUtf8 <= 0) {
		return u8str;
	}

	u8str.resize(cbUtf8);
	WideCharToMultiByte(CP_UTF8, 0, wcs.get(), cchWcs, &u8str[0], cbUtf8, nullptr, nullptr);
	return u8str;
}

}

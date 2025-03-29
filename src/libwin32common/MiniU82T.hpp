/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * MiniU82T.hpp: Mini-U82T header.                                         *
 * Include this in .cpp files *only*! Static functions are defined.        *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "RpWin32_sdk.h"
#include "tcharx.h"

// C++ STL classes
#include <memory>
#include <string>

namespace LibWin32Common {

static std::wstring U82W_int(const char *mbs)
{
	const int cchWcs = MultiByteToWideChar(CP_UTF8, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0) {
		return {};
	}

	std::wstring wstr;
	wstr.resize(cchWcs);
	MultiByteToWideChar(CP_UTF8, 0, mbs, -1, &wstr[0], cchWcs);
	return wstr;
}
#define U82W_c(str) (U82W_int(str).c_str())
#define U82W_s(str) (U82W_int(str.c_str()).c_str())

#ifdef _UNICODE
#  define U82T_c(str) (U82W_int(str).c_str())
#  define U82T_s(str) (U82W_int(str.c_str()).c_str())
#else /* _UNICODE */
static inline const char *U82T_c(const char *str)
{
	return str;
}
#endif /* _UNICODE */

static std::string W2U8(const wchar_t *wcs)
{
	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return {};
	}

	std::string str;
	str.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wcs, -1, &str[0], cbMbs, nullptr, nullptr);
	return str;
}

static std::string W2U8(const std::wstring wstr)
{
	const int cbMbs = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		return {};
	}

	std::string str;
	str.resize(cbMbs);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], cbMbs, nullptr, nullptr);
	return str;
}

static std::string A2U8(const char *mbs)
{
	const int cchWcs = MultiByteToWideChar(CP_ACP, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0) {
		return {};
	}

	std::unique_ptr<wchar_t[]> wcs(new wchar_t[cchWcs]);
	MultiByteToWideChar(CP_ACP, 0, mbs, -1, wcs.get(), cchWcs);

	const int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wcs.get(), -1, nullptr, 0, nullptr, nullptr);
	if (cbUtf8 <= 0) {
		return {};
	}

	std::string u8str;
	u8str.resize(cbUtf8);
	WideCharToMultiByte(CP_UTF8, 0, wcs.get(), -1, &u8str[0], cbUtf8, nullptr, nullptr);
	return u8str;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpWin32.hpp: Windows-specific functions.                                *
 *                                                                         *
 * Copyright (c) 2009-2016 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_RPWIN32_HPP__
#define __ROMPROPERTIES_LIBROMDATA_RPWIN32_HPP__

#ifndef _WIN32
#error RpWin32.hpp should only be included in Windows builds.
#endif

// Define this symbol to get XP themes. See:
// http://msdn.microsoft.com/library/en-us/dnwxp/html/xptheming.asp
// for more info. Note that as of May 2006, the page says the symbols should
// be called "SIDEBYSIDE_COMMONCONTROLS" but the headers in my SDKs in VC 6 & 7
// don't reference that symbol. If ISOLATION_AWARE_ENABLED doesn't work for you,
// try changing it to SIDEBYSIDE_COMMONCONTROLS
#define ISOLATION_AWARE_ENABLED 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

/** Windows-specific wrappers for wchar_t. **/

/**
 * NOTE: These are defined outside of the LibRomData
 * namespace because macros are needed for the UTF-8
 * versions.
 *
 * NOTE 2: The UTF-8 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::wstring or rp_string if
 * storing it, since a wchar_t* or rp_char* will
 * result in a dangling pointer.
 */

#include "libromdata/TextFuncs.hpp"

#include <windows.h>

#if defined(RP_UTF8)

/**
 * Get const wchar_t* from const rp_char*.
 * @param str const rp_char*
 * @return const wchar_t*
 */
#define RP2W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRomData::rp_string_to_utf16(str, strlen(str)).c_str()))

/**
 * Get const wchar_t* from rp_string.
 * @param rps rp_string
 * @return const wchar_t*
 */
#define RP2W_s(rps) \
	(reinterpret_cast<const wchar_t*>( \
		LibRomData::rp_string_to_utf16(rps).c_str()))

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*
 * @return const rp_char*
 */
#define W2RP_c(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), \
		wcslen(wcs)).c_str())

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring
 * @return const rp_char*
 */
#define W2RP_s(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs.data()), \
		wcs.size()).c_str())

// FIXME: In-place conversion of std::u16string to std::wstring?

/**
 * Get rp_string from const wchar_t*.
 * @param wcs const wchar_t*
 * @return rp_string
 */
#define W2RP_cs(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), \
		wcslen(wcs)))

/**
 * Get rp_string from std::wstring.
 * @param wcs std::wstring
 * @return rp_string
 */
#define W2RP_ss(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs.data()), \
		wcs.size()))

#elif defined(RP_UTF16)

/**
 * Get const wchar_t* from const rp_char*.
 * @param str const rp_char*
 * @return const wchar_t*
 */
static inline const wchar_t *RP2W_c(const rp_char *str)
{
	return reinterpret_cast<const wchar_t*>(str);
}

/**
 * Get const wchar_t* from rp_string.
 * @param rps rp_string
 * @return const wchar_t*
 */
static inline const wchar_t *RP2W_s(const LibRomData::rp_string &rps)
{
	return reinterpret_cast<const wchar_t*>(rps.c_str());
}

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*
 * @return const rp_char*
 */
static inline const rp_char *W2RP_c(const wchar_t *wcs)
{
	return reinterpret_cast<const rp_char*>(wcs);
}

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring
 * @return const rp_char*
 */
static inline const rp_char *W2RP_s(const std::wstring &wcs)
{
	return reinterpret_cast<const rp_char*>(wcs.c_str());
}

// FIXME: In-place conversion of std::u16string to std::wstring?

/**
 * Get rp_string from const wchar_t*.
 * @param wcs const wchar_t*
 * @return const rp_char*
 */
static inline const LibRomData::rp_string W2RP_cs(const wchar_t *wcs)
{
	return LibRomData::rp_string(
		reinterpret_cast<const rp_char*>(wcs));
}

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring
 * @return const rp_char*
 */
static inline const LibRomData::rp_string W2RP_ss(const std::wstring &wcs)
{
	return LibRomData::rp_string(
		reinterpret_cast<const rp_char*>(wcs.data()), wcs.size());
}

#endif /* RP_UTF16 */

#endif /* __ROMPROPERTIES_LIBROMDATA_RPWIN32_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_win32.cpp: Text encoding functions. (Win32 version)           *
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

#include "librpbase/config.librpbase.h"
#include "TextFuncs.hpp"

#ifndef _WIN32
#error TextFuncs_win32.cpp is for Windows only.
#endif

// Windows
#include "libwin32common/RpWin32_sdk.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::u16string;
using std::wstring;

// Shared internal functions.
#include "TextFuncs_int.hpp"

namespace LibRpBase {

/** OS-specific text conversion functions. **/

/**
 * Convert a null-terminated multibyte string to UTF-16.
 * @param mbs		[in] Multibyte string. (null-terminated)
 * @param codepage	[in] mbs codepage.
 * @param dwFlags	[in, opt] Conversion flags.
 * @return Allocated UTF-16 string, or NULL on error. (Must be free()'d after use!)
 */
static char16_t *W32U_mbs_to_UTF16(const char *mbs, unsigned int codepage, DWORD dwFlags = 0)
{
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t is not 16-bit!");
	int cchWcs = MultiByteToWideChar(codepage, dwFlags, mbs, -1, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	wchar_t *wcs = (wchar_t*)malloc(cchWcs * sizeof(wchar_t));
	MultiByteToWideChar(codepage, dwFlags, mbs, -1, wcs, cchWcs);
	return reinterpret_cast<char16_t*>(wcs);
}

/**
 * Convert a multibyte string to UTF-16.
 * @param mbs		[in] Multibyte string.
 * @param cbMbs		[in] Length of mbs, in bytes.
 * @param codepage	[in] mbs codepage.
 * @param cchWcs_ret	[out, opt] Number of characters in the returned string.
 * @param dwFlags	[in, opt] Conversion flags.
 * @return Allocated UTF-16 string, or NULL on error. (Must be free()'d after use!)
 * NOTE: Returned string might NOT be NULL-terminated!
 */
static char16_t *W32U_mbs_to_UTF16(const char *mbs, int cbMbs,
		unsigned int codepage, int *cchWcs_ret, DWORD dwFlags = 0)
{
	int cchWcs = MultiByteToWideChar(codepage, dwFlags, mbs, cbMbs, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	wchar_t *wcs = (wchar_t*)malloc(cchWcs * sizeof(wchar_t));
	MultiByteToWideChar(codepage, dwFlags, mbs, cbMbs, wcs, cchWcs);

	if (cchWcs_ret)
		*cchWcs_ret = cchWcs;
	return reinterpret_cast<char16_t*>(wcs);
}

/**
 * Convert a null-terminated UTF-16 string to multibyte.
 * @param wcs		[in] UTF-16 string. (null-terminated)
 * @param codepage	[in] mbs codepage.
 * @return Allocated multibyte string, or NULL on error. (Must be free()'d after use!)
 */
static char *W32U_UTF16_to_mbs(const char16_t *wcs, unsigned int codepage)
{
	int cbMbs = WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(wcs), -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0)
		return nullptr;
 
	char *mbs = (char*)malloc(cbMbs);
	WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(wcs), -1, mbs, cbMbs, nullptr, nullptr);
	return mbs;
}

/**
 * Convert a UTF-16 string to multibyte.
 * @param wcs		[in] UTF-16 string.
 * @param cchWcs	[in] Length of wcs, in characters.
 * @param codepage	[in] mbs codepage.
 * @param cbMbs_ret	[out, opt] Number of bytes in the returned string.
 * @return Allocated multibyte string, or NULL on error. (Must be free()'d after use!)
 * NOTE: Returned string might NOT be NULL-terminated!
 */
static char *W32U_UTF16_to_mbs(const char16_t *wcs, int cchWcs,
		unsigned int codepage, int *cbMbs_ret)
{
	int cbMbs = WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(wcs), cchWcs, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0)
		return nullptr;

	char *mbs = (char*)malloc(cbMbs);
	WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(wcs), cchWcs, mbs, cbMbs, nullptr, nullptr);

	if (cbMbs_ret)
		*cbMbs_ret = cbMbs;
	return mbs;
}

/** Public text conversion functions. **/

/** Code Page 1252 **/

/**
 * Convert cp1252 text to UTF-8.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string cp1252_to_utf8(const char *str, int len)
{
	REMOVE_TRAILING_NULLS(string, str, len);

	string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs, 0);
	if (wcs && cchWcs > 0) {
		// Convert the UTF-16 to UTF-8.
		int cbMbs;
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, CP_UTF8, &cbMbs);
		if (mbs && cbMbs > 0) {
			// Remove the NULL terminator if present.
			if (mbs[cbMbs-1] == 0) {
				cbMbs--;
			}
			ret.assign(mbs, cbMbs);
		}
		free(mbs);
	}

	free(wcs);
	return ret;
}

/**
 * Convert cp1252 text to UTF-16.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
u16string cp1252_to_utf16(const char *str, int len)
{
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else /* !RP_WIS16 */
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif /* RP_WIS16 */

	REMOVE_TRAILING_NULLS(u16string, str, len);

	u16string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs, 0);
	if (wcs && cchWcs > 0) {
		// Remove the NULL terminator if present.
		if (wcs[cchWcs-1] == 0) {
			cchWcs--;
		}
		ret.assign(wcs, cchWcs);
	}

	free(wcs);
	return ret;
}

/** Code Page 1252 + Shift-JIS (932) **/

/**
 * Convert cp1252 or Shift-JIS text to UTF-8.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string cp1252_sjis_to_utf8(const char *str, int len)
{
	REMOVE_TRAILING_NULLS(string, str, len);

	// Attempt to convert from Shift-JIS to UTF-16.
	string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, 932, &cchWcs, MB_ERR_INVALID_CHARS);
	if (!wcs || cchWcs <= 0) {
		// Shift-JIS conversion failed.
		// Fall back to cp1252.
		free(wcs);
		wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs);
	}

	if (wcs && cchWcs > 0) {
		// Convert the UTF-16 to UTF-8.
		int cbMbs;
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, CP_UTF8, &cbMbs);
		if (mbs && cbMbs > 0) {
			// Remove the NULL terminator if present.
			if (mbs[cbMbs-1] == 0) {
				cbMbs--;
			}

			ret.assign(mbs, cbMbs);
		}
		free(mbs);
	}

	free(wcs);
	return ret;
}

/**
 * Convert cp1252 or Shift-JIS text to UTF-16.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
u16string cp1252_sjis_to_utf16(const char *str, int len)
{
	REMOVE_TRAILING_NULLS(u16string, str, len);

	// Attempt to convert str from Shift-JIS to UTF-16.
	u16string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, 932, &cchWcs, MB_ERR_INVALID_CHARS);
	if (!wcs || cchWcs <= 0) {
		// Shift-JIS conversion failed.
		// Fall back to cp1252.
		free(wcs);
		wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs);
	}

	if (wcs && cchWcs > 0) {
		// Remove the NULL terminator if present.
		if (wcs[cchWcs-1] == 0) {
			cchWcs--;
		}
		ret.assign(wcs, cchWcs);
	}

	free(wcs);
	return ret;
}

/** UTF-8 to UTF-16 and vice-versa **/

/**
 * Convert UTF-8 text to UTF-16.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
u16string utf8_to_utf16(const char *str, int len)
{
	REMOVE_TRAILING_NULLS(u16string, str, len);

	// Win32 version.
	u16string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, CP_UTF8, &cchWcs);
	if (wcs && cchWcs > 0) {
		// Remove the NULL terminator if present.
		if (wcs[cchWcs-1] == 0) {
			cchWcs--;
		}
		ret.assign(wcs, cchWcs);
	}

	free(wcs);
	return ret;
}

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
#error TextFuncs_win32.cpp only works on little-endian architectures.
#endif

/**
 * Convert UTF-16LE text to UTF-8.
 * @param str UTF-16LE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16le_to_utf8(const char16_t *str, int len)
{
	REMOVE_TRAILING_NULLS(string, str, len);

	string ret;
	int cbMbs;
	char *mbs = W32U_UTF16_to_mbs(str, len, CP_UTF8, &cbMbs);
	if (mbs && cbMbs > 0) {
		// Remove the NULL terminator if present.
		if (mbs[cbMbs-1] == 0) {
			cbMbs--;
		}
		ret.assign(mbs, cbMbs);
	}

	free(mbs);
	return ret;
}

/**
 * Convert UTF-16BE text to UTF-8.
 * @param str UTF-16BE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16be_to_utf8(const char16_t *str, int len)
{
	if (!str || !*str || len == 0) {
		// Empty string.
		return string();
	}

	// NOTE: NULL characters are NOT truncated in the
	// byteswap function. That's done in the regular
	// utf16le_to_utf8() function.

	// WideCharToMultiByte() doesn't support UTF-16BE.
	// Byteswap the text first.
	u16string bstr = utf16_bswap(str, len);
	if (bstr.empty()) {
		// Error byteswapping the string...
		return string();
	} else if (len > 0 && len != (int)(bstr.size())) {
		// Byteswapping failed.
		// NOTE: Only checking if an explicit length
		// is specified, since we don't want to
		// call u16_strlen() here.
		return string();
	}

	// Convert the byteswapped text.
	return utf16le_to_utf8(bstr.data(), (int)bstr.size());
}

}

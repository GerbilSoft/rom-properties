/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_win32.cpp: Text encoding functions. (Win32 version)           *
 *                                                                         *
 * Copyright (c) 2009-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "TextFuncs.hpp"
#include "TextFuncs_NULL.hpp"

#ifndef _WIN32
# error TextFuncs_win32.cpp is for Windows only.
#endif
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
# error TextFuncs_win32.cpp only works on little-endian architectures.
#endif

// Windows
#include "libwin32common/RpWin32_sdk.h"

// C++ STL classes.
using std::string;
using std::u16string;
using std::wstring;

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
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else /* !RP_WIS16 */
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif /* RP_WIS16 */

	int cchWcs = MultiByteToWideChar(codepage, dwFlags, mbs, -1, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	wchar_t *wcs = static_cast<wchar_t*>(malloc(cchWcs * sizeof(wchar_t)));
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

	wchar_t *wcs = static_cast<wchar_t*>(malloc(cchWcs * sizeof(wchar_t)));
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
 
	char *mbs = static_cast<char*>(malloc(cbMbs));
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

	char *mbs = static_cast<char*>(malloc(cbMbs));
	WideCharToMultiByte(codepage, 0, reinterpret_cast<const wchar_t*>(wcs), cchWcs, mbs, cbMbs, nullptr, nullptr);

	if (cbMbs_ret)
		*cbMbs_ret = cbMbs;
	return mbs;
}

/** Generic code page functions. **/

/**
 * Convert 8-bit text to UTF-8.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] ANSI text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @param flags	[in] Flags. (See TextConv_Flags_e.)
 * @return UTF-8 string.
 */
string cpN_to_utf8(unsigned int cp, const char *str, int len, unsigned int flags)
{
	len = check_NULL_terminator(str, len);
	DWORD dwFlags = 0;
	if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
		// Fallback is enabled.
		// Fail on invalid characters in the first pass.
		dwFlags = MB_ERR_INVALID_CHARS;
	}

	// Convert from `cp` to UTF-16.
	string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp, &cchWcs, dwFlags);
	if (!wcs || cchWcs == 0) {
		if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
			// Try again using cp1252.
			wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs, 0);
		}
	}

	if (wcs && cchWcs > 0) {
		// Convert from UTF-16 to UTF-8.
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
 * Convert 8-bit text to UTF-16.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] ANSI text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @param flags	[in] Flags. (See TextConv_Flags_e.)
 * @return UTF-16 string.
 */
u16string cpN_to_utf16(unsigned int cp, const char *str, int len, unsigned int flags)
{
	len = check_NULL_terminator(str, len);
	DWORD dwFlags = 0;
	if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
		// Fallback is enabled.
		// Fail on invalid characters in the first pass.
		dwFlags = MB_ERR_INVALID_CHARS;
	}

	// Convert from `cp` to UTF-16.
	u16string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp, &cchWcs, dwFlags);
	if (!wcs || cchWcs == 0) {
		if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
			// Try again using cp1252.
			wcs = W32U_mbs_to_UTF16(str, len, 1252, &cchWcs, 0);
		}
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

/**
 * Convert UTF-8 to 8-bit text.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 * Invalid characters will be ignored.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] UTF-8 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return 8-bit text.
 */
string utf8_to_cpN(unsigned int cp, const char *str, int len)
{
	len = check_NULL_terminator(str, len);

	// Convert from UTF-8 to UTF-16.
	string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, CP_UTF8, &cchWcs, 0);
	if (wcs && cchWcs > 0) {
		// Convert from UTF-16 to `cp`.
		int cbMbs;
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, cp, &cbMbs);
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
 * Convert UTF-16 to 8-bit text.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 * Invalid characters will be ignored.
 *
 * @param cp	[in] Code page number.
 * @param wcs	[in] UTF-16 text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return 8-bit text.
 */
string utf16_to_cpN(unsigned int cp, const char16_t *wcs, int len)
{
	len = check_NULL_terminator(wcs, len);

	// Convert from UTF-16 to `cp`.
	string ret;
	int cbMbs;
	char *mbs = W32U_UTF16_to_mbs(wcs, len, cp, &cbMbs);
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

/** Specialized UTF-16 conversion functions. **/

/**
 * Convert UTF-16LE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16LE text.
 * @param len	[in] Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16le_to_utf8(const char16_t *wcs, int len)
{
	// Wrapper around utf16_to_cpN().
	return utf16_to_cpN(CP_UTF8, wcs, len);
}

/**
 * Convert UTF-16BE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16BE text.
 * @param len	[in] Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16be_to_utf8(const char16_t *wcs, int len)
{
	if (!wcs || !*wcs || len == 0) {
		// Empty string.
		return string();
	}

	// NOTE: NULL characters are NOT truncated in the
	// byteswap function. That's done in the regular
	// utf16le_to_utf8() function.

	// WideCharToMultiByte() doesn't support UTF-16BE.
	// Byteswap the text first.
	u16string bwcs = utf16_bswap(wcs, len);
	if (bwcs.empty()) {
		// Error byteswapping the string...
		return string();
	} else if (len > 0 && len != static_cast<int>(bwcs.size())) {
		// Byteswapping failed.
		// NOTE: Only checking if an explicit length
		// is specified, since we don't want to
		// call u16_strlen() here.
		return string();
	}

	// Convert the byteswapped text.
	return utf16_to_cpN(CP_UTF8, bwcs.data(), static_cast<int>(bwcs.size()));
}

}

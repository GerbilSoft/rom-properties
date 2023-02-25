/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * conversion_win32.cpp: Text encoding functions (Win32 version)           *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "conversion.hpp"
#include "NULL-check.hpp"

#ifndef _WIN32
#  error conversion_win32.cpp is for Windows only.
#endif
#if SYS_BYTEORDER != SYS_LIL_ENDIAN
#  error conversion_win32.cpp only works on little-endian architectures.
#endif

// Windows
#include "libwin32common/RpWin32_sdk.h"

// C++ STL classes.
using std::string;
using std::u16string;
using std::wstring;

namespace LibRpText {

/** OS-specific text conversion functions. **/

/**
 * Convert a multibyte string to UTF-16.
 * @param s_wcs		[out] Output UTF-16 string (std::u16string)
 * @param mbs		[in] Multibyte string
 * @param cbMbs		[in] Length of mbs, in bytes
 * @param cp		[in] mbs codepage
 * @param dwFlags	[in, opt] Conversion flags
 * @return 0 on success; non-zero on error.
 * NOTE: Returned string might NOT be NULL-terminated!
 */
static int W32U_mbs_to_UTF16(
	u16string &s_wcs,
	const char *mbs, int cbMbs,
	unsigned int cp, DWORD dwFlags = 0)
{
	const int cchWcs = MultiByteToWideChar(cp, dwFlags, mbs, cbMbs, nullptr, 0);
	if (cchWcs <= 0) {
		s_wcs.clear();
		return -1;
	}

	s_wcs.resize(cchWcs);
	MultiByteToWideChar(cp, dwFlags, mbs, cbMbs, reinterpret_cast<wchar_t*>(&s_wcs[0]), cchWcs);
	return 0;
}

/**
 * Convert a UTF-16 string to multibyte.
 * @param s_mbs		[out] Output multibyte string (std::string)
 * @param wcs		[in] UTF-16 string
 * @param cchWcs	[in] Length of wcs, in characters
 * @param cp		[in] mbs codepage
 * @return Allocated multibyte string, or NULL on error. (Must be free()'d after use!)
 * NOTE: Returned string might NOT be NULL-terminated!
 */
static int W32U_UTF16_to_mbs(
	string &s_mbs,
	const char16_t *wcs, int cchWcs,
	unsigned int cp)
{
	const int cbMbs = WideCharToMultiByte(cp, 0, reinterpret_cast<const wchar_t*>(wcs), cchWcs, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0) {
		s_mbs.clear();
		return -1;
	}

	s_mbs.resize(cbMbs);
	WideCharToMultiByte(cp, 0, reinterpret_cast<const wchar_t*>(wcs), cchWcs, &s_mbs[0], cbMbs, nullptr, nullptr);
	return 0;
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
	if (cp & CP_RP_BASE) {
		// RP-custom code page.
		return cpRP_to_utf8(cp, str, len);
	}

	len = check_NULL_terminator(str, len);
	DWORD dwFlags = 0;
	if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
		// Fallback is enabled.
		// Fail on invalid characters in the first pass.
		dwFlags = MB_ERR_INVALID_CHARS;
	}

	// Convert from `cp` to UTF-16.
	u16string s_wcs;
	if (W32U_mbs_to_UTF16(s_wcs, str, len, cp, dwFlags) != 0) {
		if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
			// Try again using cp1252.
			if (W32U_mbs_to_UTF16(s_wcs, str, len, 1252, 0) != 0) {
				// Failed.
				s_wcs.clear();
			}
		}
	}

	string s_mbs;
	if (!s_wcs.empty()) {
		// Convert from UTF-16 to UTF-8.
		if (!W32U_UTF16_to_mbs(s_mbs, s_wcs.data(), static_cast<int>(s_wcs.size()), CP_UTF8)) {
			// Remove the NULL terminator if present.
			if (!s_mbs.empty() && s_mbs[s_mbs.size()-1] == 0) {
				s_mbs.resize(s_mbs.size()-1);
			}
		} else {
			// Failed.
			s_mbs.clear();
		}
	}

	return s_mbs;
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
	u16string s_wcs;
	if (W32U_mbs_to_UTF16(s_wcs, str, len, cp, dwFlags) != 0) {
		if (flags & TEXTCONV_FLAG_CP1252_FALLBACK) {
			// Try again using cp1252.
			if (W32U_mbs_to_UTF16(s_wcs, str, len, 1252, 0) != 0) {
				// Failed.
				s_wcs.clear();
			}
		}
	}

	// Remove the NULL terminator if present.
	if (!s_wcs.empty() && s_wcs[s_wcs.size()-1] == 0) {
		s_wcs.resize(s_wcs.size()-1);
	}
	return s_wcs;
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
	string s_mbs;
	u16string s_wcs;
	if (!W32U_mbs_to_UTF16(s_wcs, str, len, CP_UTF8, 0)) {
		// Convert from UTF-16 to `cp`.
		if (!W32U_UTF16_to_mbs(s_mbs, s_wcs.data(), static_cast<int>(s_wcs.size()), cp)) {
			// Remove the NULL terminator if present.
			if (!s_mbs.empty() && s_mbs[s_mbs.size()-1] == 0) {
				s_mbs.resize(s_mbs.size()-1);
			}
		} else {
			// Failed.
			s_mbs.clear();
		}
	} else {
		// Failed.
		s_mbs.clear();
	}

	return s_mbs;
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
	string s_mbs;
	if (!W32U_UTF16_to_mbs(s_mbs, wcs, len, cp)) {
		// Remove the NULL terminator if present.
		if (!s_mbs.empty() && s_mbs[s_mbs.size()-1] == 0) {
			s_mbs.resize(s_mbs.size()-1);
		}
	} else {
		// Failed.
		s_mbs.clear();
	}

	return s_mbs;;
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

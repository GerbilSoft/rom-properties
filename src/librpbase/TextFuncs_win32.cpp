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

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <string>
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

// Overloaded NULL terminator checks for ICONV_FUNCTION_*.
static FORCEINLINE int check_NULL_terminator(const char *str, int len)
{
	if (len < 0) {
		return (int)strlen(str);
	} else {
		return (int)strnlen(str, len);
	}
}

static FORCEINLINE int check_NULL_terminator(const char16_t *wcs, int len)
{
	if (len < 0) {
		return (int)u16_strlen(wcs);
	} else {
		return (int)u16_strnlen(wcs, len);
	}
}

/**
 * Convert from an 8-bit encoding to another 8-bit encoding.
 * Base version with no encoding fallbacks.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param cp_src0	Source code page.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param cp_dest	Destination code page.
 */
#define W32U_8TO8_1(src_prefix, cp_src0, dest_suffix, cp_dest) \
string src_prefix##_to_##dest_suffix(const char *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* Convert from cp_src0 to UTF-16. */ \
	string ret; \
	int cchWcs; \
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp_src0, &cchWcs, 0); \
	if (wcs && cchWcs > 0) { \
		/* Convert from UTF-16 to cp_dest. */ \
		int cbMbs; \
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, cp_dest, &cbMbs); \
		if (mbs && cbMbs > 0) { \
			/* Remove the NULL terminator if present. */ \
			if (mbs[cbMbs-1] == 0) { \
				cbMbs--; \
			} \
			ret.assign(mbs, cbMbs); \
		} \
		free(mbs); \
	} \
	\
	free(wcs); \
	return ret; \
}

/**
 * Convert from an 8-bit encoding to another 8-bit encoding.
 * One encoding fallback is provided.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param cp_src0	Source code page.
 * @param cp_src1	Fallback code page.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param cp_dest	Destination code page.
 */
#define W32U_8TO8_2(src_prefix, cp_src0, cp_src1, dest_suffix, cp_dest) \
string src_prefix##_to_##dest_suffix(const char *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* Convert from cp_src0 to UTF-16. */ \
	string ret; \
	int cchWcs; \
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp_src0, &cchWcs, MB_ERR_INVALID_CHARS); \
	if (!wcs || cchWcs <= 0) { \
		/* Conversion from cp_src0 failed. Try cp_src1. */ \
		free(wcs); \
		wcs = W32U_mbs_to_UTF16(str, len, cp_src1, &cchWcs); \
	} \
	if (wcs && cchWcs > 0) { \
		/* Convert from UTF-16 to cp_dest. */ \
		int cbMbs; \
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, cp_dest, &cbMbs); \
		if (mbs && cbMbs > 0) { \
			/* Remove the NULL terminator if present. */ \
			if (mbs[cbMbs-1] == 0) { \
				cbMbs--; \
			} \
			ret.assign(mbs, cbMbs); \
		} \
		free(mbs); \
	} \
	\
	free(wcs); \
	return ret; \
}

/**
 * Convert from an 8-bit encoding to UTF-16.
 * Base version with no encoding fallbacks.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param cp_src0	Source code page.
 */
#define W32U_8TO16_1(src_prefix, cp_src0) \
u16string src_prefix##_to_utf16(const char *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* Convert from cp_src0 to UTF-16. */ \
	u16string ret; \
	int cchWcs; \
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp_src0, &cchWcs, 0); \
	if (wcs && cchWcs > 0) { \
		/* Remove the NULL terminator if present. */ \
		if (wcs[cchWcs-1] == 0) { \
			cchWcs--; \
		} \
		ret.assign(wcs, cchWcs); \
	} \
	\
	free(wcs); \
	return ret; \
}


/**
 * Convert from an 8-bit encoding to UTF-16.
 * Base version with no encoding fallbacks.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param cp_src0	Source code page.
 * @param cp_src1	Fallback code page.
 */
#define W32U_8TO16_2(src_prefix, cp_src0, cp_src1) \
u16string src_prefix##_to_utf16(const char *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* Convert from cp_src0 to UTF-16. */ \
	u16string ret; \
	int cchWcs; \
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp_src0, &cchWcs, MB_ERR_INVALID_CHARS); \
	if (!wcs || cchWcs <= 0) { \
		/* Conversion from cp_src0 failed. Try cp_src1. */ \
		free(wcs); \
		wcs = W32U_mbs_to_UTF16(str, len, cp_src1, &cchWcs); \
	} \
	if (wcs && cchWcs > 0) { \
		/* Remove the NULL terminator if present. */ \
		if (wcs[cchWcs-1] == 0) { \
			cchWcs--; \
		} \
		ret.assign(wcs, cchWcs); \
	} \
	\
	free(wcs); \
	return ret; \
}

/**
 * Convert from an 8-bit encoding to UTF-16.
 * Base version with no encoding fallbacks.
 * Trailing NULL bytes will be removed.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param cp_dest	Destination code page.
 */
#define W32U_16TO8_1(dest_suffix, cp_dest0) \
string utf16_to_##dest_suffix(const char16_t *wcs, int len) \
{ \
	len = check_NULL_terminator(wcs, len); \
	\
	/* Convert from UTF-16 to cp_dest0. */ \
	string ret; \
	int cbMbs; \
	char *mbs = W32U_UTF16_to_mbs(wcs, len, cp_dest0, &cbMbs); \
	if (mbs && cbMbs > 0) { \
		/* Remove the NULL terminator if present. */ \
		if (mbs[cbMbs-1] == 0) { \
			cbMbs--; \
		} \
		ret.assign(mbs, cbMbs); \
	} \
	\
	free(mbs); \
	return ret; \
}

/** Code Page 1252 **/
W32U_8TO8_1(cp1252, 1252, utf8, CP_UTF8)
W32U_8TO16_1(cp1252, 1252)

W32U_8TO8_1(utf8, CP_UTF8, cp1252, 1252)
W32U_16TO8_1(cp1252, 1252)

/** Code Page 1252 + Shift-JIS (932) **/
W32U_8TO8_2(cp1252_sjis, 932, 1252, utf8, CP_UTF8)
W32U_8TO16_2(cp1252_sjis, 932, 1252)

/** Latin-1 (ISO-8859-1) */
W32U_8TO8_1(latin1, 28591, utf8, CP_UTF8)
W32U_8TO16_1(latin1, 28591)

W32U_8TO8_1(utf8, CP_UTF8, latin1, 28591)
W32U_16TO8_1(latin1, 28591)

/** UTF-8 to UTF-16 and vice-versa **/
W32U_8TO16_1(utf8, CP_UTF8)

// Reuse W32U_16TO8_1 for utf16le.
#define utf16_to_utf8 utf16le_to_utf8
W32U_16TO8_1(utf8, CP_UTF8)
#undef utf16_to_utf8

#if SYS_BYTEORDER != SYS_LIL_ENDIAN
#error TextFuncs_win32.cpp only works on little-endian architectures.
#endif

/**
 * Convert UTF-16BE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16BE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
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
	} else if (len > 0 && len != (int)(bwcs.size())) {
		// Byteswapping failed.
		// NOTE: Only checking if an explicit length
		// is specified, since we don't want to
		// call u16_strlen() here.
		return string();
	}

	// Convert the byteswapped text.
	return utf16le_to_utf8(bwcs.data(), (int)bwcs.size());
}

/** Generic code page functions. **/

W32U_8TO8_1(ansi, CP_ACP, utf8, CP_UTF8)

/**
 * Convert 8-bit text to UTF-8.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 * Invalid characters will be ignored.
 *
 * @param cp	[in] Code page number.
 * @param str	[in] ANSI text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cpN_to_utf8(unsigned int cp, const char *str, int len)
{
	len = check_NULL_terminator(str, len);

	/* Convert from `cp` to UTF-16. */
	string ret;
	int cchWcs;
	char16_t *wcs = W32U_mbs_to_UTF16(str, len, cp, &cchWcs, 0);
	if (wcs && cchWcs > 0) {
		/* Convert from UTF-16 to UTF-8. */
		int cbMbs;
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, CP_UTF8, &cbMbs);
		if (mbs && cbMbs > 0) {
			/* Remove the NULL terminator if present. */
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

}

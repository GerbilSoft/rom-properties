/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_iconv.cpp: Text encoding functions. (iconv version)           *
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

#if defined(_WIN32)
#error TextFuncs_iconv.cpp is not supported on Windows.
#elif !defined(HAVE_ICONV)
#error TextFuncs_iconv.cpp requires the iconv() function.
#endif

// Determine the system encodings.
#include "byteorder.h"
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
# define RP_ICONV_UTF16_ENCODING "UTF-16BE"
#else
# define RP_ICONV_UTF16_ENCODING "UTF-16LE"
#endif
#if defined(RP_UTF8)
# define RP_ICONV_ENCODING "UTF-8"
#elif defined(RP_UTF16)
# define RP_ICONV_ENCODING RP_ICONV_UTF16_ENCODING
#endif

// iconv
#include <iconv.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstdlib>
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// Shared internal functions.
#include "TextFuncs_int.hpp"

namespace LibRpBase {

/** OS-specific text conversion functions. **/

/**
 * Convert a string from one character set to another.
 * @param src 		[in] Source string.
 * @param len           [in] Source length, in bytes.
 * @param src_charset	[in] Source character set.
 * @param dest_charset	[in] Destination character set.
 * @return malloc()'d UTF-8 string, or nullptr on error.
 */
static char *rp_iconv(const char *src, int len,
		const char *src_charset, const char *dest_charset)
{
	if (!src || len <= 0)
		return nullptr;

	if (!src_charset)
		src_charset = "";
	if (!dest_charset)
		dest_charset = "";

	// Based on examples from:
	// * http://www.delorie.com/gnu/docs/glibc/libc_101.html
	// * http://www.codase.com/search/call?name=iconv

	// Open an iconv descriptor.
	iconv_t cd;
	cd = iconv_open(dest_charset, src_charset);
	if (cd == (iconv_t)(-1)) {
		// Error opening iconv.
		return nullptr;
	}

	// Allocate the output buffer.
	// UTF-8 is variable length, and the largest UTF-8 character is 4 bytes long.
	size_t src_bytes_len = (size_t)len;
	const size_t out_bytes_len = (src_bytes_len * 4) + 4;
	size_t out_bytes_remaining = out_bytes_len;
	char *outbuf = (char*)malloc(out_bytes_len);

	// Input and output pointers.
	char *inptr = (char*)(src);	// Input pointer.
	char *outptr = &outbuf[0];	// Output pointer.

	bool success = true;
	while (src_bytes_len > 0) {
		if (iconv(cd, &inptr, &src_bytes_len, &outptr, &out_bytes_remaining) == (size_t)(-1)) {
			// An error occurred while converting the string.
			// FIXME: Flag to indicate that we want to have
			// a partial Shift-JIS conversion?
			// Madou Monogatari I (MD) has a broken Shift-JIS
			// code point, which breaks conversion.
			// (Reported by andlabs.)
			success = false;
			break;
		}
	}

	// Close the iconv descriptor.
	iconv_close(cd);

	if (success) {
		// The string was converted successfully.

		// Make sure the string is null-terminated.
		size_t null_bytes = (out_bytes_remaining > 4 ? 4 : out_bytes_remaining);
		for (size_t i = null_bytes; i > 0; i--) {
			*outptr++ = 0x00;
		}

		// Return the output buffer.
		return outbuf;
	}

	// The string was not converted successfully.
	free(outbuf);
	return nullptr;
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
	REMOVE_TRAILING_NULLS_STRLEN(string, strlen, str, len);

	string ret;
	char *mbs = rp_iconv((char*)str, len, "CP1252//IGNORE", "UTF-8");
	if (mbs) {
		ret = mbs;
		free(mbs);
	}

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

	REMOVE_TRAILING_NULLS_STRLEN(u16string, strlen, str, len);

	u16string ret;
	char16_t *wcs = (char16_t*)rp_iconv((char*)str, len, "CP1252//IGNORE", RP_ICONV_UTF16_ENCODING);
	if (wcs) {
		ret = wcs;
		free(wcs);
	}

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
	REMOVE_TRAILING_NULLS_STRLEN(string, strlen, str, len);

	// Try Shift-JIS first.
	// NOTE: Using CP932 instead of SHIFT-JIS due to issues with Wave Dash.
	// References:
	// - https://en.wikipedia.org/wiki/Tilde#Unicode_and_Shift_JIS_encoding_of_wave_dash
	// - https://en.wikipedia.org/wiki/Wave_dash
	string ret;
	char *mbs = rp_iconv((char*)str, len, "CP932", "UTF-8");
	if (mbs) {
		ret = mbs;
		free(mbs);
	} else {
		// Try cp1252.
		mbs = rp_iconv((char*)str, len, "CP1252//IGNORE", "UTF-8");
		if (mbs) {
			ret = mbs;
			free(mbs);
		}
	}

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
	REMOVE_TRAILING_NULLS_STRLEN(u16string, strlen, str, len);

	// Try Shift-JIS first.
	// NOTE: Using CP932 instead of SHIFT-JIS due to issues with Wave Dash.
	// References:
	// - https://en.wikipedia.org/wiki/Tilde#Unicode_and_Shift_JIS_encoding_of_wave_dash
	// - https://en.wikipedia.org/wiki/Wave_dash
	u16string ret;
	char16_t *wcs = (char16_t*)rp_iconv((char*)str, len, "CP932", RP_ICONV_UTF16_ENCODING);
	if (wcs) {
		ret = wcs;
		free(wcs);
	} else {
		// Try cp1252.
		wcs = (char16_t*)rp_iconv((char*)str, len, "CP1252//IGNORE", RP_ICONV_UTF16_ENCODING);
		if (wcs) {
			ret = wcs;
			free(wcs);
		}
	}

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
	REMOVE_TRAILING_NULLS_STRLEN(u16string, strlen, str, len);

	u16string ret;
	char16_t *wcs = (char16_t*)rp_iconv((char*)str, len, "UTF-8", RP_ICONV_UTF16_ENCODING);
	if (wcs) {
		ret = wcs;
		free(wcs);
	}

	return ret;
}

/**
 * Convert UTF-16 text to UTF-8. (INTERNAL FUNCTION)
 * @param str UTF-16 text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @param encoding iconv encoding.
 * @return UTF-8 string.
 */
static inline string utf16_to_utf8_int(const char16_t *str, int len, const char *encoding)
{
	// NOTE: u16_strlen() works for both BE and LE, since 0x0000 is
	// still 0x0000 when byteswapped.
	REMOVE_TRAILING_NULLS_STRLEN(string, u16_strlen, str, len);

	string ret;
	char *mbs = (char*)rp_iconv((char*)str, len*sizeof(*str), encoding, "UTF-8");
	if (mbs) {
		ret = mbs;
		free(mbs);
	}

	return ret;
}

/**
 * Convert UTF-16LE text to UTF-8.
 * @param str UTF-16LE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16le_to_utf8(const char16_t *str, int len)
{
	return utf16_to_utf8_int(str, len, "UTF-16LE");
}

/**
 * Convert UTF-16BE text to UTF-8.
 * @param str UTF-16BE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string utf16be_to_utf8(const char16_t *str, int len)
{
	return utf16_to_utf8_int(str, len, "UTF-16BE");
}

}

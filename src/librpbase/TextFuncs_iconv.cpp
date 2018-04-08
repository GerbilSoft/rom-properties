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

#include "config.librpbase.h"
#include "common.h"
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

// iconv
#include <iconv.h>

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

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
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else /* !RP_WIS16 */
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif /* RP_WIS16 */

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

// TODO: Use templates instead of macros?
// TODO: Instead of inlining, specify encodings as parameters?
// This will reduce inlining overhead.

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
 * Convert from one encoding to another.
 * Base version with no encoding fallbacks.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param src_type	Source character type.
 * @param iconv_src0	Source encoding.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param dest_type	Destination character type.
 * @param iconv_dest	Destination encoding.
 */
#define ICONV_FUNCTION_1(src_prefix, src_type, iconv_src0, dest_suffix, dest_type, iconv_dest) \
std::basic_string<dest_type> src_prefix##_to_##dest_suffix(const src_type *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	std::basic_string<dest_type> ret; \
	dest_type *mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src0, iconv_dest)); \
	if (mbs) { \
		ret = mbs; \
		free(mbs); \
	} \
	\
	return ret; \
}

/**
 * Convert from one encoding to another.
 * One encoding fallback is provided.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param src_type	Source character type.
 * @param iconv_src0	Source encoding.
 * @param iconv_src1	Fallback encoding.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param dest_type	Destination character type.
 * @param iconv_dest	Destination encoding.
 */
#define ICONV_FUNCTION_2(src_prefix, src_type, iconv_src0, iconv_src1, dest_suffix, dest_type, iconv_dest) \
std::basic_string<dest_type> src_prefix##_to_##dest_suffix(const src_type *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* First encoding. */ \
	std::basic_string<dest_type> ret; \
	dest_type *mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src0, iconv_dest)); \
	if (mbs) { \
		ret = mbs; \
		free(mbs); \
	} else { \
		/* Second encoding. */ \
		mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src1, iconv_dest)); \
		if (mbs) { \
			ret = mbs; \
			free(mbs); \
		} \
	} \
	\
	return ret; \
}

/**
 * Convert from one encoding to another.
 * Two encoding fallbacks are provided.
 * Trailing NULL bytes will be removed.
 * @param src_prefix	Function suffix. (destination encoding)
 * @param src_type	Source character type.
 * @param iconv_src0	Source encoding.
 * @param iconv_src1	Fallback encoding #1.
 * @param iconv_src2	Fallback encoding #2.
 * @param dest_suffix	Function prefix. (source encoding)
 * @param dest_type	Destination character type.
 * @param iconv_dest	Destination encoding.
 */
#define ICONV_FUNCTION_3(src_prefix, src_type, iconv_src0, iconv_src1, iconv_src2, dest_suffix, dest_type, iconv_dest) \
std::basic_string<dest_type> src_prefix##_to_##dest_suffix(const src_type *str, int len) \
{ \
	len = check_NULL_terminator(str, len); \
	\
	/* First encoding. */ \
	std::basic_string<dest_type> ret; \
	dest_type *mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src0, iconv_dest)); \
	if (mbs) { \
		ret = mbs; \
		free(mbs); \
	} else { \
		/* Second encoding. */ \
		mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src1, iconv_dest)); \
		if (mbs) { \
			ret = mbs; \
			free(mbs); \
		} else { \
			/* Third encoding. */ \
			mbs = reinterpret_cast<dest_type*>(rp_iconv((char*)str, len*sizeof(src_type), iconv_src2, iconv_dest)); \
			if (mbs) { \
				ret = mbs; \
				free(mbs); \
			} \
		} \
	} \
	\
	return ret; \
}

/** Code Page 1252 **/
ICONV_FUNCTION_2(cp1252, char, "CP1252//IGNORE", "LATIN1//IGNORE", utf8, char, "UTF-8")
ICONV_FUNCTION_2(cp1252, char, "CP1252//IGNORE", "LATIN1//IGNORE", utf16, char16_t, RP_ICONV_UTF16_ENCODING)

ICONV_FUNCTION_1(utf8, char, "UTF-8", cp1252, char, "CP1252//IGNORE")
ICONV_FUNCTION_1(utf16, char16_t, RP_ICONV_UTF16_ENCODING, cp1252, char, "CP1252//IGNORE")

/** Code Page 1252 + Shift-JIS (932) **/
ICONV_FUNCTION_3(cp1252_sjis, char, "CP932", "CP1252//IGNORE", "LATIN1//IGNORE", utf8, char, "UTF-8")
ICONV_FUNCTION_3(cp1252_sjis, char, "CP932", "CP1252//IGNORE", "LATIN1//IGNORE", utf16, char16_t, RP_ICONV_UTF16_ENCODING)

/** Latin-1 (ISO-8859-1) **/
ICONV_FUNCTION_1(latin1, char, "LATIN1//IGNORE", utf8, char, "UTF-8")
ICONV_FUNCTION_1(latin1, char, "LATIN1//IGNORE", utf16, char16_t, RP_ICONV_UTF16_ENCODING)
ICONV_FUNCTION_1(utf8, char, "UTF-8", latin1, char, "LATIN1//IGNORE")
ICONV_FUNCTION_1(utf16, char16_t, RP_ICONV_UTF16_ENCODING, latin1, char, "LATIN1//IGNORE")

/** UTF-8 to UTF-16 and vice-versa **/
ICONV_FUNCTION_1(utf8, char, "UTF-8", utf16, char16_t, RP_ICONV_UTF16_ENCODING)
ICONV_FUNCTION_1(utf16le, char16_t, "UTF-16LE", utf8, char, "UTF-8")
ICONV_FUNCTION_1(utf16be, char16_t, "UTF-16BE", utf8, char, "UTF-8")

/** Generic code page functions. **/

/**
 * Convert ANSI text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str	[in] ANSI text.
 * @param len	[in] Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string ansi_to_utf8(const char *str, int len)
{
	// FIXME: Determine the correct 8-bit encoding on non-Windows systems.
	// Assuming Latin-1. (ISO-8859-1)
	return latin1_to_utf8(str, len);
}

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

	// Create the iconv encoding name.
	char cp_name[32];
	snprintf(cp_name, sizeof(cp_name), "CP%u//IGNORE", cp);

	std::string ret;
	char *mbs = reinterpret_cast<char*>(rp_iconv((char*)str, len, cp_name, "UTF-8"));
	if (mbs) {
		ret = mbs;
		free(mbs);
	}

	return ret;
}

}

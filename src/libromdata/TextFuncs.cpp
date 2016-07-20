/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TextFuncs.cpp: Text encoding functions.                                 *
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

#include "TextFuncs.hpp"
#include "config.libromdata.h"

// Determine which character set decoder to use.
#if defined(_WIN32)
# include <windows.h>
#elif defined(HAVE_ICONV)
# include <iconv.h>
# if defined(RP_UTF8)
#  define RP_ICONV_ENCODING "UTF-8"
# elif defined(RP_UTF16)
#  include <byteorder.h>
#  if SYS_BYTEORDER == SYS_BIG_ENDIAN
#   define RP_ICONV_ENCODING "UTF-16BE"
#  else
#   define RP_ICONV_ENCODING "UTF-16LE"
#  endif
# endif
#endif

// C++ includes.
#include <string>
using std::string;
using std::u16string;
using std::wstring;

// C includes. (C++ namespace)
#include <cstdlib>
#include <cstring>

// TODO: Use std::auto_ptr<>?

namespace LibRomData {

#if defined(_WIN32)
/**
 * Convert a null-terminated multibyte string to UTF-16.
 * @param mbs		[in] Multibyte string. (null-terminated)
 * @param codepage	[in] mbs codepage.
 * @return Allocated UTF-16 string, or NULL on error. (Must be free()'d after use!)
 */
static char16_t *W32U_mbs_to_UTF16(const char *mbs, unsigned int codepage)
{
	int cchWcs = MultiByteToWideChar(codepage, 0, mbs, -1, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	char16_t *wcs = (char16_t*)malloc(cchWcs * sizeof(char16_t));
	MultiByteToWideChar(codepage, 0, mbs, -1, (wchar_t*)wcs, cchWcs);
	return wcs;
}

/**
 * Convert a multibyte string to UTF-16.
 * @param mbs		[in] Multibyte string.
 * @param cbMbs		[in] Length of mbs, in bytes.
 * @param codepage	[in] mbs codepage.
 * @param cchWcs_ret	[out, opt] Number of characters in the returned string.
 * @return Allocated UTF-16 string, or NULL on error. (Must be free()'d after use!)
 * NOTE: Returned string might NOT be NULL-terminated!
 */
static char16_t *W32U_mbs_to_UTF16(const char *mbs, int cbMbs,
		unsigned int codepage, int *cchWcs_ret)
{
	int cchWcs = MultiByteToWideChar(codepage, 0, mbs, cbMbs, nullptr, 0);
	if (cchWcs <= 0)
		return nullptr;

	wchar_t *wcs = (char16_t*)malloc(cchWcs * sizeof(char16_t));
	MultiByteToWideChar(codepage, 0, mbs, cbMbs, (wchar_t*)wcs, cchWcs);

	if (cchWcs_ret)
		*cchWcs_ret = cchWcs;
	return wcs;
}

/**
 * Convert a null-terminated UTF-16 string to multibyte.
 * @param wcs		[in] UTF-16 string. (null-terminated)
 * @param codepage	[in] mbs codepage.
 * @return Allocated multibyte string, or NULL on error. (Must be free()'d after use!)
 */
static char *W32U_UTF16_to_mbs(const char16_t *wcs, unsigned int codepage)
{
	int cbMbs = WideCharToMultiByte(codepage, 0, (const wchar_t*)wcs, -1, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0)
		return nullptr;
 
	char *mbs = (char*)malloc(cbMbs);
	WideCharToMultiByte(codepage, 0, (const wchar_t*)wcs, -1, mbs, cbMbs, nullptr, nullptr);
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
	int cbMbs = WideCharToMultiByte(codepage, 0, (const wchar_t*)wcs, cchWcs, nullptr, 0, nullptr, nullptr);
	if (cbMbs <= 0)
		return nullptr;

	char *mbs = (char*)malloc(cbMbs);
	WideCharToMultiByte(codepage, 0, (const wchar_t*)wcs, cchWcs, mbs, cbMbs, nullptr, nullptr);

	if (cbMbs_ret)
		*cbMbs_ret = cbMbs;
	return mbs;
}

#elif defined(HAVE_ICONV)

/**
 * Convert a string from one character set to another.
 * @param src 		[in] Source string.
 * @param src_bytes_len [in] Source length, in bytes.
 * @param src_charset	[in] Source character set.
 * @param dest_charset	[in] Destination character set.
 * @return malloc()'d UTF-8 string, or nullptr on error.
 */
static char *gens_iconv(const char *src, size_t src_bytes_len,
			const char *src_charset, const char *dest_charset)
{
	if (!src || src_bytes_len == 0)
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
	const size_t out_bytes_len = (src_bytes_len * 4) + 4;
	size_t out_bytes_remaining = out_bytes_len;
	char *outbuf = (char*)malloc(out_bytes_len);

	// Input and output pointers.
	char *inptr = (char*)(src);	// Input pointer.
	char *outptr = &outbuf[0];	// Output pointer.

	int success = 1;

	while (src_bytes_len > 0) {
		if (iconv(cd, &inptr, &src_bytes_len, &outptr, &out_bytes_remaining) == (size_t)(-1)) {
			// An error occurred while converting the string.
			if (outptr == &outbuf[0]) {
				// No bytes were converted.
				success = 0;
			} else {
				// Some bytes were converted.
				// Accept the string up to this point.
				// Madou Monogatari I has a broken Shift-JIS sequence
				// at position 9, which resulted in no conversion.
				// (Reported by andlabs.)
				success = 1;
			}
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
#endif /* HAVE_ICONV */

/**
 * Convert cp1252 or Shift-JIS text to rp_string.
 * @param str cp1252 or Shift-JIS text.
 * @param n Length of str.
 * @return rp_string.
 */
rp_string cp1252_sjis_to_rp_string(const char *str, size_t n)
{
	// Attempt to convert str from Shift-JIS to UTF-16.
#if defined(_WIN32)
	// Win32 version.
	int cchWcs;
	wchar_t *wcs = W32U_mbs_to_UTF16(str, n, 932, &cchWcs);
	if (wcs) {
#if defined(RP_UTF8)
		// Convert the UTF-16 to UTF-8.
		int cbMbs;
		char *mbs = W32U_UTF16_to_mbs(wcs, cchWcs, CP_UTF8, &cbMbs);
		free(wcs);
		if (!mbs)
			return rp_string();
		rp_string ret(mbs, cbMbs);
		free(mbs);
		return ret;
#elif defined(RP_UTF16)
		// Return the UTF-16 as-is.
		return rp_string(reinterpret_cast<const char16_t*>(wcs), cchWcs);
#endif
	}
#elif defined(HAVE_ICONV)
	// iconv version.
	// Try Shift-JIS first.
	rp_char *rps = (rp_char*)gens_iconv((char*)str, n, "SHIFT-JIS", RP_ICONV_ENCODING);
	if (rps) {
		rp_string ret(rps);
		free(rps);
		return ret;
	}

	// Try cp1252.
	rps = (rp_char*)gens_iconv((char*)str, n, "CP1252", RP_ICONV_ENCODING);
	if (rps) {
		rp_string ret(rps);
		free(rps);
		return ret;
	}

	// Unable to convert the string.
	return rp_string();
#else
#error Text conversion not available on this system.
#endif
}

#ifdef RP_UTF16
/**
 * Convert UTF-8 text to rp_string.
 * @param str UTF-8 text.
 * @param n Length of str.
 * @return rp_string.
 */
rp_string utf8_to_rp_string(const char *str, size_t n)
{
#if defined(_WIN32)
	// Win32 version.
	int cchWcs;
	wchar_t *wcs = W32U_mbs_to_UTF16(str, n, 932, &cchWcs);
	if (wcs) {
		rp_string ret(reinterpret_cast<const char16_t*>(wcs), cchWcs);
		free(wcs);
		return ret;
	}
	return rp_string();
#elif defined(HAVE_ICONV)
	// iconv version.
	rp_char *rps = (rp_char*)gens_iconv((char*)str, n, "UTF-8", RP_ICONV_ENCODING);
	if (rps) {
		rp_string ret(rps);
		free(rps);
		return ret;
	}

	// Unable to convert the string.
	return rp_string();
#else
#error Text conversion not available on this system.
#endif
}
#endif /* RP_UTF16 */

}

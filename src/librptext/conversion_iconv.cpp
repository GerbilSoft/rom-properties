/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * conversion_iconv.cpp: Text encoding functions (iconv version)           *
 *                                                                         *
 * Copyright (c) 2009-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "conversion.hpp"
#include "NULL-check.hpp"

#if defined(_WIN32)
#  error conversion_iconv.cpp is not supported on Windows.
#elif !defined(HAVE_ICONV)
#  error conversion_iconv.cpp requires the iconv() function.
#endif

// Determine the system encodings.
#include "librpbyteswap/byteorder.h"
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
static const char RP_ICONV_UTF16_ENCODING[] = "UTF-16BE";
#else
static const char RP_ICONV_UTF16_ENCODING[] = "UTF-16LE";
#endif

// iconv
#if (defined(__FreeBSD__) || defined(__DragonFly__)) && !defined(HAVE_ICONV_LIBICONV)
// FreeBSD 10.0 has its own iconv() implementation, but if
// GNU libiconv is installed (/usr/local), its header gets
// used first, so explicitly use the system iconv.h on
// non-Linux systems.
#  include </usr/include/iconv.h>
#else
// iconv() is either in libiconv only, or this is Linux.
// Use iconv.h normally.
#  include <iconv.h>
#endif

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
using std::string;
using std::u16string;

namespace LibRpText {

/** OS-specific text conversion functions. **/

/**
 * Convert a string from one character set to another.
 * @param src 		[in] Source string.
 * @param len           [in] Source length, in bytes.
 * @param src_charset	[in] Source character set.
 * @param dest_charset	[in] Destination character set.
 * @param ignoreErr	[in] If true, ignore errors. ("//IGNORE" on glibc/libiconv)
 * @return malloc()'d UTF-8 string, or nullptr on error.
 */
static char *rp_iconv(const char *src, int len,
	const char *src_charset, const char *dest_charset,
	bool ignoreErr = false)
{
#ifdef RP_WIS16
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "RP_WIS16 is defined, but wchar_t is not 16-bit!");
#else /* !RP_WIS16 */
	static_assert(sizeof(wchar_t) != sizeof(char16_t), "RP_WIS16 is not defined, but wchar_t is 16-bit!");
#endif /* RP_WIS16 */
	assert(src != nullptr);
	//assert(len > 0);	// Don't assert on empty strings.
	assert(src_charset != nullptr);
	assert(dest_charset != nullptr);
	if (!src || len <= 0 || !src_charset || !dest_charset)
		return nullptr;

	// Based on examples from:
	// * http://www.delorie.com/gnu/docs/glibc/libc_101.html
	// * http://www.codase.com/search/call?name=iconv

	// Open an iconv descriptor.
	iconv_t cd;
#if defined(__linux__) || defined(HAVE_ICONV_LIBICONV)
	// glibc/libiconv: Append "//IGNORE" to the source character set
	// if ignoreErr == true.
	// TODO: Destination, not source?
	if (ignoreErr) {
		char tmpsrc[32];
		snprintf(tmpsrc, sizeof(tmpsrc), "%s//IGNORE", src_charset);
		cd = iconv_open(dest_charset, tmpsrc);
	} else {
		// Not ignoring errors.
		cd = iconv_open(dest_charset, src_charset);
	}
#else
	cd = iconv_open(dest_charset, src_charset);
#endif

	if (cd == (iconv_t)(-1)) {
		// Error opening iconv.
		return nullptr;
	}

	// Allocate the output buffer.
	// UTF-8 is variable length, and the largest UTF-8 character is 4 bytes long.
	size_t src_bytes_len = (size_t)len;
	const size_t out_bytes_len = (src_bytes_len * 4) + 4;
	size_t out_bytes_remaining = out_bytes_len;
	char *outbuf = static_cast<char*>(malloc(out_bytes_len));

	// Input and output pointers.
	char *inptr = const_cast<char*>(src);	// Input pointer.
	char *outptr = &outbuf[0];		// Output pointer.

#ifdef __FreeBSD__
	// Flags for FreeBSD's __iconv().
	const uint32_t iconv_flags = (ignoreErr ? __ICONV_F_HIDE_INVALID : 0);
#endif /* __FreeBSD */

	bool success = true;
	while (src_bytes_len > 0) {
		size_t size;

#ifdef __FreeBSD__
		// Use FreeBSD's __iconv() to ignore errors if specified.
		size = __iconv(cd, &inptr, &src_bytes_len, &outptr, &out_bytes_remaining, iconv_flags, nullptr);
#else /* !__FreeBSD__ */
		size = iconv(cd, &inptr, &src_bytes_len, &outptr, &out_bytes_remaining);
#endif

		if (size == static_cast<size_t>(-1)) {
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
		const unsigned int null_bytes = (out_bytes_remaining > 4U) ? 4U : static_cast<unsigned int>(out_bytes_remaining);
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

/** Generic code page functions **/

/**
 * Convert a code page to an encoding name.
 * @param enc_name	[out] Buffer for encoding name.
 * @param len		[in] Length of enc_name.
 * @param cp		[in] Code page number.
 */
static string codePageToEncName(unsigned int cp)
{
	// Check for "special" code pages.
	switch (cp) {
		case CP_ACP:
			// TODO: Get the system code page.
			// Assuming cp1252 for now.
			return "CP1252";
		case CP_LATIN1:
			return "LATIN1";
		case CP_UTF8:
			return "UTF-8";
		default: {
			string str = "CP";
			str += std::to_string(cp);
			return str;
		}
	}
}

/**
 * Convert 8-bit text to UTF-8 or UTF-16.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @tparam T char (UTF-8) or char16_t (UTF-16)
 * @param out_encoding [in] Output encoding
 * @param cp	[in] Code page number
 * @param str	[in] 8-bit text
 * @param len	[in] Length of str, in bytes (-1 for NULL-terminated string)
 * @param flags	[in] Flags (See TextConv_Flags_e)
 * @return UTF-8 string
 */
template<typename T>
static std::basic_string<T> T_cpN_to_unicode(const char *out_encoding, unsigned int cp, const char *str, int len, unsigned int flags)
{
	len = check_NULL_terminator(str, len);

	// Get the encoding name for the primary code page.
	const string cp_name = codePageToEncName(cp);

	// If we *want* to fall back to cp1252 on error,
	// then the first conversion should fail on errors.
	const bool ignoreErr = !(flags & TEXTCONV_FLAG_CP1252_FALLBACK);

	// Attempt to convert the text to UTF-8.
	// NOTE: "//IGNORE" sometimes doesn't work, so we won't
	// check for TEXTCONV_FLAG_CP1252_FALLBACK here.
	std::basic_string<T> ret;
	T *out_str = nullptr;

	if ((flags & TEXTCONV_FLAG_JIS_X_0208) && len >= 1) {
		// Check if the string might be JIS X 0208.
		// If it is, make it EUC-JP compatible, then convert it.
		bool is0208 = false;
		// Heuristic: First character should be 0x21-0x24.
		if (*str >= 0x21 && *str <= 0x24) {
			is0208 = true;
			const char *const p_end = str + len;
			for (const char *p = str + 1; p < p_end; p++) {
				const uint8_t chr = static_cast<uint8_t>(*p);
				if (chr == 0) {
					// End of string
					break;
				} else if (chr & 0x80) {
					// High bit cannot be set
					is0208 = false;
					break;
				}
			}
		}

		if (is0208) {
			// Make the string EUC-JP compatible.
			string eucJP(str, 0, len);
			for (char &c : eucJP) {
				c |= 0x80;
			}
			out_str = reinterpret_cast<T*>(rp_iconv((char*)eucJP.data(), eucJP.size(), "EUC-JP", out_encoding, ignoreErr));
		}
	}

	if (!out_str) {
		// Standard string conversion
		out_str = reinterpret_cast<T*>(rp_iconv((char*)str, len*sizeof(*str), cp_name.c_str(), out_encoding, ignoreErr));
	}

	if (!out_str /*&& (flags & TEXTCONV_FLAG_CP1252_FALLBACK)*/) {
		// Try cp1252 fallback.
		// NOTE: Sometimes cp1252 fails, even with ignore set.
		if (cp != 1252) {
			out_str = reinterpret_cast<T*>(rp_iconv((char*)str, len*sizeof(*str), "CP1252", out_encoding, true));
		}
		if (!out_str) {
			// Try Latin-1 fallback.
			if (cp != CP_LATIN1) {
				out_str = reinterpret_cast<T*>(rp_iconv((char*)str, len*sizeof(*str), "LATIN1", out_encoding, true));
			}
		}
	}

	if (out_str) {
		ret.assign(out_str);
		free(out_str);

#ifdef HAVE_ICONV_LIBICONV
		if (cp == CP_SJIS) {
			// Some versions of libiconv map characters differently compared to cp932:
			// - FreeBSD Shift-JIS: 8160: mapped to U+301C (WAVE DASH); cp932 uses U+FF5E (FULLWIDTH TILDE)
			// - Termux libiconv: 817C: mapped to U+2212 (MINUS SIGN); cp932 uses U+FF0D (FULLWIDTH HYPHEN-MINUS)
			const auto ret_end = ret.end();
			for (auto p = ret.begin(); p != ret_end; ++p) {
				if ((uint8_t)p[0] == 0xE3 && (uint8_t)p[1] == 0x80 && (uint8_t)p[2] == 0x9C) {
					// Found U+301C: WAVE DASH
					// Convert to U+FF5E: FULLWIDTH TILDE
					p[0] = (uint8_t)0xEF;
					p[1] = (uint8_t)0xBD;
					p[2] = (uint8_t)0x9E;
					p += 2;
				} else if ((uint8_t)p[0] == 0xE2 && (uint8_t)p[1] == 0x88 && (uint8_t)p[2] == 0x92) {
					// Found U+2212: MINUS SIGN
					// Convert to U+FF0D: FULLWIDTH HYPHEN-MINUS
					p[0] = (uint8_t)0xEF;
					p[1] = (uint8_t)0xBC;
					p[2] = (uint8_t)0x8D;
					p += 2;
				}
			}
		}
#endif /* HAVE_ICONV_LIBICONV */
	}
	return ret;
}

/**
 * Convert 8-bit text to UTF-8.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number
 * @param str	[in] 8-bit text
 * @param len	[in] Length of str, in bytes (-1 for NULL-terminated string)
 * @param flags	[in] Flags. (See TextConv_Flags_e)
 * @return UTF-8 string
 */
string cpN_to_utf8(unsigned int cp, const char *str, int len, unsigned int flags)
{
	if (cp & CP_RP_BASE) {
		// RP-custom code page.
		return cpRP_to_utf8(cp, str, len);
	}

	return T_cpN_to_unicode<char>("UTF-8", cp, str, len, flags);
}

/**
 * Convert 8-bit text to UTF-16.
 * Trailing NULL bytes will be removed.
 *
 * The specified code page number will be used.
 *
 * @param cp	[in] Code page number
 * @param str	[in] 8-bit text
 * @param len	[in] Length of str, in bytes (-1 for NULL-terminated string)
 * @param flags	[in] Flags (See TextConv_Flags_e)
 * @return UTF-16 string
 */
u16string cpN_to_utf16(unsigned int cp, const char *str, int len, unsigned int flags)
{
	return T_cpN_to_unicode<char16_t>(RP_ICONV_UTF16_ENCODING, cp, str, len, flags);
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

	// Get the encoding name for the primary code page.
	const string cp_name = codePageToEncName(cp);

	// Attempt to convert the text from UTF-8.
	string ret;
	char *mbs = reinterpret_cast<char*>(rp_iconv((char*)str, len*sizeof(*str), "UTF-8", cp_name.c_str(), true));
	if (mbs) {
		ret.assign(mbs);
		free(mbs);
	}
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

	// Get the encoding name for the primary code page.
	const string cp_name = codePageToEncName(cp);

	// Ignore errors if converting to anything other than UTF-8.
	const bool ignoreErr = (cp != CP_UTF8);

	// Attempt to convert the text from UTF-8.
	string ret;
	char *mbs = reinterpret_cast<char*>(rp_iconv((char*)wcs, len*sizeof(*wcs), RP_ICONV_UTF16_ENCODING, cp_name.c_str(), ignoreErr));
	if (mbs) {
		ret.assign(mbs);
		free(mbs);
	}
	return ret;
}

/** Specialized UTF-16 conversion functions. **/

/**
 * Convert 16-bit Unicode text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param src_encoding [in] Source encoding
 * @param wcs	[in] 16-bit Unicode text
 * @param len	[in] Length of wcs, in characters (-1 for NULL-terminated string)
 * @return UTF-8 string
 */
static string INT_utf16_to_utf8(const char *src_encoding, const char16_t *wcs, int len)
{
	len = check_NULL_terminator(wcs, len);

	// Attempt to convert the text from UTF-16LE to UTF-8.
	string ret;
	char *mbs = reinterpret_cast<char*>(rp_iconv((char*)wcs, len*sizeof(*wcs), src_encoding, "UTF-8"));
	if (mbs) {
		ret.assign(mbs);
		free(mbs);
	}
	return ret;
}

/**
 * Convert UTF-16LE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16LE text
 * @param len	[in] Length of wcs, in characters (-1 for NULL-terminated string)
 * @return UTF-8 string
 */
string utf16le_to_utf8(const char16_t *wcs, int len)
{
	return INT_utf16_to_utf8("UTF-16LE", wcs, len);
}

/**
 * Convert UTF-16BE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs	[in] UTF-16BE text
 * @param len	[in] Length of wcs, in characters (-1 for NULL-terminated string)
 * @return UTF-8 string
 */
string utf16be_to_utf8(const char16_t *wcs, int len)
{
	return INT_utf16_to_utf8("UTF-16BE", wcs, len);
}

}

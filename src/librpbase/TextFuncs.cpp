/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs.cpp: Text encoding functions.                                 *
 *                                                                         *
 * Copyright (c) 2009-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"
#include "TextFuncs.hpp"
#include "byteswap.h"

// libi18n
#include "libi18n/i18n.h"

// C includes.
#include <stdlib.h>
#ifdef HAVE_NL_LANGINFO
# include <langinfo.h>
#endif /* HAVE_NL_LANGINFO */

// C includes. (C++ namespace)
#include <cassert>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwctype>

// C++ includes.
#include <iomanip>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
using std::ostringstream;
using std::string;
using std::u16string;
using std::unique_ptr;

namespace LibRpBase {

/** OS-independent text conversion functions. **/

/**
 * Byteswap and return UTF-16 text.
 * @param str UTF-16 text to byteswap.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return Byteswapped UTF-16 string.
 */
u16string utf16_bswap(const char16_t *str, int len)
{
	if (len == 0) {
		return u16string();
	} else if (len < 0) {
		// NULL-terminated string.
		len = (int)u16_strlen(str);
		if (len <= 0) {
			return u16string();
		}
	}

	// TODO: Optimize this?
	u16string ret;
	ret.reserve(len);
	for (; len > 0; len--, str++) {
		ret += __swab16(*str);
	}

	return ret;
}


/** Miscellaneous functions. **/

#ifndef RP_WIS16
/**
 * char16_t strlen().
 * @param wcs 16-bit string.
 * @return Length of str, in characters.
 */
size_t u16_strlen(const char16_t *wcs)
{
	size_t len = 0;
	while (*wcs++)
		len++;
	return len;
}

/**
 * char16_t strlen().
 * @param wcs 16-bit string.
 * @param maxlen Maximum length.
 * @return Length of str, in characters.
 */
size_t u16_strnlen(const char16_t *wcs, size_t maxlen)
{
	size_t len = 0;
	while (*wcs++ && len < maxlen)
		len++;
	return len;
}

/**
 * char16_t strdup().
 * @param wcs 16-bit string.
 * @return Copy of wcs.
 */
char16_t *u16_strdup(const char16_t *wcs)
{
	size_t len = u16_strlen(wcs)+1;	// includes terminator
	char16_t *ret = (char16_t*)malloc(len*sizeof(*wcs));
	memcpy(ret, wcs, len*sizeof(*wcs));
	return ret;
}

/**
 * char16_t strcmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcmp() result.
 */
int u16_strcmp(const char16_t *wcs1, const char16_t *wcs2)
{
	// References:
	// - http://stackoverflow.com/questions/20004458/optimized-strcmp-implementation
	// - http://clc-wiki.net/wiki/C_standard_library%3astring.h%3astrcmp
	while (*wcs1 && (*wcs1 == *wcs2)) {
		wcs1++;
		wcs2++;
	}

	return ((int)*wcs1 - (int)*wcs2);
}

/**
 * char16_t strcasecmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcasecmp() result.
 */
int u16_strcasecmp(const char16_t *wcs1, const char16_t *wcs2)
{
	// References:
	// - http://stackoverflow.com/questions/20004458/optimized-strcmp-implementation
	// - http://clc-wiki.net/wiki/C_standard_library%3astring.h%3astrcmp
	while (*wcs1 && (towupper(*wcs1) == towupper(*wcs2))) {
		wcs1++;
		wcs2++;
	}

	return ((int)towupper(*wcs1) - (int)towupper(*wcs2));
}
#endif /* !RP_WIS16 */

/**
 * sprintf()-style function for std::string.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string
 */
string rp_sprintf(const char *fmt, ...)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];
	va_list ap;
#if defined(_MSC_VER) && _MSC_VER < 1900
	// MSVC 2013 and older isn't C99 compliant.
	// Use the non-standard _vscprintf() to count characters.
	va_start(ap, fmt);
	int len = _vscprintf(fmt, ap);
	va_end(ap);
	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		va_start(ap, fmt);
		vsnprintf(locbuf, sizeof(locbuf), fmt, ap);
		va_end(ap);
		return string(locbuf, len);
	}
#else
	// C99-compliant vsnprintf().
	va_start(ap, fmt);
	int len = vsnprintf(locbuf, sizeof(locbuf), fmt, ap);
	va_end(ap);
	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		return string(locbuf, len);
	}
#endif

	// Temporarily allocate a buffer large enough for the string,
	// then call vsnprintf() again.
	unique_ptr<char[]> buf(new char[len+1]);
	va_start(ap, fmt);
	int len2 = vsnprintf(buf.get(), len+1, fmt, ap);
	va_end(ap);
	assert(len == len2);
	return (len == len2 ? string(buf.get(), len) : string());
}

#ifdef _MSC_VER
/**
 * sprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string
 */
std::string rp_sprintf_p(const char *fmt, ...) ATTR_PRINTF(1, 2)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];
	va_list ap;

	// _vsprintf_p() isn't C99 compliant.
	// Use the non-standard _vscprintf_p() to count characters.
	va_start(ap, fmt);
	int len = _vscprintf_p(fmt, ap);
	va_end(ap);
	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		va_start(ap, fmt);
		_vsprintf_p(locbuf, sizeof(locbuf), fmt, ap);
		va_end(ap);
		return string(locbuf, len);
	}

	// Temporarily allocate a buffer large enough for the string,
	// then call vsnprintf() again.
	unique_ptr<char[]> buf(new char[len+1]);
	va_start(ap, fmt);
	int len2 = _vsprintf_p(buf.get(), len+1, fmt, ap);
	va_end(ap);
	assert(len == len2);
	return (len == len2 ? string(buf.get(), len) : string());
}
#endif /* _MSC_VER */

/** Other useful text functions **/

static inline int calc_frac_part(int64_t size, int64_t mask)
{
	float f = (float)(size & (mask - 1)) / (float)mask;
	int frac_part = (int)(f * 1000.0f);

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	int round_adj = (frac_part % 10 > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Format a file size.
 * @param size File size.
 * @return Formatted file size.
 */
string formatFileSize(int64_t size)
{
	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	// TODO: Localize this?
	if (size < 0) {
		// Invalid size. Print the value as-is.
		suffix = nullptr;
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 10)) {
		// tr: Bytes (< 1,024)
		suffix = NC_("TextFuncs|FileSize", "byte", "bytes", (int)size);
		whole_part = (int)size;
		frac_part = 0;
	} else if (size < (2LL << 20)) {
		// tr: Kilobytes
		suffix = C_("TextFuncs|FileSize", "KiB");
		whole_part = (int)(size >> 10);
		frac_part = calc_frac_part(size, (1LL << 10));
	} else if (size < (2LL << 30)) {
		// tr: Megabytes
		suffix = C_("TextFuncs|FileSize", "MiB");
		whole_part = (int)(size >> 20);
		frac_part = calc_frac_part(size, (1LL << 20));
	} else if (size < (2LL << 40)) {
		// tr: Gigabytes
		suffix = C_("TextFuncs|FileSize", "GiB");
		whole_part = (int)(size >> 30);
		frac_part = calc_frac_part(size, (1LL << 30));
	} else if (size < (2LL << 50)) {
		// tr: Terabytes
		suffix = C_("TextFuncs|FileSize", "TiB");
		whole_part = (int)(size >> 40);
		frac_part = calc_frac_part(size, (1LL << 40));
	} else if (size < (2LL << 60)) {
		// tr: Petabytes
		suffix = C_("TextFuncs|FileSize", "PiB");
		whole_part = (int)(size >> 50);
		frac_part = calc_frac_part(size, (1LL << 50));
	} else /*if (size < (2ULL << 70))*/ {
		// tr: Exabytes
		suffix = C_("TextFuncs|FileSize", "EiB");
		whole_part = (int)(size >> 60);
		frac_part = calc_frac_part(size, (1LL << 60));
	}

	// Localize the whole part.
	ostringstream s_value;
	s_value << whole_part;

	if (size >= (2LL << 10)) {
		// Fractional part.
		int frac_digits = 2;
		if (whole_part >= 10) {
			int round_adj = (frac_part % 10 > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}

		// Get the localized decimal point.
#if defined(_WIN32)
		// Use localeconv(). (Windows: Convert from UTF-16 to UTF-8.)
# if defined(HAVE_STRUCT_LCONV_WCHAR_T)
		// MSVCRT: `struct lconv` has wchar_t fields.
		s_value << W2U8(localeconv()->_W_decimal_point);
# else /* !HAVE_STRUCT_LCONV_WCHAR_T */
		// MinGW: `struct lconv` does not have wchar_t fields.
		// NOTE: The `char` fields are ANSI.
		s_value << ansi_to_utf8(localeconv()->decimal_point);
# endif /* HAVE_STRUCT_LCONV_WCHAR_T */
#elif defined(HAVE_NL_LANGINFO)
		// Use nl_langinfo().
		// Reference: https://www.gnu.org/software/libc/manual/html_node/The-Elegant-and-Fast-Way.html
		s_value << nl_langinfo(DECIMAL_POINT);
#else
		// Use localeconv(). (Assuming UTF-8)
		s_value << localeconv()->decimal_point;
#endif

		// Append the fractional part using the required number of digits.
		s_value << std::setw(frac_digits) << std::setfill('0') << frac_part;
	}

	if (suffix) {
		// tr: %1$s == localized value, %2$s == suffix (e.g. MiB)
		return rp_sprintf_p(C_("TextFuncs|FileSize", "%1$s %2$s"),
			s_value.str().c_str(), suffix);
	} else {
		return s_value.str();
	}

	// Should not get here...
	assert(!"Invalid code path.");
	return "QUACK";
}

}

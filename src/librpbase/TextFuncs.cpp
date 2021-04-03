/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs.cpp: Text encoding functions.                                 *
 *                                                                         *
 * Copyright (c) 2009-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"
#include "TextFuncs.hpp"

// libi18n
#include "libi18n/i18n.h"

// C includes.
#ifdef HAVE_NL_LANGINFO
# include <langinfo.h>
#else /* !HAVE_NL_LANGINFO */
# include <clocale>
#endif /* HAVE_NL_LANGINFO */

// C includes. (C++ namespace)
#include <cstdarg>
#include <cwctype>

// for strnlen() if it's not available in <string.h>
#include "TextFuncs_libc.h"

// C++ STL classes.
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
		len = static_cast<int>(u16_strlen(str));
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
	char16_t *const ret = static_cast<char16_t*>(malloc(len * sizeof(*wcs)));
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
 * char16_t strncmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strncmp() result.
 */
int u16_strncmp(const char16_t *wcs1, const char16_t *wcs2, size_t n)
{
	// References:
	// - http://stackoverflow.com/questions/20004458/optimized-strcmp-implementation
	// - http://clc-wiki.net/wiki/C_standard_library%3astring.h%3astrcmp
	while (n > 0 && *wcs1 && (*wcs1 == *wcs2)) {
		wcs1++;
		wcs2++;
		n--;
	}

	if (n == 0) {
		return 0;
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

/**
 * char16_t memchr().
 * @param wcs 16-bit string.
 * @param c Character to search for.
 * @param n Size of wcs.
 * @return Pointer to c within wcs, or nullptr if not found.
 */
const char16_t *u16_memchr(const char16_t *wcs, char16_t c, size_t n)
{
	for (; n > 0; wcs++, n--) {
		if (*wcs == c)
			return wcs;
	}
	return nullptr;
}
#endif /* !RP_WIS16 */

/**
 * vsprintf()-style function for std::string.
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
string rp_vsprintf(const char *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];
	va_list ap_tmp;

#if defined(_MSC_VER) && _MSC_VER < 1900
	// MSVC 2013 and older isn't C99 compliant.
	// Use the non-standard _vscprintf() to count characters.
	va_copy(ap_tmp, ap);
	int len = _vscprintf(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		vsnprintf(locbuf, sizeof(locbuf), fmt, ap);
		return string(locbuf, len);
	}
#else
	// C99-compliant vsnprintf().
	va_copy(ap_tmp, ap);
	int len = vsnprintf(locbuf, sizeof(locbuf), fmt, ap_tmp);
	va_end(ap_tmp);
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
	int len2 = vsnprintf(buf.get(), len+1, fmt, ap);

	string s_ret;
	assert(len == len2);
	if (len == len2) {
		s_ret.assign(buf.get(), len);
	}
	return s_ret;
}

#if defined(_MSC_VER) || defined(__MINGW32__)
/**
 * vsprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * printf() functions. Instead, it has printf_p().
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
std::string rp_vsprintf_p(const char *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];

	// _vsprintf_p() isn't C99 compliant.
	// Use the non-standard _vscprintf_p() to count characters.
	va_list ap_tmp;
	va_copy(ap_tmp, ap);
	int len = _vscprintf_p(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		_vsprintf_p(locbuf, sizeof(locbuf), fmt, ap);
		return string(locbuf, len);
	}

	// Temporarily allocate a buffer large enough for the string,
	// then call vsnprintf() again.
	unique_ptr<char[]> buf(new char[len+1]);
	int len2 = _vsprintf_p(buf.get(), len+1, fmt, ap);
	assert(len == len2);
	return (len == len2 ? string(buf.get(), len) : string());
}
#endif /* _MSC_VER || __MINGW32__ */

/** Other useful text functions **/

static inline int calc_frac_part(off64_t size, off64_t mask)
{
	float f = static_cast<float>(size & (mask - 1)) / static_cast<float>(mask);
	int frac_part = static_cast<int>(f * 1000.0f);

	// MSVC added round() and roundf() in MSVC 2013.
	// Use our own rounding code instead.
	int round_adj = ((frac_part % 10) > 5);
	frac_part /= 10;
	frac_part += round_adj;
	return frac_part;
}

/**
 * Format a file size.
 * @param size File size.
 * @return Formatted file size.
 */
string formatFileSize(off64_t size)
{
	const char *suffix;
	// frac_part is always 0 to 100.
	// If whole_part >= 10, frac_part is divided by 10.
	int whole_part, frac_part;

	// TODO: Optimize this?
	if (size < 0) {
		// Invalid size. Print the value as-is.
		suffix = nullptr;
		whole_part = static_cast<int>(size);
		frac_part = 0;
	} else if (size < (2LL << 10)) {
		// tr: Bytes (< 1,024)
		suffix = NC_("TextFuncs|FileSize", "byte", "bytes", static_cast<int>(size));
		whole_part = static_cast<int>(size);
		frac_part = 0;
	} else if (size < (2LL << 20)) {
		// tr: Kilobytes
		suffix = C_("TextFuncs|FileSize", "KiB");
		whole_part = static_cast<int>(size >> 10);
		frac_part = calc_frac_part(size, (1LL << 10));
	} else if (size < (2LL << 30)) {
		// tr: Megabytes
		suffix = C_("TextFuncs|FileSize", "MiB");
		whole_part = static_cast<int>(size >> 20);
		frac_part = calc_frac_part(size, (1LL << 20));
	} else if (size < (2LL << 40)) {
		// tr: Gigabytes
		suffix = C_("TextFuncs|FileSize", "GiB");
		whole_part = static_cast<int>(size >> 30);
		frac_part = calc_frac_part(size, (1LL << 30));
	} else if (size < (2LL << 50)) {
		// tr: Terabytes
		suffix = C_("TextFuncs|FileSize", "TiB");
		whole_part = static_cast<int>(size >> 40);
		frac_part = calc_frac_part(size, (1LL << 40));
	} else if (size < (2LL << 60)) {
		// tr: Petabytes
		suffix = C_("TextFuncs|FileSize", "PiB");
		whole_part = static_cast<int>(size >> 50);
		frac_part = calc_frac_part(size, (1LL << 50));
	} else /*if (size < (2LL << 70))*/ {
		// tr: Exabytes
		suffix = C_("TextFuncs|FileSize", "EiB");
		whole_part = static_cast<int>(size >> 60);
		frac_part = calc_frac_part(size, (1LL << 60));
	}

	// Localize the whole part.
	ostringstream s_value;
	s_value << whole_part;

	if (size >= (2LL << 10)) {
		// Fractional part.
		int frac_digits = 2;
		if (whole_part >= 10) {
			int round_adj = ((frac_part % 10) > 5);
			frac_part /= 10;
			frac_part += round_adj;
			frac_digits = 1;
		}

		// Get the localized decimal point.
#if defined(_WIN32)
		// Use localeconv(). (Windows: Convert from UTF-16 to UTF-8.)
#  if defined(HAVE_STRUCT_LCONV_WCHAR_T)
		// MSVCRT: `struct lconv` has wchar_t fields.
		s_value << utf16_to_utf8(
			reinterpret_cast<const char16_t*>(localeconv()->_W_decimal_point), -1);
#  else /* !HAVE_STRUCT_LCONV_WCHAR_T */
		// MinGW v5,v6: `struct lconv` does not have wchar_t fields.
		// NOTE: The `char` fields are ANSI.
		s_value << ansi_to_utf8(localeconv()->decimal_point, -1);
#  endif /* HAVE_STRUCT_LCONV_WCHAR_T */
#elif defined(HAVE_NL_LANGINFO)
		// Use nl_langinfo().
		// Reference: https://www.gnu.org/software/libc/manual/html_node/The-Elegant-and-Fast-Way.html
		// NOTE: RADIXCHAR is the portable version of DECIMAL_POINT.
		s_value << nl_langinfo(RADIXCHAR);
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

/**
 * Format a file size, in KiB.
 *
 * This function expects the size to be a multiple of 1024,
 * so it doesn't do any fractional rounding or printing.
 *
 * @param size File size.
 * @return Formatted file size.
 */
std::string formatFileSizeKiB(unsigned int size)
{
	return rp_sprintf("%u %s", (size / 1024), C_("TextFuncs|FileSize", "KiB"));
}

/**
 * Remove trailing spaces from a string.
 * NOTE: This modifies the string *in place*.
 * @param str String.
 */
void trimEnd(string &str)
{
	// NOTE: No str.empty() check because that's usually never the case here.
	// TODO: Check for U+3000? (UTF-8: "\xE3\x80\x80")
	size_t sz = str.size();
	const char *p_start = str.c_str();
	for (const char *p = p_start + sz - 1; p >= p_start; p--) {
		if (*p != ' ')
			break;
		sz--;
	}
	str.resize(sz);
}

/**
 * Convert DOS (CRLF) line endings to UNIX (LF) line endings.
 * @param str_dos	[in] String with DOS line endings.
 * @param len		[in] Length of str, in characters. (-1 for NULL-terminated string)
 * @param lf_count	[out,opt] Number of CRLF pairs found.
 * @return String with UNIX line endings.
 */
std::string dos2unix(const char *str_dos, int len, int *lf_count)
{
	// TODO: Optimize this!
	string str_unix;
	if (len < 0) {
		len = static_cast<int>(strlen(str_dos));
	}
	str_unix.reserve(len);

	int lf = 0;
	for (; len > 1; str_dos++, len--) {
		if (str_dos[0] == '\r' && str_dos[1] == '\n') {
			str_unix += '\n';
			str_dos++;
			lf++;
			len--;
		} else {
			str_unix += *str_dos;
		}
	}
	// Last character cannot be '\r\n'.
	// If it's '\r', assume it's a newline.
	if (*str_dos == '\r') {
		str_unix += '\n';
		lf++;
	} else {
		str_unix += *str_dos;
	}

	if (lf_count) {
		*lf_count = lf;
	}
	return str_unix;
}

/** Audio functions. **/

/**
 * Format a sample value as m:ss.cs.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return m:ss.cs
 */
string formatSampleAsTime(unsigned int sample, unsigned int rate)
{
	char buf[32];
	unsigned int min, sec, cs;

	assert(rate != 0);
	if (rate == 0) {
		// Division by zero! Someone goofed.
		return "#DIV/0!";
	}

	assert(rate < 21474836);	// debug assert on overflow
	const unsigned int cs_frames = (sample % rate);
	if (cs_frames != 0) {
		// Calculate centiseconds.
		cs = (cs_frames * 100) / rate;
	} else {
		// No centiseconds.
		cs = 0;
	}

	sec = sample / rate;
	min = sec / 60;
	sec %= 60;

	int len = snprintf(buf, sizeof(buf), "%u:%02u.%02u", min, sec, cs);
	if (len >= (int)sizeof(buf))
		len = (int)sizeof(buf)-1;
	return string(buf, len);
}

/**
 * Convert a sample value to milliseconds.
 * @param sample Sample value.
 * @param rate Sample rate.
 * @return Milliseconds.
 */
unsigned int convSampleToMs(unsigned int sample, unsigned int rate)
{
	unsigned int sec, ms;

	assert(rate < 2147483);		// debug assert on overflow
	const unsigned int ms_frames = (sample % rate);
	if (ms_frames != 0) {
		// Calculate milliseconds.
		ms = (ms_frames * 1000) / rate;
	} else {
		// No milliseconds.
		ms = 0;
	}

	// Calculate seconds.
	sec = sample / rate;

	// Convert to milliseconds and add the milliseconds value.
	return (sec * 1000) + ms;
}

}

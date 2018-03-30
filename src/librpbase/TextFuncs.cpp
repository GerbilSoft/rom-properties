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

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwctype>

// C++ includes.
#include <memory>
#include <string>
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

}

/** Reimplementations of libc functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param maxlen Maximum length of the string
 * @returns equivivalent to min(strlen(str), maxlen) without buffer overruns
 */
size_t strnlen(const char *str, size_t maxlen)
{
	const char *ptr = memchr(str, 0, maxlen);
	if (!ptr)
		return maxlen;
	return ptr - str;
}
#endif /* HAVE_STRNLEN */

#ifndef HAVE_MEMMEM
/**
 * Find a string within a block of memory.
 * @param haystack Block of memory.
 * @param haystacklen Length of haystack.
 * @param needle String to search for.
 * @param needlelen Length of needle.
 * @return Location of needle in haystack, or nullptr if not found.
 */
void *memmem(const void *haystack, size_t haystacklen,
	     const void *needle, size_t needlelen)
{
	// Reference: https://opensource.apple.com/source/Libc/Libc-1044.1.2/string/FreeBSD/memmem.c
	// NOTE: haystack was originally 'l'; needle was originally 's'.
	register const char *cur, *last;
	const char *cl = (const char *)haystack;
	const char *cs = (const char *)needle;

	/* we need something to compare */
	if (haystacklen == 0 || needlelen == 0)
		return nullptr;

	/* "s" must be smaller or equal to "l" */
	if (haystacklen < needlelen)
		return nullptr;

	/* special case where s_len == 1 */
	if (needlelen == 1)
		return (void*)memchr(haystack, (int)*cs, needlelen);

	/* the last position where its possible to find "s" in "l" */
	last = (char *)cl + haystacklen - needlelen;

	for (cur = (const char *)cl; cur <= last; cur++) {
		if (cur[0] == cs[0] && memcmp(cur, cs, needlelen) == 0)
			return (void*)cur;
	}

	return nullptr;	
}
#endif /* HAVE_MEMMEM */

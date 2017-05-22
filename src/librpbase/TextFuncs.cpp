/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
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

#include "librpbase/config.librpbase.h"
#include "TextFuncs.hpp"
#include "byteswap.h"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>
#include <cwctype>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// Shared internal functions.
#include "TextFuncs_int.hpp"

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

/** Latin-1 (ISO-8859-1) **/

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8.
 * NOTE: 0x80-0x9F (cp1252) is converted to U+FFFD.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string latin1_to_utf8(const char *str, int len)
{
	REMOVE_TRAILING_NULLS_STRLEN(string, strlen, str, len);

	string mbs;
	mbs.reserve(len*2);
	for (; *str != 0 && len > 0; len--, str++) {
		// TODO: Optimize the branches?
		if ((*str & 0x80) == 0) {
			// ASCII.
			mbs.push_back(*str);
		} else if ((*str & 0xE0) == 0x80) {
			// Characters 0x80-0x9F. Replace with U+FFFD.
			mbs.append("\xEF\xBF\xBD");
		} else {
			// Other character. 2 bytes are needed.
			mbs.push_back(0xC0 | ((*str >> 6) & 0x03));
			mbs.push_back(0x80 | (*str & 0x3F));
		}
	}
	return mbs;
}

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-16.
 * NOTE: 0x80-0x9F (cp1252) is converted to U+FFFD.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
u16string latin1_to_utf16(const char *str, int len)
{
	REMOVE_TRAILING_NULLS_STRLEN(u16string, strlen, str, len);

	u16string wcs;
	wcs.reserve(len);
	for (; *str != 0 && len > 0; len--, str++) {
		if ((*str & 0xE0) == 0x80) {
			// Characters 0x80-0x9F. Replace with U+FFFD.
			wcs.push_back((char16_t)0xFFFD);
		} else {
			// Other character.
			wcs.push_back((char16_t)(uint8_t)*str);
		}
	}
	return wcs;
}

/** Miscellaneous functions. **/

#if !defined(RP_WIS16)
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
#endif /* RP_UTF16 && !RP_WIS16 */

}

/** Reimplementations of libc functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param len Maximum length of the string
 * @returns equivivalent to min(strlen(str), len) without buffer overruns
 */
size_t strnlen(const char *str, size_t len)
{
	size_t rv = 0;
	for (rv = 0; rv < len; rv++) {
		if (!*(str++))
			break;
	}
	return rv;
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

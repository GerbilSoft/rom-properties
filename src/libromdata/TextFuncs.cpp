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

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cstring>

// C++ includes.
#include <string>
using std::string;
using std::u16string;

// Shared internal functions.
#include "TextFuncs_int.hpp"

namespace LibRomData {

/** OS-independent text conversion functions. **/

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8.
 * NOTE: 0x80-0x9F (cp1252) is converted to '?'.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
string latin1_to_utf8(const char* str, int len)
{
	REMOVE_TRAILING_NULLS(string, str, len);
	if (len < 0) {
		// NULL-terminated string.
		len = (int)strlen(str);
	}

	string mbs;
	mbs.reserve(len*2);
	for (; len > 0; len--, str++) {
		// TODO: Optimize the branches?
		if (!*str) {
			// NULL. End of string.
			break;
		} else if ((*str & 0x80) == 0) {
			// ASCII.
			mbs.push_back(*str);
		} else if ((*str & 0xE0) == 0x80) {
			// Characters 0x80-0x9F. Replace with '?'.
			mbs.push_back('?');
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
 * NOTE: 0x80-0x9F (cp1252) is converted to '?'.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
u16string latin1_to_utf16(const char *str, int len)
{
	REMOVE_TRAILING_NULLS(u16string, str, len);
	if (len < 0) {
		// NULL-terminated string.
		len = (int)strlen(str);
	}

	u16string wcs;
	wcs.reserve(len);
	for (; len > 0; len--, str++) {
		if ((*str & 0xE0) == 0x80) {
			// Characters 0x80-0x9F. Replace with '?'.
			wcs.push_back(_RP_U16_CHR('?'));
		} else {
			// Other character.
			wcs.push_back(*str);
		}
	}
	return wcs;
}

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

	return (int)(*(const uint16_t*)wcs1 - *(const uint16_t*)wcs2);
}
#endif /* RP_UTF16 && !RP_WIS16 */

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param len Maximum length of the string
 * @returns equivivalent to min(strlen(str), len) without buffer overruns
 */
static inline size_t strnlen(const char *str, size_t len)
{
	size_t rv = 0;
	for (rv = 0; rv < len; rv++) {
		if (!*(str++))
			break;
	}
	return rv;
}
#endif /* HAVE_STRNLEN */

}

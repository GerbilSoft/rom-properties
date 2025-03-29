/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * conversion.cpp: Text encoding functions                                 *
 *                                                                         *
 * Copyright (c) 2009-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.librptext.h"
#include "conversion.hpp"

// Other rom-properties libraries
#include "librpbyteswap/byteswap_rp.h"

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes and STL classes
#include <string>
using std::string;
using std::u16string;

namespace LibRpText {

/** UTF-16 string functions **/

/**
 * Byteswap and return UTF-16 text.
 * @param str UTF-16 text to byteswap.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return Byteswapped UTF-16 string.
 */
u16string utf16_bswap(const char16_t *str, int len)
{
	if (len == 0) {
		return {};
	} else if (len < 0) {
		// NULL-terminated string.
		len = static_cast<int>(u16_strlen(str));
		if (len <= 0) {
			return {};
		}
	}

	// TODO: Optimize this?
	u16string ret;
	ret.reserve(len);
	for (; len > 0; len--, str++) {
		ret += static_cast<char16_t>(__swab16(static_cast<uint16_t>(*str)));
	}

	return ret;
}

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

/** Other useful text functions **/

/**
 * Remove trailing spaces from a string.
 * NOTE: This modifies the string *in place*.
 * @param str String
 */
void trimEnd(string &str)
{
	// NOTE: No str.empty() check because that's usually never the case here.
	// TODO: Check for U+3000? (UTF-8: "\xE3\x80\x80")
	size_t sz = str.size();
	const auto iter_rend = str.rend();
	for (auto iter = str.rbegin(); iter != iter_rend; ++iter) {
		if (*iter != ' ')
			break;
		sz--;
	}
	str.resize(sz);
}

/**
 * Remove trailing spaces from a string.
 * NOTE: This modifies the string *in place*.
 * @param str String
 */
void trimEnd(char *str)
{
	// TODO: Check for U+3000? (UTF-8: "\xE3\x80\x80")
	if (unlikely(!str || str[0] == '\0'))
		return;
	size_t sz = strlen(str);
	for (char *p = &str[sz-1]; sz > 0; p--) {
		if (*p != ' ')
			break;
		sz--;
	}

	// NULL out the trailing spaces.
	// NOTE: If no trailing spaces were found, then
	// this will simply overwrite the existing NULL terminator.
	str[sz] = '\0';
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
		switch (str_dos[0]) {
			case '\r':
				// Handle all '\r' characters as newlines,
				// even if a '\n' isn't found after it.
				str_unix += '\n';
				lf++;
				if (str_dos[1] == '\n') {
					// Skip the '\n' after the '\r'.
					str_dos++;
					len--;
				}
				break;
			case '\n':
				// Standalone '\n'. Count it.
				str_unix += '\n';
				lf++;
				break;
			default:
				// Other character. Add it.
				str_unix += *str_dos;
				break;
		}
	}

	// Check the last character.
	switch (*str_dos) {
		case '\0':
			// End of string
			break;
		case '\r':
			// '\r'; assume it's a newline.
			str_unix += '\n';
			lf++;
			break;
		default:
			// Some other character.
			str_unix += *str_dos;
			break;
	}

	if (lf_count) {
		*lf_count = lf;
	}
	return str_unix;
}

}

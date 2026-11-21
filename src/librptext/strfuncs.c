/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * strfuncs.c: String functions.                                           *
 *                                                                         *
 * SPDX-License-Identifier: LGPL-2.1-or-later                              *
 ***************************************************************************/

#include "strfuncs.h"

// C includes
#include <string.h>

/** Reimplementations of libc string functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param maxlen Maximum length of the string
 * @returns equivivalent to min(strlen(str), maxlen) without buffer overruns
 */
size_t rp_strnlen(const char *str, size_t maxlen)
{
	const char *ptr = memchr(str, 0, maxlen);
	if (!ptr) {
		return maxlen;
	}
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
 * @return Location of needle in haystack, or NULL if not found.
 */
void *rp_memmem(const void *haystack, size_t haystacklen,
	        const void *needle, size_t needlelen)
{
	// Reference: https://opensource.apple.com/source/Libc/Libc-1044.1.2/string/FreeBSD/memmem.c
	// NOTE: haystack was originally 'l'; needle was originally 's'.
	// NOTE: 'register' keywords were removed.
	const char *cur, *last;
	const char *cl = (const char *)haystack;
	const char *cs = (const char *)needle;

	/* we need something to compare */
	if (haystacklen == 0 || needlelen == 0) {
		return NULL;
	}

	/* "s" must be smaller or equal to "l" */
	if (haystacklen < needlelen) {
		return NULL;
	}

	/* special case where s_len == 1 */
	if (needlelen == 1) {
		return (void*)memchr(haystack, (int)*cs, needlelen);
	}

	/* the last position where its possible to find "s" in "l" */
	last = (const char *)cl + haystacklen - needlelen;

	for (cur = (const char *)cl; cur <= last; cur++) {
		if (cur[0] == cs[0] && memcmp(cur, cs, needlelen) == 0) {
			return (void*)cur;
		}
	}

	return NULL;
}
#endif /* HAVE_MEMMEM */

#ifndef HAVE_STRLCAT
/**
 * strcat() but with a length parameter to prevent buffer overflows.
 * @param dst [in,out] Destination string
 * @param src [in] Source string
 * @param size [in] Size of destination string
 * @return Total length of the string that was attempted to be created: strlen(dst) + strlen(src)
 */
size_t rp_strlcat(char *dst, const char *src, size_t size)
{
	// Reference: https://opensource.apple.com/source/Libc/Libc-262/string/strlcat.c.auto.html
	// NOTE: 'size' was originally 'siz'.
	// NOTE: 'register' keywords were removed.
	register char *d = dst;
	register const char *s = src;
	register size_t n = size;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0') {
		d++;
	}
	dlen = d - dst;
	n = size - dlen;

	if (n == 0) {
		return(dlen + strlen(s));
	}
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#endif /* HAVE_STRLCAT */

/** ASCII character functions **/
/* from glib-2.90 (LGPL-2.1+) */
/* Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald */

const uint16_t rp_ascii_table[256] = {
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
	0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
	0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
	0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
	0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
	0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
	0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
	0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
	0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
	0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004,

	/* the upper 128 are all zeroes */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// NOTE: Windows does not allow delay-loading a DLL if importing a data symbol.
// On Windows, rp_ascii_table[] will not be exported; instead, an accessor
// function will be used. This *will* add slight overhead...
#ifdef _WIN32
uint16_t rp_ascii_table_get(char c)
{
	return rp_ascii_table[(uint8_t)c];
}
#endif /* _WIN32 */

/**
 * Convert an ASCII character to lowercase.
 * @param c ASCII character
 * @return Lowercase ASCII character
 */
char rp_ascii_tolower(char c)
{
	return (rp_ascii_isupper(c)) ? (c & ~0x20) : c;
}

/**
 * Convert an ASCII character to uppercase.
 * @param c ASCII character
 * @return Uppercase ASCII character
 */
char rp_ascii_toupper(char c)
{
	return (rp_ascii_islower(c)) ? (c | ~0x20) : c;
}

/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * libc.c: Reimplementations of libc functions that aren't present on      *
 * this system.                                                            *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "libc.h"

// C includes
#include <string.h>

/** Reimplementations of libc functions that aren't present on this system. **/

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
	if (haystacklen == 0 || needlelen == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (haystacklen < needlelen)
		return NULL;

	/* special case where s_len == 1 */
	if (needlelen == 1)
		return (void*)memchr(haystack, (int)*cs, needlelen);

	/* the last position where its possible to find "s" in "l" */
	last = (const char *)cl + haystacklen - needlelen;

	for (cur = (const char *)cl; cur <= last; cur++) {
		if (cur[0] == cs[0] && memcmp(cur, cs, needlelen) == 0)
			return (void*)cur;
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
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
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

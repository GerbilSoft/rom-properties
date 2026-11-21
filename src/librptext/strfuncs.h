/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * strfuncs.h: String functions.                                           *
 *                                                                         *
 * SPDX-License-Identifier: LGPL-2.1-or-later                              *
 ***************************************************************************/

#pragma once

#include "config.librptext.h"
#include "compiler-attrs.h"	// for ATTR_*
#include "dll-macros.h"		// for RP_LIBROMDATA_PUBLIC

// C includes
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reimplementations of libc string functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param maxlen Maximum length of the string
 * @returns equivivalent to min(strlen(str), maxlen) without buffer overruns
 */
ATTR_ACCESS_SIZE(read_only, 1, 2)
size_t rp_strnlen(const char *str, size_t maxlen);

#  ifdef strnlen
#    undef strnlen
#  endif
#  define strnlen(str, maxlen) rp_strnlen((str), (maxlen))
#endif /* !HAVE_STRNLEN */

#ifndef HAVE_MEMMEM
/**
 * Find a string within a block of memory.
 * @param haystack Block of memory.
 * @param haystacklen Length of haystack.
 * @param needle String to search for.
 * @param needlelen Length of needle.
 * @return Location of needle in haystack, or NULL if not found.
 */
ATTR_ACCESS_SIZE(read_only, 1, 2)
ATTR_ACCESS_SIZE(read_only, 3, 4)
void *rp_memmem(const void *haystack, size_t haystacklen,
	        const void *needle, size_t needlelen);

#  ifdef memmem
#    undef memmem
#  endif
#  define memmem(haystack, haystacklen, needle, needlelen) rp_memmem((haystack), (haystacklen), (needle), (needlelen))
#endif /* !HAVE_MEMMEM */

#ifndef HAVE_STRLCAT
/**
 * strcat() but with a length parameter to prevent buffer overflows.
 * @param dst [in,out] Destination string
 * @param src [in] Source string
 * @param size [in] Size of destination string
 * @return Total length of the string that was attempted to be created: strlen(dst) + strlen(src)
 */
size_t rp_strlcat(char *dst, const char *src, size_t size);

#  ifdef strlcat
#    undef strlcat
#  endif
#  define strlcat(dst, src, size) rp_strlcat((dst), (src), (size))
#endif /* !HAVE_STRLCAT */

/** ASCII character functions **/
/* from glib-2.90 (LGPL-2.1+) */
/* Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald */

typedef enum {
	RP_ASCII_ALNUM  = 1 << 0,
	RP_ASCII_ALPHA  = 1 << 1,
	RP_ASCII_CNTRL  = 1 << 2,
	RP_ASCII_DIGIT  = 1 << 3,
	RP_ASCII_GRAPH  = 1 << 4,
	RP_ASCII_LOWER  = 1 << 5,
	RP_ASCII_PRINT  = 1 << 6,
	RP_ASCII_PUNCT  = 1 << 7,
	RP_ASCII_SPACE  = 1 << 8,
	RP_ASCII_UPPER  = 1 << 9,
	RP_ASCII_XDIGIT = 1 << 10
} ATTR_FLAG_ENUM RpAsciiType;

// NOTE: Windows does not allow delay-loading a DLL if importing a data symbol.
// On Windows, rp_ascii_table[] will not be exported; instead, an accessor
// function will be used. This *will* add slight overhead...
#ifdef _WIN32
RP_LIBROMDATA_PUBLIC
uint16_t rp_ascii_table_get(char c);
#else /* !_WIN32 */
RP_LIBROMDATA_PUBLIC
extern const uint16_t rp_ascii_table[256];
#define rp_ascii_table_get(c) (rp_ascii_table[(uint8_t) (c)])
#endif /* _WIN32 */

#define rp_ascii_isalnum(c) \
	((rp_ascii_table_get(c) & RP_ASCII_ALNUM) != 0)

#define rp_ascii_isalpha(c) \
	((rp_ascii_table_get(c) & RP_ASCII_ALPHA) != 0)

#define rp_ascii_iscntrl(c) \
	((rp_ascii_table_get(c) & RP_ASCII_CNTRL) != 0)

#define rp_ascii_isdigit(c) \
	((rp_ascii_table_get(c) & RP_ASCII_DIGIT) != 0)

#define rp_ascii_isgraph(c) \
	((rp_ascii_table_get(c) & RP_ASCII_GRAPH) != 0)

#define rp_ascii_islower(c) \
	((rp_ascii_table_get(c) & RP_ASCII_LOWER) != 0)

#define rp_ascii_isprint(c) \
	((rp_ascii_table_get(c) & RP_ASCII_PRINT) != 0)

#define rp_ascii_ispunct(c) \
	((rp_ascii_table_get(c) & RP_ASCII_PUNCT) != 0)

#define rp_ascii_isspace(c) \
	((rp_ascii_table_get(c) & RP_ASCII_SPACE) != 0)

#define rp_ascii_isupper(c) \
	((rp_ascii_table_get(c) & RP_ASCII_UPPER) != 0)

#define rp_ascii_isxdigit(c) \
	((rp_ascii_table_get(c) & RP_ASCII_XDIGIT) != 0)

/**
 * Convert an ASCII character to lowercase.
 * @param c ASCII character
 * @return Lowercase ASCII character
 */
RP_LIBROMDATA_PUBLIC
char rp_ascii_tolower(char c) ATTR_CONST;

/**
 * Convert an ASCII character to uppercase.
 * @param c ASCII character
 * @return Uppercase ASCII character
 */
RP_LIBROMDATA_PUBLIC
char rp_ascii_toupper(char c) ATTR_CONST;

#ifdef _WIN32
/**
 * Convert an ASCII character to lowercase.
 * @param c ASCII character
 * @return Lowercase ASCII character
 */
RP_LIBROMDATA_PUBLIC
wchar_t rp_ascii_towlower(wchar_t c) ATTR_CONST;

/**
 * Convert an ASCII character to uppercase.
 * @param c ASCII character
 * @return Uppercase ASCII character
 */
RP_LIBROMDATA_PUBLIC
wchar_t rp_ascii_towupper(wchar_t c) ATTR_CONST;
#endif /* _WIN32 */

#if defined(_WIN32) && defined(_UNICODE)
#  define rp_ascii_istlower(c) (((c) < 256) && rp_ascii_islower((uint8_t)(c)))
#  define rp_ascii_istspace(c) (((c) < 256) && rp_ascii_isspace((uint8_t)(c)))
#  define rp_ascii_istupper(c) (((c) < 256) && rp_ascii_isupper((uint8_t)(c)))
#  define rp_ascii_totlower(c) rp_ascii_towlower(c)
#  define rp_ascii_totupper(c) rp_ascii_towupper(c)
#else /* !(_WIN32 && _UNICODE) */
#  define rp_ascii_istlower(c) rp_ascii_islower(c)
#  define rp_ascii_istspace(c) rp_ascii_isspace(c)
#  define rp_ascii_istupper(c) rp_ascii_isupper(c)
#  define rp_ascii_totlower(c) rp_ascii_tolower(c)
#  define rp_ascii_totupper(c) rp_ascii_toupper(c)
#endif /* _WIN32 && _UNICODE */

#ifdef __cplusplus
}
#endif

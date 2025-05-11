/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * ctypex.h: ctype functions with unsigned char casting.                   *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
#  include <cctype>
#else
#  include <ctype.h>
#endif

#define ISALNUM(c) isalnum((unsigned char)(c))
#define ISALPHA(c) isalpha((unsigned char)(c))
#define ISCNTRL(c) iscntrl((unsigned char)(c))
#define ISDIGIT(c) isdigit((unsigned char)(c))
#define ISGRAPH(c) isgraph((unsigned char)(c))
#define ISLOWER(c) islower((unsigned char)(c))
#define ISPRINT(c) isprint((unsigned char)(c))
#define ISPUNCT(c) ispunct((unsigned char)(c))
#define ISSPACE(c) isspace((unsigned char)(c))
#define ISUPPER(c) isupper((unsigned char)(c))
#define ISXDIGIT(c) isxdigit((unsigned char)(c))

#define ISASCII(c) isascii((unsigned char)(c))
#define ISBLANK(c) isblank((unsigned char)(c))

#define TOUPPER(c) toupper((unsigned char)(c))
#define TOLOWER(c) tolower((unsigned char)(c))

/** Explicit ASCII versions **/

#include "stdboolx.h"

#ifdef __cplusplus
#  define CTYPEX_CONSTEXPR constexpr
#else /* !__cplusplus */
#  define CTYPEX_CONSTEXPR
#endif

/** is*_ascii() functions **/

/**
 * Non-localized isdigit() implementation.
 * @param c Character
 * @param True if c is a digit; false if not.
 */
static CTYPEX_CONSTEXPR inline bool isdigit_ascii(int c)
{
	return (c >= '0' && c <= '9');
}

/**
 * Non-localized isxdigit() implementation.
 * @param c Character
 * @param True if c is a digit; false if not.
 */
static CTYPEX_CONSTEXPR inline bool isxdigit_ascii(int c)
{
	return isdigit_ascii(c) ||
	       (c >= 'A' && c <= 'F') ||
	       (c >= 'a' && c <= 'f');
}

/**
 * Non-localized isupper() implementation.
 * @param c Character
 * @param True if c is an uppercase letter; false if not.
 */
static CTYPEX_CONSTEXPR inline bool isupper_ascii(int c)
{
	return (c >= 'A' && c <= 'Z');
}

/**
 * Non-localized islower() implementation.
 * @param c Character
 * @param True if c is an uppercase letter; false if not.
 */
static CTYPEX_CONSTEXPR inline bool islower_ascii(int c)
{
	return (c >= 'a' && c <= 'z');
}

/**
 * Non-localized isalpha() implementation.
 * @param c Character
 * @param True if c is a letter; false if not.
 */
static CTYPEX_CONSTEXPR inline bool isalpha_ascii(int c)
{
	return isupper_ascii(c) || islower_ascii(c);
}

/**
 * Non-localized isalnum() implementation.
 * @param c Character
 * @param True if c is a letter or a number; false if not.
 */
static CTYPEX_CONSTEXPR inline bool isalnum_ascii(int c)
{
	return isalpha_ascii(c) || isdigit_ascii(c);
}

/** to*_ascii() functions **/

/**
 * Non-localized toupper() implementation.
 * @param c Character
 * @return Uppercase character if it's lowercase, or the original character otherwise.
 */
static CTYPEX_CONSTEXPR inline int toupper_ascii(int c)
{
	return islower_ascii(c) ? (c & ~0x20) : c;
}

/**
 * Non-localized tolower() implementation.
 * @param c Character
 * @return Lowercase character if it's uppercase, or the original character otherwise.
 */
static CTYPEX_CONSTEXPR inline int tolower_ascii(int c)
{
	return isupper_ascii(c) ? (c | 0x20) : c;
}

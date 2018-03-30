/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs.hpp: Text encoding functions.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__

#include "config.librpbase.h"

// System byteorder is needed for conversions from UTF-16.
// Conversions to UTF-16 always use host-endian.
#include "byteorder.h"

// printf()-style function attribute.
#ifndef ATTR_PRINTF
# ifdef __GNUC__
#  define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
# else
#  define ATTR_PRINTF(fmt, args)
# endif
#endif /* ATTR_PRINTF */

// C includes.
#include <stddef.h>	/* size_t */

// C++ includes.
#include <string>

/** Reimplementations of libc functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param maxlen Maximum length of the string
 * @returns equivivalent to min(strlen(str), maxlen) without buffer overruns
 */
extern "C"
size_t strnlen(const char *str, size_t maxlen);
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
	     const void *needle, size_t needlelen);
#endif /* HAVE_MEMMEM */

namespace LibRpBase {

/** UTF-16 string functions. **/

/**
 * char16_t strlen().
 * @param wcs 16-bit string.
 * @return Length of wcs, in characters.
 */
#ifdef RP_WIS16
static inline size_t u16_strlen(const char16_t *wcs)
{
	return wcslen(reinterpret_cast<const wchar_t*>(wcs));
}
#else /* !RP_WIS16 */
size_t u16_strlen(const char16_t *wcs);
#endif /* RP_WIS16 */

/**
 * char16_t strnlen().
 * @param wcs 16-bit string.
 * @param maxlen Maximum length.
 * @return Length of wcs, in characters.
 */
#ifdef RP_WIS16
static inline size_t u16_strnlen(const char16_t *wcs, size_t maxlen)
{
	return wcsnlen(reinterpret_cast<const wchar_t*>(wcs), maxlen);
}
#else /* !RP_WIS16 */
size_t u16_strnlen(const char16_t *wcs, size_t maxlen);
#endif /* RP_WIS16 */

/**
 * char16_t strdup().
 * @param wcs 16-bit string.
 * @return Copy of wcs.
 */
#ifdef RP_WIS16
static inline char16_t *u16_strdup(const char16_t *wcs)
{
	return reinterpret_cast<char16_t*>(
		wcsdup(reinterpret_cast<const wchar_t*>(wcs)));
}
#else /* !RP_WIS16 */
char16_t *u16_strdup(const char16_t *wcs);
#endif /* RP_WIS16 */

/**
 * char16_t strcmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcmp() result.
 */
#ifdef RP_WIS16
static inline int u16_strcmp(const char16_t *wcs1, const char16_t *wcs2)
{
	return wcscmp(reinterpret_cast<const wchar_t*>(wcs1),
		      reinterpret_cast<const wchar_t*>(wcs2));
}
#else /* !RP_WIS16 */
int u16_strcmp(const char16_t *wcs1, const char16_t *wcs2);
#endif /* RP_WIS16 */

/**
 * char16_t strcmp().
 * @param wcs1 16-bit string 1.
 * @param wcs2 16-bit string 2.
 * @return strcmp() result.
 */
#ifdef RP_WIS16
static inline int u16_strcasecmp(const char16_t *wcs1, const char16_t *wcs2)
{
	return wcscasecmp(reinterpret_cast<const wchar_t*>(wcs1),
			  reinterpret_cast<const wchar_t*>(wcs2));
}
#else /* !RP_WIS16 */
int u16_strcasecmp(const char16_t *wcs1, const char16_t *wcs2);
#endif /* RP_WIS16 */

/** Generic text conversion functions. **/

// NOTE: All of these functions will remove trailing
// NULL characters from strings.

/** Code Page 1252 **/

/**
 * Convert cp1252 text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cp1252_to_utf8(const char *str, int len);

/**
 * Convert cp1252 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string cp1252_to_utf16(const char *str, int len);

/**
 * Convert UTF-8 text to cp1252.
 * Trailing NULL bytes will be removed.
 * Invalid characters will be ignored.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return cp1252 string.
 */
std::string utf8_to_cp1252(const char *str, int len);

/**
 * Convert UTF-16 text to cp1252.
 * Trailing NULL bytes will be removed.
 * Invalid characters will be ignored.
 * @param wcs UTF-16 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return cp1252 string.
 */
std::string utf16_to_cp1252(const char16_t *wcs, int len);

/** Code Page 1252 + Shift-JIS (932) **/

/**
 * Convert cp1252 or Shift-JIS text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cp1252_sjis_to_utf8(const char *str, int len);

/**
 * Convert cp1252 or Shift-JIS text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string cp1252_sjis_to_utf16(const char *str, int len);

/** Latin-1 (ISO-8859-1) **/

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string latin1_to_utf8(const char *str, int len);

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string latin1_to_utf16(const char *str, int len);

/**
 * Convert UTF-8 text to Latin-1 (ISO-8859-1).
 * Trailing NULL bytes will be removed.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return Latin-1 string.
 */
std::string utf8_to_latin1(const char *str, int len);

/**
 * Convert UTF-16 Latin-1 (ISO-8859-1).
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return Latin-1 string.
 */
std::string utf16_to_latin1(const char16_t *wcs, int len);

/** UTF-8 to UTF-16 and vice-versa **/

/**
 * Convert UTF-8 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string utf8_to_utf16(const char *str, int len);

/**
 * Convert UTF-8 text to UTF-16.
 * Trailing NULL bytes will be removed.
 * @param str UTF-8 string.
 * @return UTF-16 string.
 */
static inline std::u16string utf8_to_utf16(const std::string &str)
{
	return utf8_to_utf16(str.data(), (int)str.size());
}

/**
 * Convert UTF-16LE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16LE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16le_to_utf8(const char16_t *wcs, int len);

/**
 * Convert UTF-16BE text to UTF-8.
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16BE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16be_to_utf8(const char16_t *wcs, int len);

/**
 * Convert UTF-16 text to UTF-8. (host-endian)
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16 text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string utf16_to_utf8(const char16_t *wcs, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16le_to_utf8(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16be_to_utf8(wcs, len);
#endif
}

/**
 * Byteswap and return UTF-16 text.
 * @param wcs UTF-16 text to byteswap.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Byteswapped UTF-16 string.
 */
std::u16string utf16_bswap(const char16_t *wcs, int len);

/**
 * Convert UTF-16LE text to host-endian UTF-16.
 * Trailing NULL bytes will be removed.
 * @param wcs UTF-16LE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16le_to_utf16(const char16_t *wcs, int len)
{
	// Check for a NULL terminator.
	if (len < 0) {
		len = (int)u16_strlen(wcs);
	} else {
		len = (int)u16_strnlen(wcs, len);
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return std::u16string(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16_bswap(wcs, len);
#endif
}

/**
 * Convert UTF-16BE text to host-endian UTF-16.
 * @param wcs UTF-16BLE text.
 * @param len Length of wcs, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16be_to_utf16(const char16_t *wcs, int len)
{
	// Check for a NULL terminator.
	if (len < 0) {
		len = (int)u16_strlen(wcs);
	} else {
		len = (int)u16_strnlen(wcs, len);
	}

#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16_bswap(wcs, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return std::u16string(wcs, len);
#endif
}

/**
 * sprintf()-style function for std::string.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string
 */
std::string rp_sprintf(const char *fmt, ...) ATTR_PRINTF(1, 2);

#ifdef _MSC_VER
/**
 * sprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * @param fmt Format string.
 * @param ... Arguments.
 * @return std::string.
 */
std::string rp_sprintf_p(const char *fmt, ...) ATTR_PRINTF(1, 2);
#else
// glibc supports positional format string arguments
// in the standard printf() functions.
#define rp_sprintf_p(fmt, ...) rp_sprintf(fmt, ##__VA_ARGS__)
#endif

}

#ifdef _WIN32
// wchar_t text conversion macros.
// Generally used only on Windows.
#include "TextFuncs_wchar.hpp"
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__ */

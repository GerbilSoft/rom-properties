/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs.hpp: Text encoding functions.                                 *
 *                                                                         *
 * Copyright (c) 2009-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__

#include "librpbase/config.librpbase.h"

#ifdef RP_UTF8
// rp_str*() are inlined versions of the standard
// str*() functions in the UTF-8 build.
#include <cstring>
#endif

#if defined(RP_UTF16) && defined(RP_WIS16)
// rp_str*() are inlined versions of the standard
// wcs*() functions in the UTF-16 build if
// wchar_t is 16-bit.
#include <cwchar>
#endif

// System byteorder is needed for conversions from UTF-16.
// Conversions to UTF-16 always use host-endian.
#include "byteorder.h"

// Shared internal functions.
#include "TextFuncs_int.hpp"

namespace LibRpBase {

/** Generic text conversion functions. **/

// NOTE: All of these functions will remove trailing
// NULL characters from strings.

// TODO: #ifdef out functions that aren't used
// in RP_UTF8 and/or RP_UTF16 builds?

/** Code Page 1252 **/

/**
 * Convert cp1252 text to UTF-8.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cp1252_to_utf8(const char *str, int len);

/**
 * Convert cp1252 text to UTF-16.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string cp1252_to_utf16(const char *str, int len);

/** Code Page 1252 + Shift-JIS (932) **/

/**
 * Convert cp1252 or Shift-JIS text to UTF-8.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string cp1252_sjis_to_utf8(const char *str, int len);

/**
 * Convert cp1252 or Shift-JIS text to UTF-16.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string cp1252_sjis_to_utf16(const char *str, int len);

/** UTF-8 to UTF-16 and vice-versa **/

/**
 * Convert UTF-8 text to UTF-16.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string utf8_to_utf16(const char *str, int len);

/**
 * Convert UTF-16LE text to UTF-8.
 * @param str UTF-16LE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16le_to_utf8(const char16_t *str, int len);

/**
 * Convert UTF-16BE text to UTF-8.
 * @param str UTF-16BE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string utf16be_to_utf8(const char16_t *str, int len);

/**
 * Convert UTF-16 text to UTF-8. (host-endian)
 * @param str UTF-16 text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string utf16_to_utf8(const char16_t *str, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16le_to_utf8(str, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16be_to_utf8(str, len);
#endif
}

/**
 * Byteswap and return UTF-16 text.
 * @param str UTF-16 text to byteswap.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return Byteswapped UTF-16 string.
 */
std::u16string utf16_bswap(const char16_t *str, int len);

/**
 * Convert UTF-16LE text to host-endian UTF-16.
 * @param str UTF-16LE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16le_to_utf16(const char16_t *str, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	REMOVE_TRAILING_NULLS_RP_WRAPPER(std::u16string, str, len);
	return std::u16string(str, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	REMOVE_TRAILING_NULLS_RP_WRAPPER_NOIMPLICIT(std::u16string, str, len);
	return utf16_bswap(str, len);
#endif
}

/**
 * Convert UTF-16BE text to host-endian UTF-16.
 * @param str UTF-16BLE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return Host-endian UTF-16 string.
 */
static inline std::u16string utf16be_to_utf16(const char16_t *str, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	REMOVE_TRAILING_NULLS_RP_WRAPPER_NOIMPLICIT(std::u16string, str, len);
	return utf16_bswap(str, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	REMOVE_TRAILING_NULLS_RP_WRAPPER(std::u16string, str, len);
	return std::u16string(str, len);
#endif
}

/** Latin-1 (ISO-8859-1) **/

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-8.
 * NOTE: 0x80-0x9F (cp1252) is converted to '?'.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
std::string latin1_to_utf8(const char *str, int len);

/**
 * Convert Latin-1 (ISO-8859-1) text to UTF-16.
 * NOTE: 0x80-0x9F (cp1252) is converted to '?'.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
std::u16string latin1_to_utf16(const char *str, int len);

/** Miscellaneous functions. **/

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

/** rp_string wrappers. **/

/**
 * Convert cp1252 text to rp_string.
 * @param str cp1252 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string cp1252_to_rp_string(const char *str, int len)
{
#ifdef RP_UTF8
	return cp1252_to_utf8(str, len);
#else
	return cp1252_to_utf16(str, len);
#endif
}

/**
 * Convert cp1252 or Shift-JIS text to rp_string.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string cp1252_sjis_to_rp_string(const char *str, int len)
{
#ifdef RP_UTF8
	return cp1252_sjis_to_utf8(str, len);
#else
	return cp1252_sjis_to_utf16(str, len);
#endif
}

/**
 * Convert UTF-8 text to rp_string.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string utf8_to_rp_string(const char *str, int len)
{
#if defined(RP_UTF8)
	REMOVE_TRAILING_NULLS_RP_WRAPPER(rp_string, str, len);
	return rp_string(str, len);
#elif defined(RP_UTF16)
	return utf8_to_utf16(str, len);
#endif
}

/**
 * Convert UTF-8 text to rp_string.
 * @param str UTF-8 text.
 * @return rp_string.
 */
static inline rp_string utf8_to_rp_string(const std::string &str)
{
#if defined(RP_UTF8)
	return str;
#elif defined(RP_UTF16)
	return utf8_to_utf16(str.data(), (int)str.size());
#endif
}

/**
 * Convert rp_string to UTF-8 text.
 * @param str rp_string.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-8 string.
 */
static inline std::string rp_string_to_utf8(const rp_char *str, int len)
{
#if defined(RP_UTF8)
	REMOVE_TRAILING_NULLS_RP_WRAPPER(std::string, str, len);
	return std::string(str, len);
#elif defined(RP_UTF16)
	return utf16_to_utf8(str, len);
#endif
}

/**
 * Convert rp_string to UTF-8 text.
 * @param rps rp_string.
 * @return UTF-8 string.
 */
static inline std::string rp_string_to_utf8(const rp_string &rps)
{
#if defined(RP_UTF8)
	return rps;
#elif defined(RP_UTF16)
	return utf16_to_utf8(rps.data(), (int)rps.size());
#endif
}

/**
 * Convert UTF-16LE text to rp_string.
 * @param str UTF-16LE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string utf16le_to_rp_string(const char16_t *str, int len)
{
#if defined(RP_UTF8)
	return utf16le_to_utf8(str, len);
#elif defined(RP_UTF16)
	return utf16le_to_utf16(str, len);
#endif
}

/**
 * Convert UTF-16BE text to rp_string.
 * @param str UTF-16BE text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string utf16be_to_rp_string(const char16_t *str, int len)
{
#if defined(RP_UTF8)
	return utf16be_to_utf8(str, len);
#elif defined(RP_UTF16)
	return utf16be_to_utf16(str, len);
#endif
}

/**
 * Convert UTF-16 text to rp_string. (host-endian)
 * @param str UTF-16 text.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string utf16_to_rp_string(const char16_t *str, int len)
{
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return utf16le_to_rp_string(str, len);
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16be_to_rp_string(str, len);
#endif
}

/**
 * Convert UTF-16LE text to rp_string.
 * @param str UTF-16LE text.
 * @return rp_string.
 */
static inline rp_string utf16le_to_rp_string(const std::u16string &str)
{
#if defined(RP_UTF8)
	return utf16le_to_utf8(str.data(), (int)str.size());
#elif defined(RP_UTF16)
# if SYS_BYTEORDER == SYS_LIL_ENDIAN
	return str;
# else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16le_to_utf16(str.data(), (int)str.size());
# endif
#endif
}

/**
 * Convert UTF-16BE text to rp_string.
 * @param str UTF-16BE text.
 * @return rp_string.
 */
static inline rp_string utf16be_to_rp_string(const std::u16string &str)
{
#if defined(RP_UTF8)
	return utf16be_to_utf8(str.data(), (int)str.size());
#elif defined(RP_UTF16)
# if SYS_BYTEORDER == SYS_BIG_ENDIAN
	return str;
# else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	return utf16be_to_utf16(str.data(), (int)str.size());
# endif
#endif
}

/**
 * Convert UTF-16 text to rp_string. (host-endian)
 * @param str UTF-16 text.
 * @return rp_string.
 */
static inline rp_string utf16_to_rp_string(const std::u16string &str)
{
#if defined(RP_UTF8)
	return utf16_to_utf8(str.data(), (int)str.size());
#elif defined(RP_UTF16)
	return str;
#endif
}

/**
 * Convert rp_string to UTF-16 text.
 * @param str rp_string.
 * @param len Length of str, in characters. (-1 for NULL-terminated string)
 * @return UTF-16 string.
 */
static inline std::u16string rp_string_to_utf16(const rp_char *str, int len)
{
#if defined(RP_UTF8)
	return utf8_to_utf16(str, len);
#elif defined(RP_UTF16)
	REMOVE_TRAILING_NULLS_RP_WRAPPER(std::u16string, str, len);
	return std::u16string(str, len);
#endif
}

/**
 * Convert rp_string to UTF-16 text.
 * @param rps rp_string.
 * @return UTF-16 string.
 */
static inline std::u16string rp_string_to_utf16(const rp_string &rps)
{
#if defined(RP_UTF8)
	return utf8_to_utf16(rps.data(), (int)rps.size());
#elif defined(RP_UTF16)
	return rps;
#endif
}

/**
 * Convert Latin-1 (ISO-8859-1) text to rp_string.
 * NOTE: 0x80-0x9F (cp1252) is converted to '?'.
 * @param str Latin-1 text.
 * @param len Length of str, in bytes. (-1 for NULL-terminated string)
 * @return rp_string.
 */
static inline rp_string latin1_to_rp_string(const char *str, int len)
{
#if defined(RP_UTF8)
	return latin1_to_utf8(str, len);
#elif defined(RP_UTF16)
	return latin1_to_utf16(str, len);
#endif
}

/**
 * strlen() function for rp_char strings.
 * @param str rp_string.
 * @return Length of str, in characters.
 */
static inline size_t rp_strlen(const rp_char *str)
{
#if defined(RP_UTF8)
	return strlen(str);
#elif defined(RP_UTF16)
	return u16_strlen(str);
#endif
}

/**
 * strdup() function for rp_char strings.
 * @param str String.
 * @return Duplicated string.
 */
static inline rp_char *rp_strdup(const rp_char *str)
{
#if defined(RP_UTF8)
	return strdup(str);
#elif defined(RP_UTF16)
	return u16_strdup(str);
#endif
}
static inline rp_char *rp_strdup(const rp_string &str)
{
#if defined(RP_UTF8)
	return strdup(str.c_str());
#elif defined(RP_UTF16)
	return u16_strdup(str.c_str());
#endif
}

/**
 * strcmp() function for rp_char strings.
 * @param str1 rp_string 1.
 * @param str2 rp_string 2.
 * @return strcmp() result.
 */
static inline int rp_strcmp(const rp_char *str1, const rp_char *str2)
{
#if defined(RP_UTF8)
	return strcmp(str1, str2);
#elif defined(RP_UTF16)
	return u16_strcmp(str1, str2);
#endif
}

/**
 * strcasecmp() function for rp_char strings.
 * @param str1 rp_string 1.
 * @param str2 rp_string 2.
 * @return strcasecmp() result.
 */
static inline int rp_strcasecmp(const rp_char *str1, const rp_char *str2)
{
#if defined(RP_UTF8)
	return strcasecmp(str1, str2);
#elif defined(RP_UTF16)
	return u16_strcasecmp(str1, str2);
#endif
}

}

/** Reimplementations of libc functions that aren't present on this system. **/

#ifndef HAVE_STRNLEN
/**
 * String length with limit. (8-bit strings)
 * @param str The string itself
 * @param len Maximum length of the string
 * @returns equivivalent to min(strlen(str), len) without buffer overruns
 */
extern "C"
size_t strnlen(const char *str, size_t len);
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

#ifdef _WIN32
// wchar_t text conversion macros.
// Generally used only on Windows.
#include "TextFuncs_wchar.hpp"
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_HPP__ */

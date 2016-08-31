/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TextFuncs.hpp: Text encoding functions.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_HPP__
#define __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_HPP__

#include "libromdata/config.libromdata.h"

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

namespace LibRomData {

/** Generic text conversion functions. **/

// TODO: #ifdef out functions that aren't used
// in RP_UTF8 and/or RP_UTF16 builds?

/**
 * Convert cp1252 or Shift-JIS text to UTF-8.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes.
 * @return UTF-8 string.
 */
std::string cp1252_sjis_to_utf8(const char *str, size_t len);

/**
 * Convert cp1252 or Shift-JIS text to UTF-16.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str, in bytes.
 * @return UTF-16 string.
 */
std::u16string cp1252_sjis_to_utf16(const char *str, size_t len);

/**
 * Convert UTF-8 text to UTF-16.
 * @param str UTF-8 text.
 * @param len Length of str, in bytes.
 * @return UTF-16 string.
 */
std::u16string utf8_to_utf16(const char *str, size_t len);

/**
 * Convert UTF-16 text to UTF-8.
 * @param str UTF-16 text.
 * @param len Length of str, in characters.
 * @return UTF-8 string.
 */
std::string utf16_to_utf8(const char16_t *str, size_t len);

/**
 * Convert ASCII text to UTF-16.
 * NOTE: The text MUST be ASCII, NOT Latin-1 or UTF-8!
 * Those aren't handled here for performance reasons.
 * @param str ASCII text.
 * @param len Length of str, in bytes.
 * @return UTF-16 string.
 */
std::u16string ascii_to_utf16(const char *str, size_t len);

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

/** rp_string wrappers. **/

/**
 * Convert cp1252 or Shift-JIS text to rp_string.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str.
 * @return rp_string.
 */
static inline rp_string cp1252_sjis_to_rp_string(const char *str, size_t len)
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
 * @param len Length of str.
 * @return rp_string.
 */
static inline rp_string utf8_to_rp_string(const char *str, size_t len)
{
#if defined(RP_UTF8)
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
	return utf8_to_utf16(str.data(), str.size());
#endif
}

/**
 * Convert rp_string to UTF-8 text.
 * @param str rp_string.
 * @param len Length of str, in characters.
 * @return UTF-8 string.
 */
static inline std::string rp_string_to_utf8(const rp_char *str, size_t len)
{
#if defined(RP_UTF8)
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
	return utf16_to_utf8(rps.data(), rps.size());
#endif
}

/**
 * Convert rp_string to UTF-16 text.
 * @param str rp_string.
 * @param len Length of str, in characters.
 * @return UTF-16 string.
 */
static inline std::u16string rp_string_to_utf16(const rp_char *str, size_t len)
{
#if defined(RP_UTF8)
	return utf8_to_utf16(str, len);
#elif defined(RP_UTF16)
	return std::u16string(str, len);
#endif
}

/**
 * Convert rp_string to UTF-8 text.
 * @param rps rp_string.
 * @return UTF-8 string.
 */
static inline std::u16string rp_string_to_utf16(const rp_string &rps)
{
#if defined(RP_UTF8)
	return utf8_to_utf16(rps.data(), rps.size());
#elif defined(RP_UTF16)
	return rps;
#endif
}

/**
 * Convert ASCII text to rp_string.
 * NOTE: The text MUST be ASCII, NOT Latin-1 or UTF-8!
 * Those aren't handled here for performance reasons.
 * @param str ASCII text.
 * @param len Length of str, in bytes.
 * @return rp_string.
 */
static inline rp_string ascii_to_rp_string(const char *str, size_t len)
{
#if defined(RP_UTF8)
	return rp_string(str, len);
#elif defined(RP_UTF16)
	return ascii_to_utf16(str, len);
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

}

#if defined(_WIN32) && defined(RP_WIS16)
/** Windows-specific wrappers for wchar_t. **/

/**
 * NOTE: These are defined outside of the LibRomData
 * namespace because macros are needed for the UTF-8
 * versions.
 *
 * NOTE 2: The UTF-8 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::wstring or rp_string if
 * storing it, since a wchar_t* or rp_char* will
 * result in a dangling pointer.
 */

#if defined(RP_UTF8)

/**
 * Get const wchar_t* from const rp_char*.
 * @param str const rp_char*.
 * @return const wchar_t*
 */
#define RP2W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRomData::rp_string_to_utf16(str, strlen(str)).c_str()))

/**
 * Get const wchar_t* from rp_string.
 * @param rps rp_string.
 * @return const wchar_t*
 */
#define RP2W_s(rps) \
	(reinterpret_cast<const wchar_t*>( \
		LibRomData::rp_string_to_utf16(rps).c_str()))

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*.
 * @return const rp_char*
 */
#define W2RP_c(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), \
		wcslen(wcs)))

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring.
 * @return const rp_char*
 */
#define W2RP_s(wcs) \
	(LibRomData::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs.data()), \
		wcs.size()))

#elif defined(RP_UTF16)

/**
 * Get const wchar_t* from const rp_char*.
 * @param str const rp_char*.
 * @return const wchar_t*
 */
static inline const wchar_t *RP2W_c(const rp_char *str)
{
	return reinterpret_cast<const wchar_t*>(str);
}

/**
 * Get const wchar_t* from rp_string.
 * @param rps rp_string.
 * @return const wchar_t*
 */
static inline const wchar_t *RP2W_s(const LibRomData::rp_string &rps)
{
	return reinterpret_cast<const wchar_t*>(rps.c_str());
}

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*.
 * @return const rp_char*
 */
static inline const rp_char *W2RP_c(const wchar_t *wcs)
{
	return reinterpret_cast<const rp_char*>(wcs);
}

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring.
 * @return const rp_char*
 */
static inline const rp_char *W2RP_s(const std::wstring &wcs)
{
	return reinterpret_cast<const rp_char*>(wcs.c_str());
}

#endif /* RP_UTF16 */

#endif /* _WIN32 && RP_WIS16 */

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_HPP__ */

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

/**
 * Convert cp1252 or Shift-JIS text to rp_string.
 * @param str cp1252 or Shift-JIS text.
 * @param len Length of str.
 * @return rp_string.
 */
rp_string cp1252_sjis_to_rp_string(const char *str, size_t len);

/**
 * Convert UTF-8 text to rp_string.
 * @param str UTF-8 text.
 * @param len Length of str.
 * @return rp_string.
 */
#if defined(RP_UTF8)
static inline rp_string utf8_to_rp_string(const char *str, size_t len)
{
	return rp_string(str, len);
}
#elif defined(RP_UTF16)
rp_string utf8_to_rp_string(const char *str, size_t len);
#endif

/**
 * Convert an rp_string to UTF-8.
 * @param rps rp_string.
 * @return UTF-8 text in an std::string.
 */
#if defined(RP_UTF8)
static inline rp_string rp_string_to_utf8(const rp_string &rps)
{
	return rps;
}
#elif defined(RP_UTF16)
std::string rp_string_to_utf8(const rp_string &rps);
#endif

/**
 * Convert ASCII text to rp_string.
 * NOTE: The text MUST be ASCII, NOT Latin-1 or UTF-8!
 * Those aren't handled here for performance reasons.
 * @param str ASCII text.
 * @param len Length of str.
 * @return rp_string.
 */
#if defined(RP_UTF8)
static inline rp_string ascii_to_rp_string(const char *str, size_t len)
{
	return rp_string(str, len);
}
#elif defined(RP_UTF16)
rp_string ascii_to_rp_string(const char *str, size_t len);
#endif

/**
 * strlen() function for rp_char strings.
 * @param str String.
 * @return String length.
 */
#if defined(RP_UTF8)
static inline size_t rp_strlen(const rp_char *str)
{
	return strlen(str);
}
#elif defined(RP_UTF16)
#ifdef RP_WIS16
static inline size_t rp_strlen(const rp_char *str)
{
	return wcslen(reinterpret_cast<const wchar_t*>(str));
}
#else /* !RP_WIS16 */
size_t rp_strlen(const rp_char *str);
#endif /* RP_WIS16 */
#endif

/**
 * strdup() function for rp_char strings.
 * @param str String.
 * @return Duplicated string.
 */
#if defined(RP_UTF8)
static inline rp_char *rp_strdup(const rp_char *str)
{
	return strdup(str);
}
static inline rp_char *rp_strdup(const rp_string &str)
{
	return strdup(str.c_str());
}
#elif defined(RP_UTF16)
#ifdef RP_WIS16
static inline rp_char *rp_strdup(const rp_char *str)
{
	return reinterpret_cast<rp_char*>(
		wcsdup(reinterpret_cast<const wchar_t*>(str)));
}
static inline rp_char *rp_strdup(const rp_string &str)
{
	return reinterpret_cast<rp_char*>(
		wcsdup(reinterpret_cast<const wchar_t*>(str.c_str())));
}
#else /* !RP_WIS16 */
rp_char *rp_strdup(const rp_char *str);
rp_char *rp_strdup(const rp_string &str);
#endif /* RP_WIS16 */
#endif

}

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_utf8.hpp: UTF-8 text conversion macros.                       *
 * Generally used on non-Windows platforms.                                *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_UTF8_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_UTF8_HPP__

/**
 * NOTE: These are defined outside of the LibRpBase
 * namespace because macros are needed for the UTF-16
 * versions.
 *
 * NOTE 2: The UTF-16 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::string or rp_string if
 * storing it, since a char* or rp_char* will
 * result in a dangling pointer.
 */

// Make sure TextFuncs.hpp was included.
#include "librpbase/TextFuncs.hpp"
#include "librpbase/common.h"

#if defined(RP_UTF16)

/**
 * Get const char* from const rp_char*.
 * @param str const rp_char*
 * @return const char*
 */
#define RP2U8_c(str) \
	(reinterpret_cast<const char*>( \
		LibRpBase::rp_string_to_utf8(str, -1).c_str()))

/**
 * Get const char* from rp_string.
 * @param rps rp_string
 * @return const char*
 */
#define RP2U8_s(rps) \
	(reinterpret_cast<const char*>( \
		LibRpBase::rp_string_to_utf8(rps).c_str()))

/**
 * Get const rp_char* from const char*.
 * @param str const char*
 * @return const rp_char*
 */
#define U82RP_c(str) \
	(LibRpBase::utf8_to_rp_string( \
		reinterpret_cast<const char*>(str), -1).c_str())

/**
 * Get const rp_char* from const char*.
 * @param str const char*
 * @param len Length of str.
 * @return const rp_char*
 */
#define U82RP_cl(str, len) \
	(LibRpBase::utf8_to_rp_string( \
		reinterpret_cast<const char*>(str), len).c_str())

/**
 * Get const rp_char* from std::string.
 * @param str std::string
 * @return const rp_char*
 */
#define U82RP_s(str) \
	(LibRpBase::utf8_to_rp_string( \
		reinterpret_cast<const char*>(str.data()), (int)str.size()).c_str())

/**
 * Get rp_string from const char*.
 * @param str const char*
 * @return rp_string
 */
#define U82RP_cs(str) \
	(LibRpBase::utf8_to_rp_string( \
		reinterpret_cast<const char*>(str), -1))

/**
 * Get rp_string from std::string.
 * @param str std::string
 * @return rp_string
 */
#define U82RP_ss(str) \
	(LibRpBase::utf8_to_rp_string( \
		reinterpret_cast<const char*>(str.data()), (int)str.size()))

#elif defined(RP_UTF8)

/**
 * Get const char* from const rp_char*.
 * @param str const rp_char*
 * @return const char*
 */
static inline const char *RP2U8_c(const rp_char *str)
{
	return reinterpret_cast<const char*>(str);
}

/**
 * Get const char* from rp_string.
 * @param rps rp_string
 * @return const char*
 */
static inline const char *RP2U8_s(const LibRpBase::rp_string &rps)
{
	return reinterpret_cast<const char*>(rps.c_str());
}

/**
 * Get const rp_char* from const char*.
 * @param str const char*
 * @return const rp_char*
 */
static inline const rp_char *U82RP_c(const char *str)
{
	return reinterpret_cast<const rp_char*>(str);
}

/**
 * Get const rp_char* from const char*.
 * @param str const char*
 * @param len Length of str.
 * @return const rp_char*
 */
static inline const rp_char *U82RP_cl(const char *str, int len)
{
	RP_UNUSED(len);
	return reinterpret_cast<const rp_char*>(str);
}

/**
 * Get const rp_char* from std::string.
 * @param str std::string
 * @return const rp_char*
 */
static inline const rp_char *U82RP_s(const std::string &str)
{
	return reinterpret_cast<const rp_char*>(str.c_str());
}

/**
 * Get rp_string from const char*.
 * @param str const char*
 * @return const rp_char*
 */
static inline const LibRpBase::rp_string U82RP_cs(const char *str)
{
	return LibRpBase::rp_string(
		reinterpret_cast<const rp_char*>(str));
}

/**
 * Get const rp_char* from std::string.
 * @param str std::string
 * @return const rp_char*
 */
static inline const LibRpBase::rp_string U82RP_ss(const std::string &str)
{
	return LibRpBase::rp_string(
		reinterpret_cast<const rp_char*>(str.data()), str.size());
}

#endif /* RP_UTF8 */

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_UTF8_HPP__ */

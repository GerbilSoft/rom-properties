/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_wchar.hpp: wchar_t text conversion macros.                    *
 * Generally only used on Windows.                                         *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__

#ifndef RP_WIS16
#error Cannot use TextFuncs_wchar.hpp if sizeof(wchar_t) != 2
#endif /* RP_WIS16 */

/**
 * NOTE: These are defined outside of the LibRpBase
 * namespace because macros are needed for the UTF-8
 * versions.
 *
 * NOTE 2: The UTF-8 versions return c_str() from
 * temporary strings. Therefore, you *must* assign
 * the result to an std::wstring or rp_string if
 * storing it, since a wchar_t* or rp_char* will
 * result in a dangling pointer.
 */

// Make sure TextFuncs.hpp was included.
#include "librpbase/TextFuncs.hpp"
#include "librpbase/common.h"

/**
 * Get const wchar_t* from const rp_char*.
 * @param str const rp_char*
 * @return const wchar_t*
 */
#define RP2W_c(str) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpBase::rp_string_to_utf16(str, -1).c_str()))

/**
 * Get const wchar_t* from rp_string.
 * @param rps rp_string
 * @return const wchar_t*
 */
#define RP2W_s(rps) \
	(reinterpret_cast<const wchar_t*>( \
		LibRpBase::rp_string_to_utf16(rps).c_str()))

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*
 * @return const rp_char*
 */
#define W2RP_c(wcs) \
	(LibRpBase::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), -1).c_str())

/**
 * Get const rp_char* from const wchar_t*.
 * @param wcs const wchar_t*
 * @param len Length of wcs.
 * @return const rp_char*
 */
#define W2RP_cl(wcs, len) \
	(LibRpBase::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), len).c_str())

/**
 * Get const rp_char* from std::wstring.
 * @param wcs std::wstring
 * @return const rp_char*
 */
#define W2RP_s(wcs) \
	(LibRpBase::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs.data()), (int)wcs.size()).c_str())

// FIXME: In-place conversion of std::u16string to std::wstring?

/**
 * Get rp_string from const wchar_t*.
 * @param wcs const wchar_t*
 * @return rp_string
 */
#define W2RP_cs(wcs) \
	(LibRpBase::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs), -1))

/**
 * Get rp_string from std::wstring.
 * @param wcs std::wstring
 * @return rp_string
 */
#define W2RP_ss(wcs) \
	(LibRpBase::utf16_to_rp_string( \
		reinterpret_cast<const char16_t*>(wcs.data()), (int)wcs.size()))

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_WCHAR_HPP__ */

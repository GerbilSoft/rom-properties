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

namespace LibRomData {

/**
 * Convert cp1252 or Shift-JIS text to rp_string.
 * @param str cp1252 or Shift-JIS text.
 * @param n Length of str.
 * @return rp_string.
 */
rp_string cp1252_sjis_to_rp_string(const char *str, size_t n);

/**
 * Convert UTF-8 text to rp_string.
 * @param str UTF-8 text.
 * @param n Length of str.
 * @return rp_string.
 */
#if defined(RP_UTF8)
static inline rp_string utf8_to_rp_string(const char *str, size_t n)
{
	return rp_string(str, n);
}
#elif defined(RP_UTF16)
rp_string utf8_to_rp_string(const char *str, size_t n);
#endif

}

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_HPP__ */

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TextFuncs_int.hpp: Text encoding functions. (internal functions)        *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_INT_HPP__
#define __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_INT_HPP__

namespace LibRomData {

/**
 * Remove trailing NULLs from the source string by adjusting the length.
 * If the resulting string is empty, an empty string is returned.
 * @param return_type Return type, e.g. string or u16string.
 * @param str String.
 * @param len Length. (If less than 0, implicit length string.)
 */
#define REMOVE_TRAILING_NULLS(return_type, str, len) \
	do { \
		if (len >= 0) { \
			/* Remove trailing NULLs. */ \
			for (; len > 0; len--) { \
				if (str[len-1] != 0) \
					break; \
			} \
			\
			if (len <= 0) { \
				/* Empty string. */ \
				return return_type(); \
			} \
		} \
	} while (0)

}

#endif /* __ROMPROPERTIES_LIBROMDATA_TEXTFUNCS_INT_HPP__ */

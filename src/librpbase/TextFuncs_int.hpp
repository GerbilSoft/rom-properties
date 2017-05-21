/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_INT_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_INT_HPP__

namespace LibRpBase {

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

/**
 * Remove trailing NULLs from the source string by adjusting the length.
 * If the resulting string is empty, an empty string is returned.
 * @param return_type Return type, e.g. string or u16string.
 * @param strlen_fn strlen function to use if len < 0.
 * @param str String.
 * @param len Length. (If less than 0, implicit length string.)
 */
#define REMOVE_TRAILING_NULLS_STRLEN(return_type, strlen_fn, str, len) \
	do { \
		if (len < 0) { \
			/* iconv doesn't support NULL-terminated strings directly. */ \
			/* Get the string length. */ \
			len = (int)strlen_fn(str); \
		} else { \
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

/**
 * Remove trailing NULLs from the source string by adjusting the length.
 * If the resulting string is empty, an empty string is returned.
 * This version returns the original string if the length is implicit.
 * (Intended use is the rp_string wrapper functions.)
 * @param return_type Return type, e.g. string or u16string.
 * @param str String.
 * @param len Length. (If less than 0, implicit length string.)
 */
#define REMOVE_TRAILING_NULLS_RP_WRAPPER(return_type, str, len) \
	do { \
		if (len < 0) { \
			return return_type(str); \
		} \
		for (; len > 0; len--) { \
			if (str[len-1] != 0) \
				break; \
		} \
		if (len <= 0) { \
			/* Empty string. */ \
			return return_type(); \
		} \
	} while (0)

/**
 * Remove trailing NULLs from the source string by adjusting the length.
 * If the resulting string is empty, an empty string is returned.
 * This version does nothing if the length is implicit.
 * (Intended use is the UTF-16 byteswapping rp_string wrapper functions.)
 * @param return_type Return type, e.g. string or u16string.
 * @param str String.
 * @param len Length. (If less than 0, implicit length string.)
 */
#define REMOVE_TRAILING_NULLS_RP_WRAPPER_NOIMPLICIT(return_type, str, len) \
	do { \
		if (len >= 0) { \
			for (; len > 0; len--) { \
				if (str[len-1] != 0) \
					break; \
			} \
			if (len <= 0) { \
				/* Empty string. */ \
				return return_type(); \
			} \
		} \
	} while (0)

}

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_INT_HPP__ */

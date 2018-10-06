/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_NULL.hpp: Text encoding functions. (NULL terminator checks)   *
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

#include "config.librpbase.h"
#include "common.h"

// C includes. (C++ namespace)
#include <cstring>

// Reimplementations of libc functions that aren't present on this system.
#include "TextFuncs_libc.h"

namespace LibRpBase {

// Overloaded NULL terminator checks for ICONV_FUNCTION_*.
static FORCEINLINE int check_NULL_terminator(const char *str, int len)
{
	if (len < 0) {
		return static_cast<int>(strlen(str));
	} else {
		return static_cast<int>(strnlen(str, len));
	}
}

static FORCEINLINE int check_NULL_terminator(const char16_t *wcs, int len)
{
	if (len < 0) {
		return static_cast<int>(u16_strlen(wcs));
	} else {
		return static_cast<int>(u16_strnlen(wcs, len));
	}
}

}

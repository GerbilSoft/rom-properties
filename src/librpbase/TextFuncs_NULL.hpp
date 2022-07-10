/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_NULL.hpp: Text encoding functions. (NULL terminator checks)   *
 *                                                                         *
 * Copyright (c) 2009-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_NULL_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_NULL_HPP__

#include "config.librpbase.h"
#include "common.h"

// C includes. (C++ namespace)
#include <cstring>

// for strnlen() if it's not available in <string.h>
#include "librpbase/TextFuncs_libc.h"

namespace LibRpBase {

// Overloaded NULL terminator checks for ICONV_FUNCTION_*.
template<typename T>
static FORCEINLINE int check_NULL_terminator(const T *str, int len)
{
	static_assert(sizeof(T) == sizeof(char), "check_NULL_terminator<T> overload is for 8-bit types only!");
	if (len < 0) {
		return static_cast<int>(strlen(reinterpret_cast<const char*>(str)));
	} else {
		return static_cast<int>(strnlen(reinterpret_cast<const char*>(str), len));
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

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_NULL_HPP__ */

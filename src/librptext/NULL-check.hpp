/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * NULL-check.hpp: NULL terminator checks                                  *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.librptext.h"
#include "common.h"

// C includes. (C++ namespace)
#include <cstring>

// for strnlen() if it's not available in <string.h>
#include "libc.h"

namespace LibRpText {

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

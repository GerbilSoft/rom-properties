/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_printf.hpp: printf()-style functions.                         *
 *                                                                         *
 * Copyright (c) 2009-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_PRINTF_HPP__
#define __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_PRINTF_HPP__

#include "common.h"	// for ATTR_PRINTF()
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes (C++ namespace)
#include <cstdarg>

// C++ includes
#include <string>

namespace LibRpBase {

/**
 * vsprintf()-style function for std::string.
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
ATTR_PRINTF(1, 0)
RP_LIBROMDATA_PUBLIC
std::string rp_vsprintf(const char *fmt, va_list ap);

/**
 * sprintf()-style function for std::string.
 *
 * @param fmt Format string
 * @param ... Arguments
 * @return std::string
 */
ATTR_PRINTF(1, 2)
static inline std::string rp_sprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	std::string s_ret = rp_vsprintf(fmt, ap);
	va_end(ap);
	return s_ret;
}

#if defined(_MSC_VER) || defined(__MINGW32__)
/**
 * sprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * printf() functions. Instead, it has printf_p().
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
ATTR_PRINTF(1, 0)
RP_LIBROMDATA_PUBLIC
std::string rp_vsprintf_p(const char *fmt, va_list ap);

/**
 * sprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * printf() functions. Instead, it has printf_p().
 *
 * @param fmt Format string
 * @param ... Arguments
 * @return std::string
 */
ATTR_PRINTF(1, 2)
static inline std::string rp_sprintf_p(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	std::string s_ret = rp_vsprintf_p(fmt, ap);
	va_end(ap);
	return s_ret;
}

#else /* !_MSC_VER && !__MINGW32__ */

// glibc supports positional format string arguments
// in the standard printf() functions.
// NOTE: gcc can't inline varargs functions, so simply #define them as macros.
#define rp_vsprintf_p(fmt, ap)	rp_vsprintf((fmt), (ap))
#define rp_sprintf_p(fmt, ...)	rp_sprintf((fmt), ##__VA_ARGS__)

#endif /* _MSC_VER && __MINGW32__ */

}

#endif /* __ROMPROPERTIES_LIBRPBASE_TEXTFUNCS_PRINTF_HPP__ */

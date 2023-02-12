/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * printf.hpp: printf()-style functions                                    *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"	// for ATTR_PRINTF()
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes (C++ namespace)
#include <cstdarg>

// C++ includes
#include <string>

namespace LibRpText {

/** UTF-8 (char) **/

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
 * vsprintf()-style function for std::string.
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

#else /* !_MSC_VER || !__MINGW32__ */

// glibc supports positional format string arguments
// in the standard printf() functions.
// NOTE: gcc can't inline varargs functions, so simply #define them as macros.
#define rp_vsprintf_p(fmt, ap)	rp_vsprintf((fmt), (ap))
#define rp_sprintf_p(fmt, ...)	rp_sprintf((fmt), ##__VA_ARGS__)

#endif /* _MSC_VER || __MINGW32__ */

#ifdef _WIN32
/** UTF-16 (wchar_t) **/

// FIXME: ATTR_PRINTF() doesn't work with wchar_t.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64862

/**
 * vswprintf()-style function for std::wstring.
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::wstring
 */
//ATTR_PRINTF(1, 0)
RP_LIBROMDATA_PUBLIC
std::wstring rp_vswprintf(const wchar_t *fmt, va_list ap);

/**
 * swprintf()-style function for std::wstring.
 *
 * @param fmt Format string
 * @param ... Arguments
 * @return std::wstring
 */
//ATTR_PRINTF(1, 2)
static inline std::wstring rp_swprintf(const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	std::wstring ws_ret = rp_vswprintf(fmt, ap);
	va_end(ap);
	return ws_ret;
}

#if defined(_MSC_VER) || defined(__MINGW32__)
/**
 * vswprintf()-style function for std::wstring.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * wprintf() functions. Instead, it has wprintf_p().
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
//ATTR_PRINTF(1, 0)
RP_LIBROMDATA_PUBLIC
std::wstring rp_vswprintf_p(const wchar_t *fmt, va_list ap);

/**
 * swprintf()-style function for std::wstring.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * wprintf() functions. Instead, it has wprintf_p().
 *
 * @param fmt Format string
 * @param ... Arguments
 * @return std::string
 */
//ATTR_PRINTF(1, 2)
static inline std::wstring rp_swprintf_p(const wchar_t *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	std::wstring ws_ret = rp_vswprintf_p(fmt, ap);
	va_end(ap);
	return ws_ret;
}

#else /* !_MSC_VER || !__MINGW32__ */

// glibc supports positional format string arguments
// in the standard wprintf() functions.
// NOTE: gcc can't inline varargs functions, so simply #define them as macros.
#define rp_vswprintf_p(fmt, ap)	rp_vswprintf((fmt), (ap))
#define rp_swprintf_p(fmt, ...)	rp_swprintf((fmt), ##__VA_ARGS__)

#endif /* _MSC_VER || __MINGW32__ */

// TCHAR macros
#ifdef _UNICODE
#  define rp_vstprintf		rp_vswprintf
#  define rp_stprintf		rp_swprintf
#  define rp_vstprintf_p	rp_vswprintf_p
#  define rp_stprintf_p		rp_swprintf_p
#else /* !_UNICODE */
#  define rp_vstprintf		rp_vsprintf
#  define rp_stprintf		rp_sprintf
#  define rp_vstprintf_p	rp_vsprintf_p
#  define rp_stprintf_p		rp_sprintf_p
#endif /* _UNICODE */

#endif /* _WIN32 */

}

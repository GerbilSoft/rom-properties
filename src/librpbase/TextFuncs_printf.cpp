/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextFuncs_printf.cpp: printf()-style functions.                         *
 *                                                                         *
 * Copyright (c) 2009-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TextFuncs_printf.hpp"

// C includes
#include <stdlib.h>

// C++ STL classes
using std::string;

namespace LibRpBase {

/**
 * vsprintf()-style function for std::string.
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
string rp_vsprintf(const char *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];
	va_list ap_tmp;

#if defined(_MSC_VER) && _MSC_VER < 1900
	// MSVC 2013 and older isn't C99 compliant.
	// Use the non-standard _vscprintf() to count characters.
	va_copy(ap_tmp, ap);
	int len = _vscprintf(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		vsnprintf(locbuf, sizeof(locbuf), fmt, ap);
		return string(locbuf, len);
	}
#else
	// C99-compliant vsnprintf().
	va_copy(ap_tmp, ap);
	int len = vsnprintf(locbuf, sizeof(locbuf), fmt, ap_tmp);
	va_end(ap_tmp);
	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		return string(locbuf, len);
	}
#endif

	// Temporarily allocate a buffer large enough for the string,
	// then call vsnprintf() again.
	char *const buf = static_cast<char*>(malloc(len+1));
	int len2 = vsnprintf(buf, len+1, fmt, ap);

	string s_ret;
	assert(len == len2);
	if (len == len2) {
		s_ret.assign(buf, len);
	}
	free(buf);
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
std::string rp_vsprintf_p(const char *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];

	// _vsprintf_p() isn't C99 compliant.
	// Use the non-standard _vscprintf_p() to count characters.
	va_list ap_tmp;
	va_copy(ap_tmp, ap);
	int len = _vscprintf_p(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return string();
	} else if (len < (int)sizeof(locbuf)) {
		// The string fits in the local buffer.
		_vsprintf_p(locbuf, sizeof(locbuf), fmt, ap);
		return string(locbuf, len);
	}

	// Temporarily allocate a buffer large enough for the string,
	// then call vsnprintf() again.
	char *const buf = static_cast<char*>(malloc(len+1));
	int len2 = _vsprintf_p(buf, len+1, fmt, ap);
	assert(len == len2);
	string s_ret = (len == len2 ? string(buf, len) : string());
	free(buf);
	return s_ret;
}
#endif /* _MSC_VER || __MINGW32__ */

}

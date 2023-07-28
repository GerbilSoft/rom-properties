/***************************************************************************
 * ROM Properties Page shell extension. (librptext)                        *
 * printf.cpp: printf()-style functions                                    *
 *                                                                         *
 * Copyright (c) 2009-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "printf.hpp"

// C includes
#include <stdlib.h>

// C includes (C++ namespace)
#include <cassert>
#include <cstring>

// C++ STL classes
using std::string;
#ifdef _WIN32
using std::wstring;
#endif /* _WIN32 */

namespace LibRpText {

/** UTF-8 (char) **/

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

	// Verify that fmt has at least one format specifier.
	// If it doesn't, the rp_sprintf() should be removed.
	// (Debug builds only!)
	assert(strchr(fmt, '%') != nullptr);

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
	char *const buf = static_cast<char*>(malloc((len+1)*sizeof(char)));
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
string rp_vsprintf_p(const char *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	char locbuf[128];

	// Verify that fmt has at least one format specifier.
	// If it doesn't, the rp_sprintf() should be removed.
	// (Debug builds only!)
	assert(strchr(fmt, '%') != nullptr);

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
	char *const buf = static_cast<char*>(malloc((len+1)*sizeof(char)));
	int len2 = _vsprintf_p(buf, len+1, fmt, ap);
	assert(len == len2);
	string s_ret;
	if (len == len2) {
		s_ret.assign(buf, len);
	}
	free(buf);
	return s_ret;
}
#endif /* _MSC_VER || __MINGW32__ */

#ifdef _WIN32
/** UTF-16 (wchar_t) **/

/**
 * vswprintf()-style function for std::wstring.
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::wstring
 */
wstring rp_vswprintf(const wchar_t *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	wchar_t locbuf[128];
	va_list ap_tmp;

#if defined(_MSC_VER) && _MSC_VER < 1900
	// MSVC 2013 and older isn't C99 compliant.
	// Use the non-standard _vscwprintf() to count characters.
	va_copy(ap_tmp, ap);
	int len = _vscwprintf(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return wstring();
	} else if (len < (int)_countof(locbuf)) {
		// The string fits in the local buffer.
		vswprintf(locbuf, _countof(locbuf), fmt, ap);
		return wstring(locbuf, len);
	}
#else
	// C99-compliant vsnprintf().
	va_copy(ap_tmp, ap);
	int len = vswprintf(locbuf, _countof(locbuf), fmt, ap_tmp);
	va_end(ap_tmp);
	if (len <= 0) {
		// Nothing to format...
		return wstring();
	} else if (len < (int)_countof(locbuf)) {
		// The string fits in the local buffer.
		return wstring(locbuf, len);
	}
#endif

	// Temporarily allocate a buffer large enough for the string,
	// then call vswprintf() again.
	wchar_t *const buf = static_cast<wchar_t*>(malloc((len+1)*sizeof(wchar_t)));
	int len2 = vswprintf(buf, len+1, fmt, ap);

	wstring ws_ret;
	assert(len == len2);
	if (len == len2) {
		ws_ret.assign(buf, len);
	}
	free(buf);
	return ws_ret;
}

#if defined(_MSC_VER) || defined(__MINGW32__)
/**
 * vswprintf()-style function for std::string.
 * This version supports positional format string arguments.
 *
 * MSVCRT doesn't support positional arguments in the standard
 * wprintf() functions. Instead, it has wprintf_p().
 *
 * @param fmt Format string
 * @param ap Arguments
 * @return std::string
 */
wstring rp_vswprintf_p(const wchar_t *fmt, va_list ap)
{
	// Local buffer optimization to reduce memory allocation.
	wchar_t locbuf[128];

	// _vswprintf_p() isn't C99 compliant.
	// Use the non-standard _vscwprintf_p() to count characters.
	va_list ap_tmp;
	va_copy(ap_tmp, ap);
	int len = _vscwprintf_p(fmt, ap_tmp);
	va_end(ap_tmp);

	if (len <= 0) {
		// Nothing to format...
		return wstring();
	} else if (len < (int)_countof(locbuf)) {
		// The string fits in the local buffer.
		_vswprintf_p(locbuf, _countof(locbuf), fmt, ap);
		return wstring(locbuf, len);
	}

	// Temporarily allocate a buffer large enough for the string,
	// then call vswprintf() again.
	wchar_t *const buf = static_cast<wchar_t*>(malloc((len+1)*sizeof(wchar_t)));
	int len2 = _vswprintf_p(buf, len+1, fmt, ap);
	assert(len == len2);
	wstring ws_ret;
	if (len == len2) {
		ws_ret.assign(buf, len);
	}
	free(buf);
	return ws_ret;
}
#endif /* _MSC_VER || __MINGW32__ */
#endif /* _WIN32 */

}

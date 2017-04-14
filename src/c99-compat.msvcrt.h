/***************************************************************************
 * c99-compat.msvcrt.h: C99 compatibility header. (MSVC)                   *
 *                                                                         *
 * Copyright (c) 2011-2017 by David Korth.                                 *
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

#ifndef __C99_COMPAT_MSVCRT_H__
#define __C99_COMPAT_MSVCRT_H__

#ifndef _WIN32
#error c99-compat.msvcrt.h should only be included in Win32 builds.
#endif

#if defined(__GNUC__)

// MinGW-w64.
// TODO: Check standard MinGW?
#if !defined(__USE_MINGW_ANSI_STDIO)
// Using MSVCRT's snprintf().
#define ENABLE_C99_SNPRINTF_WRAPPERS 1
#endif

#elif defined(_MSC_VER)

/**
 * MSVC 2015 added proper support for C99 snprintf().
 * Older versions have _snprintf(), which isn't fully compatible.
 * In addition, MSVC 2015 added support for 'inline' in C mode.
 * Previous versions only support 'inline' in C++ mode, but they
 * do support '__inline' in both C and C++.
 */
#if _MSC_VER < 1900
#define ENABLE_C99_SNPRINTF_WRAPPERS 1

#if !defined(__cplusplus)
#define inline __inline
#endif /* !defined(__cplusplus) */
#endif /* _MSC_VER < 1900 */

#endif /* __GNUC__, _MSC_VER */

#ifdef ENABLE_C99_SNPRINTF_WRAPPERS
#include <stdarg.h>
#include <stdio.h>

/**
 * MSVC 2005 added support for variadic macros.
 * https://msdn.microsoft.com/en-US/library/ms177415(v=vs.80).aspx
 * TODO: Verify MSVC 2002 and 2003.
 */
#if defined(_MSC_VER) && _MSC_VER < 1400
/* MSVC 2003 and older. Don't use variadic macros. */
#define snprintf  C99_snprintf
#define vsnprintf C99_vsnprintf
#else
/* MSVC 2005 or later, or gcc. Use variadic macros. */
#define snprintf(str, size, format, ...)  C99_snprintf(str, size, format, __VA_ARGS__)
#define vsnprintf(str, size, format, ...) C99_vsnprintf(str, size, format, __VA_ARGS__)
#endif /* defined(_MSC_VER) && _MSC_VER < 1400 */

static
#if defined(_MSC_VER)
__forceinline
#elif defined(__GNUC__)
__inline
__attribute__ ((__format__ (gnu_printf, 3, 0)))
__attribute__ ((__nonnull__ (3)))
__attribute__ ((always_inline))
__attribute__ ((unused))
#endif
int C99_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int ret = _vsnprintf(str, size, format, ap);
	if (ret >= (int)size) {
		// Make sure the buffer is NULL-terminated.
		str[size-1] = 0;
	} else if (ret < 0) {
		// Make sure the buffer is empty.
		// MSVCRT *should* do this, but just in case...
		str[0] = 0;
	}
	return ret;
}

static
#if defined(_MSC_VER)
__forceinline
#elif defined(__GNUC__)
__inline
__attribute__ ((__format__ (gnu_printf, 3, 0)))
__attribute__ ((__nonnull__ (3)))
/* NOTE: gcc complains that this function cannot be inlined
 * becuase it uses variable argument lists.
__attribute__ ((always_inline))*/
__attribute__ ((unused))
#endif
int C99_snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = C99_vsnprintf(str, size, format, ap);
	va_end(ap);
	return ret;
}
#endif /* ENABLE_C99_SNPRINTF_WRAPPERS */

/** fseeko(), ftello() **/
// On Linux, Large File Support redefines fseeko() and ftello()
// to fseeko64() and ftello64(). fseek() and ftell() are left as-is.
// MSVCRT doesn't have fseeko() or ftello(), so we'll define them
// as _fseeki64() and _ftelli64().
// (NOTE: MinGW-w64 does have fseeko(), ftello(), fseeko64() and
//  ftello64(), and it uses the FILE_OFFSET_BITS macro. LFS appears
//  to be required on both 32-bit and 64-bit Windows, unlike on Linux
//  where it's only required on 32-bit.)
// TODO: Use _fseeki64() and _ftelli64() on MinGW-w64 to avoid
// use of wrapper functions?
#ifdef _MSC_VER
#define fseeko(stream, offset, origin) _fseeki64(stream, offset, origin)
#define ftello(stream) _ftelli64(stream)
#endif /* _MSC_VER */

/** C99 **/

/**
 * C library functions that have different names in MSVCRT
 * compared to POSIX and C99.
 *
 * Note that later versions of MSVC (esp. 2013 and 2015)
 * have added more C99 functionality, since C99 is included
 * in C++ 2011.
 */

/** strtoll(), strtoull() **/

/**
 * MSVC 2013 (12.0) added proper support for strtoll() and strtoull().
 * Older verisons don't have these functions, but they do have
 * the equivalent functions _strtoi64() and _strtoui64().
 */
#if _MSC_VER < 1800
#define strtoll(nptr, endptr, base)  _strtoi64(nptr, endptr, base)
#define strtoull(nptr, endptr, base) _strtoui64(nptr, endptr, base)
#define wcstoll(nptr, endptr, base)  _wcstoi64(nptr, endptr, base)
#define wcstoull(nptr, endptr, base) _wcstoui64(nptr, endptr, base)
#endif /* _MSC_VER < 1800 */

/** strcasecmp() and related **/

/**
 * MSVCRT case-insensitive string comparison functions.
 * MinGW-w64 is missing some of these, so redefine the
 * function names if they haven't been defined already.
 */
#ifndef strcasecmp
#define strcasecmp(s1, s2)     _stricmp(s1, s2)
#endif
#ifndef strncasecmp
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#endif
#ifndef wcscasecmp
#define wcscasecmp(s1, s2)     _wcsicmp(s1, s2)
#endif
#ifndef wcsncasecmp
#define wcsncasecmp(s1, s2, n) _wcsnicmp(s1, s2, n)
#endif

#endif /* __C99_COMPAT_MSVCRT_H__ */

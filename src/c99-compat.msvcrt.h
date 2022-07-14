/***************************************************************************
 * c99-compat.msvcrt.h: C99 compatibility header. (MSVC)                   *
 *                                                                         *
 * Copyright (c) 2011-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __C99_COMPAT_MSVCRT_H__
#define __C99_COMPAT_MSVCRT_H__

#ifndef _WIN32
#  error c99-compat.msvcrt.h should only be included in Win32 builds.
#endif

/**
 * MSVC 2015 added proper support for C99 snprintf().
 * Older versions have _snprintf(), which isn't fully compatible,
 * but works fine in most cases. The only incompatibility is the
 * return value, which is -1 if the string doesn't fit instead of
 * the number of characters (minus NULL terminator) required.
 */

/** snprintf() **/
#ifdef _MSC_VER
#  if _MSC_VER < 1400
/* MSVC 2003 and older. Don't use variadic macros. */
#    define snprintf  _snprintf
#  elif _MSC_VER < 1900
/* MSVC 2005 through MSVC 2013. Use variadic macros. */
#    define snprintf(str, size, format, ...)  _snprintf(str, size, format, __VA_ARGS__)
#  endif
#endif /* _MSC_VER */

/**
 * MSVC 2015 added support for 'inline' in C mode.
 * Previous versions only support 'inline' in C++ mode,
 * but they do support '__inline' in both C and C++.
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#  if !defined(__cplusplus)
#    define inline __inline
#  endif /* !defined(__cplusplus) */
#endif /* defined(_MSC_VER) && _MSC_VER < 1900 */

/** fseeko(), ftello() **/
// On Linux, Large File Support redefines fseeko() and ftello()
// to fseeko64() and ftello64(). fseek() and ftell() are left as-is.
// MSVCRT doesn't have fseeko() or ftello(), so we'll define them
// as _fseeki64() and _ftelli64().
// (NOTE: MinGW-w64 does have fseeko(), ftello(), fseeko64() and
//  ftello64(), and it uses the FILE_OFFSET_BITS macro. LFS appears
//  to be required on both 32-bit and 64-bit Windows, unlike on Linux
//  where it's only required on 32-bit.)
// (NOTE 2: MinGW-w64 redirects to _fseeki64() and _ftelli64() when
//  building with MSVCRT 14.0 [2015] or later.)
#ifdef _MSC_VER
#  define fseeko(stream, offset, origin) _fseeki64(stream, offset, origin)
#  define ftello(stream) _ftelli64(stream)
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
#if defined(_MSC_VER) && _MSC_VER < 1800
#  define strtoll(nptr, endptr, base)  _strtoi64(nptr, endptr, base)
#  define strtoull(nptr, endptr, base) _strtoui64(nptr, endptr, base)
#  define wcstoll(nptr, endptr, base)  _wcstoi64(nptr, endptr, base)
#  define wcstoull(nptr, endptr, base) _wcstoui64(nptr, endptr, base)
#endif /* defined(_MSC_VER) && _MSC_VER < 1800 */

/** strcasecmp() and related **/

/**
 * MSVCRT case-insensitive string comparison functions.
 * MinGW-w64 has the ANSI versions, but not the Unicode versions.
 */
#ifdef _MSC_VER
#  ifndef strcasecmp
#    define strcasecmp(s1, s2)     _stricmp(s1, s2)
#  endif
#  ifndef strncasecmp
#    define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#  endif
#endif /* _MSC_VER */
#ifndef wcscasecmp
#  define wcscasecmp(s1, s2)     _wcsicmp(s1, s2)
#endif
#ifndef wcsncasecmp
#  define wcsncasecmp(s1, s2, n) _wcsnicmp(s1, s2, n)
#endif

/** strtok_r() **/

// MSVC has strtok_s(), which is basically the same as strtok_r().
#ifdef _MSC_VER
#  define strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)
#endif

/** ssize_t **/

#ifdef _MSC_VER
#  ifdef _WIN64
typedef __int64 ssize_t;
#  else /* !_WIN64 */
typedef int ssize_t;
#  endif /* _WIN64 */
#endif /* _MSC_VER */

#endif /* __C99_COMPAT_MSVCRT_H__ */

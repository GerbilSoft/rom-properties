/***************************************************************************
 * c++11-compat.h: C++ 2011 compatibility header.                          *
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

#ifndef __CXX11_COMPAT_H__
#define __CXX11_COMPAT_H__

#ifndef __cplusplus
/**
 * We're compiling C code.
 * Provide replacements for C++ 2011 functionality.
 */
#define CXX11_COMPAT_NULLPTR
#define CXX11_COMPAT_OVERRIDE
#define CXX11_COMPAT_CHARTYPES

// static_assert() is present in C11.
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L
#define CXX11_COMPAT_STATIC_ASSERT
#endif
#endif /* !__cplusplus */

/** Compiler-specific headers. **/

#if defined(__clang__)
#include "c++11-compat.clang.h"
#elif defined(__GNUC__)
#include "c++11-compat.gcc.h"
#elif defined(_MSC_VER)
#include "c++11-compat.msvc.h"
#else
#error Unknown compiler, please update c++11-compat.h.
#endif

/* nullptr: Represents a NULL pointer. NULL == 0 */
#ifdef CXX11_COMPAT_NULLPTR
# if defined __GNUG__ && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define nullptr (__null)
# else
#  if !defined(__cplusplus)
#   define nullptr ((void*)0)
#  else
#   define nullptr (0)
#  endif
# endif
#endif

/* static_assert(): Compile-time assertions. */
#ifdef CXX11_COMPAT_STATIC_ASSERT
#define static_assert(expr, msg) switch (0) { case 0: case (expr): ; }
#endif

/* Unicode characters and strings. */
#ifdef CXX11_COMPAT_CHARTYPES
#include <stdint.h>
typedef uint16_t char16_t;
typedef uint32_t char32_t;

#ifdef __cplusplus
#include <string>
namespace std {
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;
}
#endif /* __cplusplus */
#endif /* CXX11_COMPAT_CHARTYPES */

/* Explicit override/final. */
#ifdef CXX11_COMPAT_OVERRIDE
#define override
/* final may be defined as 'sealed' on older MSVC */
#ifndef final
#define final
#endif
#endif

/** Other compatibility stuff. **/

/**
 * MSVC (and old gcc) doesn't have __func__.
 * Reference: http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# if (defined(__GNUC__) && __GNUC__ >= 2) || defined(_MSC_VER)
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

/**
 * MSVCRT prior to MSVC 2015 has a non-compliant _snprintf().
 * Note that MinGW-w64 uses MSVCRT.
 */
#ifdef _WIN32
#include "c99-compat.msvcrt.h"
#endif

#endif /* __CXX11_COMPAT_H__ */

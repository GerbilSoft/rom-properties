/***************************************************************************
 * compiler-compat.h: Compiler compatibility header.                       *
 *                                                                         *
 * Copyright (c) 2011-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// config.libc.h must be included externally for
// off_t/off64_t checks.
#ifndef __ROMPROPERTIES_CONFIG_LIBC_H__
#  error config.libc.h was not included.
#endif /* __ROMPROPERTIES_CONFIG_LIBC_H__ */

/**
 * MSVC (and old gcc) doesn't have __func__.
 * Reference: http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#  if (defined(__GNUC__) && __GNUC__ >= 2) || defined(_MSC_VER)
#    define __func__ __FUNCTION__
#  else
#    define __func__ "<unknown>"
#  endif
#endif

#if !defined(SIZEOF_OFF64_T)
// off64_t isn't available.
#  if defined(SIZEOF_OFF_T) && SIZEOF_OFF_T >= 8
// off_t is at least 64-bit. Use it as off64_t.
#    include <unistd.h>
typedef off_t off64_t;
#  else /* !(defined(SIZEOF_OFF_T) && SIZEOF_OFF_T >= 8) */
// off_t either doesn't exist or is not at least 64-bit.
#    ifdef _MSC_VER
typedef __int64 off64_t;
#    else /* !_MSC_VER */
#      include <stdint.h>
typedef int64_t off64_t;
#    endif /* _MSC_VER */
#  endif /* defined(SIZEOF_OFF_T) && SIZEOF_OFF_T >= 8 */
#endif /* !defined(SIZEOF_OFF64_t) */

/**
 * MSVC doesn't have typeof(), but as of MSVC 2010,
 * it has decltype(), which is essentially the same thing.
 * FIXME: Doesn't work in C mode.
 * TODO: Handle older versions.
 * Possible option for C++:
 * - http://www.nedproductions.biz/blog/implementing-typeof-in-microsofts-c-compiler
 */
#if defined(_MSC_VER) && defined(__cplusplus)
#  define __typeof__(x) decltype(x)
#endif

/** alignas() **/
// FIXME: Older MSVC (2015, 2017?) doesn't have stdalign.h.
#if defined(_MSC_VER)
#  define ALIGNAS(x) __declspec(align(x))
#elif defined(__GNUC__) /*|| (defined(_MSC_VER) && _MSC_VER >= 1900)*/
#  ifndef __cplusplus
#    include <stdalign.h>
#  endif
#  define ALIGNAS(x) alignas(x)
#else
// TODO: alignas() compatibility macros.
#  define ALIGNAS(x)
#endif

/**
 * MSVCRT prior to MSVC 2015 has a non-compliant _snprintf().
 * Note that MinGW-w64 uses MSVCRT.
 */
#ifdef _WIN32
#  include "c99-compat.msvcrt.h"
#endif

// Some MSVC intrinsics aren't constexpr.
#ifdef _MSC_VER
#  define CONSTEXPR_NO_MSVC
#else /* !_MSC_VER */
#  define CONSTEXPR_NO_MSVC constexpr
#endif

// MSVC prior to MSVC 2022 does not support constexpr on multi-line functions.
#if !defined(__cplusplus) || (defined(_MSC_VER) && _MSC_VER < 1930)
#  define CONSTEXPR_IF_MSVC2022
#else
#  define CONSTEXPR_IF_MSVC2022 constexpr
#endif

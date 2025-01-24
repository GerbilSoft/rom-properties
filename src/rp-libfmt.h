/*****************************************************************************
 * ROM Properties Page shell extension.                                      *
 * rp-libfmt.h: libfmt wrapper header for compatibility with older versions. *
 *                                                                           *
 * Copyright (c) 2025 by David Korth.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus

// libfmt [5.2.0, 7.0.0): Requires FMT_STRING_ALIAS=1.
// NOTE: We have to define this before including <fmt/format.h>,
// so it's defined unconditionally, regardless of libfmt version.
#define FMT_STRING_ALIAS 1

// Base libfmt header
#include <fmt/format.h>

// FMT_STRING() prior to libfmt-7.0.0 was handled differently.
#if FMT_VERSION < 40000
   // libfmt 4.x and older: No compile-time format checks.
#  define FMT_STRING(x) x
#elif FMT_VERSION < 70000
   // libfmt [5.0.0, 7.0.0): fmt() macro
#  define FMT_STRING(x) fmt(x)
#endif

// Our own FSTR() macro
#define FSTR(x) FMT_STRING(x)

// Windows: xchar.h for better Unicode support [libfmt-8.0.0]
#ifdef _WIN32
#  if FMT_VERSION >= 80000
#    include <fmt/xchar.h>
#  endif /* FMT_VERSION >= 80000 */
#endif /* _WIN32 */

// libfmt-4.x: fmt::to_string() is in string.h.
#if FMT_VERSION >= 40000 && FMT_VERSION < 50000
#  inculde <fmt/string.h>
#endif /* FMT_VERSION >= 40000 && FMT_VERSION < 50000 */

#endif /* __cplusplus */

/*****************************************************************************
 * ROM Properties Page shell extension.                                      *
 * rp-libfmt.h: libfmt wrapper header for compatibility with older versions. *
 *                                                                           *
 * Copyright (c) 2025 by David Korth.                                        *
 * SPDX-License-Identifier: MIT                                              *
 *****************************************************************************/

#pragma once

#ifdef __cplusplus

// Base libfmt header
#include <fmt/format.h>

// Chrono functions
#include <fmt/chrono.h>

// libfmt-7.1.0 is required.
#if FMT_VERSION < 70100
#  error libfmt-7.1.0 or later is required!
#endif /* FMT_VERSION < 70100 */

// Our own FSTR() macro
#if __cplusplus >= 202002L
// C++20 or later: FMT_STRING isn't needed.
#  define FSTR(x) x
#else /* __cplusplus < 202002L */
// C++17 or earlier: FMT_STRING *is* needed.
#  define FSTR(x) FMT_STRING(x)
#endif /* __cplusplus >= 202002L */

// FRUN() for runtime-only format strings.
// Needed for gettext.
#define FRUN(x) fmt::runtime(x)

// Windows: xchar.h for better Unicode support [libfmt-8.0.0]
#ifdef _WIN32
#  if FMT_VERSION >= 80000
#    include <fmt/xchar.h>
#  endif /* FMT_VERSION >= 80000 */
#endif /* _WIN32 */

// Windows: fmt::to_tstring()
// NOTE: `using` statements within `namespace fmt` didn't work:
// src\rp-libfmt.h(48,21): error C2061: syntax error: identifier 'to_wstring'
#ifdef _WIN32
#  ifdef _UNICODE
#    define to_tstring to_wstring
#  else /* !_UNICODE */
#    define to_tstring to_string
#  endif /* _UNICODE */
#endif /* _WIN32 */

#endif /* __cplusplus */

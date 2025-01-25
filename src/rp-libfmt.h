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
#define FSTR(x) FMT_STRING(x)

// Windows: xchar.h for better Unicode support [libfmt-8.0.0]
#ifdef _WIN32
#  if FMT_VERSION >= 80000
#    include <fmt/xchar.h>
#  endif /* FMT_VERSION >= 80000 */
#endif /* _WIN32 */

#endif /* __cplusplus */

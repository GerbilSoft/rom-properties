/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * compiler-attrs.h: Compiler attributes.                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// flag_enum: enum values should be handled as bit flags.
// - clang: 5.0
// - gcc: 15.1
#ifndef ATTR_FLAG_ENUM
#  if defined(__clang__) && __clang_major__ >= 5
#    define ATTR_FLAG_ENUM __attribute__((flag_enum))
#  elif defined(__GNUC__) && __GNUC__ >= 15
#    define ATTR_FLAG_ENUM __attribute__((flag_enum))
#  else
#    define ATTR_FLAG_ENUM
#  endif
#endif

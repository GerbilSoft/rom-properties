/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * compiler-attrs.h: Compiler attributes.                                  *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// printf()-style function attribute.
#ifndef ATTR_PRINTF
#  if defined(__clang__) || defined(__llvm__)
#    define ATTR_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#  elif defined(__GNUC__)
#    if !defined(_WIN32) || (defined(_UCRT) || __USE_MINGW_ANSI_STDIO)
#      define ATTR_PRINTF(fmt, args) __attribute__((format(gnu_printf, fmt, args)))
#    else
#      define ATTR_PRINTF(fmt, args) __attribute__((format(ms_printf, fmt, args)))
#    endif
#  else
#    define ATTR_PRINTF(fmt, args)
#  endif
#endif /* ATTR_PRINTF */

// gcc-10 adds an "access" attribute to mark pointers as
// read-only, read-write, write-only, or none, and an
// optional object size.
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 10
#  define ATTR_ACCESS(access_mode, ref_index) __attribute__((access(access_mode, (ref_index))))
#  define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index) __attribute__((access(access_mode, (ref_index), (size_index))))
#else
#  define ATTR_ACCESS(access_mode, ref_index)
#  define ATTR_ACCESS_SIZE(access_mode, ref_index, size_index)
#endif

// gcc-12 enables automatic vectorization using SSE2.
// This results in massive code bloat compared to the manually-optimized
// versions in some cases, e.g. SMD decoding, so we'll explicitly disable
// vectorization for some of the non-optimized functions.
// (clang and MSVC do not have this issue.)
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))
#  define ATTR_GCC_NO_VECTORIZE __attribute__((optimize("no-tree-vectorize")))
#else
#  define ATTR_GCC_NO_VECTORIZE
#endif

// novtable: suppress vtable generation for abstract base classes.
// NOTE: MSVC and Clang (MS ABI) only!
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__clang__))
#  define NOVTABLE __declspec(novtable)
#else
#  define NOVTABLE
#endif

// pure: Function does not have any effects except on the return value,
// and it only depends on the input parameters and/or global variables.
#ifndef ATTR_PURE
#  ifdef __GNUC__
#    define ATTR_PURE __attribute__((pure))
#  else /* !__GNUC__ */
#    define ATTR_PURE
#  endif /* __GNUC__ */
#endif /* ATTR_PURE */

// const: Function does not have any effects except on the return value,
// and it only depends on the input parameters. (similar to constexpr)
#ifndef ATTR_CONST
#  ifdef __GNUC__
#    define ATTR_CONST __attribute__((const))
#  else /* !__GNUC__ */
#    define ATTR_CONST
#  endif /* __GNUC__ */
#endif /* ATTR_CONST */

// `if constexpr` is available starting with C++17.
#ifndef if_constexpr
#  if defined(__cplusplus) && __cplusplus >= 201703L
#    define if_constexpr if constexpr
#  else
#    define if_constexpr if
#  endif
#endif /* if_constexpr */

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

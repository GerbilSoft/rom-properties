/***************************************************************************
 * c++11-compat.clang.h: C++ 2011 compatibility header. (clang)            *
 *                                                                         *
 * Copyright (c) 2011-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __CXX11_COMPAT_CLANG_H__
#define __CXX11_COMPAT_CLANG_H__

#if !defined(__clang__)
#  error c++11-compat.clang.h should only be included in LLVM/clang builds.
#endif

/**
 * LLVM/clang defines __GNUC__ for compatibility, but the reported gcc
 * version number doesn't match clang's featureset. For example,
 * clang-3.7.0 claims to be gcc-4.2.1, which doesn't support any
 * C++ 2011 functionality.
 *
 * A clang version matrix is available at:
 * - http://clang.llvm.org/cxx_status.html#cxx11
 */

/** C++ 2011 **/

#ifdef __cplusplus

/**
 * Enable compatibility for C++ 2011 features that aren't
 * present in older versions of LLVM/clang.
 *
 * These are all automatically enabled when compiling C code.
 *
 * Reference: https://clang.llvm.org/cxx_status.html
 */

/* For clang-3.1+, make sure we're compiling with -std=c++11 or -std=gnu++11. */
/* Older versions didn't set the correct value for __cplusplus in order to    */
/* maintain compatibility with gcc-4.6 and earlier. */
#if (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1))
#  if __cplusplus < 201103L
#    error Please compile with -std=c++11 or -std=gnu++11.
#  endif /* __cplusplus */
#endif /* __clang_major__ */

/* clang-7: char8_t */
#if __clang_major__ < 7
#  define CXX20_COMPAT_CHAR8_T 1
#endif

/* clang-3.1: constexpr */
#if !__has_feature(cxx_constexpr)
#  define CXX11_COMPAT_CONSTEXPR
#endif

/* clang-3.0: nullptr, explicit virtual override */
#if !__has_feature(cxx_nullptr)
#  define CXX11_COMPAT_NULLPTR
#endif
#if !__has_feature(cxx_override_control)
#  define CXX11_COMPAT_OVERRIDE
#endif

/* clang-2.9: New character types, static assertions */
// NOTE: Can't find a feature macro for char16_t/char32_t,
// so using Unicode literals instead.
#if !__has_feature(cxx_unicode_literals)
#  define CXX11_COMPAT_CHARTYPES
#endif

#if !__has_feature(cxx_static_assert)
#  define CXX11_COMPAT_STATIC_ASSERT
#endif

#endif /* __cplusplus */

#endif /* __CXX11_COMPAT_CLANG_H__ */

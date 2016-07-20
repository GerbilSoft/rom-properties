/***************************************************************************
 * c++11-compat.clang.h: C++ 2011 compatibility header. (clang)            *
 *                                                                         *
 * Copyright (c) 2011-2015 by David Korth.                                 *
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

#ifndef __CXX11_COMPAT_GCC_H__
#define __CXX11_COMPAT_GCC_H__

#if !defined(__clang__)
#error c++11-compat.clang.h should only be included in LLVM/clang builds.
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
 */

/* For clang-3.1+, make sure we're compiling with -std=c++11 or -std=gnu++11. */
/* Older versions didn't set the correct value for __cplusplus in order to    */
/* maintain compatibility with gcc-4.6 and earlier. */
#if (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1))
#if __cplusplus < 201103L
#error Please compile with -std=c++11 or -std=gnu++11.
#endif /* __cplusplus */
#endif /* __clang_major__ */

/**
 * clang-3.0 added the following:
 * - nullptr
 * - Explicit virtual override
 */
#if (__clang_major__ < 3)
#define CXX11_COMPAT_NULLPTR
#define CXX11_COMPAT_OVERRIDE
#endif

/**
 * clang-2.9 added the following:
 * - New character types
 * - Static assertions
 */
#if (__clang_major__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 9))
#define CXX11_COMPAT_CHARTYPES
#define CXX11_COMPAT_STATIC_ASSERT
#endif

#endif /* __cplusplus */

#endif /* __CXX11_COMPAT_CLANG_H__ */

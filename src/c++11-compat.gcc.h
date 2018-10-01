/***************************************************************************
 * c++11-compat.gcc.h: C++ 2011 compatibility header. (gcc)                *
 *                                                                         *
 * Copyright (c) 2011-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __CXX11_COMPAT_GCC_H__
#define __CXX11_COMPAT_GCC_H__

#if !defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#error c++11-compat.gcc.h should only be included in gcc builds.
#endif

/** C++ 2011 **/

#ifdef __cplusplus

/**
 * Enable compatibility for C++ 2011 features that aren't
 * present in older versions of gcc.
 *
 * These are all automatically enabled when compiling C code.
 *
 * Reference: https://gcc.gnu.org/projects/cxx-status.html
 */

/* For gcc-4.7+, make sure we're compiling with -std=c++11 or -std=gnu++11. */
/* Older versions didn't set the correct value for __cplusplus. */
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#if __cplusplus < 201103L
#error Please compile with -std=c++11 or -std=gnu++11.
#endif /* __cplusplus */
#endif /* __GNUC__ */

/* gcc-4.7: Explicit virtual override */
#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#define CXX11_COMPAT_OVERRIDE
#endif

/* gcc-4.6: nullptr, constexpr */
#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6))
#define CXX11_COMPAT_NULLPTR
#define CXX11_COMPAT_CONSTEXPR
#endif

/* gcc-4.4: New character types */
#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4))
#define CXX11_COMPAT_CHARTYPES
#endif

/* gcc-4.3: Static assertions (first version to support C++11) */
#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))
#define CXX11_COMPAT_STATIC_ASSERT
#endif

#endif /* __cplusplus */

#endif /* __CXX11_COMPAT_GCC_H__ */

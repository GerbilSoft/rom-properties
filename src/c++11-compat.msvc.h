/***************************************************************************
 * c++11-compat.msvc.h: C++ 2011 compatibility header. (MSVC)              *
 *                                                                         *
 * Copyright (c) 2011-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __CXX11_COMPAT_MSVC_H__
#define __CXX11_COMPAT_MSVC_H__

#ifndef _MSC_VER
#error c++11-compat.msvc.h should only be included in MSVC builds.
#endif

/** C++ 2011 **/

#ifdef __cplusplus

#if (_MSC_VER < 1700)
#  error Minimum supported MSVC version is MSVC 2012 (11.0)
#endif

/**
 * Enable compatibility for C++ 2011 features that aren't
 * present in older versions of MSVC.
 *
 * These are all automatically enabled when compiling C code.
 *
 * Reference: https://docs.microsoft.com/en-us/previous-versions/hh567368(v=vs.140)
 */

#if (_MSC_VER < 1900)
/**
 * MSVC 2015 (14.0) added support for Unicode character types.
 * (char16_t, char32_t, related string types)
 */
#define CXX11_COMPAT_CHARTYPES

/**
 * MSVC 2015 also added support for constexpr.
 */
#define CXX11_COMPAT_CONSTEXPR
#endif

#endif /* __cplusplus */

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

#endif /* __CXX11_COMPAT_MSVC_H__ */

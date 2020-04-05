/***************************************************************************
 * ROM Properties Page shell extension. (librpcpu)                         *
 * cpu_dispatch.h: CPU dispatch macros.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__
#define __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__

#include "librpcpu/config.byteswap.h"
#ifdef HAVE_FEATURES_H
# include <features.h>
#endif

// Check for certain CPUs.
#if defined(__i386__) || defined(__i386) || defined(_M_IX86)
# define RP_CPU_I386 1
#endif
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
# define RP_CPU_AMD64 1
#endif
#if defined(__arm__) || defined(__thumb__) || defined(__arm) || defined(_ARM) || defined(_M_ARM)
# define RP_CPU_ARM 1
#endif
#if defined(__aarch64__)
# define RP_CPU_ARM64 1
#endif

// Check for IFUNC.
// Requires gcc-4.6.0, binutils-2.20.1, and glibc-2.11.1.
// clang-3.9.0 also supports IFUNC.
// On ARM, requires glibc-2.18.
// References:
// - https://gcc.gnu.org/onlinedocs/gcc-4.6.0/gcc/Function-Attributes.html
// - https://willnewton.name/uncategorized/using-gnu-indirect-functions/
// - https://reviews.llvm.org/D15524
#if defined(__clang__)
/* FIXME: Not working on travis-ci since it upgraded from
 * clang-3.5.0 to clang-3.9.0.
# if (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 9))
#  define RP_COMPILER_HAS_IFUNC 1
# endif
*/
#elif defined(__GNUC__)
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#  define RP_COMPILER_HAS_IFUNC 1
# endif
#endif

#ifdef RP_COMPILER_HAS_IFUNC
# if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 11))
#  define RP_HAS_IFUNC 1
# endif
#endif

// Undefine RP_HAS_IFUNC on ARM if glibc isn't new enough. (2.18+ is required.)
#ifdef RP_HAS_IFUNC
# if defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)
#  if !defined(__GLIBC__) || (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 18))
#   undef RP_HAS_IFUNC
#  endif
# endif
#endif

// IFUNC attribute.
#ifdef RP_HAS_IFUNC
# define IFUNC_ATTR(func) __attribute__((ifunc(#func)))
#else
# define IFUNC_ATTR(func)
#endif

#endif /* __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__ */

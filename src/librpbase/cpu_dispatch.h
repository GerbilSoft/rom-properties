/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * cpu_dispatch.h: CPU dispatch macros.                                    *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_CPU_DISPATCH_H__
#define __ROMPROPERTIES_LIBRPBASE_CPU_DISPATCH_H__

#include "config.librpbase.h"
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
// - IFUNC_SSE2_INLINE: inline if CPU always has SSE2.
#ifdef RP_HAS_IFUNC
# define IFUNC_INLINE
# ifdef RP_CPU_AMD64
#  define IFUNC_SSE2_INLINE inline
# else
#  define IFUNC_SSE2_INLINE
# endif
# define IFUNC_ATTR(func) __attribute__((ifunc(#func)))
typedef void (*RP_IFUNC_ptr_t)(void);
#else
# define IFUNC_INLINE inline
# define IFUNC_SSE2_INLINE inline
# define IFUNC_ATTR(func)
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_CPU_DISPATCH_H__ */

/***************************************************************************
 * ROM Properties Page shell extension. (librpcpu)                         *
 * cpu_dispatch.h: CPU dispatch macros.                                    *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__
#define __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__

#include "librpcpu/config.librpcpu.h"

// Check for certain CPUs.
#if defined(__i386__) || defined(__i386) || defined(_M_IX86)
#  define RP_CPU_I386 1
#endif
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#  define RP_CPU_AMD64 1
#endif
#if defined(__arm__) || defined(__thumb__) || defined(__arm) || defined(_ARM) || defined(_M_ARM)
#  define RP_CPU_ARM 1
#endif
#if defined(__aarch64__)
#  define RP_CPU_ARM64 1
#endif

// IFUNC attribute.
// - IFUNC_SSE2_INLINE: inline if CPU always has SSE2.
#ifdef HAVE_IFUNC
#  define IFUNC_INLINE
#  define IFUNC_STATIC_INLINE
#  ifdef RP_CPU_AMD64
#    define IFUNC_SSE2_INLINE inline
#    define IFUNC_SSE2_STATIC_INLINE static inline
#  else
#    define IFUNC_SSE2_INLINE
#    define IFUNC_SSE2_STATIC_INLINE
#  endif
#  define IFUNC_ATTR(func) __attribute__((ifunc(#func)))
#else
#  define IFUNC_INLINE inline
#  define IFUNC_STATIC_INLINE static inline
#  define IFUNC_SSE2_INLINE inline
#  define IFUNC_SSE2_STATIC_INLINE static inline
#  define IFUNC_ATTR(func)
#endif

#endif /* __ROMPROPERTIES_LIBRPCPU_CPU_DISPATCH_H__ */

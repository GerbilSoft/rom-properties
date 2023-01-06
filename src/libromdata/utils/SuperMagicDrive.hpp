/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

#include <stdint.h>

#include "librpcpu/cpu_dispatch.h"
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "librpcpu/cpuflags_x86.h"
/* MSVC does not support MMX intrinsics in 64-bit builds. */
/* Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160 */
/* In addition, amd64 CPUs all support SSE2 as a minimum, */
/* so there's no point in building MMX code for 64-bit. */
#  if (defined(_M_IX86) || defined(__i386__)) && \
      !(defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__))
#    define SMD_HAS_MMX 1
#  endif
#  define SMD_HAS_SSE2 1
#endif
#ifdef RP_CPU_AMD64
#  define SMD_ALWAYS_HAS_SSE2 1
#endif

namespace LibRomData { namespace SuperMagicDrive {

// SMD block size.
static const unsigned int SMD_BLOCK_SIZE = 16384;

/** Internal algorithms. **/
// NOTE: These are public to allow for unit tests and benchmarking.

/**
 * Decode a Super Magic Drive interleaved block.
 * Standard version using regular C++ code.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void RP_LIBROMDATA_PUBLIC decodeBlock_cpp(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
	ATTR_GCC_NO_VECTORIZE;
// NOTE: Disabling auto-vectorization on gcc because it massively
// bloats the code with no performance benefit. The MMX and SSE2
// implementations using intrinsics are significantly faster.
// (clang and MSVC do not have this issue.)

#if SMD_HAS_MMX
/**
 * Decode a Super Magic Drive interleaved block.
 * MMX-optimized version.
 * NOTE: Pointers must be 16-byte aligned.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void RP_LIBROMDATA_PUBLIC decodeBlock_mmx(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif /* SMD_HAS_MMX */

#if SMD_HAS_SSE2
/**
 * Decode a Super Magic Drive interleaved block.
 * SSE2-optimized version.
 * NOTE: Pointers must be 16-byte aligned.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void RP_LIBROMDATA_PUBLIC decodeBlock_sse2(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif /* SMD_HAS_SSE2 */

/** Dispatch functions. **/

#if defined(HAVE_IFUNC) && (defined(RP_CPU_I386) || defined(RP_CPU_AMD64))
#  if defined(SMD_ALWAYS_HAS_SSE2)

// System does support IFUNC, but it's always guaranteed to have SSE2.
// Eliminate the IFUNC dispatch on this system.

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
static inline void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// amd64 always has SSE2.
	decodeBlock_sse2(pDest, pSrc);
}

#  else /* !defined(SMD_ALWAYS_HAS_SSE2) */
// System does support IFUNC, and we have to use a dispatch function.
void RP_LIBROMDATA_PUBLIC decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#  endif /* defined(SMD_ALWAYS_HAS_SSE2) */
#else /* !(HAVE_IFUNC && (RP_CPU_I386 || RP_CPU_AMD64)) */

// System does not have IFUNC, or is not i386/amd64.

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
static inline void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
#ifdef SMD_ALWAYS_HAS_SSE2
	// amd64 always has SSE2.
	decodeBlock_sse2(pDest, pSrc);
#else /* SMD_ALWAYS_HAS_SSE2 */
# ifdef SMD_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		decodeBlock_sse2(pDest, pSrc);
	} else
# endif /* SMD_HAS_SSE2 */
# ifdef SMD_HAS_MMX
	if (RP_CPU_HasMMX()) {
		decodeBlock_mmx(pDest, pSrc);
	} else
#endif /* SMD_HAS_MMX */
	{
		decodeBlock_cpp(pDest, pSrc);
	}
#endif /* SMD_ALWAYS_HAS_SSE2 */
}

#endif /* HAVE_IFUNC && (RP_CPU_I386 || RP_CPU_AMD64) */

} }

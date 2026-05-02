/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.libromdata.h"

#include "common.h"
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes (C++ namespace)
#include <cstdint>

#include "librpcpuid/cpu_dispatch.h"
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "librpcpuid/cpuflags_x86.h"
/* MSVC does not support MMX intrinsics in 64-bit builds. */
/* Reference: https://docs.microsoft.com/en-us/cpp/cpp/m64?view=msvc-160 */
/* In addition, amd64 CPUs all support SSE2 as a minimum, */
/* so there's no point in building MMX code for 64-bit. */
#  if (defined(_M_IX86) || defined(__i386__)) && \
      !(defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__))
#    define SMD_HAS_MMX 1
#  endif
#  define SMD_HAS_SSE2 1
#elif defined(HAVE_ARM_NEON_H)
#  if defined(RP_CPU_ARM) || defined(RP_CPU_ARM64)
#    include "librpcpuid/cpuflags_arm.h"
#    define SMD_HAS_NEON 1
#  endif
#endif
#ifdef RP_CPU_AMD64
#  define SMD_ALWAYS_HAS_SSE2 1
#endif
#ifdef HAVE_ARM_NEON_H
#  ifdef RP_CPU_ARM64
#    define SMD_ALWAYS_HAS_NEON 1
#  endif
#endif

namespace LibRomData { namespace SuperMagicDrive {

// SMD block size
static constexpr unsigned int SMD_BLOCK_SIZE = 16384;

/** Internal algorithms **/
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

#if SMD_HAS_NEON
/**
 * Decode a Super Magic Drive interleaved block.
 * NEON-optimized version.
 * NOTE: Pointers must be 16-byte aligned.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void RP_LIBROMDATA_PUBLIC decodeBlock_neon(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif /* SMD_HAS_NEON */

/** Dispatch functions. **/

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
static inline void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
#if defined(SMD_ALWAYS_HAS_NEON)
	return decodeBlock_neon(pDest, pSrc);
#elif defined(SMD_ALWAYS_HAS_SSE2)
	// amd64 always has SSE2.
	decodeBlock_sse2(pDest, pSrc);
#else
#  ifdef SMD_HAS_SSE2
	if (RP_CPU_x86_HasSSE2()) {
		decodeBlock_sse2(pDest, pSrc);
	} else
#  endif /* SMD_HAS_SSE2 */
#  ifdef SMD_HAS_MMX
	if (RP_CPU_x86_HasMMX()) {
		decodeBlock_mmx(pDest, pSrc);
	} else
#  endif /* SMD_HAS_MMX */
#  ifdef SMD_HAS_NEON
	if (RP_CPU_arm_HasNEON()) {
		return decodeBlock_neon(pDest, pSrc);
	} else
#  endif /* SMD_HAS_NEON */
	{
		decodeBlock_cpp(pDest, pSrc);
	}
#endif
}

} }

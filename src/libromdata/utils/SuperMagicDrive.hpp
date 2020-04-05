/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__

#include "common.h"
#include "librpcpu/cpu_dispatch.h"

#include <stdint.h>

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpcpu/cpuflags_x86.h"
/* MSVC does not support MMX intrinsics in 64-bit builds. */
/* Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx */
/* In addition, amd64 CPUs all support SSE2 as a minimum, */
/* so there's no point in building MMX code for 64-bit. */
# if (defined(_M_IX86) || defined(__i386__)) && !defined(_M_X64) && !defined(__amd64__)
#  define SMD_HAS_MMX 1
# endif
# define SMD_HAS_SSE2 1
#endif
#ifdef RP_CPU_AMD64
# define SMD_ALWAYS_HAS_SSE2 1
#elif defined(RP_CPU_I386)
# ifdef RP_HAS_IFUNC
#  define SMD_HAS_IFUNC 1
# endif
#endif

namespace LibRomData {

class SuperMagicDrive
{
	private:
		// Static class.
		SuperMagicDrive();
		~SuperMagicDrive();
		RP_DISABLE_COPY(SuperMagicDrive)

	public:
		/** Internal algorithms. **/
		// NOTE: These are public to allow for unit tests and benchmarking.

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * Standard version using regular C++ code.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_cpp(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);

#if SMD_HAS_MMX
		/**
		 * Decode a Super Magic Drive interleaved block.
		 * MMX-optimized version.
		 * NOTE: Pointers must be 16-byte aligned.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_mmx(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif /* SMD_HAS_MMX */

#if SMD_HAS_SSE2
		/**
		 * Decode a Super Magic Drive interleaved block.
		 * SSE2-optimized version.
		 * NOTE: Pointers must be 16-byte aligned.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock_sse2(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
#endif /* SMD_HAS_SSE2 */

	public:
		// SMD block size.
		static const unsigned int SMD_BLOCK_SIZE = 16384;

		// TODO: Use gcc target-specific function attributes if available?
		// (IFUNC dispatcher, etc.)

#ifdef SMD_HAS_IFUNC
		/* System has IFUNC. Use it for dispatching. */

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * NOTE: Pointers must be 16-byte aligned if using SSE2.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);

#else
		// System does not support IFUNC, or we don't have optimizations for these CPUs.
		// Use standard inline dispatch.

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

#endif
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__ */

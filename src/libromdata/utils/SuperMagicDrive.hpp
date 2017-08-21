/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__

#include "librpbase/common.h"
#include "cpu_dispatch.h"

#include <stdint.h>

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpbase/cpuflags_x86.h"
/* MSVC does not support MMX intrinsics in 64-bit builds. */
/* Reference: https://msdn.microsoft.com/en-us/library/08x3t697(v=vs.110).aspx */
/* TODO: Disable MMX on all 64-bit builds? */
# if !defined(_MSC_VER) || (defined(_M_IX86) && !defined(_M_X64))
#  define SMD_HAS_MMX 1
# endif
# define SMD_HAS_SSE2 1
#endif
#ifdef RP_CPU_AMD64
# define SMD_ALWAYS_HAS_SSE2 1
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

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * NOTE: Pointers must be 16-byte aligned if using SSE2.
		 * @param pDest	[out] Destination block. (Must be 16 KB.)
		 * @param pSrc	[in] Source block. (Must be 16 KB.)
		 */
		static IFUNC_SSE2_INLINE void decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc);
};

// TODO: Use gcc target-specific function attributes if available?
// (IFUNC dispatcher, etc.)

/** Dispatch functions. **/

#if defined(RP_HAS_IFUNC) && defined(SMD_ALWAYS_HAS_SSE2)

// System does support IFUNC, but it's always guaranteed to have SSE2.
// Eliminate the IFUNC dispatch on this system.

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
inline void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// amd64 always has SSE2.
	decodeBlock_sse2(pDest, pSrc);
}

#endif /* defined(RP_HAS_IFUNC) && defined(SMD_ALWAYS_HAS_SSE2) */

#if !defined(RP_HAS_IFUNC) || (!defined(RP_CPU_I386) && !defined(RP_CPU_AMD64))

/**
 * Decode a Super Magic Drive interleaved block.
 * NOTE: Pointers must be 16-byte aligned if using SSE2.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
inline void SuperMagicDrive::decodeBlock(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
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

#endif /* !defined(RP_HAS_IFUNC) || (!defined(RP_CPU_I386) && !defined(RP_CPU_AMD64)) */

}

#endif /* __ROMPROPERTIES_LIBROMDATA_UTILS_SUPERMAGICDRIVE_HPP__ */

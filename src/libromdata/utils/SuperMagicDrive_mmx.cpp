/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive_sse2.cpp: Super Magic Drive deinterleaving function.    *
 * MMX-optimized version.                                                  *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SuperMagicDrive.hpp"

// MMX intrinsics.
#include <mmintrin.h>

namespace LibRomData {

/**
 * Decode a Super Magic Drive interleaved block.
 * Standard version using regular C++ code.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void SuperMagicDrive::decodeBlock_mmx(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// First 8 KB of the source block is ODD bytes.
	// Second 8 KB of the source block is EVEN bytes.
	const __m64 *pSrc_odd = reinterpret_cast<const __m64*>(pSrc);
	const __m64 *pSrc_even = reinterpret_cast<const __m64*>(pSrc + (SMD_BLOCK_SIZE / 2));
	const __m64 *const pDest_end = reinterpret_cast<const __m64*>(pDest + SMD_BLOCK_SIZE);

	// Process 32 bytes (256 bits) at a time.
	for (__m64 *p = reinterpret_cast<__m64*>(pDest);
	     p < pDest_end; p += 4, pSrc_odd += 2, pSrc_even += 2)
	{
		// Unpack odd/even bytes into the destination.
		p[0] = _mm_unpacklo_pi8(pSrc_even[0], pSrc_odd[0]);
		p[1] = _mm_unpackhi_pi8(pSrc_even[0], pSrc_odd[0]);
		p[2] = _mm_unpacklo_pi8(pSrc_even[1], pSrc_odd[1]);
		p[3] = _mm_unpackhi_pi8(pSrc_even[1], pSrc_odd[1]);
	}

	// `emms` instruction.
	_mm_empty();
}

}

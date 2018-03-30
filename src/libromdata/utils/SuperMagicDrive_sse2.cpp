/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive_sse2.cpp: Super Magic Drive deinterleaving function.    *
 * SSE2-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "SuperMagicDrive.hpp"
#include "librpbase/common.h"

// C includes. (C++ namespace)
#include <cassert>

// SSE2 intrinsics.
#include <emmintrin.h>

namespace LibRomData {

/**
 * Decode a Super Magic Drive interleaved block.
 * Standard version using regular C++ code.
 * NOTE: Pointers must be 16-byte aligned.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void SuperMagicDrive::decodeBlock_sse2(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// FIXME: MSVC 2017 generates `movdqu` instead of `movdqa`.
	// https://developercommunity.visualstudio.com/content/problem/48123/perf-regression-movdqu-instructions-are-generated.html
	ASSERT_ALIGNMENT(16, pDest);
	ASSERT_ALIGNMENT(16, pSrc);

	// First 8 KB of the source block is ODD bytes.
	// Second 8 KB of the source block is EVEN bytes.
	const __m128i *pSrc_odd = reinterpret_cast<const __m128i*>(pSrc);
	const __m128i *pSrc_even = reinterpret_cast<const __m128i*>(pSrc + (SMD_BLOCK_SIZE / 2));
	const __m128i *const pDest_end = reinterpret_cast<const __m128i*>(pDest + SMD_BLOCK_SIZE);

	// Process 64 bytes (512 bits) at a time.
	for (__m128i *p = reinterpret_cast<__m128i*>(pDest);
	     p < pDest_end; p += 4, pSrc_odd += 2, pSrc_even += 2)
	{
		// Unpack odd/even bytes into the destination.
		p[0] = _mm_unpacklo_epi8(pSrc_even[0], pSrc_odd[0]);
		p[1] = _mm_unpackhi_epi8(pSrc_even[0], pSrc_odd[0]);
		p[2] = _mm_unpacklo_epi8(pSrc_even[1], pSrc_odd[1]);
		p[3] = _mm_unpackhi_epi8(pSrc_even[1], pSrc_odd[1]);
	}
}

}

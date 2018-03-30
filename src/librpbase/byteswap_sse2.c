/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap_sse2.c: Byteswapping functions.                                *
 * SSE2-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2008-2018 by David Korth                                  *
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

#include "byteswap.h"

// C includes.
#include <assert.h>

// SSE2 intrinsics.
#include <emmintrin.h>

/**
 * 16-bit byteswap function.
 * SSE2-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array_sse2(uint16_t *ptr, unsigned int n)
{
	// Verify the block is 16-bit aligned
	// and is a multiple of 2 bytes.
	assert(((uintptr_t)ptr & 1) == 0);
	assert((n & 1) == 0);
	n &= ~1;

	// TODO: Don't bother with SSE2 if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap WORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}

	// Process 16 WORDs per iteration using SSE2.
	for (; n >= 32; n -= 32, ptr += 16) {
		__m128i *xmm_ptr = (__m128i*)ptr;

		__m128i xmm0 = _mm_load_si128(&xmm_ptr[0]);
		__m128i xmm1 = _mm_load_si128(&xmm_ptr[1]);

		// Original WORD: AA BB

		// Shift WORDs by 8 bits:
		// - xmm0 == BB 00
		// - xmm2 == 00 AA
		// - OR'd == BB AA
		__m128i xmm2 = _mm_srli_epi16(xmm0, 8);
		__m128i xmm3 = _mm_srli_epi16(xmm1, 8);
		xmm0 = _mm_or_si128(_mm_slli_epi16(xmm0, 8), xmm2);
		xmm1 = _mm_or_si128(_mm_slli_epi16(xmm1, 8), xmm3);

		_mm_store_si128(&xmm_ptr[0], xmm0);
		_mm_store_si128(&xmm_ptr[1], xmm1);
	}

	// Process the remaining data, one WORD at a time.
	for (; n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}
}

/**
 * 32-bit byteswap function.
 * SSE2-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void __byte_swap_32_array_sse2(uint32_t *ptr, unsigned int n)
{
	// Verify the block is 32-bit aligned
	// and is a multiple of 4 bytes.
	assert(((uintptr_t)ptr & 3) == 0);
	assert((n & 3) == 0);
	n &= ~3;

	// TODO: Don't bother with SSE2 if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap DWORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}

	// Process 8 DWORDs per iteration using SSE2.
	for (; n >= 32; n -= 32, ptr += 8) {
		__m128i *xmm_ptr = (__m128i*)ptr;

		__m128i xmm0 = _mm_load_si128(&xmm_ptr[0]);
		__m128i xmm1 = _mm_load_si128(&xmm_ptr[1]);
		__m128i xmm2, xmm3;

		// Original DWORD: AA BB CC DD

		// Wordswap the DWORDs.
		// - xmm0 == CC DD AA BB
		xmm0 = _mm_shufflelo_epi16(xmm0, 0xB1);
		xmm1 = _mm_shufflelo_epi16(xmm1, 0xB1);
		xmm0 = _mm_shufflehi_epi16(xmm0, 0xB1);
		xmm1 = _mm_shufflehi_epi16(xmm1, 0xB1);

		// Shift WORDs by 8 bits:
		// - xmm0 == DD 00 BB 00
		// - xmm2 == 00 CC 00 AA
		// - OR'd == DD CC BB AA
		xmm2 = _mm_srli_epi16(xmm0, 8);
		xmm3 = _mm_srli_epi16(xmm1, 8);
		xmm0 = _mm_or_si128(_mm_slli_epi16(xmm0, 8), xmm2);
		xmm1 = _mm_or_si128(_mm_slli_epi16(xmm1, 8), xmm3);
		
		_mm_store_si128(&xmm_ptr[0], xmm0);
		_mm_store_si128(&xmm_ptr[1], xmm1);
	}

	// Process the remaining data, one DWORD at a time.
	for (; n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}
}

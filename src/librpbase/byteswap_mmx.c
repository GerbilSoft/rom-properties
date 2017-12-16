/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap_sse2.c: Byteswapping functions.                                *
 * SSE2-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2008-2017 by David Korth                                  *
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

#include "byteswap.h"

// C includes.
#include <assert.h>

// MMX intrinsics.
#include <mmintrin.h>

/**
 * 16-bit byteswap function.
 * MMX-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array_mmx(uint16_t *ptr, unsigned int n)
{
	// Verify the block is 16-bit aligned
	// and is a multiple of 2 bytes.
	assert(((uintptr_t)ptr & 1) == 0);
	assert((n & 1) == 0);
	n &= ~1;

	// TODO: Don't bother with MMX if n is below a certain size?

	// NOTE: 16-byte alignment is not required for MMX.

	// Process 16 WORDs per iteration using MMX.
	for (; n >= 32; n -= 32, ptr += 16) {
		__m64 *mm_ptr = (__m64*)ptr;

		// TODO: Benchmark 16 WORDs vs. 8 WORDs.
		__m64 mm0 = mm_ptr[0];
		__m64 mm1 = mm_ptr[1];
		__m64 mm2 = mm_ptr[2];
		__m64 mm3 = mm_ptr[3];
		__m64 mm4 = mm0;
		__m64 mm5 = mm1;
		__m64 mm6 = mm2;
		__m64 mm7 = mm3;

		mm0 = _mm_slli_pi16(mm0, 8);
		mm1 = _mm_slli_pi16(mm1, 8);
		mm2 = _mm_slli_pi16(mm2, 8);
		mm3 = _mm_slli_pi16(mm3, 8);
		mm4 = _mm_srli_pi16(mm4, 8);
		mm5 = _mm_srli_pi16(mm5, 8);
		mm6 = _mm_srli_pi16(mm6, 8);
		mm7 = _mm_srli_pi16(mm7, 8);

		mm_ptr[0] = _mm_or_si64(mm0, mm4);
		mm_ptr[1] = _mm_or_si64(mm1, mm5);
		mm_ptr[2] = _mm_or_si64(mm2, mm6);
		mm_ptr[3] = _mm_or_si64(mm3, mm7);
	}

	// `emms` instruction.
	_mm_empty();

	// Process the remaining data, one WORD at a time.
	for (; n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}
}

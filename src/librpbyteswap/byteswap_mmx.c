/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * byteswap_mmx.c: Byteswapping functions.                                 *
 * MMX-optimized version.                                                  *
 *                                                                         *
 * Copyright (c) 2008-2024 by David Korth                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "byteswap_rp.h"

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
void RP_C_API rp_byte_swap_16_array_mmx(uint16_t *ptr, size_t n)
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

		__m64 mm0 = mm_ptr[0];
		__m64 mm1 = mm_ptr[1];
		__m64 mm2 = mm_ptr[2];
		__m64 mm3 = mm_ptr[3];

		// Original WORD: AA BB

		// Shift WORDs by 8 bits:
		// -  mm0 == BB 00
		// -  mm2 == 00 AA
		// - OR'd == BB AA
		__m64 mm4 = _mm_srli_pi16(mm0, 8);
		__m64 mm5 = _mm_srli_pi16(mm1, 8);
		__m64 mm6 = _mm_srli_pi16(mm2, 8);
		__m64 mm7 = _mm_srli_pi16(mm3, 8);
		mm0 = _mm_or_si64(_mm_slli_pi16(mm0, 8), mm4);
		mm1 = _mm_or_si64(_mm_slli_pi16(mm1, 8), mm5);
		mm2 = _mm_or_si64(_mm_slli_pi16(mm2, 8), mm6);
		mm3 = _mm_or_si64(_mm_slli_pi16(mm3, 8), mm7);

		mm_ptr[0] = mm0;
		mm_ptr[1] = mm1;
		mm_ptr[2] = mm2;
		mm_ptr[3] = mm3;
	}

	// `emms` instruction.
	_mm_empty();

	// Process the remaining data, one WORD at a time.
	for (; n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}
}

/**
 * 32-bit byteswap function.
 * MMX-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void RP_C_API rp_byte_swap_32_array_mmx(uint32_t *ptr, size_t n)
{
	// Verify the block is 32-bit aligned
	// and is a multiple of 4 bytes.
	assert(((uintptr_t)ptr & 3) == 0);
	assert((n & 3) == 0);
	n &= ~3;

	// TODO: Don't bother with MMX if n is below a certain size?

	// NOTE: 16-byte alignment is not required for MMX.

	// Process 8 DWORDs per iteration using MMX.
	for (; n >= 32; n -= 32, ptr += 8) {
		__m64 *mm_ptr = (__m64*)ptr;

		__m64 mm0 = mm_ptr[0];
		__m64 mm1 = mm_ptr[1];
		__m64 mm2 = mm_ptr[2];
		__m64 mm3 = mm_ptr[3];

		// Original DWORD: AA BB CC DD

		// Shift DWORDs by 16 bits:
		// -  mm0 == CC DD 00 00
		// -  mm2 == 00 00 AA BB
		// - OR'd == CC DD AA BB
		__m64 mm4 = _mm_srli_pi32(mm0, 16);
		__m64 mm5 = _mm_srli_pi32(mm1, 16);
		__m64 mm6 = _mm_srli_pi32(mm2, 16);
		__m64 mm7 = _mm_srli_pi32(mm3, 16);
		mm0 = _mm_or_si64(_mm_slli_pi32(mm0, 16), mm4);
		mm1 = _mm_or_si64(_mm_slli_pi32(mm1, 16), mm5);
		mm2 = _mm_or_si64(_mm_slli_pi32(mm2, 16), mm6);
		mm3 = _mm_or_si64(_mm_slli_pi32(mm3, 16), mm7);

		// Shift WORDs by 8 bits:
		// -  mm0 == DD 00 BB 00
		// -  mm2 == 00 CC 00 AA
		// - OR'd == DD CC BB AA
		mm4 = _mm_srli_pi16(mm0, 8);
		mm5 = _mm_srli_pi16(mm1, 8);
		mm6 = _mm_srli_pi16(mm2, 8);
		mm7 = _mm_srli_pi16(mm3, 8);
		mm0 = _mm_or_si64(_mm_slli_pi16(mm0, 8), mm4);
		mm1 = _mm_or_si64(_mm_slli_pi16(mm1, 8), mm5);
		mm2 = _mm_or_si64(_mm_slli_pi16(mm2, 8), mm6);
		mm3 = _mm_or_si64(_mm_slli_pi16(mm3, 8), mm7);

		mm_ptr[0] = mm0;
		mm_ptr[1] = mm1;
		mm_ptr[2] = mm2;
		mm_ptr[3] = mm3;
	}

	// `emms` instruction.
	_mm_empty();

	// Process the remaining data, one DWORD at a time.
	for (; n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}
}

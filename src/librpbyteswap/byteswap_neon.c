/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * byteswap_arm64.c: Byteswapping functions.                               *
 * NEON-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2008-2025 by David Korth                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "byteswap_rp.h"

// C includes
#include <assert.h>

// ARM NEON intrinsics
#include <arm_neon.h>

/**
 * 16-bit byteswap function.
 * NEON-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void RP_C_API rp_byte_swap_16_array_neon(uint16_t *ptr, size_t n)
{
	// Verify the block is 16-bit aligned
	// and is a multiple of 2 bytes.
	assert(((uintptr_t)ptr & 1) == 0);
	assert((n & 1) == 0);
	n &= ~1;

	// TODO: Don't bother with ARM NEON intrinsics if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap WORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}

	// Process 16 WORDs per iteration using ARM NEON intrinsics.
	for (; n >= 32; n -= 32, ptr += 16) {
		uint8x16_t vec0 = vld1q_u16(ptr);
		uint8x16_t vec1 = vld1q_u16(ptr+8);

		vec0 = vrev16q_u8(vec0);
		vec1 = vrev16q_u8(vec1);

		vst1q_u16(ptr, vec0);
		vst1q_u16(ptr+8, vec1);
	}

	// Process the remaining data, one WORD at a time.
	for (; n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}
}

/**
 * 32-bit byteswap function.
 * NEON-optimized version.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void RP_C_API rp_byte_swap_32_array_neon(uint32_t *ptr, size_t n)
{
	// Verify the block is 32-bit aligned
	// and is a multiple of 4 bytes.
	assert(((uintptr_t)ptr & 3) == 0);
	assert((n & 3) == 0);
	n &= ~3;

	// TODO: Don't bother with ARM NEON intrinsics if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap DWORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}

	// Process 8 DWORDs per iteration using ARM NEON intrinsics.
	for (; n >= 32; n -= 32, ptr += 8) {
		uint8x16_t vec0 = vld1q_u32(ptr);
		uint8x16_t vec1 = vld1q_u32(ptr+4);

		vec0 = vrev32q_u8(vec0);
		vec1 = vrev32q_u8(vec1);

		vst1q_u32(ptr, vec0);
		vst1q_u32(ptr+4, vec1);
	}

	// Process the remaining data, one DWORD at a time.
	for (; n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}
}

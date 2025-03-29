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

// FIXME: MSVC for 32-bit ARM is missing 128-bit vtbl intrinsics.
// I'm not sure if it's an MSVC issue or if 32-bit ARM NEON doesn't have it...
// TODO: Test the 32-bit ARM NEON code.
// TODO: uint8x8x4_t for 32-bit, and uint8x16x2_t for 64-bit?

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

#ifdef RP_CPU_ARM
	static const uint8_t shuf_mask_u8[8] = {1,0, 3,2, 5,4, 7,6};
	uint8x8_t shuf_mask = vld1_u8(shuf_mask_u8);
#else /* RP_CPU_ARM64 */
	static const uint8_t shuf_mask_u8[16] = {1,0, 3,2, 5,4, 7,6, 9,8, 11,10, 13,12, 15,14};
	uint8x16_t shuf_mask = vld1q_u8(shuf_mask_u8);
#endif

	// TODO: Don't bother with ARM64 vector intrinsics if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap WORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}

	// Process 16 WORDs per iteration using ARM64 vector intrinsics.
	for (; n >= 32; n -= 32, ptr += 16) {
#ifdef RP_CPU_ARM
		uint8x8_t vec0 = vld1_u16(ptr);
		uint8x8_t vec1 = vld1_u16(ptr+4);
		uint8x8_t vec2 = vld1_u16(ptr+8);
		uint8x8_t vec3 = vld1_u16(ptr+12);

		vec0 = vtbl1_u8(vec0, shuf_mask);
		vec1 = vtbl1_u8(vec1, shuf_mask);
		vec2 = vtbl1_u8(vec2, shuf_mask);
		vec3 = vtbl1_u8(vec3, shuf_mask);

		vst1_u16(ptr, vec0);
		vst1_u16(ptr+4, vec1);
		vst1_u16(ptr+8, vec2);
		vst1_u16(ptr+12, vec3);
#else /* RP_CPU_ARM64 */
		uint8x16_t vec0 = vld1q_u16(ptr);
		uint8x16_t vec1 = vld1q_u16(ptr+8);

		vec0 = vqtbl1q_u8(vec0, shuf_mask);
		vec1 = vqtbl1q_u8(vec1, shuf_mask);

		vst1q_u16(ptr, vec0);
		vst1q_u16(ptr+8, vec1);
#endif
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

#ifdef RP_CPU_ARM
	static const uint8_t shuf_mask_u8[8] = {3,2,1,0, 7,6,5,4};
	uint8x8_t shuf_mask = vld1_u8(shuf_mask_u8);
#else /* RP_CPU_ARM64 */
	static const uint8_t shuf_mask_u8[16] = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};
	uint8x16_t shuf_mask = vld1q_u8(shuf_mask_u8);
#endif

	// TODO: Don't bother with ARM64 vector intrinsics if n is below a certain size?

	// If vptr isn't 16-byte aligned, swap DWORDs
	// manually until we get to 16-byte alignment.
	for (; ((uintptr_t)ptr % 16 != 0) && n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}

	// Process 8 DWORDs per iteration using ARM64 vector intrinsics.
	for (; n >= 32; n -= 32, ptr += 8) {
#ifdef RP_CPU_ARM
		uint8x8_t vec0 = vld1_u32(ptr);
		uint8x8_t vec1 = vld1_u32(ptr+2);
		uint8x8_t vec2 = vld1_u32(ptr+4);
		uint8x8_t vec3 = vld1_u32(ptr+6);

		vec0 = vtbl1_u8(vec0, shuf_mask);
		vec1 = vtbl1_u8(vec1, shuf_mask);
		vec2 = vtbl1_u8(vec2, shuf_mask);
		vec3 = vtbl1_u8(vec3, shuf_mask);

		vst1_u32(ptr, vec0);
		vst1_u32(ptr+2, vec1);
		vst1_u32(ptr+4, vec2);
		vst1_u32(ptr+6, vec3);
#else /* RP_CPU_ARM64 */
		uint8x16_t vec0 = vld1q_u32(ptr);
		uint8x16_t vec1 = vld1q_u32(ptr+4);

		vec0 = vqtbl1q_u8(vec0, shuf_mask);
		vec1 = vqtbl1q_u8(vec1, shuf_mask);

		vst1q_u32(ptr, vec0);
		vst1q_u32(ptr+4, vec1);
#endif
	}

	// Process the remaining data, one DWORD at a time.
	for (; n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}
}

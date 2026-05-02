/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive_neon.cpp: Super Magic Drive deinterleaving function.    *
 * NEON-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SuperMagicDrive.hpp"

// ARM NEON intrinsics
#include <arm_neon.h>

namespace LibRomData { namespace SuperMagicDrive {

/**
 * Decode a Super Magic Drive interleaved block.
 * NEON version using ARM NEON intrinsics.
 * NOTE: Pointers must be 16-byte aligned.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void decodeBlock_neon(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	ASSERT_ALIGNMENT(16, pDest);
	ASSERT_ALIGNMENT(16, pSrc);

	// First 8 KB of the source block is ODD bytes.
	// Second 8 KB of the source block is EVEN bytes.
	const uint8_t *pSrc_odd = pSrc;
	const uint8_t *pSrc_even = pSrc + (SMD_BLOCK_SIZE / 2);
	const uint8_t *const pDest_end = pDest + SMD_BLOCK_SIZE;

	// Process 64 bytes (512 bits) at a time.
	for (; pDest < pDest_end; pDest += 64, pSrc_odd += 32, pSrc_even += 32) {
		uint8x16x2_t src_odd;
		uint8x16x2_t src_even;
		uint8x16x4_t dest;

		src_odd.val[0] = vld1q_u8(&pSrc_odd[ 0]);
		src_odd.val[1] = vld1q_u8(&pSrc_odd[16]);
		src_even.val[0] = vld1q_u8(&pSrc_even[0]);
		src_even.val[1] = vld1q_u8(&pSrc_even[16]);

#if defined(RP_CPU_ARM64)
		// Unpack odd/even bytes into the destination.
		// vzip1q_u8 => _mm_unpacklo_epi8
		// vzip2q_u8 => _mm_unpackhi_epi8
		dest.val[0] = vzip1q_u8(src_even.val[0], src_odd.val[0]);
		dest.val[1] = vzip2q_u8(src_even.val[0], src_odd.val[0]);
		dest.val[2] = vzip1q_u8(src_even.val[1], src_odd.val[1]);
		dest.val[3] = vzip2q_u8(src_even.val[1], src_odd.val[1]);
#elif defined(RP_CPU_ARM)
		// Unpack odd/even bytes into the destination.
		uint8x8_t a1, b1;
		uint8x8x2_t result;

		a1 = vreinterpret_u8_u16(vget_low_u16(vreinterpretq_u16_u8(src_even.val[0])));
		b1 = vreinterpret_u8_u16(vget_low_u16(vreinterpretq_u16_u8(src_odd.val[0])));
		result = vzip_u8(a1, b1);
		dest.val[0] = vcombine_u8(result.val[0], result.val[1]);

		a1 = vreinterpret_u8_u16(vget_high_u16(vreinterpretq_u16_u8(src_even.val[0])));
		b1 = vreinterpret_u8_u16(vget_high_u16(vreinterpretq_u16_u8(src_odd.val[0])));
		result = vzip_u8(a1, b1);
		dest.val[1] = vcombine_u8(result.val[0], result.val[1]);

		a1 = vreinterpret_u8_u16(vget_low_u16(vreinterpretq_u16_u8(src_even.val[1])));
		b1 = vreinterpret_u8_u16(vget_low_u16(vreinterpretq_u16_u8(src_odd.val[1])));
		result = vzip_u8(a1, b1);
		dest.val[2] = vcombine_u8(result.val[0], result.val[1]);

		a1 = vreinterpret_u8_u16(vget_high_u16(vreinterpretq_u16_u8(src_even.val[1])));
		b1 = vreinterpret_u8_u16(vget_high_u16(vreinterpretq_u16_u8(src_odd.val[1])));
		result = vzip_u8(a1, b1);
		dest.val[3] = vcombine_u8(result.val[0], result.val[1]);
#endif

		vst1q_u8(&pDest[ 0], dest.val[0]);
		vst1q_u8(&pDest[16], dest.val[1]);
		vst1q_u8(&pDest[32], dest.val[2]);
		vst1q_u8(&pDest[48], dest.val[3]);
	}
}

} }

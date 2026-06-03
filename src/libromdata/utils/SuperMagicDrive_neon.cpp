/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive_neon.cpp: Super Magic Drive deinterleaving function.    *
 * NEON-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "SuperMagicDrive.hpp"

// ARM NEON intrinsics
#include "arm_neon_aligned.h"

namespace LibRomData { namespace SuperMagicDrive {

/**
 * Decode a Super Magic Drive interleaved block.
 * NEON-optimized version.
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
		uint8x16x2_t sa, sb;
		sa.val[0] = vld1q_u8_ex(&pSrc_even[ 0], 128);
		sa.val[1] = vld1q_u8_ex(&pSrc_odd[  0], 128);
		sb.val[0] = vld1q_u8_ex(&pSrc_even[16], 128);
		sb.val[1] = vld1q_u8_ex(&pSrc_odd[ 16], 128);

		// vst2q automatically interleaves the first and second
		// vectors from even/odd in the destination.
		vst2q_u8_ex(&pDest[ 0], sa, 128);
		vst2q_u8_ex(&pDest[32], sb, 128);
	}
}

} }

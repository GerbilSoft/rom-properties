/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
 * Standard version. (C++ code only)                                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SuperMagicDrive.hpp"

namespace LibRomData { namespace SuperMagicDrive {

/**
 * Decode a Super Magic Drive interleaved block.
 * Standard version using regular C++ code.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void decodeBlock_cpp(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
{
	// First 8 KB of the source block is ODD bytes.
	const uint8_t *pSrc_end = pSrc + (SMD_BLOCK_SIZE / 2);
	for (uint8_t *pDest_odd = pDest + 1; pSrc < pSrc_end; pDest_odd += 16, pSrc += 8) {
		pDest_odd[ 0] = pSrc[0];
		pDest_odd[ 2] = pSrc[1];
		pDest_odd[ 4] = pSrc[2];
		pDest_odd[ 6] = pSrc[3];
		pDest_odd[ 8] = pSrc[4];
		pDest_odd[10] = pSrc[5];
		pDest_odd[12] = pSrc[6];
		pDest_odd[14] = pSrc[7];
	}

	// Second 8 KB of the source block is EVEN bytes.
	pSrc_end = pSrc + (SMD_BLOCK_SIZE / 2);
	for (uint8_t *pDest_even = pDest; pSrc < pSrc_end; pDest_even += 16, pSrc += 8) {
		pDest_even[ 0] = pSrc[ 0];
		pDest_even[ 2] = pSrc[ 1];
		pDest_even[ 4] = pSrc[ 2];
		pDest_even[ 6] = pSrc[ 3];
		pDest_even[ 8] = pSrc[ 4];
		pDest_even[10] = pSrc[ 5];
		pDest_even[12] = pSrc[ 6];
		pDest_even[14] = pSrc[ 7];
	}
}

} }

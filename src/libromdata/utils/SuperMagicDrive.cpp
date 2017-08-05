/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SuperMagicDrive.cpp: Super Magic Drive deinterleaving function.         *
 * Standard version. (C++ code only)                                       *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "SuperMagicDrive.hpp"

namespace LibRomData {

/**
 * Decode a Super Magic Drive interleaved block.
 * Standard version using regular C++ code.
 * @param pDest	[out] Destination block. (Must be 16 KB.)
 * @param pSrc	[in] Source block. (Must be 16 KB.)
 */
void SuperMagicDrive::decodeBlock_cpp(uint8_t *RESTRICT pDest, const uint8_t *RESTRICT pSrc)
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

}

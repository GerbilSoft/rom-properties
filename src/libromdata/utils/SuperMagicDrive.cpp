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
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
void SuperMagicDrive::decodeBlock_cpp(uint8_t *RESTRICT dest, const uint8_t *RESTRICT src)
{
	// First 8 KB of the source block is ODD bytes.
	const uint8_t *end_block = src + 8192;
	for (uint8_t *odd = dest + 1; src < end_block; odd += 16, src += 8) {
		odd[ 0] = src[0];
		odd[ 2] = src[1];
		odd[ 4] = src[2];
		odd[ 6] = src[3];
		odd[ 8] = src[4];
		odd[10] = src[5];
		odd[12] = src[6];
		odd[14] = src[7];
	}

	// Second 8 KB of the source block is EVEN bytes.
	end_block = src + 8192;
	for (uint8_t *even = dest; src < end_block; even += 16, src += 8) {
		even[ 0] = src[ 0];
		even[ 2] = src[ 1];
		even[ 4] = src[ 2];
		even[ 6] = src[ 3];
		even[ 8] = src[ 4];
		even[10] = src[ 5];
		even[12] = src[ 6];
		even[14] = src[ 7];
	}
}

}

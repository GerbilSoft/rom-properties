/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * byteswap.c: Byteswapping functions.                                     *
 * Standard version. (C code only)                                         *
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

/**
 * Byteswap two 16-bit WORDs in a 32-bit DWORD.
 * @param dword DWORD containing two 16-bit WORDs.
 * @return DWORD containing the byteswapped 16-bit WORDs.
 */
static inline uint32_t swap_two_16_in_32(uint32_t dword)
{
	uint32_t tmp1 = (dword >> 8) & 0x00FF00FF;
	uint32_t tmp2 = (dword << 8) & 0xFF00FF00;
	return (tmp1 | tmp2);
}

/**
 * 16-bit byteswap function.
 * Standard version using regular C code.
 * @param ptr Pointer to array to swap. (MUST be 16-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array_c(uint16_t *ptr, unsigned int n)
{
	uint32_t *dwptr;

	// Verify the block is 16-bit aligned
	// and is a multiple of 2 bytes.
	assert(((uintptr_t)ptr & 1) == 0);
	assert((n & 1) == 0);
	n &= ~1;

	// Check if ptr is 32-bit aligned.
	if (((uintptr_t)ptr & 3) != 0) {
		// Byteswap the first WORD to fix alignment.
		*ptr = __swab16(*ptr);
		ptr++;
	}

	// Process 8 WORDs per iteration,
	// using 32-bit accesses.
	dwptr = (uint32_t*)ptr;
	for (; n >= 16; n -= 16, dwptr += 4) {
		*(dwptr+0) = swap_two_16_in_32(*(dwptr+0));
		*(dwptr+1) = swap_two_16_in_32(*(dwptr+1));
		*(dwptr+2) = swap_two_16_in_32(*(dwptr+2));
		*(dwptr+3) = swap_two_16_in_32(*(dwptr+3));
	}
	ptr = (uint16_t*)dwptr;

	// Process remaining WORDs.
	for (; n > 0; n -= 2, ptr++) {
		*ptr = __swab16(*ptr);
	}
}

/**
 * 32-bit byteswap function.
 * Standard version using regular C code.
 * @param ptr Pointer to array to swap. (MUST be 32-bit aligned!)
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void __byte_swap_32_array_c(uint32_t *ptr, unsigned int n)
{
	// Verify the block is 32-bit aligned
	// and is a multiple of 4 bytes.
	assert(((uintptr_t)ptr & 3) == 0);
	assert((n & 3) == 0);
	n &= ~3;

	// Process 4 DWORDs per iteration.
	for (; n >= 16; n -= 16, ptr += 4) {
		*(ptr+0) = __swab32(*(ptr+0));
		*(ptr+1) = __swab32(*(ptr+1));
		*(ptr+2) = __swab32(*(ptr+2));
		*(ptr+3) = __swab32(*(ptr+3));
	}

	// Process remaining DWORDs.
	for (; n > 0; n -= 4, ptr++) {
		*ptr = __swab32(*ptr);
	}
}

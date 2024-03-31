/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * bitstuff.c: Bit manipulation functions.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "bitstuff.h"

/**
 * Population count function. (C version)
 * @param x Value
 * @return Population count
 */
ATTR_CONST
unsigned int popcount_c(unsigned int x)
{
	// References:
	// - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36041
	// - https://gcc.gnu.org/bugzilla/attachment.cgi?id=15529
	// - https://gcc.gnu.org/viewcvs/gcc?view=revision&revision=200506
	x = (x & 0x55555555U) + ((x >> 1) & 0x55555555U);
	x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
	x = (x & 0x0F0F0F0FU) + ((x >> 4) & 0x0F0F0F0FU);
	return (x * 0x01010101U) >> 24;
}

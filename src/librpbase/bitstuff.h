/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * bitstuff.h: Bit manipulation inline functions.                          *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPBASE_BITSTUFF_H__
#define __ROMPROPERTIES_LIBRPBASE_BITSTUFF_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(inline)
# include <intrin.h>
# define inline __inline
#endif

/**
 * Unsigned integer log2(n).
 * @param n Value
 * @return uilog2(n)
 */
static inline unsigned int uilog2(unsigned int n)
{
#if defined(__GNUC__)
	return (n == 0 ? 0 : __builtin_ctz(n));
#elif defined(_MSC_VER)
	if (n == 0)
		return 0;
	unsigned long index;
	unsigned char x = _BitScanReverse(&index, n);
	return (x ? index : 0);
#else
	unsigned int ret = 0;
	while (n >>= 1)
		ret++;
	return ret;
#endif
}

/**
 * Population count function.
 * @param x Value.
 * @return Population count.
 */
static inline unsigned int popcount(unsigned int x)
{
#if defined(__GNUC__)
	return __builtin_popcount(x);
#else
	// References:
	// - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=36041
	// - https://gcc.gnu.org/bugzilla/attachment.cgi?id=15529
	// - https://gcc.gnu.org/viewcvs/gcc?view=revision&revision=200506
	x = (x & 0x55555555U) + ((x >> 1) & 0x55555555U);
	x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
	x = (x & 0x0F0F0F0FU) + ((x >> 4) & 0x0F0F0F0FU);
	return (x * 0x01010101U) >> 24;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBRPBASE_BITSTUFF_H__ */

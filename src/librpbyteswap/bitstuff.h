/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * bitstuff.h: Bit manipulation inline functions.                          *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef _MSC_VER
#  include <intrin.h>
#  ifndef inline
#    define inline __inline
#  endif /* !inline */
#endif /* _MSC_VER */

#include "stdboolx.h"

// constexpr is not valid in C.
#ifndef __cplusplus
#  ifndef constexpr
#    define constexpr
#  endif /* !constexpr */
#endif /* !__cplusplus */

/**
 * Unsigned integer log2(n).
 * @param n Value
 * @return uilog2(n)
 */
static inline unsigned int uilog2(unsigned int n)
{
#if defined(__GNUC__)
	// NOTE: XOR is needed to return the bit index
	// instead of the number of leading zeroes.
	return (n == 0 ? 0 : 31^__builtin_clz(n));
#elif defined(_MSC_VER)
	// FIXME: _BitScanReverse() is not constexpr on MSVC 2022.
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
static inline constexpr unsigned int popcount(unsigned int x)
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

/**
 * Check if a value is a power of 2. (also must be non-zero)
 * @tparam t Type
 * @param x Value
 * @return True if this is value is a power of 2 and is non-zero; false if not.
 */
#ifdef __cplusplus
template<typename T>
static inline constexpr bool isPow2(T x)
#else
static inline bool isPow2(unsigned int x)
#endif
{
	// References:
	// - https://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2
	// - https://stackoverflow.com/a/600492
	return (x != 0 && ((x & (x - 1)) == 0));
}

/**
 * Get the next power of 2.
 * @param x Value
 * @return Next power of 2.
 */
static inline unsigned int nextPow2(unsigned int x)
{
	// FIXME: _BitScanReverse() [in uilog2()] is not constexpr on MSVC 2022.
	return (1U << (uilog2(x) + 1));
}

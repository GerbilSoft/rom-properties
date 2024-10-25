/***************************************************************************
 * ROM Properties Page shell extension. (librpbyteswap)                    *
 * bitstuff.h: Bit manipulation functions.                                 *
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

// const: Function does not have any effects except on the return value,
// and it only depends on the input parameters. (similar to constexpr)
#ifndef ATTR_CONST
#  ifdef __GNUC__
#    define ATTR_CONST __attribute__((const))
#  else /* !__GNUC__ */
#    define ATTR_CONST
#  endif /* __GNUC__ */
#endif /* ATTR_CONST */

/**
 * Unsigned integer log2(n).
 * @param n Value
 * @return uilog2(n)
 */
ATTR_CONST
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
 * Population count function. (C version)
 * @param x Value
 * @return Population count
 */
#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */
ATTR_CONST
unsigned int popcount_c(unsigned int x);

/**
 * Population count function.
 * Inline version that uses a compiler built-in if available.
 * @param x Value
 * @return Population count
 */
ATTR_CONST
static inline unsigned int popcount(unsigned int x)
{
#ifdef __GNUC__
	return __builtin_popcount(x);
#else
	return popcount_c(x);
#endif
}

/**
 * Check if a value is a power of 2. (also must be non-zero)
 * @tparam T Type
 * @param x Value
 * @return True if this is value is a power of 2 and is non-zero; false if not.
 */
#ifdef __cplusplus
template<typename T>
ATTR_CONST
static inline constexpr bool isPow2(T x)
#else
ATTR_CONST
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
 * @tparam T Type
 * @param x Value
 * @return Next power of 2.
 */
#ifdef __cplusplus
template<typename T>
ATTR_CONST
#  ifndef _MSC_VER
static inline constexpr T nextPow2(T x)
#  else /* _MSC_VER */
static inline T nextPow2(T x)
#  endif /* !_MSC_VER */
#else /* !__cplusplus */
static inline unsigned int nextPow2(unsigned int x)
#endif /* __cplusplus */
{
	// FIXME: _BitScanReverse() [in uilog2()] is not constexpr on MSVC 2022.
#ifdef __cplusplus
	return static_cast<T>(1U << (uilog2(static_cast<unsigned int>(x)) + 1));
#else /* !__cplusplus */
	return (1U << (uilog2(x) + 1));
#endif /* __cplusplus */
}

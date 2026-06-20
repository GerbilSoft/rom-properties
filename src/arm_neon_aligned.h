/***************************************************************************
 * ROM Properties Page shell extension.                                    *
 * arm_neon_aligned.h: ARM NEON _ex() intrinsics for aligned data hints.   *
 *                                                                         *
 * Copyright (c) 2026 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#if !defined(_M_ARM) && !defined(__arm__) && \
    !defined(_M_ARMT) && !defined(__thumb__) && \
    !defined(_M_ARM64) && !defined(__aarch64__) && \
    !defined(_M_ARM64EC)
#  error arm_neon_aligned.h is for ARM systems only!
#endif

// MSVC provides _ex() versions of ARM NEON intrinsics to specify alignment.
// On other compilers, we can emulate it using __builtin_assume_aligned().
// NOTE: The _ex() functions take bits, not bytes.
#include <arm_neon.h>

#ifndef _MSC_VER

// for HINT_ALIGNED()
#include "alignment_macros.h"

/** 64-bit vectors **/

#define vld1_u8_ex(p, align) vld1_u8(HINT_ALIGNED((p), (align)/8))
#define vld2_u8_ex(p, align) vld2_u8(HINT_ALIGNED((p), (align)/8))
#define vld3_u8_ex(p, align) vld3_u8(HINT_ALIGNED((p), (align)/8))
#define vld4_u8_ex(p, align) vld4_u8(HINT_ALIGNED((p), (align)/8))

#define vst1_u8_ex(p, a, align) vst1_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst2_u8_ex(p, a, align) vst2_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u8_ex(p, a, align) vst3_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u8_ex(p, a, align) vst4_u8(HINT_ALIGNED((p), (align)/8), (a))

#define vld1_u16_ex(p, align) vld1_u16(HINT_ALIGNED((p), (align)/8))
#define vld2_u16_ex(p, align) vld2_u16(HINT_ALIGNED((p), (align)/8))
#define vld3_u16_ex(p, align) vld3_u16(HINT_ALIGNED((p), (align)/8))
#define vld4_u16_ex(p, align) vld4_u16(HINT_ALIGNED((p), (align)/8))

#define vst1_u16_ex(p, a, align) vst1_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst2_u16_ex(p, a, align) vst2_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u16_ex(p, a, align) vst3_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u16_ex(p, a, align) vst4_u16(HINT_ALIGNED((p), (align)/8), (a))

#define vld1_u32_ex(p, align) vld1_u32(HINT_ALIGNED((p), (align)/8))
#define vld2_u32_ex(p, align) vld2_u32(HINT_ALIGNED((p), (align)/8))
#define vld3_u32_ex(p, align) vld3_u32(HINT_ALIGNED((p), (align)/8))
#define vld4_u32_ex(p, align) vld4_u32(HINT_ALIGNED((p), (align)/8))

#define vst1_u32_ex(p, a, align) vst1_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst2_u32_ex(p, a, align) vst2_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u32_ex(p, a, align) vst3_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u32_ex(p, a, align) vst4_u32(HINT_ALIGNED((p), (align)/8), (a))

#define vld1_u8_x2_ex(p, align) vld1_u8_x2(HINT_ALIGNED((p), (align)/8))
#define vld1_u8_x3_ex(p, align) vld1_u8_x3(HINT_ALIGNED((p), (align)/8))
#define vld1_u8_x4_ex(p, align) vld1_u8_x4(HINT_ALIGNED((p), (align)/8))

#define vst2_u8_x2_ex(p, a, align) vst2_u8_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u8_x3_ex(p, a, align) vst3_u8_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u8_x4_ex(p, a, align) vst4_u8_x4(HINT_ALIGNED((p), (align)/8), (a))

#define vld1_u16_x2_ex(p, align) vld1_u16_x2(HINT_ALIGNED((p), (align)/8))
#define vld1_u16_x3_ex(p, align) vld1_u16_x3(HINT_ALIGNED((p), (align)/8))
#define vld1_u16_x4_ex(p, align) vld1_u16_x4(HINT_ALIGNED((p), (align)/8))

#define vst2_u16_x2_ex(p, a, align) vst2_u16_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u16_x3_ex(p, a, align) vst3_u16_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u16_x4_ex(p, a, align) vst4_u16_x4(HINT_ALIGNED((p), (align)/8), (a))

#define vld1_u32_x2_ex(p, align) vld1_u32_x2(HINT_ALIGNED((p), (align)/8))
#define vld1_u32_x3_ex(p, align) vld1_u32_x3(HINT_ALIGNED((p), (align)/8))
#define vld1_u32_x4_ex(p, align) vld1_u32_x4(HINT_ALIGNED((p), (align)/8))

#define vst2_u32_x2_ex(p, a, align) vst2_u32_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst3_u32_x3_ex(p, a, align) vst3_u32_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst4_u32_x4_ex(p, a, align) vst4_u32_x4(HINT_ALIGNED((p), (align)/8), (a))

/** 128-bit vectors **/

#define vld1q_u8_ex(p, align) vld1q_u8(HINT_ALIGNED((p), (align)/8))
#define vld2q_u8_ex(p, align) vld2q_u8(HINT_ALIGNED((p), (align)/8))
#define vld3q_u8_ex(p, align) vld3q_u8(HINT_ALIGNED((p), (align)/8))
#define vld4q_u8_ex(p, align) vld4q_u8(HINT_ALIGNED((p), (align)/8))

#define vst1q_u8_ex(p, a, align) vst1q_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst2q_u8_ex(p, a, align) vst2q_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst3q_u8_ex(p, a, align) vst3q_u8(HINT_ALIGNED((p), (align)/8), (a))
#define vst4q_u8_ex(p, a, align) vst4q_u8(HINT_ALIGNED((p), (align)/8), (a))

#define vld1q_u16_ex(p, align) vld1q_u16(HINT_ALIGNED((p), (align)/8))
#define vld2q_u16_ex(p, align) vld2q_u16(HINT_ALIGNED((p), (align)/8))
#define vld3q_u16_ex(p, align) vld3q_u16(HINT_ALIGNED((p), (align)/8))
#define vld4q_u16_ex(p, align) vld4q_u16(HINT_ALIGNED((p), (align)/8))

#define vst1q_u16_ex(p, a, align) vst1q_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst2q_u16_ex(p, a, align) vst2q_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst3q_u16_ex(p, a, align) vst3q_u16(HINT_ALIGNED((p), (align)/8), (a))
#define vst4q_u16_ex(p, a, align) vst4q_u16(HINT_ALIGNED((p), (align)/8), (a))

#define vld1q_u32_ex(p, align) vld1q_u32(HINT_ALIGNED((p), (align)/8))
#define vld2q_u32_ex(p, align) vld2q_u32(HINT_ALIGNED((p), (align)/8))
#define vld3q_u32_ex(p, align) vld3q_u32(HINT_ALIGNED((p), (align)/8))
#define vld4q_u32_ex(p, align) vld4q_u32(HINT_ALIGNED((p), (align)/8))

#define vst1q_u32_ex(p, a, align) vst1q_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst2q_u32_ex(p, a, align) vst2q_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst3q_u32_ex(p, a, align) vst3q_u32(HINT_ALIGNED((p), (align)/8), (a))
#define vst4q_u32_ex(p, a, align) vst4q_u32(HINT_ALIGNED((p), (align)/8), (a))

#define vld1q_u8_x2_ex(p, align) vld1q_u8_x2(HINT_ALIGNED((p), (align)/8))
#define vld1q_u8_x3_ex(p, align) vld1q_u8_x3(HINT_ALIGNED((p), (align)/8))
#define vld1q_u8_x4_ex(p, align) vld1q_u8_x4(HINT_ALIGNED((p), (align)/8))

#define vst1q_u8_x2_ex(p, a, align) vst1q_u8_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u8_x3_ex(p, a, align) vst1q_u8_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u8_x4_ex(p, a, align) vst1q_u8_x4(HINT_ALIGNED((p), (align)/8), (a))

#define vld1q_u16_x2_ex(p, align) vld1q_u16_x2(HINT_ALIGNED((p), (align)/8))
#define vld1q_u16_x3_ex(p, align) vld1q_u16_x3(HINT_ALIGNED((p), (align)/8))
#define vld1q_u16_x4_ex(p, align) vld1q_u16_x4(HINT_ALIGNED((p), (align)/8))

#define vst1q_u16_x2_ex(p, a, align) vst1q_u16_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u16_x3_ex(p, a, align) vst1q_u16_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u16_x4_ex(p, a, align) vst1q_u16_x4(HINT_ALIGNED((p), (align)/8), (a))

#define vld1q_u32_x2_ex(p, align) vld1q_u32_x2(HINT_ALIGNED((p), (align)/8))
#define vld1q_u32_x3_ex(p, align) vld1q_u32_x3(HINT_ALIGNED((p), (align)/8))
#define vld1q_u32_x4_ex(p, align) vld1q_u32_x4(HINT_ALIGNED((p), (align)/8))

#define vst1q_u32_x2_ex(p, a, align) vst1q_u32_x2(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u32_x3_ex(p, a, align) vst1q_u32_x3(HINT_ALIGNED((p), (align)/8), (a))
#define vst1q_u32_x4_ex(p, a, align) vst1q_u32_x4(HINT_ALIGNED((p), (align)/8), (a))

#endif /* _MSC_VER */

/** MSVC for 32-bit ARM is missing some _x2/_x4 intrinsics. **/
/** gcc 7.x is also missing these intrinsics. **/

#if (defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARMT))) || \
    (defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8)

static __forceinline uint32x2x2_t vld1_u32_x2(const uint32_t *src)
{
	uint32x2x2_t vec;
	vec.val[0] = vld1_u32(&src[0]);
	vec.val[1] = vld1_u32(&src[2]);
	return vec;
}
#ifdef vld1_u32_x2_ex
#  undef vld1_u32_x2_ex
#endif
#define vld1_u32_x2_ex(p, align) vld1_u32_x2(HINT_ALIGNED((p), (align)/8))

static __forceinline void vst1_u32_x2(uint32_t *dest, uint32x2x2_t vec)
{
	vst1_u32(&dest[0], vec.val[0]);
	vst1_u32(&dest[2], vec.val[1]);
}
#ifdef vst1_u32_x2_ex
#  undef vst1_u32_x2_ex
#endif
#define vst1_u32_x2_ex(p, a, align) vst1_u32_x2(HINT_ALIGNED((p), (align)/8), (a))

static __forceinline uint32x2x4_t vld1_u32_x4(const uint32_t *src)
{
	uint32x2x4_t vec;
	vec.val[0] = vld1_u32(&src[0]);
	vec.val[1] = vld1_u32(&src[2]);
	vec.val[2] = vld1_u32(&src[4]);
	vec.val[3] = vld1_u32(&src[6]);
	return vec;
}
#ifdef vld1_u32_x4_ex
#  undef vld1_u32_x4_ex
#endif
#define vld1_u32_x4_ex(p, align) vld1_u32_x4(HINT_ALIGNED((p), (align)/8))

static __forceinline void vst1_u32_x4(uint32_t *dest, uint32x2x4_t vec)
{
	vst1_u32(&dest[0], vec.val[0]);
	vst1_u32(&dest[2], vec.val[1]);
	vst1_u32(&dest[4], vec.val[2]);
	vst1_u32(&dest[6], vec.val[3]);
}
#ifdef vst1_u32_x4_ex
#  undef vst1_u32_x4_ex
#endif
#define vst1_u32_x4_ex(p, a, align) vst1_u32_x4(HINT_ALIGNED((p), (align)/8), (a))

static __forceinline uint16x8x2_t vld1q_u16_x2(const uint16_t *src)
{
	uint16x8x2_t vec;
	vec.val[0] = vld1q_u16(&src[0]);
	vec.val[1] = vld1q_u16(&src[8]);
	return vec;
}
#ifdef vld1q_u16_x2_ex
#  undef vld1q_u16_x2_ex
#endif
#define vld1q_u16_x2_ex(p, align) vld1q_u16_x2(HINT_ALIGNED((p), (align)/8))

static __forceinline void vst1q_u16_x2(uint16_t *dest, uint16x8x2_t vec)
{
	vst1q_u16(&dest[0], vec.val[0]);
	vst1q_u16(&dest[8], vec.val[1]);
}
#ifdef vst1q_u16_x2_ex
#  undef vst1q_u16_x2_ex
#endif
#define vst1q_u16_x2_ex(p, a, align) vst1q_u16_x2(HINT_ALIGNED((p), (align)/8), (a))

static __forceinline uint32x4x2_t vld1q_u32_x2(const uint32_t *src)
{
	uint32x4x2_t vec;
	vec.val[0] = vld1q_u32(&src[0]);
	vec.val[1] = vld1q_u32(&src[4]);
	return vec;
}
#ifdef vld1q_u32_x2_ex
#  undef vld1q_u32_x2_ex
#endif
#define vld1q_u32_x2_ex(p, align) vld1q_u32_x2(HINT_ALIGNED((p), (align)/8))

static __forceinline void vst1q_u32_x2(uint32_t *dest, uint32x4x2_t vec)
{
	vst1q_u32(&dest[0], vec.val[0]);
	vst1q_u32(&dest[4], vec.val[1]);
}
#ifdef vst1q_u32_x2_ex
#  undef vst1q_u32_x2_ex
#endif
#define vst1q_u32_x2_ex(p, a, align) vst1q_u32_x2(HINT_ALIGNED((p), (align)/8), (a))

#endif /* (_MSC_VER && (_M_ARM || _M_ARMT)) || (__GNUC__ && !__clang__ && __GNUC__ < 8) */

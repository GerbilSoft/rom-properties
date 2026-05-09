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
    !defined(_M_ARM64) && !defined(__aarch64__)
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


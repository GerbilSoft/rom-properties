/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image_ops.cpp: Image class. (operations)                             *
 * NEON-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C++ STL classes
using std::array;

// ARM NEON intrinsics
#include <arm_neon.h>

// NOTE: vtbl only supports 64-bit vectors on ARMv7 (32-bit).
// TODO: Consolidate this with ImageDecoder_Linear_neon.cpp?
#if defined(RP_CPU_ARM64)
//static constexpr size_t VEC_LEN_U8 = 16;
static constexpr size_t VEC_LEN_U32 = 4;
typedef uint8x16_t uint8xVTBL_t;
typedef uint32x4_t uint32xVTBL_t;
#define vld1VTBL_u8  vld1q_u8
#define vld1VTBL_u32 vld1q_u32
#define vst1VTBL_u8  vst1q_u8
#define vst1VTBL_u32 vst1q_u32
#elif defined(RP_CPU_ARM)
//static constexpr size_t VEC_LEN_U8 = 8;
static constexpr size_t VEC_LEN_U32 = 2;
typedef uint8x8_t uint8xVTBL_t;
typedef uint32x2_t uint32xVTBL_t;
#define vld1VTBL_u8  vld1_u8
#define vld1VTBL_u32 vld1_u32
#define vst1VTBL_u8  vst1_u8
#define vst1VTBL_u32 vst1_u32
#else
#  error Unsupported CPU?
#endif

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpTexture {

/** Image operations **/

/**
 * Swizzle the image channels.
 * SSSE3-optimized version.
 *
 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::swizzle_neon(const char *swz_spec)
{
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend.get();
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// ARGB32 is required.
		// TODO: Automatically convert the image?
		return -EINVAL;
	}

	// TODO: Verify swz_spec.
	typedef union _u8_32 {
		uint8_t u8[4];
		uint32_t u32;
	} u8_32;
	u8_32 swz_ch;
	memcpy(&swz_ch, swz_spec, sizeof(swz_ch));
	if (swz_ch.u32 == 'rgba') {
		// 'rgba' == NULL swizzle. Don't bother doing anything.
		return 0;
	}

	// NOTE: Texture uses ARGB format, but swizzle uses rgba.
	// Rotate swz_ch to convert it to argb.
	// The entire thing needs to be byteswapped to match the internal order, too.
	// TODO: Verify on big-endian.
	swz_ch.u32 = (swz_ch.u32 >> 24) | (swz_ch.u32 << 8);
	swz_ch.u32 = be32_to_cpu(swz_ch.u32);

	// Determine the pshufb mask.
	// This can be used for [rgba0].
	// For 1, we'll need a separate "por" mask.
	// NOTE: NEON doesn't support bit 7 for "zero the byte", so we'll need to
	// use a separate "pand" mask as well.
	u8_32 pshufb_mask_vals;
	u8_32 pand_mask_vals;
	u8_32 por_mask_vals;
#define SET_MASK_VALS(n, shuf, pand, por) do { \
		pshufb_mask_vals.u8[n] = (shuf); \
		pand_mask_vals.u8[n] = (pand); \
		por_mask_vals.u8[n] = (por); \
	} while (0)
#define SWIZZLE_MASK_VAL(n) do { \
		switch (swz_ch.u8[n]) { \
					/*             n   shuf  pand   por */ \
			case 'b':	SET_MASK_VALS((n),    0, 0xFF, 0x00);	break; \
			case 'g':	SET_MASK_VALS((n),    1, 0xFF, 0x00);	break; \
			case 'r':	SET_MASK_VALS((n),    2, 0xFF, 0x00);	break; \
			case 'a':	SET_MASK_VALS((n),    3, 0xFF, 0x00);	break; \
			case '0':	SET_MASK_VALS((n), 0xFF, 0x00, 0x00);	break; \
			case '1':	SET_MASK_VALS((n), 0xFF, 0x00, 0xFF);	break; \
			default: \
				assert(!"Invalid swizzle value."); \
				SET_MASK_VALS((n), 0xFF, 0x00, 0x00); \
				break; \
		} \
	} while (0)

	SWIZZLE_MASK_VAL(0);
	SWIZZLE_MASK_VAL(1);
	SWIZZLE_MASK_VAL(2);
	SWIZZLE_MASK_VAL(3);

	const array<uint32_t, VEC_LEN_U32> pshufb_mask_u32 = {{
		pshufb_mask_vals.u32,
		pshufb_mask_vals.u32 + 0x04040404,
#ifdef RP_CPU_ARM64
		pshufb_mask_vals.u32 + 0x08080808,
		pshufb_mask_vals.u32 + 0x0C0C0C0C
#endif /* RP_CPU_ARM64 */
	}};
	const array<uint32_t, VEC_LEN_U32> pand_mask_u32 = {{
		pand_mask_vals.u32,
		pand_mask_vals.u32,
#ifdef RP_CPU_ARM64
		pand_mask_vals.u32,
		pand_mask_vals.u32
#endif /* RP_CPU_ARM64 */
	}};
	const array<uint32_t, VEC_LEN_U32> por_mask_u32 = {{
		por_mask_vals.u32,
		por_mask_vals.u32,
#ifdef RP_CPU_ARM64
		por_mask_vals.u32,
		por_mask_vals.u32
#endif /* RP_CPU_ARM64 */
	}};

	uint32xVTBL_t shuf_mask = vld1VTBL_u32(pshufb_mask_u32.data());
	uint32xVTBL_t and_mask = vld1VTBL_u32(pand_mask_u32.data());
	uint32xVTBL_t or_mask = vld1VTBL_u32(por_mask_u32.data());

	// Channel indexes
	static constexpr unsigned int SWZ_CH_B = 0U;
	static constexpr unsigned int SWZ_CH_G = 1U;
	static constexpr unsigned int SWZ_CH_R = 2U;
	static constexpr unsigned int SWZ_CH_A = 3U;

	uint32_t *bits = static_cast<uint32_t*>(backend->data());
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	const int width = backend->width;
	for (int y = backend->height; y > 0; y--) {
		// Process 16 pixels at a time using NEON.
		int x;
		for (x = width; x > 15; x -= 16, bits += 16) {
#if defined(RP_CPU_ARM64)
			uint32x4x4_t sa = vld4q_u32(bits);

			sa.val[0] = vqtbl1q_u8(sa.val[0], shuf_mask);
			sa.val[1] = vqtbl1q_u8(sa.val[1], shuf_mask);
			sa.val[2] = vqtbl1q_u8(sa.val[2], shuf_mask);
			sa.val[3] = vqtbl1q_u8(sa.val[3], shuf_mask);

			sa.val[0] = vandq_u32(sa.val[0], and_mask);
			sa.val[1] = vandq_u32(sa.val[1], and_mask);
			sa.val[2] = vandq_u32(sa.val[2], and_mask);
			sa.val[3] = vandq_u32(sa.val[3], and_mask);

			sa.val[0] = vorrq_u32(sa.val[0], or_mask);
			sa.val[1] = vorrq_u32(sa.val[1], or_mask);
			sa.val[2] = vorrq_u32(sa.val[2], or_mask);
			sa.val[3] = vorrq_u32(sa.val[3], or_mask);

			vst4q_u32(bits, sa);
#elif defined(RP_CPU_ARM)
			uint32x2x4_t sa = vld4_u32(&bits[0]);
			uint32x2x4_t sb = vld4_u32(&bits[8]);

			sa.val[0] = vtbl1_u8(sa.val[0], shuf_mask);
			sa.val[1] = vtbl1_u8(sa.val[1], shuf_mask);
			sa.val[2] = vtbl1_u8(sa.val[2], shuf_mask);
			sa.val[3] = vtbl1_u8(sa.val[3], shuf_mask);
			sb.val[0] = vtbl1_u8(sb.val[0], shuf_mask);
			sb.val[1] = vtbl1_u8(sb.val[1], shuf_mask);
			sb.val[2] = vtbl1_u8(sb.val[2], shuf_mask);
			sb.val[3] = vtbl1_u8(sb.val[3], shuf_mask);

			sa.val[0] = vand_u8(sa.val[0], and_mask);
			sa.val[1] = vand_u8(sa.val[1], and_mask);
			sa.val[2] = vand_u8(sa.val[2], and_mask);
			sa.val[3] = vand_u8(sa.val[3], and_mask);
			sb.val[0] = vand_u8(sb.val[0], and_mask);
			sb.val[1] = vand_u8(sb.val[1], and_mask);
			sb.val[2] = vand_u8(sb.val[2], and_mask);
			sb.val[3] = vand_u8(sb.val[3], and_mask);

			sa.val[0] = vorr_u8(sa.val[0], or_mask);
			sa.val[1] = vorr_u8(sa.val[1], or_mask);
			sa.val[2] = vorr_u8(sa.val[2], or_mask);
			sa.val[3] = vorr_u8(sa.val[3], or_mask);
			sb.val[0] = vorr_u8(sb.val[0], or_mask);
			sb.val[1] = vorr_u8(sb.val[1], or_mask);
			sb.val[2] = vorr_u8(sb.val[2], or_mask);
			sb.val[3] = vorr_u8(sb.val[3], or_mask);

			vst4_u32(&bits[0], sa);
			vst4_u32(&bits[8], sb);
#endif
		}

		// Process remaining pixels using regular swizzling
		for (; x > 0; x--, bits++) {
			u8_32 cur, swz;
			cur.u32 = *bits;

		// TODO: Verify on big-endian.
#define SWIZZLE_CHANNEL(n) do { \
				switch (swz_ch.u8[n]) { \
					case 'b':	swz.u8[n] = cur.u8[SWZ_CH_B];	break; \
					case 'g':	swz.u8[n] = cur.u8[SWZ_CH_G];	break; \
					case 'r':	swz.u8[n] = cur.u8[SWZ_CH_R];	break; \
					case 'a':	swz.u8[n] = cur.u8[SWZ_CH_A];	break; \
					case '0':	swz.u8[n] = 0;			break; \
					case '1':	swz.u8[n] = 255;		break; \
					default: \
						assert(!"Invalid swizzle value."); \
						swz.u8[n] = 0; \
						break; \
				} \
			} while (0)

			SWIZZLE_CHANNEL(0);
			SWIZZLE_CHANNEL(1);
			SWIZZLE_CHANNEL(2);
			SWIZZLE_CHANNEL(3);

			*bits = swz.u32;
		}

		// Next row.
		bits += stride_diff;
	}

	// Swizzle the sBIT value, if set.
	if (d->has_sBIT) {
		// TODO: If gray is set, move its values to rgb?
		const rp_image::sBIT_t sBIT_old = d->sBIT;

#define SWIZZLE_sBIT(n, ch) do { \
				switch (swz_ch.u8[n]) { \
					case 'b':	d->sBIT.ch = sBIT_old.blue;	break; \
					case 'g':	d->sBIT.ch = sBIT_old.green;	break; \
					case 'r':	d->sBIT.ch = sBIT_old.red;	break; \
					case 'a':	d->sBIT.ch = sBIT_old.alpha;	break; \
					case '0': case '1': \
							d->sBIT.ch = 1;			break; \
				} \
			} while (0)

			SWIZZLE_sBIT(SWZ_CH_B, blue);
			SWIZZLE_sBIT(SWZ_CH_G, green);
			SWIZZLE_sBIT(SWZ_CH_R, red);
			SWIZZLE_sBIT(SWZ_CH_A, alpha);
	}

	return 0;
}

}

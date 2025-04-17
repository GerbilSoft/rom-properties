/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image_ops.cpp: Image class. (operations)                             *
 * SSSE3-optimized version.                                                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// SSSE3 intrinsics
#include <emmintrin.h>
#include <tmmintrin.h>

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
int rp_image::swizzle_ssse3(const char *swz_spec)
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
	// For 1, we'll need a separate por mask.
	// N.B.: For pshufb, only bit 7 needs to be set to indicate "zero the byte".
	u8_32 pshufb_mask_vals;
	u8_32 por_mask_vals;
#define SWIZZLE_MASK_VAL(n) do { \
		switch (swz_ch.u8[n]) { \
			case 'b':	pshufb_mask_vals.u8[n] = 0;	por_mask_vals.u8[n] = 0;	break; \
			case 'g':	pshufb_mask_vals.u8[n] = 1;	por_mask_vals.u8[n] = 0;	break; \
			case 'r':	pshufb_mask_vals.u8[n] = 2;	por_mask_vals.u8[n] = 0;	break; \
			case 'a':	pshufb_mask_vals.u8[n] = 3;	por_mask_vals.u8[n] = 0;	break; \
			case '0':	pshufb_mask_vals.u8[n] = 0x80;	por_mask_vals.u8[n] = 0;	break; \
			case '1':	pshufb_mask_vals.u8[n] = 0x80;	por_mask_vals.u8[n] = 0xFF;	break; \
			default: \
				assert(!"Invalid swizzle value."); \
				pshufb_mask_vals.u8[n] = 0xFF; \
				por_mask_vals.u8[n] = 0; \
				break; \
		} \
	} while (0)

	SWIZZLE_MASK_VAL(0);
	SWIZZLE_MASK_VAL(1);
	SWIZZLE_MASK_VAL(2);
	SWIZZLE_MASK_VAL(3);

	const __m128i pshufb_mask = _mm_setr_epi32(
		pshufb_mask_vals.u32,
		pshufb_mask_vals.u32 + 0x04040404,
		pshufb_mask_vals.u32 + 0x08080808,
		pshufb_mask_vals.u32 + 0x0C0C0C0C
	);
	const __m128i por_mask = _mm_setr_epi32(
		por_mask_vals.u32,
		por_mask_vals.u32,
		por_mask_vals.u32,
		por_mask_vals.u32
	);

	// Channel indexes
	static constexpr unsigned int SWZ_CH_B = 0U;
	static constexpr unsigned int SWZ_CH_G = 1U;
	static constexpr unsigned int SWZ_CH_R = 2U;
	static constexpr unsigned int SWZ_CH_A = 3U;

	uint32_t *bits = static_cast<uint32_t*>(backend->data());
	const unsigned int stride_diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	const int width = backend->width;
	for (int y = backend->height; y > 0; y--) {
		// Process 16 pixels at a time using SSSE3.
		__m128i *xmm_bits = reinterpret_cast<__m128i*>(bits);
		int x;
		for (x = width; x > 15; x -= 16, xmm_bits += 4) {
			__m128i sa = _mm_load_si128(&xmm_bits[0]);
			__m128i sb = _mm_load_si128(&xmm_bits[1]);
			__m128i sc = _mm_load_si128(&xmm_bits[2]);
			__m128i sd = _mm_load_si128(&xmm_bits[3]);

			_mm_store_si128(&xmm_bits[0], _mm_or_si128(_mm_shuffle_epi8(sa, pshufb_mask), por_mask));
			_mm_store_si128(&xmm_bits[1], _mm_or_si128(_mm_shuffle_epi8(sb, pshufb_mask), por_mask));
			_mm_store_si128(&xmm_bits[2], _mm_or_si128(_mm_shuffle_epi8(sc, pshufb_mask), por_mask));
			_mm_store_si128(&xmm_bits[3], _mm_or_si128(_mm_shuffle_epi8(sd, pshufb_mask), por_mask));
		}

		// Process remaining pixels using regular swizzling
		bits = reinterpret_cast<uint32_t*>(xmm_bits);
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

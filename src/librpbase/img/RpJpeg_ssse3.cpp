/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_ssse3.cpp: JPEG image handler.                                   *
 * SSSE3-optimized version.                                                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::argb32_t;

// C includes (C++ namespace)
#include <cstdio>	// jpeglib.h needs stdio included first

// libjpeg
#include <jpeglib.h>

// SSSE3 intrinsics
#include <emmintrin.h>
#include <tmmintrin.h>

namespace LibRpBase { namespace RpJpeg { namespace ssse3 {

/**
 * Decode a 24-bit BGR JPEG to 32-bit ARGB.
 * SSSE3-optimized version.
 * NOTE: This function should ONLY be called from RpJpeg::load().
 * @param img		[in/out] rp_image.
 * @param cinfo		[in/out] JPEG decompression struct.
 * @param buffer 	[in/out] Line buffer. (Must be 16-byte aligned!)
 */
void decodeBGRtoARGB(rp_image *RESTRICT img, jpeg_decompress_struct *RESTRICT cinfo, JSAMPARRAY buffer)
{
	ASSERT_ALIGNMENT(16, buffer);
	assert(img->format() == rp_image::Format::ARGB32);

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266
	__m128i shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
	__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);
	argb32_t *dest = static_cast<argb32_t*>(img->bits());
	const int dest_stride_adj = (img->stride() - img->row_bytes()) / sizeof(argb32_t);
	while (cinfo->output_scanline < cinfo->output_height) {
		jpeg_read_scanlines(cinfo, buffer, 1);
		const uint8_t *src = buffer[0];

		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = cinfo->output_width;
		for (; x > 15; x -= 16, dest += 16, src += 16*3) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(src);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(dest);

			__m128i sa = _mm_load_si128(&xmm_src[0]);
			__m128i sb = _mm_load_si128(&xmm_src[1]);
			__m128i sc = _mm_load_si128(&xmm_src[2]);

			__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[0], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sb, sa, 12), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[1], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sb, 8), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[2], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sc, 4), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[3], val);
		}

		// Remaining pixels.
		for (; x > 0; x--, dest++, src += 3) {
			dest->b = src[2];
			dest->g = src[1];
			dest->r = src[0];
			dest->a = 0xFF;
		}

		// Next line.
		dest += dest_stride_adj;
	}
}

} } }

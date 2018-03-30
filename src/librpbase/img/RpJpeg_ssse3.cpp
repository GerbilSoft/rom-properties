/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg.cpp: JPEG image handler.                                         *
 * SSSE3-optimized version.                                                *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "RpJpeg_p.hpp"

#include "../common.h"
using LibRpBase::rp_image;

// C includes. (C++ namespace)
#include <cassert>

// SSSE3 intrinsics.
#include <emmintrin.h>
#include <tmmintrin.h>

namespace LibRpBase {

/**
 * Decode a 24-bit BGR JPEG to 32-bit ARGB.
 * SSSE3-optimized version.
 * NOTE: This function should ONLY be called from RpJpeg::loadUnchecked().
 * @param img		[in/out] rp_image.
 * @param cinfo		[in/out] JPEG decompression struct.
 * @param buffer 	[in/out] Line buffer. (Must be 16-byte aligned!)
 */
void RpJpegPrivate::decodeBGRtoARGB(rp_image *RESTRICT img, jpeg_decompress_struct *RESTRICT cinfo, JSAMPARRAY buffer)
{
	ASSERT_ALIGNMENT(16, buffer);
	assert(img->format() == rp_image::FORMAT_ARGB32);

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266
	__m128i shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
	__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);
	argb32_t *dest = static_cast<argb32_t*>(img->bits());
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	while (cinfo->output_scanline < cinfo->output_height) {
		jpeg_read_scanlines(cinfo, buffer, 1);
		const uint8_t *src = buffer[0];

		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = cinfo->output_width;
		for (; x > 15; x -= 16, dest += 16, src += 16*3) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(src);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(dest);

			__m128i sa = _mm_load_si128(xmm_src);
			__m128i sb = _mm_load_si128(xmm_src+1);
			__m128i sc = _mm_load_si128(xmm_src+2);

			__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sb, sa, 12), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+1, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sb, 8), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+2, val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sc, 4), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(xmm_dest+3, val);
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

}

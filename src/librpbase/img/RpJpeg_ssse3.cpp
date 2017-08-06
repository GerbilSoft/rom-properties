/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg.cpp: JPEG image handler.                                         *
 * SSSE3-optimized version.                                                *
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

#include "RpJpeg_p.hpp"
using LibRpBase::rp_image;

// C includes. (C++ namespace)
#include <cassert>

// SSSE3 headers.
#include <xmmintrin.h>
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
void RpJpegPrivate::decodeBGRtoARGB(rp_image *img, jpeg_decompress_struct *cinfo, JSAMPARRAY buffer)
{
	assert(reinterpret_cast<intptr_t>(buffer) % 16 == 0);

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266
	while (cinfo->output_scanline < cinfo->output_height) {
		// NOTE: jpeg_read_scanlines() increments the scanline value,
		// so we have to save the rp_image scanline pointer first.
		argb32_t *dest = static_cast<argb32_t*>(img->scanLine(cinfo->output_scanline));
		jpeg_read_scanlines(cinfo, buffer, 1);
		const uint8_t *src = buffer[0];

		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = cinfo->output_width;
		__m128i shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
		__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);
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

		// Process 2 pixels per iteration.
		for (; x > 1; x -= 2, dest += 2, src += 2*3) {
			dest[0].a = 0xFF;
			dest[0].r = src[0];
			dest[0].g = src[1];
			dest[0].b = src[2];

			dest[1].a = 0xFF;
			dest[1].r = src[3];
			dest[1].g = src[4];
			dest[1].b = src[5];
		}
		// Remaining pixels.
		for (; x > 0; x--, dest++, src += 3) {
			dest->a = 0xFF;
			dest->r = src[0];
			dest->g = src[1];
			dest->b = src[2];
		}
	}
}

}

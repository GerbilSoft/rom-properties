/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
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

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

// C includes. (C++ namespace)
#include <cassert>

// SSSE3 headers.
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

namespace LibRpBase {

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear24_ssse3(PixelFormat px_format,
	int width, int height,
	const uint8_t *img_buf, int img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 3;

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * bytespp))
	{
		return nullptr;
	}

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		if (stride % bytespp != 0) {
			// Invalid stride.
			return nullptr;
		}
		// Byte addressing, so keep it in units of bytespp.
		src_stride_adj = (width * bytespp) - stride;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	argb32_t *px_dest = static_cast<argb32_t*>(img->bits());

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266

	// 24-bit RGB images don't have an alpha channel.
	__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);

	// Determine the byte shuffle mask.
	__m128i shuf_mask;
	switch (px_format) {
		case PXF_RGB888:
			shuf_mask = _mm_setr_epi8(0,1,2,-1, 3,4,5,-1, 6,7,8,-1, 9,10,11,-1);
			break;
		case PXF_BGR888:
			shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
			break;
		default:
			assert(!"Unsupported 24-bit pixel format.");
			return nullptr;
	}

	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = (unsigned int)width;
		for (; x > 15; x -= 16, px_dest += 16, img_buf += 16*3) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

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

		switch (px_format) {
			case PXF_RGB888:
				// Remaining pixels.
				for (; x > 0; x--, px_dest++, img_buf += 3) {
					px_dest->b = img_buf[0];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[2];
					px_dest->a = 0xFF;
				}
				break;

			case PXF_BGR888:
				// Remaining pixels.
				for (; x > 0; x--, px_dest++, img_buf += 3) {
					px_dest->b = img_buf[2];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[0];
					px_dest->a = 0xFF;
				}
				break;

			default:
				assert(!"Unsupported 24-bit pixel format.");
				return nullptr;
		}

		// Next line.
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

}

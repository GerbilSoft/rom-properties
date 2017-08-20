/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
 * SSE2-optimized version.                                                *
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

// SSE2 intrinsics.
#include <xmmintrin.h>
#include <emmintrin.h>

namespace LibRpBase {

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * SSE2-optimized version.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.(must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear16_sse2(PixelFormat px_format,
	int width, int height,
	const uint16_t *img_buf, int img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 2;

	// FIXME: Add support for more formats.
	// For now, redirect back to the C++ version.
	if (px_format != PXF_RGB565) {
		return fromLinear16_cpp(px_format, width, height, img_buf, img_siz, stride);
	}

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
		assert(stride >= (width * bytespp));
		if (unlikely(stride % bytespp != 0 || stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		src_stride_adj = (stride / bytespp) - width;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Alpha mask.
	__m128i A32_mask = _mm_setr_epi32(0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);
	// AND masks for R, G, and B.
	__m128i R_mask   = _mm_setr_epi16(0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800);
	__m128i G_mask   = _mm_setr_epi16(0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0);
	__m128i B_mask   = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);
	__m128i Glo_mask = _mm_setr_epi16(0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00);

	// Convert RGB565 to ARGB32.
	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Process 8 pixels per iteration using SSE2.
		unsigned int x = (unsigned int)width;
		for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

			// Mask the G and B components and shift them into place.
			__m128i sG = _mm_slli_epi16(_mm_and_si128(G_mask, *xmm_src), 5);
			__m128i sB = _mm_slli_epi16(_mm_and_si128(B_mask, *xmm_src), 3);
			sG = _mm_or_si128(sG, _mm_srli_epi16(sG, 6));
			sB = _mm_or_si128(sB, _mm_srli_epi16(sB, 5));
			// Combine G and B.
			// NOTE: G low byte has to be masked due to the shift.
			sB = _mm_or_si128(sB, _mm_and_si128(sG, Glo_mask));

			// Mask the R component and shift it into place.
			__m128i sR = _mm_srli_epi16(_mm_and_si128(R_mask, *xmm_src), 8);
			sR = _mm_or_si128(sR, _mm_srli_epi16(sR, 5));

			// Unpack R and GB into DWORDs.
			__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), A32_mask);
			__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), A32_mask);

			_mm_store_si128(xmm_dest, px0);
			_mm_store_si128(xmm_dest+1, px1);
		}

		// Remaining pixels.
		for (; x > 0; x--) {
			*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(*img_buf);
			img_buf++;
			px_dest++;
		}

		// Next line.
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT_RGB565 = {5,6,5,0,0};
	img->set_sBIT(&sBIT_RGB565);

	// Image has been converted.
	return img;
}

}

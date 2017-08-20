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

// MSVC complains when the high bit is set in hex values
// when setting SSE2 registers.
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4309)
#endif

namespace LibRpBase {

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * SSE2-optimized version.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
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
	switch (px_format) {
		case PXF_RGB565:
		case PXF_BGR565:
		case PXF_RGB555:
		case PXF_BGR555:
			break;

		default:
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

	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

	// Alpha mask.
	__m128i Mask32_A = _mm_setr_epi32(0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);
	// Mask for the high byte for Green.
	__m128i MaskG_Hi8   = _mm_setr_epi16(0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00);

	// AND masks for 565 channels.
	__m128i Mask565_Hi5  = _mm_setr_epi16(0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800);
	__m128i Mask565_Mid6 = _mm_setr_epi16(0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0);
	__m128i Mask565_Lo5  = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);

	// AND masks for 555 channels.
	__m128i Mask555_Hi5  = _mm_setr_epi16(0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00);
	__m128i Mask555_Mid5 = _mm_setr_epi16(0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0);
	__m128i Mask555_Lo5  = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);

	switch (px_format) {
		/** 16-bit **/

		case PXF_RGB565:
			// Convert RGB565 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Mask the G and B components and shift them into place.
					__m128i sG = _mm_slli_epi16(_mm_and_si128(Mask565_Mid6, *xmm_src), 5);
					__m128i sB = _mm_slli_epi16(_mm_and_si128(Mask565_Lo5, *xmm_src), 3);
					sG = _mm_or_si128(sG, _mm_srli_epi16(sG, 6));
					sB = _mm_or_si128(sB, _mm_srli_epi16(sB, 5));
					// Combine G and B.
					// NOTE: G low byte has to be masked due to the shift.
					sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));

					// Mask the R component and shift it into place.
					__m128i sR = _mm_srli_epi16(_mm_and_si128(Mask565_Hi5, *xmm_src), 8);
					sR = _mm_or_si128(sR, _mm_srli_epi16(sR, 5));

					// Unpack R and GB into DWORDs.
					__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), Mask32_A);
					__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), Mask32_A);

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
			break;

		case PXF_BGR565:
			// Convert BGR565 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Mask the G and B components and shift them into place.
					__m128i sG = _mm_slli_epi16(_mm_and_si128(Mask565_Mid6, *xmm_src), 5);
					__m128i sB = _mm_srli_epi16(_mm_and_si128(Mask565_Hi5, *xmm_src), 8);
					sG = _mm_or_si128(sG, _mm_srli_epi16(sG, 6));
					sB = _mm_or_si128(sB, _mm_srli_epi16(sB, 5));
					// Combine G and B.
					// NOTE: G low byte has to be masked due to the shift.
					sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));

					// Mask the R component and shift it into place.
					__m128i sR = _mm_slli_epi16(_mm_and_si128(Mask565_Lo5, *xmm_src), 3);
					sR = _mm_or_si128(sR, _mm_srli_epi16(sR, 5));

					// Unpack R and GB into DWORDs.
					__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), Mask32_A);
					__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), Mask32_A);

					_mm_store_si128(xmm_dest, px0);
					_mm_store_si128(xmm_dest+1, px1);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::BGR565_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		/** 15-bit **/

		case PXF_RGB555:
			// Convert RGB555 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Mask the G and B components and shift them into place.
					__m128i sG = _mm_slli_epi16(_mm_and_si128(Mask555_Mid5, *xmm_src), 6);
					__m128i sB = _mm_slli_epi16(_mm_and_si128(Mask555_Lo5, *xmm_src), 3);
					sG = _mm_or_si128(sG, _mm_srli_epi16(sG, 5));
					sB = _mm_or_si128(sB, _mm_srli_epi16(sB, 5));
					// Combine G and B.
					// NOTE: G low byte has to be masked due to the shift.
					sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));

					// Mask the R component and shift it into place.
					__m128i sR = _mm_srli_epi16(_mm_and_si128(Mask555_Hi5, *xmm_src), 7);
					sR = _mm_or_si128(sR, _mm_srli_epi16(sR, 5));

					// Unpack R and GB into DWORDs.
					__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), Mask32_A);
					__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), Mask32_A);

					_mm_store_si128(xmm_dest, px0);
					_mm_store_si128(xmm_dest+1, px1);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RGB555_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		case PXF_BGR555:
			// Convert BGR555 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Mask the G and B components and shift them into place.
					__m128i sG = _mm_slli_epi16(_mm_and_si128(Mask555_Mid5, *xmm_src), 6);
					__m128i sB = _mm_srli_epi16(_mm_and_si128(Mask555_Hi5, *xmm_src), 7);
					sG = _mm_or_si128(sG, _mm_srli_epi16(sG, 5));
					sB = _mm_or_si128(sB, _mm_srli_epi16(sB, 5));
					// Combine G and B.
					// NOTE: G low byte has to be masked due to the shift.
					sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));

					// Mask the R component and shift it into place.
					__m128i sR = _mm_slli_epi16(_mm_and_si128(Mask555_Lo5, *xmm_src), 3);
					sR = _mm_or_si128(sR, _mm_srli_epi16(sR, 5));

					// Unpack R and GB into DWORDs.
					__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), Mask32_A);
					__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), Mask32_A);

					_mm_store_si128(xmm_dest, px0);
					_mm_store_si128(xmm_dest+1, px1);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::BGR555_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;


		default:
			assert(!"Pixel format not supported.");
			delete img;
			return nullptr;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT_RGB565 = {5,6,5,0,0};
	img->set_sBIT(&sBIT_RGB565);

	// Image has been converted.
	return img;
}

}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

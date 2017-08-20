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
 * Templated function for 15/16-bit RGB conversion using SSE2. (no alpha channel)
 * Processes 8 pixels per iteration.
 * Use this in the inner loop of the main code.
 *
 * @tparam Rshift_W	[in] Red shift amount in the high word.
 * @tparam Gshift_W	[in] Green shift amount in the low word.
 * @tparam Bshift_W	[in] Blue shift amount in the low word.
 * @tparam Rbits	[in] Red bit count.
 * @tparam Gbits	[in] Green bit count.
 * @tparam Bbits	[in] Blue bit count.
 * @tparam isBGR	[in] If true, this is BGR instead of RGB.
 * @param Rmask		[in] SSE2 mask for the Red channel.
 * @param Gmask		[in] SSE2 mask for the Green channel.
 * @param Bmask		[in] SSE2 mask for the Blue channel.
 * @param img_buf	[in] 16-bit image buffer.
 * @param px_dest	[out] Destination image buffer.
 */
template<uint8_t Rshift_W, uint8_t Gshift_W, uint8_t Bshift_W,
	uint8_t Rbits, uint8_t Gbits, uint8_t Bbits, bool isBGR>
static inline void T_RGB16_sse2(
	const __m128i &Rmask, const __m128i &Gmask, const __m128i &Bmask,
	const uint16_t *RESTRICT img_buf, uint32_t *RESTRICT px_dest)
{
	// Alpha mask.
	static const __m128i Mask32_A  = _mm_setr_epi32(0xFF000000,0xFF000000,0xFF000000,0xFF000000);
	// Mask for the high byte for Green.
	static const __m128i MaskG_Hi8 = _mm_setr_epi16(0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00);

	const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
	__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

	// TODO: For xRGB4444, we should be able to optimize out some of the shifts.

	// Mask the G and B components and shift them into place.
	__m128i sG = _mm_slli_epi16(_mm_and_si128(Gmask, *xmm_src), Gshift_W);
	__m128i sB;
	if (isBGR) {
		sB = _mm_srli_epi16(_mm_and_si128(Bmask, *xmm_src), Bshift_W);
	} else {
		sB = _mm_slli_epi16(_mm_and_si128(Bmask, *xmm_src), Bshift_W);
	}
	sG = _mm_or_si128(sG, _mm_srli_epi16(sG, Gbits));
	sB = _mm_or_si128(sB, _mm_srli_epi16(sB, Bbits));
	// Combine G and B.
	// NOTE: G low byte has to be masked due to the shift.
	sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));

	// Mask the R component and shift it into place.
	__m128i sR;
	if (isBGR) {
		sR = _mm_slli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	} else {
		sR = _mm_srli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	}
	sR = _mm_or_si128(sR, _mm_srli_epi16(sR, Rbits));

	// Unpack R and GB into DWORDs.
	__m128i px0 = _mm_or_si128(_mm_unpacklo_epi16(sB, sR), Mask32_A);
	__m128i px1 = _mm_or_si128(_mm_unpackhi_epi16(sB, sR), Mask32_A);

	_mm_store_si128(xmm_dest, px0);
	_mm_store_si128(xmm_dest+1, px1);
}

/**
 * Templated function for 15/16-bit RGB conversion using SSE2. (with alpha channel)
 * Processes 8 pixels per iteration.
 * Use this in the inner loop of the main code.
 *
 * @tparam Ashift_W	[in] Alpha shift amount in the high word.
 * @tparam Rshift_W	[in] Red shift amount in the high word.
 * @tparam Gshift_W	[in] Green shift amount in the low word.
 * @tparam Bshift_W	[in] Blue shift amount in the low word.
 * @tparam Abits	[in] Alpha bit count.
 * @tparam Rbits	[in] Red bit count.
 * @tparam Gbits	[in] Green bit count.
 * @tparam Bbits	[in] Blue bit count.
 * @tparam isBGR	[in] If true, this is BGR instead of RGB.
 * @param Amask		[in] SSE2 mask for the Alpha channel.
 * @param Rmask		[in] SSE2 mask for the Red channel.
 * @param Gmask		[in] SSE2 mask for the Green channel.
 * @param Bmask		[in] SSE2 mask for the Blue channel.
 * @param img_buf	[in] 16-bit image buffer.
 * @param px_dest	[out] Destination image buffer.
 */
template<uint8_t Ashift_W, uint8_t Rshift_W, uint8_t Gshift_W, uint8_t Bshift_W,
	uint8_t Abits, uint8_t Rbits, uint8_t Gbits, uint8_t Bbits, bool isBGR>
static inline void T_ARGB16_sse2(
	const __m128i &Amask, const __m128i &Rmask, const __m128i &Gmask, const __m128i &Bmask,
	const uint16_t *RESTRICT img_buf, uint32_t *RESTRICT px_dest)
{
	// Mask for the high byte for Green and Alpha.
	static const __m128i MaskAG_Hi8 = _mm_setr_epi16(0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00,0xFF00);

	const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
	__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

	// TODO: For ARGB4444, we should be able to optimize out some of the shifts.

	// Mask the G and B components and shift them into place.
	__m128i sG = _mm_slli_epi16(_mm_and_si128(Gmask, *xmm_src), Gshift_W);
	__m128i sB;
	if (isBGR) {
		sB = _mm_srli_epi16(_mm_and_si128(Bmask, *xmm_src), Bshift_W);
	} else {
		sB = _mm_slli_epi16(_mm_and_si128(Bmask, *xmm_src), Bshift_W);
	}
	sG = _mm_or_si128(sG, _mm_srli_epi16(sG, Gbits));
	sB = _mm_or_si128(sB, _mm_srli_epi16(sB, Bbits));
	// Combine G and B.
	// NOTE: G low byte has to be masked due to the shift.
	sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskAG_Hi8));

	// Mask the A and R components and shift them into place.
	__m128i sA = _mm_slli_epi16(_mm_and_si128(Amask, *xmm_src), Ashift_W);
	__m128i sR;
	if (isBGR) {
		sR = _mm_slli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	} else {
		sR = _mm_srli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	}
	sA = _mm_or_si128(sA, _mm_srli_epi16(sA, Abits));
	sR = _mm_or_si128(sR, _mm_srli_epi16(sR, Rbits));
	// Combine A and R.
	// NOTE: A low byte has to be masked due to the shift.
	sR = _mm_or_si128(sR, _mm_and_si128(sA, MaskAG_Hi8));

	// Unpack AR and GB into DWORDs.
	__m128i px0 = _mm_unpacklo_epi16(sB, sR);
	__m128i px1 = _mm_unpackhi_epi16(sB, sR);

	_mm_store_si128(xmm_dest, px0);
	_mm_store_si128(xmm_dest+1, px1);
}

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
		case PXF_ARGB4444:
		case PXF_xRGB4444:
		case PXF_xBGR4444:
		case PXF_RGBx4444:
		case PXF_BGRx4444:
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

	// AND masks for 565 channels.
	static const __m128i Mask565_Hi5  = _mm_setr_epi16(0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800);
	static const __m128i Mask565_Mid6 = _mm_setr_epi16(0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0,0x07E0);
	static const __m128i Mask565_Lo5  = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);

	// AND masks for 555 channels.
	static const __m128i Mask555_Hi5  = _mm_setr_epi16(0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00);
	static const __m128i Mask555_Mid5 = _mm_setr_epi16(0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0);
	static const __m128i Mask555_Lo5  = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);

	// AND masks for 4444 channels.
	static const __m128i Mask4444_Nyb3 = _mm_setr_epi16(0xF000,0xF000,0xF000,0xF000,0xF000,0xF000,0xF000,0xF000);
	static const __m128i Mask4444_Nyb2 = _mm_setr_epi16(0x0F00,0x0F00,0x0F00,0x0F00,0x0F00,0x0F00,0x0F00,0x0F00);
	static const __m128i Mask4444_Nyb1 = _mm_setr_epi16(0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0,0x00F0);
	static const __m128i Mask4444_Nyb0 = _mm_setr_epi16(0x000F,0x000F,0x000F,0x000F,0x000F,0x000F,0x000F,0x000F);

	switch (px_format) {
		/** RGB565 **/

		case PXF_RGB565:
			// Convert RGB565 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<8, 5, 3, 5, 6, 5, false>(Mask565_Hi5, Mask565_Mid6, Mask565_Lo5, img_buf, px_dest);
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
					T_RGB16_sse2<3, 5, 8, 5, 6, 5, true>(Mask565_Lo5, Mask565_Mid6, Mask565_Hi5, img_buf, px_dest);
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

		/** ARGB4444 **/

		case PXF_ARGB4444:
			// Convert ARGB4444 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_ARGB16_sse2<0, 4, 8, 4, 4, 4, 4, 4, false>(Mask4444_Nyb3, Mask4444_Nyb2, Mask4444_Nyb1, Mask4444_Nyb0, img_buf, px_dest);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		/** xRGB4444 **/

		case PXF_xRGB4444:
			// Convert xRGB4444 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<4, 8, 4, 4, 4, 4, false>(Mask4444_Nyb2, Mask4444_Nyb1, Mask4444_Nyb0, img_buf, px_dest);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::xRGB4444_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		case PXF_xBGR4444:
			// Convert xBGR4444 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<4, 8, 4, 4, 4, 4, true>(Mask4444_Nyb0, Mask4444_Nyb1, Mask4444_Nyb2, img_buf, px_dest);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::xBGR4444_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		case PXF_RGBx4444:
			// Convert xRGB4444 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<8, 4, 0, 4, 4, 4, false>(Mask4444_Nyb3, Mask4444_Nyb2, Mask4444_Nyb1, img_buf, px_dest);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RGBx4444_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		case PXF_BGRx4444:
			// Convert xBGR4444 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<0, 4, 8, 4, 4, 4, true>(Mask4444_Nyb1, Mask4444_Nyb2, Mask4444_Nyb3, img_buf, px_dest);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::BGRx4444_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		/** RGB555 **/

		case PXF_RGB555:
			// Convert RGB555 to ARGB32.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 8 pixels per iteration using SSE2.
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					T_RGB16_sse2<7, 6, 3, 5, 5, 5, false>(Mask555_Hi5, Mask555_Mid5, Mask565_Lo5, img_buf, px_dest);
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
					T_RGB16_sse2<3, 6, 7, 5, 5, 5, true>(Mask555_Lo5, Mask555_Mid5, Mask565_Hi5, img_buf, px_dest);
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

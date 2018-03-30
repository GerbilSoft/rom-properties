/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
 * SSE2-optimized version.                                                 *
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

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

// SSE2 intrinsics.
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
	if (Gbits > 4) {
		// NOTE: G low byte has to be masked due to the shift.
		sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskG_Hi8));
	} else {
		// Not enough Gbits to need masking.
		// FIXME: If less than 4, need to shift multiple times.
		sB = _mm_or_si128(sB, sG);
	}

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
 * @tparam Ashift_W	[in] Alpha shift amount in the high word. (16 for 1555 alpha handling; 17 for 5551 alpha handling)
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
	static_assert(Ashift_W <= 17, "Ashift_W is invalid.");
	static_assert(Rshift_W < 16, "Rshift_W is invalid.");
	static_assert(Gshift_W < 16, "Gshift_W is invalid.");
	static_assert(Bshift_W < 16, "Bshift_W is invalid.");
	static_assert(Abits < 16, "Abits is invalid.");
	static_assert(Rbits < 16, "Rbits is invalid.");
	static_assert(Gbits < 16, "Gbits is invalid.");
	static_assert(Bbits < 16, "Bbits is invalid.");
	static_assert(Abits + Rbits + Gbits + Bbits <= 16, "Total number of bits is invalid.");

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
	if (Gbits > 4) {
		// NOTE: G low byte has to be masked due to the shift.
		sB = _mm_or_si128(sB, _mm_and_si128(sG, MaskAG_Hi8));
	} else {
		// Not enough Gbits to need masking.
		// FIXME: If less than 4, need to shift multiple times.
		sB = _mm_or_si128(sB, sG);
	}

	// Mask the R component and shift it into place.
	__m128i sR;
	if (isBGR) {
		sR = _mm_slli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	} else {
		sR = _mm_srli_epi16(_mm_and_si128(Rmask, *xmm_src), Rshift_W);
	}
	sR = _mm_or_si128(sR, _mm_srli_epi16(sR, Rbits));
	// Mask the A components, shift it into place, and combine with R.
	__m128i sA;
	if (Ashift_W == 16) {
		// 1555 alpha handling.
		// Using a bytewise comparison so we don't have to mask off the low byte.
		// NOTE: This comparison is *signed*. Amask must be 0x0080, and we're
		// checking for less than, which will match:
		// - < 0x00: 0x80-0xFF
		// - < 0x80: Nothing
		sA = _mm_cmplt_epi8(*xmm_src, Amask);
		// Combine A and R.
		sR = _mm_or_si128(sR, sA);
	} else if (Ashift_W == 17) {
		// 5551 alpha handling.
		// Amask has only bit 0 set for each word.
		// This will mask off bit 0, then compare it to the Amask value.
		// Any that have bit 0 set will be set to 0x00FF; otherwise, 0x0000.
		// This can then be shifted into place.
		sA = _mm_slli_epi16(_mm_cmpeq_epi8(_mm_and_si128(*xmm_src, Amask), Amask), 8);
		// Combine A and R.
		sR = _mm_or_si128(sR, sA);
	} else {
		// Standard alpha handling.
		sA = _mm_slli_epi16(_mm_and_si128(Amask, *xmm_src), Ashift_W);
		sA = _mm_or_si128(sA, _mm_srli_epi16(sA, Abits));
		// Combine A and R.
		// NOTE: A low byte has to be masked due to the shift.
		if (Abits > 4) {
			// NOTE: A low byte has to be masked due to the shift.
			sR = _mm_or_si128(sR, _mm_and_si128(sA, MaskAG_Hi8));
		} else {
			// Not enough Abits to need masking.
			// FIXME: If less than 4, need to shift multiple times.
			sR = _mm_or_si128(sR, sA);
		}
	}

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
	const uint16_t *RESTRICT img_buf, int img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 2;

	// FIXME: Add support for these formats.
	// For now, redirect back to the C++ version.
	switch (px_format) {
		case PXF_ARGB8332:
		case PXF_RGB5A3:
		case PXF_IA8:
		case PXF_BGR555_PS1:
		case PXF_L16:
		case PXF_A8L8:
			return fromLinear16_cpp(px_format, width, height, img_buf, img_siz, stride);

		default:
			break;
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

	// AND masks for 1555 channels.
	static const __m128i Cmp1555_A     = _mm_setr_epi16(0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080);
	static const __m128i Mask1555_Hi5  = _mm_setr_epi16(0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00,0x7C00);
	static const __m128i Mask1555_Mid5 = _mm_setr_epi16(0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0,0x03E0);
	static const __m128i Mask1555_Lo5  = _mm_setr_epi16(0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F,0x001F);

	// AND masks for 5551 channels.
	static const __m128i Cmp5551_A     = _mm_setr_epi16(0x0101,0x0101,0x0101,0x0101,0x0101,0x0101,0x0101,0x0101);
	static const __m128i Mask5551_Hi5  = _mm_setr_epi16(0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800,0xF800);
	static const __m128i Mask5551_Mid5 = _mm_setr_epi16(0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0,0x07C0);
	static const __m128i Mask5551_Lo5  = _mm_setr_epi16(0x003E,0x003E,0x003E,0x003E,0x003E,0x003E,0x003E,0x003E);

	// Alpha mask.
	static const __m128i Mask32_A  = _mm_setr_epi32(0xFF000000,0xFF000000,0xFF000000,0xFF000000);

	// GR88 mask.
	static const __m128i MaskGR88  = _mm_setr_epi32(0x00FFFF00,0x00FFFF00,0x00FFFF00,0x00FFFF00);

	// sBIT metadata.
	static const rp_image::sBIT_t sBIT_RGB565   = {5,6,5,0,0};
	static const rp_image::sBIT_t sBIT_ARGB1555 = {5,5,5,0,1};
	static const rp_image::sBIT_t sBIT_xRGB4444 = {4,4,4,0,0};
	static const rp_image::sBIT_t sBIT_ARGB4444 = {4,4,4,0,4};
	static const rp_image::sBIT_t sBIT_RGB555   = {5,5,5,0,0};

	// Macro for 16-bit formats with no alpha channel.
#define fromLinear16_convert(fmt, sBIT, Rshift_W, Gshift_W, Bshift_W, Rbits, Gbits, Bbits, isBGR, Rmask, Gmask, Bmask) \
		case PXF_##fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				/* Process 8 pixels per iteration using SSE2. */ \
				unsigned int x = (unsigned int)width; \
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) { \
					T_RGB16_sse2<Rshift_W, Gshift_W, Bshift_W, Rbits, Gbits, Bbits, isBGR>( \
						Rmask, Gmask, Bmask, img_buf, px_dest); \
				} \
				\
				/* Remaining pixels. */ \
				for (; x > 0; x--) { \
					*px_dest = ImageDecoderPrivate::fmt##_to_ARGB32(*img_buf); \
					img_buf++; \
					px_dest++; \
				} \
				\
				/* Next line. */ \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT metadata. */ \
			img->set_sBIT(&sBIT); \
		} break

	// Macro for 16-bit formats with an alpha channel.
#define fromLinear16A_convert(fmt, sBIT, Ashift_W, Rshift_W, Gshift_W, Bshift_W, Abits, Rbits, Gbits, Bbits, isBGR, Amask, Rmask, Gmask, Bmask) \
		case PXF_##fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				/* Process 8 pixels per iteration using SSE2. */ \
				unsigned int x = (unsigned int)width; \
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) { \
					T_ARGB16_sse2<Ashift_W, Rshift_W, Gshift_W, Bshift_W, Abits, Rbits, Gbits, Bbits, isBGR>( \
						Amask, Rmask, Gmask, Bmask, img_buf, px_dest); \
				} \
				\
				/* Remaining pixels. */ \
				for (; x > 0; x--) { \
					*px_dest = ImageDecoderPrivate::fmt##_to_ARGB32(*img_buf); \
					img_buf++; \
					px_dest++; \
				} \
				\
				/* Next line. */ \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT metadata. */ \
			img->set_sBIT(&sBIT); \
		} break

	switch (px_format) {
		/** RGB565 **/
		fromLinear16_convert(RGB565, sBIT_RGB565, 8, 5, 3, 5, 6, 5, false, Mask565_Hi5, Mask565_Mid6, Mask565_Lo5);
		fromLinear16_convert(BGR565, sBIT_RGB565, 3, 5, 8, 5, 6, 5, true,  Mask565_Lo5, Mask565_Mid6, Mask565_Hi5);

		/** ARGB1555 **/
		fromLinear16A_convert(ARGB1555, sBIT_ARGB1555, 16, 7, 6, 3, 1, 5, 5, 5, false, Cmp1555_A, Mask1555_Hi5, Mask1555_Mid5, Mask1555_Lo5);
		fromLinear16A_convert(ABGR1555, sBIT_ARGB1555, 16, 3, 6, 7, 1, 5, 5, 5, true,  Cmp1555_A, Mask1555_Lo5, Mask1555_Mid5, Mask1555_Hi5);
		fromLinear16A_convert(RGBA5551, sBIT_ARGB1555, 17, 8, 5, 2, 1, 5, 5, 5, false, Cmp5551_A, Mask5551_Hi5, Mask5551_Mid5, Mask5551_Lo5);
		fromLinear16A_convert(BGRA5551, sBIT_ARGB1555, 17, 2, 5, 8, 1, 5, 5, 5, true,  Cmp5551_A, Mask5551_Lo5, Mask5551_Mid5, Mask5551_Hi5);

		/** ARGB4444 **/
		fromLinear16A_convert(ARGB4444, sBIT_ARGB4444,  0, 4, 8, 4, 4, 4, 4, 4, false, Mask4444_Nyb3, Mask4444_Nyb2, Mask4444_Nyb1, Mask4444_Nyb0);
		fromLinear16A_convert(ABGR4444, sBIT_ARGB4444,  0, 4, 8, 4, 4, 4, 4, 4, true,  Mask4444_Nyb3, Mask4444_Nyb0, Mask4444_Nyb1, Mask4444_Nyb2);
		fromLinear16A_convert(RGBA4444, sBIT_ARGB4444, 12, 8, 4, 0, 4, 4, 4, 4, false, Mask4444_Nyb0, Mask4444_Nyb3, Mask4444_Nyb2, Mask4444_Nyb1);
		fromLinear16A_convert(BGRA4444, sBIT_ARGB4444, 12, 0, 4, 8, 4, 4, 4, 4, true,  Mask4444_Nyb0, Mask4444_Nyb1, Mask4444_Nyb2, Mask4444_Nyb3);

		/** xRGB4444 **/
		fromLinear16_convert(xRGB4444, sBIT_xRGB4444, 4, 8, 4, 4, 4, 4, false, Mask4444_Nyb2, Mask4444_Nyb1, Mask4444_Nyb0);
		fromLinear16_convert(xBGR4444, sBIT_xRGB4444, 4, 8, 4, 4, 4, 4, true,  Mask4444_Nyb0, Mask4444_Nyb1, Mask4444_Nyb2);
		fromLinear16_convert(RGBx4444, sBIT_xRGB4444, 8, 4, 0, 4, 4, 4, false, Mask4444_Nyb3, Mask4444_Nyb2, Mask4444_Nyb1);
		fromLinear16_convert(BGRx4444, sBIT_xRGB4444, 0, 4, 8, 4, 4, 4, true,  Mask4444_Nyb1, Mask4444_Nyb2, Mask4444_Nyb3);

		/** RGB555 **/
		fromLinear16_convert(RGB555, sBIT_RGB555, 7, 6, 3, 5, 5, 5, false, Mask555_Hi5, Mask555_Mid5, Mask555_Lo5);
		fromLinear16_convert(BGR555, sBIT_RGB555, 3, 6, 7, 5, 5, 5, true,  Mask555_Lo5, Mask555_Mid5, Mask555_Hi5);

		/** RG88 **/
		case PXF_RG88: {
			// Components are already 8-bit, so we need to
			// expand them to DWORD and add the alpha channel.
			__m128i reg_zero = _mm_setzero_si128();
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				/* Process 8 pixels per iteration using SSE2. */
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Registers now contain: [00 00 RR GG]
					__m128i px0 = _mm_unpacklo_epi16(*xmm_src, reg_zero);
					__m128i px1 = _mm_unpackhi_epi16(*xmm_src, reg_zero);

					// Shift to [00 RR GG 00].
					px0 = _mm_slli_epi32(px0, 8);
					px1 = _mm_slli_epi32(px1, 8);

					// Apply the alpha channel.
					px0 = _mm_or_si128(px0, Mask32_A);
					px1 = _mm_or_si128(px1, Mask32_A);

					// Write the pixels to the destination image buffer.
					_mm_store_si128(xmm_dest, px0);
					_mm_store_si128(xmm_dest+1, px1);
				}

				/* Remaining pixels. */
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RG88_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				/* Next line. */
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			/* Set the sBIT metadata. */
			static const rp_image::sBIT_t sBIT_RG88 = {8,8,1,0,0};
			img->set_sBIT(&sBIT_RG88);
			break;
		}

		/** GR88 **/
		case PXF_GR88: {
			// Components are already 8-bit, so we need to
			// expand them to DWORD and add the alpha channel.
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				/* Process 8 pixels per iteration using SSE2. */
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
					__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

					// Registers now contain: [GG RR GG RR]
					__m128i px0 = _mm_unpacklo_epi16(*xmm_src, *xmm_src);
					__m128i px1 = _mm_unpackhi_epi16(*xmm_src, *xmm_src);

					// Mask off the low and high bytes.
					// Registers now contain: [00 RR GG 00]
					px0 = _mm_and_si128(px0, MaskGR88);
					px1 = _mm_and_si128(px1, MaskGR88);

					// Apply the alpha channel.
					px0 = _mm_or_si128(px0, Mask32_A);
					px1 = _mm_or_si128(px1, Mask32_A);

					// Write the pixels to the destination image buffer.
					_mm_store_si128(xmm_dest, px0);
					_mm_store_si128(xmm_dest+1, px1);
				}

				/* Remaining pixels. */
				for (; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RG88_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				/* Next line. */
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			/* Set the sBIT metadata. */
			static const rp_image::sBIT_t sBIT_RG88 = {8,8,1,0,0};
			img->set_sBIT(&sBIT_RG88);
			break;
		}

		default:
			assert(!"Pixel format not supported.");
			delete img;
			return nullptr;
	}

	// Image has been converted.
	return img;
}

}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

/*****************************************************************************
 * ROM Properties Page shell extension. (librptexture)                       *
 * ImageDecoder_Linear_neon16.cpp: Image decoding functions: Linear (16-bit) *
 * NEON-optimized version.                                                   *
 *                                                                           *
 * Copyright (c) 2016-2026 by David Korth.                                   *
 * SPDX-License-Identifier: GPL-2.0-or-later                                 *
 *****************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_Linear.hpp"

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// ARM NEON intrinsics
#include "arm_neon_aligned.h"

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

// FIXME: arm64 only!

/**
 * Templated function for 15/16-bit RGB conversion using NEON. (no alpha channel)
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
 * @param Rmask16	[in] 16-bit mask for the Red channel.
 * @param Gmask16	[in] 16-bit mask for the Green channel.
 * @param Bmask16	[in] 16-bit mask for the Blue channel.
 * @param img_buf	[in] 16-bit image buffer.
 * @param px_dest	[out] Destination image buffer.
 */
template<uint8_t Rshift_W, uint8_t Gshift_W, uint8_t Bshift_W,
	uint8_t Rbits, uint8_t Gbits, uint8_t Bbits, bool isBGR>
static inline void T_RGB16_neon(
	uint16_t Rmask16, uint16_t Gmask16, uint16_t Bmask16,
	const uint16_t *RESTRICT img_buf, uint32_t *RESTRICT px_dest)
{
	// Convert the 16-bit masks into 128-bit masks.
	const uint16x8_t Rmask = vdupq_n_u16(Rmask16);
	const uint16x8_t Gmask = vdupq_n_u16(Gmask16);
	const uint16x8_t Bmask = vdupq_n_u16(Bmask16);

	// Alpha mask
	const uint32x4_t Mask32_A  = vdupq_n_u32(0xFF000000);
	// Mask for the high byte for Green
	const uint16x8_t MaskG_Hi8 = vdupq_n_u16(0xFF00);

	// TODO: For xRGB4444, we should be able to optimize out some of the shifts.
	uint16x8_t src = vld1q_u16(img_buf);

	// Mask the G and B components and shift them into place.
	uint16x8_t sG = vshlq_n_u16(vandq_u16(src, Gmask), Gshift_W);
	uint16x8_t sB = vandq_u16(src, Bmask);
	// FIXME: This check is needed in order to prevent shift-out-of-range errors.
	if_constexpr (Bshift_W > 0 && Bshift_W <= 15) {
		sB = (isBGR)
			? vshrq_n_u16(sB, Bshift_W)
			: vshlq_n_u16(sB, Bshift_W);
	}
	sG = vorrq_u16(sG, vshrq_n_u16(sG, Gbits));
	sB = vorrq_u16(sB, vshrq_n_u16(sB, Bbits));

	// Combine G and B.
	if_constexpr (Gbits > 4) {
		// NOTE: G low byte has to be masked due to the shift.
		sB = vorrq_u16(sB, vandq_u16(sG, MaskG_Hi8));
	} else {
		// Not enough Gbits to need masking.
		// FIXME: If less than 4, need to shift multiple times.
		sB = vorrq_u16(sB, sG);
	}

	// Mask the R component and shift it into place.
	uint16x8_t sR = vandq_u16(src, Rmask);
	// FIXME: This check is needed in order to prevent shift-out-of-range errors.
	if_constexpr (Rshift_W > 0 && Rshift_W <= 15) {
		sR = (isBGR)
			? vshlq_n_u16(sR, Rshift_W)
			: vshrq_n_u16(sR, Rshift_W);
	}
	sR = vorrq_u16(sR, vshrq_n_u16(sR, Rbits));

	// Unpack R and GB into DWORDs.
	uint32x4x2_t px;
	px.val[0] = vorrq_u32(vreinterpretq_u32_u16(vzip1q_u16(sB, sR)), Mask32_A);
	px.val[1] = vorrq_u32(vreinterpretq_u32_u16(vzip2q_u16(sB, sR)), Mask32_A);
	vst1q_u32_x2_ex(px_dest, px, 128);
}

/**
 * Templated function for 15/16-bit RGB conversion using NEON. (with alpha channel)
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
 * @param Amask16	[in] 16-bit mask for the Alpha channel.
 * @param Rmask16	[in] 16-bit mask for the Red channel.
 * @param Gmask16	[in] 16-bit mask for the Green channel.
 * @param Bmask16	[in] 16-bit mask for the Blue channel.
 * @param img_buf	[in] 16-bit image buffer.
 * @param px_dest	[out] Destination image buffer.
 */
template<uint8_t Ashift_W, uint8_t Rshift_W, uint8_t Gshift_W, uint8_t Bshift_W,
	uint8_t Abits, uint8_t Rbits, uint8_t Gbits, uint8_t Bbits, bool isBGR>
static inline void T_ARGB16_neon(
	uint16_t Amask16, uint16_t Rmask16, uint16_t Gmask16, uint16_t Bmask16,
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

	// Convert the 16-bit masks into 128-bit masks.
	const uint16x8_t Amask = vdupq_n_u16(Amask16);
	const uint16x8_t Rmask = vdupq_n_u16(Rmask16);
	const uint16x8_t Gmask = vdupq_n_u16(Gmask16);
	const uint16x8_t Bmask = vdupq_n_u16(Bmask16);

	// Mask for the high byte for Green and Alpha.
	const uint16x8_t MaskAG_Hi8 = vdupq_n_u16(0xFF00);

	// TODO: For ARGB4444, we should be able to optimize out some of the shifts.
	uint16x8_t src = vld1q_u16(img_buf);

	// Mask the G and B components and shift them into place.
	uint16x8_t sG = vshlq_n_u16(vandq_u16(Gmask, src), Gshift_W);
	uint16x8_t sB = vandq_u16(Bmask, src);
	// FIXME: This check is needed in order to prevent shift-out-of-range errors.
	if_constexpr (Bshift_W > 0 && Bshift_W <= 15) {
		sB = (isBGR)
			? vshrq_n_u16(sB, Bshift_W)
			: vshlq_n_u16(sB, Bshift_W);
	}
	sG = vorrq_u16(sG, vshrq_n_u16(sG, Gbits));
	sB = vorrq_u16(sB, vshrq_n_u16(sB, Bbits));

	// Combine G and B.
	if_constexpr (Gbits > 4) {
		// NOTE: G low byte has to be masked due to the shift.
		sB = vorrq_u16(sB, vandq_u16(sG, MaskAG_Hi8));
	} else {
		// Not enough Gbits to need masking.
		// FIXME: If less than 4, need to shift multiple times.
		sB = vorrq_u16(sB, sG);
	}

	// Mask the R component and shift it into place.
	uint16x8_t sR = vandq_u16(src, Rmask);
	// FIXME: This check is needed in order to prevent shift-out-of-range errors.
	if_constexpr (Rshift_W > 0 && Rshift_W <= 15) {
		sR = (isBGR)
			? vshlq_n_u16(sR, Rshift_W)
			: vshrq_n_u16(sR, Rshift_W);
	}
	sR = vorrq_u16(sR, vshrq_n_u16(sR, Rbits));

	// Mask the A components, shift it into place, and combine with R.
	uint16x8_t sA;
	if_constexpr (Ashift_W == 16) {
		// 1555 alpha handling.
		// Using a bytewise comparison so we don't have to mask off the low byte.
		// NOTE: This comparison is *signed*. Amask must be 0x0080, and we're
		// checking for less than, which will match:
		// - < 0x00: 0x80-0xFF
		// - < 0x80: Nothing
		sA = vreinterpretq_u16_u8(vcltq_s8(
			vreinterpretq_s8_u16(src), vreinterpretq_s8_u16(Amask)));
		// Combine A and R.
		sR = vorrq_u16(sR, sA);
	} else if_constexpr (Ashift_W >= 17) {
		// 5551 alpha handling.
		// Amask has only bit 0 set for each word.
		// This will mask off bit 0, then compare it to the Amask value.
		// Any that have bit 0 set will be set to 0x00FF; otherwise, 0x0000.
		// This can then be shifted into place.
		sA = vshlq_n_u16(vreinterpretq_u16_u8(vceqq_u8(
			vreinterpretq_u8_u16(vandq_u16(src, Amask)),
				vreinterpretq_u8_u16(Amask))), 8);
		// Combine A and R.
		sR = vorrq_u16(sR, sA);
	} else {
		// Standard alpha handling.
		sA = vandq_u16(Amask, src);
		// FIXME: This check is needed in order to prevent shift-out-of-range errors.
		if_constexpr (Ashift_W > 0 && Ashift_W <= 15) {
			sA = vshlq_n_u16(sA, Ashift_W);
		}
		sA = vorrq_u16(sA, vshrq_n_u16(sA, Abits));
		// Combine A and R.
		// NOTE: A low byte has to be masked due to the shift.
		if (Abits > 4) {
			// NOTE: A low byte has to be masked due to the shift.
			sR = vorrq_u16(sR, vandq_u16(sA, MaskAG_Hi8));
		} else {
			// Not enough Abits to need masking.
			// FIXME: If less than 4, need to shift multiple times.
			sR = vorrq_u16(sR, sA);
		}
	}

	// Unpack AR and GB into DWORDs.
	uint32x4x2_t px;
	px.val[0] = vreinterpretq_u32_u16(vzip1q_u16(sB, sR));
	px.val[1] = vreinterpretq_u32_u16(vzip2q_u16(sB, sR));
	vst1q_u32_x2_ex(px_dest, px, 128);
}

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * NEON-optimized version.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromLinear16_neon(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static constexpr int bytespp = 2;

	// FIXME: Add support for these formats.
	// For now, redirect back to the C++ version.
	switch (px_format) {
		case PixelFormat::ARGB8332:
		case PixelFormat::RGB5A3:
		case PixelFormat::IA8:
		case PixelFormat::BGR555_PS1:
		case PixelFormat::BGR5A3:
		case PixelFormat::L16:
		case PixelFormat::A8L8:	// TODO: SSSE3
		case PixelFormat::L8A8:	// TODO: SSSE3
			return fromLinear16_cpp(px_format, width, height, img_buf, img_siz, stride);

		default:
			break;
	}

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (static_cast<size_t>(width) * static_cast<size_t>(height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (static_cast<size_t>(width) * static_cast<size_t>(height) * bytespp))
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

	// NOTE: NEON's vld1q_*() supports unaligned loads.
	// Not falling back to the C++ version if the width + src_stride_adj
	// is not a multiple of 8 pixels.

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

	// AND masks for 565 channels.
	static constexpr uint16_t Mask565_Hi5  = 0xF800;
	static constexpr uint16_t Mask565_Mid6 = 0x07E0;
	static constexpr uint16_t Mask565_Lo5  = 0x001F;

	// AND masks for 555 channels.
	static constexpr uint16_t Mask555_Hi5  = 0x7C00;
	static constexpr uint16_t Mask555_Mid5 = 0x03E0;
	static constexpr uint16_t Mask555_Lo5  = 0x001F;

	// AND masks for 4444 channels.
	static constexpr uint16_t Mask4444_Nyb3 = 0xF000;
	static constexpr uint16_t Mask4444_Nyb2 = 0x0F00;
	static constexpr uint16_t Mask4444_Nyb1 = 0x00F0;
	static constexpr uint16_t Mask4444_Nyb0 = 0x000F;

	// AND masks for 1555 channels.
	static constexpr uint16_t Cmp1555_A     = 0x0080;
	static constexpr uint16_t Mask1555_Hi5  = 0x7C00;
	static constexpr uint16_t Mask1555_Mid5 = 0x03E0;
	static constexpr uint16_t Mask1555_Lo5  = 0x001F;

	// AND masks for 5551 channels.
	static constexpr uint16_t Cmp5551_A     = 0x0101;
	static constexpr uint16_t Mask5551_Hi5  = 0xF800;
	static constexpr uint16_t Mask5551_Mid5 = 0x07C0;
	static constexpr uint16_t Mask5551_Lo5  = 0x003E;

	// Alpha mask
	static constexpr uint32_t Mask32_A = 0xFF000000;

	// GR88 mask
	static constexpr uint32_t MaskGR88 = 0x00FFFF00;

	// sBIT metadata
	static const rp_image::sBIT_t sBIT_RGB565   = {5,6,5,0,0};
	static const rp_image::sBIT_t sBIT_ARGB1555 = {5,5,5,0,1};
	static const rp_image::sBIT_t sBIT_xRGB4444 = {4,4,4,0,0};
	static const rp_image::sBIT_t sBIT_ARGB4444 = {4,4,4,0,4};
	static const rp_image::sBIT_t sBIT_RGB555   = {5,5,5,0,0};

	// Macro for 16-bit formats with no alpha channel.
#define fromLinear16_convert(fmt, sBIT, Rshift_W, Gshift_W, Bshift_W, Rbits, Gbits, Bbits, isBGR, Rmask16, Gmask16, Bmask16) \
		case PixelFormat::fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				/* Process 8 pixels per iteration using NEON. */ \
				unsigned int x = (unsigned int)width; \
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) { \
					T_RGB16_neon<Rshift_W, Gshift_W, Bshift_W, Rbits, Gbits, Bbits, isBGR>( \
						Rmask16, Gmask16, Bmask16, img_buf, px_dest); \
				} \
				\
				/* Remaining pixels. */ \
				for (; x > 0; x--) { \
					*px_dest = fmt##_to_ARGB32(*img_buf); \
					img_buf++; \
					px_dest++; \
				} \
				\
				/* Next line. */ \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT metadata. */ \
			img->set_sBIT(sBIT); \
		} break

	// Macro for 16-bit formats with an alpha channel.
#define fromLinear16A_convert(fmt, sBIT, Ashift_W, Rshift_W, Gshift_W, Bshift_W, Abits, Rbits, Gbits, Bbits, isBGR, Amask16, Rmask16, Gmask16, Bmask16) \
		case PixelFormat::fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				/* Process 8 pixels per iteration using NEON. */ \
				unsigned int x = (unsigned int)width; \
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) { \
					T_ARGB16_neon<Ashift_W, Rshift_W, Gshift_W, Bshift_W, Abits, Rbits, Gbits, Bbits, isBGR>( \
						Amask16, Rmask16, Gmask16, Bmask16, img_buf, px_dest); \
				} \
				\
				/* Remaining pixels. */ \
				for (; x > 0; x--) { \
					*px_dest = fmt##_to_ARGB32(*img_buf); \
					img_buf++; \
					px_dest++; \
				} \
				\
				/* Next line. */ \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT metadata. */ \
			img->set_sBIT(sBIT); \
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
		case PixelFormat::RG88: {
			// Components are already 8-bit, so we need to
			// expand them to DWORD and add the alpha channel.
			const uint32x4_t Mask32_A_vec = vdupq_n_u32(Mask32_A);
			const uint16x8_t reg_zero = vdupq_n_u16(0);
			for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
				// Process 8 pixels per iteration using NEON.
				unsigned int x = static_cast<unsigned int>(width);
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					uint16x8_t src = vld1q_u16(img_buf);

					// Registers now contain: [00 00 RR GG]
					uint32x4x2_t px;
					px.val[0] = vreinterpretq_u32_u16(vzip1q_u16(src, reg_zero));
					px.val[1] = vreinterpretq_u32_u16(vzip2q_u16(src, reg_zero));

					// Shift to [00 RR GG 00].
					px.val[0] = vshlq_n_u32(px.val[0], 8);
					px.val[1] = vshlq_n_u32(px.val[1], 8);

					// Apply the alpha channel.
					px.val[0] = vorrq_u32(px.val[0], Mask32_A_vec);
					px.val[1] = vorrq_u32(px.val[1], Mask32_A_vec);

					// Write the pixels to the destination image buffer.
					vst1q_u32_x2_ex(px_dest, px, 128);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = RG88_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT_RG88 = {8,8,1,0,0};
			img->set_sBIT(sBIT_RG88);
			break;
		}

		/** GR88 **/
		case PixelFormat::GR88: {
			// Components are already 8-bit, so we need to
			// expand them to DWORD and add the alpha channel.
			const uint32x4_t Mask32_A_vec = vdupq_n_u32(Mask32_A);
			const uint16x8_t MaskGR88_vec = vdupq_n_u32(MaskGR88);
			for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
				// Process 8 pixels per iteration using NEON.
				unsigned int x = static_cast<unsigned int>(width);
				for (; x > 7; x -= 8, px_dest += 8, img_buf += 8) {
					uint16x8_t src = vld1q_u16(img_buf);

					// Registers now contain: [GG RR GG RR]
					uint32x4x2_t px;
					px.val[0] = vreinterpretq_u32_u16(vzip1q_u16(src, src));
					px.val[1] = vreinterpretq_u32_u16(vzip2q_u16(src, src));

					// Mask off the low and high bytes.
					// Registers now contain: [00 RR GG 00]
					px.val[0] = vandq_u16(px.val[0], MaskGR88_vec);
					px.val[1] = vandq_u16(px.val[1], MaskGR88_vec);

					// Apply the alpha channel.
					px.val[0] = vorrq_u16(px.val[0], Mask32_A_vec);
					px.val[1] = vorrq_u16(px.val[1], Mask32_A_vec);

					// Write the pixels to the destination image buffer.
					vst1q_u32_x2_ex(px_dest, px, 128);
				}

				// Remaining pixels.
				for (; x > 0; x--) {
					*px_dest = RG88_to_ARGB32(*img_buf);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT_RG88 = {8,8,1,0,0};
			img->set_sBIT(sBIT_RG88);
			break;
		}

		default:
			assert(!"Pixel format not supported.");
			return nullptr;
	}

	// Image has been converted.
	return img;
}

} }

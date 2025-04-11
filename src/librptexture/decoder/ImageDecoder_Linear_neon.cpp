/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear.cpp: Image decoding functions: Linear               *
 * NEON-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_Linear.hpp"

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// ARM NEON intrinsics
#include <arm_neon.h>

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

#if defined(RP_CPU_ARM64)
static constexpr size_t VEC_LEN_U8 = 16;
static constexpr size_t VEC_LEN_U32 = 4;
typedef uint8x16_t uint8xVTBL_t;
typedef uint32x4_t uint32xVTBL_t;
#define vld1VTBL_u8  vld1q_u8
#define vld1VTBL_u32 vld1q_u32
#define vst1VTBL_u8  vst1q_u8
#define vst1VTBL_u32 vst1q_u32
#elif defined(RP_CPU_ARM)
static constexpr size_t VEC_LEN_U8 = 8;
static constexpr size_t VEC_LEN_U32 = 2;
typedef uint8x8_t uint8xVTBL_t;
typedef uint32x2_t uint32xVTBL_t;
#define vld1VTBL_u8  vld1_u8
#define vld1VTBL_u32 vld1_u32
#define vst1VTBL_u8  vst1_u8
#define vst1VTBL_u32 vst1_u32
#else
#  error Unsupported CPU?
#endif

/**
 * Convert a linear 32-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromLinear32_neon(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static constexpr int bytespp = 4;

	// FIXME: Add support for these formats.
	// For now, redirect back to the C++ version.
	switch (px_format) {
		case PixelFormat::A2R10G10B10:
		case PixelFormat::A2B10G10R10:
		case PixelFormat::RGB9_E5:
			return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);

		default:
			break;
	}

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ImageSizeCalc::T_calcImageSize(width, height, bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ImageSizeCalc::T_calcImageSize(width, height, bytespp))
	{
		return nullptr;
	}

	if (px_format == PixelFormat::BGR888_ABGR7888) {
		// Not supported right now.
		// Use the C++ version.
		return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);
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
	} else {
		// Calculate stride and make sure it's a multiple of 16.
		// Exception: If the pixel format is PixelFormat::Host_ARGB32,
		// we're using memcpy(), so alignment isn't required.
		stride = width * bytespp;
		if (unlikely((stride % 16 != 0) && px_format != PixelFormat::Host_ARGB32)) {
			// Unaligned stride.
			// Use the C++ version.
			return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);
		}
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	if (px_format == PixelFormat::Host_ARGB32) {
		// Host-endian ARGB32.
		// We can directly copy the image data without conversions.
		if (stride == img->stride()) {
			// Stride is identical. Copy the whole image all at once.
			memcpy(img->bits(), img_buf, ImageSizeCalc::T_calcImageSize(stride, height));
		} else {
			// Stride is not identical. Copy each scanline.
			const int dest_stride = img->stride() / sizeof(uint32_t);
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			const unsigned int copy_len = static_cast<unsigned int>(width * bytespp);
			for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
				memcpy(px_dest, img_buf, copy_len);
				img_buf += (stride / bytespp);
				px_dest += dest_stride;
			}
		}
		// Set the sBIT metadata.
		static const rp_image::sBIT_t sBIT_A32 = {8,8,8,0,8};
		img->set_sBIT(&sBIT_A32);
		return img;
	}

	// NEON-optimized version based on the SSSE3 code here:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266
	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

	// Determine the byte shuffle mask.
	uint8xVTBL_t shuf_mask;
	enum {
		MASK_NONE,	// for ARGB formats
		MASK_ALPHA,	// for xRGB formats
		MASK_ALPHA_B,	// for GR formats
	} img_mask_type;
	switch (px_format) {
		case PixelFormat::Host_ARGB32:
			assert(!"ARGB32 is handled separately.");
			return nullptr;
		case PixelFormat::Host_xRGB32: {
			// TODO: Only apply the alpha mask instead of shuffling.
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_Host_xRGB32 = {{
				0,1,2,3, 4,5,6,7,
			#ifdef RP_CPU_ARM64
				8,9,10,11, 12,13,14,15
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_Host_xRGB32.data());
			img_mask_type = MASK_ALPHA;
			break;
		}

		case PixelFormat::Host_RGBA32:
		case PixelFormat::Host_RGBx32: {
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_Host_RGBA32 = {{
				1,2,3,0, 5,6,7,4,
			#ifdef RP_CPU_ARM64
				9,10,11,8, 13,14,15,12
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_Host_RGBA32.data());
			img_mask_type = (px_format == PixelFormat::Host_RGBA32) ? MASK_NONE : MASK_ALPHA;
			break;
		}

		case PixelFormat::Swap_ARGB32:
		case PixelFormat::Swap_xRGB32: {
			// TODO: Could optimize by using vrev instead of vtbl.
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_Swap_ARGB32 = {{
				3,2,1,0, 7,6,5,4,
			#ifdef RP_CPU_ARM64
				11,10,9,8, 15,14,13,12
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_Swap_ARGB32.data());
			img_mask_type = (px_format == PixelFormat::Swap_ARGB32) ? MASK_NONE : MASK_ALPHA;
			break;
		}

		case PixelFormat::Swap_RGBA32:
		case PixelFormat::Swap_RGBx32: {
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_Swap_RGBA32 = {{
				2,1,0,3, 6,5,4,7,
			#ifdef RP_CPU_ARM64
				10,9,8,11, 14,13,12,15
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_Swap_RGBA32.data());
			img_mask_type = (px_format == PixelFormat::Swap_RGBA32) ? MASK_NONE : MASK_ALPHA;
			break;
		}

		// TODO: Verify this?
		case PixelFormat::G16R16: {
			// NOTE: Truncates to G8R8.
			// NOTE: vld1q_u8() doesn't appear to have an equivalent to "-1" in pshufb,
			// so use an "alpha mask" instead.
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_G16R16 = {{
				255,3,1,255, 255,7,5,255,
			#ifdef RP_CPU_ARM64
				255,11,9,255, 255,15,13,255
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_G16R16.data());
			img_mask_type = MASK_ALPHA_B;
			break;
		}

		case PixelFormat::RABG8888: {
			static const array<uint8_t, VEC_LEN_U8> shuf_mask_RABG8888 = {{
				1,0,3,2, 5,4,7,6,
			#ifdef RP_CPU_ARM64
				9,8,11,10, 13,12,15,14
			#endif /* RP_CPU_ARM64 */
			}};
			shuf_mask = vld1VTBL_u8(shuf_mask_RABG8888.data());
			img_mask_type = MASK_NONE;
			break;
		}

		default:
			assert(!"Main pixels: Unsupported 32-bit pixel format.");
			return nullptr;
	}

	if (img_mask_type == MASK_NONE) {
		// Image has a valid alpha channel.
		for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
			// Process 16 pixels per iteration using NEON.
			unsigned int x = static_cast<unsigned int>(width);
			for (; x > 15; x -= 16, px_dest += 16, img_buf += 16) {
#if defined(RP_CPU_ARM64)
				uint32x4x4_t sa = vld4q_u32(img_buf);

				sa.val[0] = vqtbl1q_u8(sa.val[0], shuf_mask);
				sa.val[1] = vqtbl1q_u8(sa.val[1], shuf_mask);
				sa.val[2] = vqtbl1q_u8(sa.val[2], shuf_mask);
				sa.val[3] = vqtbl1q_u8(sa.val[3], shuf_mask);

				vst4q_u32(px_dest, sa);
#elif defined(RP_CPU_ARM)
				uint32x2x4_t sa = vld4_u32(&img_buf[0]);
				uint32x2x4_t sb = vld4_u32(&img_buf[8]);

				sa.val[0] = vtbl1_u8(sa.val[0], shuf_mask);
				sa.val[1] = vtbl1_u8(sa.val[1], shuf_mask);
				sa.val[2] = vtbl1_u8(sa.val[2], shuf_mask);
				sa.val[3] = vtbl1_u8(sa.val[3], shuf_mask);
				sb.val[0] = vtbl1_u8(sb.val[0], shuf_mask);
				sb.val[1] = vtbl1_u8(sb.val[1], shuf_mask);
				sb.val[2] = vtbl1_u8(sb.val[2], shuf_mask);
				sb.val[3] = vtbl1_u8(sb.val[3], shuf_mask);

				vst4_u32(&px_dest[0], sa);
				vst4_u32(&px_dest[8], sb);
#endif
			}

			// Remaining pixels.
			if (x > 0) {
			switch (px_format) {
				case PixelFormat::Host_RGBA32:
					// Host-endian RGBA32.
					// Pixel copy is needed, with shifting.
					for (; x > 0; x--) {
						*px_dest = (*img_buf >> 8) | (*img_buf << 24);
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::Swap_ARGB32:
					// Byteswapped ARGB32.
					// Pixel copy is needed, with byteswapping.
					for (; x > 0; x--) {
						*px_dest = __swab32(*img_buf);
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::Swap_RGBA32:
					// Byteswapped ABGR32.
					// Pixel copy is needed, with shifting.
					for (; x > 0; x--) {
						const uint32_t px = __swab32(*img_buf);
						*px_dest = (px >> 8) | (px << 24);
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::G16R16:
					// NOTE: Truncates to G8R8.
					for (; x > 0; x--) {
						*px_dest = G16R16_to_ARGB32(*img_buf);
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::RABG8888:
					// VTF "ARGB8888", which is actually RABG.
					// TODO: This might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
					// TODO: Verify on big-endian.
					// TODO: Use argb32_t?
					for (; x > 0; x--) {
						const uint32_t px = le32_to_cpu(*img_buf);

						*px_dest  = (px >> 8) & 0xFF;
						*px_dest |= (px & 0xFF) << 8;
						*px_dest |= (px << 8) & 0xFF000000;
						*px_dest |= (px >> 8) & 0x00FF0000;

						img_buf++;
						px_dest++;
					}
					break;

				default:
					assert(!"Remaining pixels: Unsupported 32-bit alpha pixel format.");
					return nullptr;
			} }

			// Next line.
			img_buf += src_stride_adj;
			px_dest += dest_stride_adj;
		}

		// Set the sBIT metadata.
		static const rp_image::sBIT_t sBIT_A32 = {8,8,8,0,8};
		img->set_sBIT(&sBIT_A32);
	} else {
		// xRGB images don't have an alpha channel.
		static const array<uint32_t, VEC_LEN_U32> or_mask_noAlpha_u32 = {{
			0xFF000000, 0xFF000000,
#ifdef RP_CPU_ARM64
			0xFF000000, 0xFF000000
#endif /* RP_CPU_ARM64 */
		}};
		// GR images don't have a blue channel.
		static const array<uint32_t, VEC_LEN_U32> and_mask_GR_u32 = {{
			0xFFFFFF00, 0xFFFFFF00,
#ifdef RP_CPU_ARM64
			0xFFFFFF00, 0xFFFFFF00
#endif /* RP_CPU_ARM64 */
		}};

		uint32xVTBL_t or_mask = vld1VTBL_u32(or_mask_noAlpha_u32.data());
		// NOTE: Only used for GR formats.
		uint32xVTBL_t and_mask = vld1VTBL_u32(and_mask_GR_u32.data());

		for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
			// Process 16 pixels per iteration using SSSE3.
			unsigned int x = static_cast<unsigned int>(width);
			for (; x > 15; x -= 16, px_dest += 16, img_buf += 16) {
#if defined(RP_CPU_ARM64)
				uint32x4x4_t sa = vld4q_u32(img_buf);

				sa.val[0] = vqtbl1q_u8(sa.val[0], shuf_mask);
				sa.val[1] = vqtbl1q_u8(sa.val[1], shuf_mask);
				sa.val[2] = vqtbl1q_u8(sa.val[2], shuf_mask);
				sa.val[3] = vqtbl1q_u8(sa.val[3], shuf_mask);

				sa.val[0] = vorrq_u32(sa.val[0], or_mask);
				sa.val[1] = vorrq_u32(sa.val[1], or_mask);
				sa.val[2] = vorrq_u32(sa.val[2], or_mask);
				sa.val[3] = vorrq_u32(sa.val[3], or_mask);

				if (unlikely(img_mask_type == MASK_ALPHA_B)) {
					sa.val[0] = vandq_u32(sa.val[0], and_mask);
					sa.val[1] = vandq_u32(sa.val[1], and_mask);
					sa.val[2] = vandq_u32(sa.val[2], and_mask);
					sa.val[3] = vandq_u32(sa.val[3], and_mask);
				}

				vst4q_u32(px_dest, sa);
#elif defined(RP_CPU_ARM)
				uint32x2x4_t sa = vld4_u32(&img_buf[0]);
				uint32x2x4_t sb = vld4_u32(&img_buf[8]);

				sa.val[0] = vtbl1_u8(sa.val[0], shuf_mask);
				sa.val[1] = vtbl1_u8(sa.val[1], shuf_mask);
				sa.val[2] = vtbl1_u8(sa.val[2], shuf_mask);
				sa.val[3] = vtbl1_u8(sa.val[3], shuf_mask);
				sb.val[0] = vtbl1_u8(sb.val[0], shuf_mask);
				sb.val[1] = vtbl1_u8(sb.val[1], shuf_mask);
				sb.val[2] = vtbl1_u8(sb.val[2], shuf_mask);
				sb.val[3] = vtbl1_u8(sb.val[3], shuf_mask);

				sa.val[0] = vorr_u8(sa.val[0], or_mask);
				sa.val[1] = vorr_u8(sa.val[1], or_mask);
				sa.val[2] = vorr_u8(sa.val[2], or_mask);
				sa.val[3] = vorr_u8(sa.val[3], or_mask);
				sb.val[0] = vorr_u8(sb.val[0], or_mask);
				sb.val[1] = vorr_u8(sb.val[1], or_mask);
				sb.val[2] = vorr_u8(sb.val[2], or_mask);
				sb.val[3] = vorr_u8(sb.val[3], or_mask);

				if (unlikely(img_mask_type == MASK_ALPHA_B)) {
					sa.val[0] = vand_u32(sa.val[0], and_mask);
					sa.val[1] = vand_u32(sa.val[1], and_mask);
					sa.val[2] = vand_u32(sa.val[2], and_mask);
					sa.val[3] = vand_u32(sa.val[3], and_mask);
					sb.val[0] = vand_u32(sb.val[0], and_mask);
					sb.val[1] = vand_u32(sb.val[1], and_mask);
					sb.val[2] = vand_u32(sb.val[2], and_mask);
					sb.val[3] = vand_u32(sb.val[3], and_mask);
				}

				vst4_u32(&px_dest[0], sa);
				vst4_u32(&px_dest[8], sb);
#endif
			}

			// Remaining pixels.
			if (x > 0) {
			switch (px_format) {
				case PixelFormat::Host_xRGB32:
					// Host-endian XRGB32.
					// Pixel copy is needed, with alpha channel masking.
					for (; x > 0; x--) {
						*px_dest = *img_buf | 0xFF000000;
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::Host_RGBx32:
					// Host-endian RGBx32.
					// Pixel copy is needed, with a right shift.
					for (; x > 0; x--) {
						*px_dest = (*img_buf >> 8) | 0xFF000000;
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::Swap_xRGB32:
					// Byteswapped XRGB32.
					// Pixel copy is needed, with byteswapping and alpha channel masking.
					for (; x > 0; x--) {
						*px_dest = __swab32(*img_buf) | 0xFF000000;
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::Swap_RGBx32:
					// Byteswapped RGBx32.
					// Pixel copy is needed, with byteswapping and a right shift.
					for (; x > 0; x--) {
						*px_dest = (__swab32(*img_buf) >> 8) | 0xFF000000;
						img_buf++;
						px_dest++;
					}
					break;

				case PixelFormat::G16R16:
					// G16R16.
					for (; x > 0; x--) {
						*px_dest = G16R16_to_ARGB32(le32_to_cpu(*img_buf));
						img_buf++;
						px_dest++;
					}
					break;

				default:
					assert(!"Unsupported 32-bit no-alpha pixel format.");
					return nullptr;
			} }

			// Next line.
			img_buf += src_stride_adj;
			px_dest += dest_stride_adj;
		}

		// Set the sBIT metadata.
		if (unlikely(px_format == PixelFormat::G16R16)) {
			static const rp_image::sBIT_t sBIT_G16R16 = {8,8,1,0,0};
			img->set_sBIT(&sBIT_G16R16);
		} else {
			static const rp_image::sBIT_t sBIT_x32 = {8,8,8,0,0};
			img->set_sBIT(&sBIT_x32);
		}
	}

	// Image has been converted.
	return img;
}

} }

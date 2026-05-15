/*****************************************************************************
 * ROM Properties Page shell extension. (librptexture)                       *
 * ImageDecoder_Linear_neon24.cpp: Image decoding functions: Linear (24-bit) *
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

// ARM NEON intrinsics
#include "arm_neon_aligned.h"

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

// FIXME: arm64 only!

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * NEON-optimized version.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromLinear24_neon(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	static constexpr int bytespp = 3;

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) * bytespp))
	{
		return nullptr;
	}

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of bytes we need to
		// add to the end of each line to get to the next row.
		assert(stride >= (width * bytespp));
		if (unlikely(stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		// NOTE: Byte addressing, so keep it in units of bytespp.
		src_stride_adj = stride - (width * bytespp);
	}

	// NOTE: vld1q *can* handle unaligned access, so image stride doesn't
	// need to be a multiple of 16, but it may be slower. (Still likely
	// to be faster than the fallback cpp decoder, though...)

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	argb32_t *px_dest = static_cast<argb32_t*>(img->bits());

	// Planar ARGB vectors:
	// - 0 == B
	// - 1 == G
	// - 2 == R
	// - 3 == A (0xFF)
	// NOTE: Declare the uint8x16x4_t within the loop so gcc will properly
	// share the registers between the vld3q_u8 and the vst1q_u8_x4, so it
	// doesn't have to copy R/G/B to a second set.
	const uint32x4_t alpha_mask = vdupq_n_u32(0xFF000000);
	uint8x16_t shuf_mask;

	// Determine the shuffle mask based on pixel format.
	bool isBGR;
	switch (px_format) {
		case PixelFormat::RGB888: {
			static const array<uint8_t, 16> shuf_mask_u8 = {{
				0,1,2,255, 3,4,5,255,
				6,7,8,255, 9,10,11,255
			}};
			shuf_mask = vld1q_u8(shuf_mask_u8.data());
			isBGR = false;
			break;
		}

		case PixelFormat::BGR888: {
			static const array<uint8_t, 16> shuf_mask_u8 = {{
				2,1,0,255, 5,4,3,255,
				8,7,6,255, 11,10,9,255
			}};
			shuf_mask = vld1q_u8(shuf_mask_u8.data());
			isBGR = true;
			break;
		}

		default:
			assert(!"Unsupported 24-bit pixel format.");
			return nullptr;
	}

	// Convert one line at a time. (24-bit -> ARGB32)
	for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
		// Convert 16 pixels at a time. (48 source bytes)
		unsigned int x = static_cast<unsigned int>(width);
		for (; x > 15; x -= 16) {
			uint8x16x3_t rgb = vld1q_u8_x3(img_buf);

			uint32x4x4_t argb;
			argb.val[0] = vreinterpretq_u32_u8(vqtbl1q_u8(rgb.val[0], shuf_mask));
			argb.val[0] = vorrq_u32(argb.val[0], alpha_mask);

			argb.val[1] = vreinterpretq_u32_u8(vqtbl1q_u8(
				vextq_u8(rgb.val[0], rgb.val[1], 12), shuf_mask));
			argb.val[1] = vorrq_u32(argb.val[1], alpha_mask);

			argb.val[2] = vreinterpretq_u32_u8(vqtbl1q_u8(
				vextq_u8(rgb.val[1], rgb.val[2], 8), shuf_mask));
			argb.val[2] = vorrq_u32(argb.val[2], alpha_mask);

			argb.val[3] = vreinterpretq_u32_u8(vqtbl1q_u8(vextq_u8(
				rgb.val[2], rgb.val[3], 4), shuf_mask));
			argb.val[3] = vorrq_u32(argb.val[3], alpha_mask);

			vst1q_u32_x4_ex(reinterpret_cast<uint32_t*>(px_dest), argb, 128);
			img_buf += (16 * 3);
			px_dest += 16;
		}

		// Remaining pixels
		if (!isBGR) {
			for (; x > 0; x--) {
				px_dest->b = img_buf[0];
				px_dest->g = img_buf[1];
				px_dest->r = img_buf[2];
				px_dest->a = 0xFF;
				img_buf += 3;
				px_dest++;
			}
		} else {
			for (; x > 0; x--) {
				px_dest->b = img_buf[2];
				px_dest->g = img_buf[1];
				px_dest->r = img_buf[0];
				px_dest->a = 0xFF;
				img_buf += 3;
				px_dest++;
			}
		}

		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(sBIT);

	// Image has been converted.
	return img;
}

} }

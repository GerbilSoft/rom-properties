/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear.cpp: Image decoding functions: Linear               *
 * SSSE3-optimized version.                                                *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_Linear.hpp"

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// SSSE3 intrinsics
#include <emmintrin.h>
#include <tmmintrin.h>

namespace LibRpTexture { namespace ImageDecoder {

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
rp_image_ptr fromLinear24_ssse3(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 3;

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
		if (unlikely(stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		} else if (unlikely(stride % 16 != 0)) {
			// Unaligned stride.
			// Use the C++ version.
			return fromLinear24_cpp(px_format, width, height, img_buf, img_siz, stride);
		}
		// NOTE: Byte addressing, so keep it in units of bytespp.
		src_stride_adj = stride - (width * bytespp);
	} else {
		// Calculate stride and make sure it's a multiple of 16.
		stride = width * bytespp;
		if (unlikely(stride % 16 != 0)) {
			// Unaligned stride.
			// Use the C++ version.
			return fromLinear24_cpp(px_format, width, height, img_buf, img_siz, stride);
		}
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
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
		case PixelFormat::RGB888:
			shuf_mask = _mm_setr_epi8(0,1,2,-1, 3,4,5,-1, 6,7,8,-1, 9,10,11,-1);
			break;
		case PixelFormat::BGR888:
			shuf_mask = _mm_setr_epi8(2,1,0,-1, 5,4,3,-1, 8,7,6,-1, 11,10,9,-1);
			break;
		default:
			assert(!"Unsupported 24-bit pixel format.");
			return nullptr;
	}

	for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
		// Process 16 pixels per iteration using SSSE3.
		unsigned int x = static_cast<unsigned int>(width);
		for (; x > 15; x -= 16, px_dest += 16, img_buf += 16*3) {
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

			__m128i sa = _mm_load_si128(&xmm_src[0]);
			__m128i sb = _mm_load_si128(&xmm_src[1]);
			__m128i sc = _mm_load_si128(&xmm_src[2]);

			__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[0], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sb, sa, 12), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[1], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sb, 8), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[2], val);
			val = _mm_shuffle_epi8(_mm_alignr_epi8(sc, sc, 4), shuf_mask);
			val = _mm_or_si128(val, alpha_mask);
			_mm_store_si128(&xmm_dest[3], val);
		}

		// Remaining pixels.
		if (x > 0) {
		switch (px_format) {
			case PixelFormat::RGB888:
				for (; x > 0; x--, px_dest++, img_buf += 3) {
					px_dest->b = img_buf[0];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[2];
					px_dest->a = 0xFF;
				}
				break;

			case PixelFormat::BGR888:
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
		} }

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
rp_image_ptr fromLinear32_ssse3(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	ASSERT_ALIGNMENT(16, img_buf);
	static const int bytespp = 4;

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

	// SSSE3-optimized version based on:
	// - https://stackoverflow.com/questions/2973708/fast-24-bit-array-32-bit-array-conversion
	// - https://stackoverflow.com/a/2974266
	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

	// Determine the byte shuffle mask.
	__m128i shuf_mask;
	bool has_alpha;
	switch (px_format) {
		case PixelFormat::Host_ARGB32:
			assert(!"ARGB32 is handled separately.");
			return nullptr;
		case PixelFormat::Host_xRGB32:
			// TODO: Only apply the alpha mask instead of shuffling.
			shuf_mask = _mm_setr_epi8(0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15);
			has_alpha = false;
			break;

		case PixelFormat::Host_RGBA32:
		case PixelFormat::Host_RGBx32:
			shuf_mask = _mm_setr_epi8(1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12);
			has_alpha = (px_format == PixelFormat::Host_RGBA32);
			break;

		case PixelFormat::Swap_ARGB32:
		case PixelFormat::Swap_xRGB32:
			shuf_mask = _mm_setr_epi8(3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);
			has_alpha = (px_format == PixelFormat::Swap_ARGB32);
			break;

		case PixelFormat::Swap_RGBA32:
		case PixelFormat::Swap_RGBx32:
			shuf_mask = _mm_setr_epi8(2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15);
			has_alpha = (px_format == PixelFormat::Swap_RGBA32);
			break;

		case PixelFormat::G16R16:
			// NOTE: Truncates to G8R8.
			shuf_mask = _mm_setr_epi8(-1,3,1,-1, -1,7,5,-1, -1,11,9,-1, -1,15,13,-1);
			has_alpha = false;
			break;

		case PixelFormat::RABG8888:
			shuf_mask = _mm_setr_epi8(1,0,3,2, 5,4,7,6, 9,8,11,10, 13,12,15,14);
			has_alpha = true;
			break;

		default:
			assert(!"Main pixels: Unsupported 32-bit pixel format.");
			return nullptr;
	}

	if (has_alpha) {
		// Image has a valid alpha channel.
		for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
			// Process 16 pixels per iteration using SSSE3.
			unsigned int x = static_cast<unsigned int>(width);
			for (; x > 15; x -= 16, px_dest += 16, img_buf += 16) {
				const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
				__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

				__m128i sa = _mm_load_si128(&xmm_src[0]);
				__m128i sb = _mm_load_si128(&xmm_src[1]);
				__m128i sc = _mm_load_si128(&xmm_src[2]);
				__m128i sd = _mm_load_si128(&xmm_src[3]);

				__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
				_mm_store_si128(&xmm_dest[0], val);

				val = _mm_shuffle_epi8(sb, shuf_mask);
				_mm_store_si128(&xmm_dest[1], val);

				val = _mm_shuffle_epi8(sc, shuf_mask);
				_mm_store_si128(&xmm_dest[2], val);

				val = _mm_shuffle_epi8(sd, shuf_mask);
				_mm_store_si128(&xmm_dest[3], val);
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
		// Image does not have an alpha channel.
		__m128i alpha_mask = _mm_setr_epi8(0,0,0,-1, 0,0,0,-1, 0,0,0,-1, 0,0,0,-1);

		for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
			// Process 16 pixels per iteration using SSSE3.
			unsigned int x = static_cast<unsigned int>(width);
			for (; x > 15; x -= 16, px_dest += 16, img_buf += 16) {
				const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
				__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);

				__m128i sa = _mm_load_si128(&xmm_src[0]);
				__m128i sb = _mm_load_si128(&xmm_src[1]);
				__m128i sc = _mm_load_si128(&xmm_src[2]);
				__m128i sd = _mm_load_si128(&xmm_src[3]);

				__m128i val = _mm_shuffle_epi8(sa, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(&xmm_dest[0], val);

				val = _mm_shuffle_epi8(sb, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(&xmm_dest[1], val);

				val = _mm_shuffle_epi8(sc, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(&xmm_dest[2], val);

				val = _mm_shuffle_epi8(sd, shuf_mask);
				val = _mm_or_si128(val, alpha_mask);
				_mm_store_si128(&xmm_dest[3], val);
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

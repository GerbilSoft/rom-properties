/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear.cpp: Image decoding functions: Linear               *
 * Standard version. (C++ code only)                                       *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_Linear.hpp"

// librptexture
#include "img/rp_image.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a linear monochrome image to rp_image.
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= (w*h)/8]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromLinearMono(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) / 8));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) / 8))
	{
		return {};
	}

	// Source stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of bytes we need to
		// add to the end of each line to get to the next row.
		src_stride_adj = stride - ((width / 8) + ((width % 8) > 0));
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return {};
	}
	const int dest_stride_adj = img->stride() - img->width();

	// Set up a default monochrome palette.
	uint32_t *palette = img->palette();
	palette[0] = 0xFFFFFFFFU;	// white
	palette[1] = 0xFF000000U;	// black
	img->set_tr_idx(-1);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (monochrome -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
		for (int x = width; x > 0; x -= 8) {
			uint8_t pxMono = *img_buf++;

			// For images where width is not a multiple of 8,
			// we'll discard the remaining bits in the last byte.
			const unsigned int max_bit = (x >= 8) ? 8 : static_cast<unsigned int>(x);

			// TODO: Unroll this loop?
			for (unsigned int bit = max_bit; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel
				*px_dest = (pxMono >> 7);
				pxMono <<= 1;
			}
		}
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	// NOTE: Setting the grayscale value, though we're
	// not saving grayscale PNGs at the moment.
	static const rp_image::sBIT_t sBIT = {1,1,1,1,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 2-bpp grayscale image to rp_image.
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= (w*h)/4]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromLinearGray2bpp(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) / 4));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) / 4))
	{
		return {};
	}

	// Source stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of bytes we need to
		// add to the end of each line to get to the next row.
		src_stride_adj = stride - ((width / 4) + ((width % 4) > 0));
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return {};
	}
	const int dest_stride_adj = img->stride() - img->width();

	// Set up a grayscale palette.
	// NOTE: Using $00/$80/$C0/$FF.
	// CGA-style $00/$55/$AA/$FF is too dark.
	uint32_t *palette = img->palette();
	palette[0] = 0xFFFFFFFFU;	// white
	palette[1] = 0xFFC0C0C0U;	// light gray
	palette[1] = 0xFF808080U;	// dark gray
	palette[3] = 0xFF000000U;	// black
	img->set_tr_idx(-1);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (2-bpp -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	for (unsigned int y = static_cast<unsigned int>(height); y > 0; y--) {
		for (int x = width; x > 0; x -= 4) {
			uint8_t px2bpp = *img_buf++;

			// For images where width is not a multiple of 4,
			// we'll discard the remaining bits in the last byte.
			const unsigned int max_bit = (x >= 4) ? 4 : static_cast<unsigned int>(x);

			// TODO: Unroll this loop?
			for (unsigned int bit = max_bit; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel
				*px_dest = (px2bpp >> 6);
				px2bpp <<= 2;
			}
		}
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	// NOTE: Setting the grayscale value, though we're
	// not saving grayscale PNGs at the moment.
	static const rp_image::sBIT_t sBIT = {2,2,2,2,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

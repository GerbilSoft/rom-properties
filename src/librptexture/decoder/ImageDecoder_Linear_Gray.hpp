/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear_Gray.hpp: Image decoding functions: Linear          *
 * Specifically, monochrome and grayscale formats.                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a linear monochrome image to rp_image.
 *
 * 0 == white; 1 == black
 *
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= (w*h)/8]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromLinearMono(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

/**
 * Convert a linear 2-bpp grayscale image to rp_image.
 *
 * 0 == white; 3 == black
 *
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= (w*h)/4]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromLinearGray2bpp(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

/**
 * Convert a linear monochrome image to rp_image.
 *
 * Windows icons are handled a bit different compared to "regular" monochrome images:
 * - Two images are required: icon and mask
 * - Transparency is supported using the mask.
 * - 0 == black; 1 == white
 *
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= ((w*h)/8)]
 * @param mask_buf	[in] Mask buffer
 * @param mask_siz	[in] Size of mask buffer [must be == img_siz]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
ATTR_ACCESS_SIZE(read_only, 5, 6)
rp_image_ptr fromLinearMono_WinIcon(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const uint8_t *RESTRICT mask_buf, size_t mask_siz, int stride = 0);

} }

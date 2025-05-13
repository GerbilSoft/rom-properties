/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_DC.hpp: Image decoding functions: Dreamcast                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Dreamcast square twiddled 16-bit image to rp_image.
 * @param px_format 16-bit pixel format.
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf 16-bit image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDreamcastSquareTwiddled16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a Dreamcast vector-quantized image to rp_image.
 * @param px_format Palette pixel format.
 * @param smallVQ If true, handle this image as SmallVQ.
 * @param hasMipmaps If true, the image has mipmaps. (Needed for SmallVQ.)
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf VQ image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 1024*2; for SmallVQ, 64*2, 256*2, or 512*2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 6, 7)
rp_image_ptr fromDreamcastVQ16(PixelFormat px_format,
	bool smallVQ, bool hasMipmaps,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const uint16_t *RESTRICT pal_buf, size_t pal_siz);

/**
 * Get the number of palette entries for Dreamcast SmallVQ textures.
 * This version is for textures without mipmaps.
 * @param width Texture width.
 * @return Number of palette entries.
 */
static inline CONSTEXPR_MULTILINE int calcDreamcastSmallVQPaletteEntries_NoMipmaps(int width)
{
	if (width <= 16) {
		return 8*4;
	} else if (width <= 32) {
		return 32*4;
	} else if (width <= 64) {
		return 128*4;
	} else {
		return 256*4;
	}
}

/**
 * Get the number of palette entries for Dreamcast SmallVQ textures.
 * This version is for textures with mipmaps.
 * @param width Texture width.
 * @return Number of palette entries.
 */
static inline CONSTEXPR_MULTILINE int calcDreamcastSmallVQPaletteEntries_WithMipmaps(int width)
{
	if (width <= 16) {
		return 16*4;
	} else if (width <= 32) {
		return 64*4;
	} else if (width <= 64) {
		return 128*4;
	} else {
		return 256*4;
	}
}

} }

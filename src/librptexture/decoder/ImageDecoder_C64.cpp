/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_C64.cpp: Image decoding functions: Commodore 64            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_C64.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Commodore 64 multi-color sprite (12x21) to rp_image.
 * A default 4-color grayscale palette is used.
 * @param img_buf CI4 image buffer
 * @param img_siz Size of image data [must be >= 12*21/4 (63)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromC64_MultiColor_Sprite(
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(img_siz >= 12*21/4);
	if (!img_buf || img_siz < 12*21/4) {
		return nullptr;
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(24, 21, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Set up a grayscale palette.
	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.
	// NOTE: Not marking color 0 as transparent, since it would
	// usually result in the icon being unreadable.
	uint32_t *const palette = img->palette();
	palette[0] = 0xFFFFFFFF;	// background (normally "transparent")
	palette[1] = 0xFFC0C0C0;	// multicolor register #0 ($D025)
	palette[2] = 0xFF000000;	// sprite color register
	palette[3] = 0xFF808080;	// multicolor register #1 ($D026)

	// Convert the sprite image data.
	// - Source: 12x21 2bpp (3 bytes per line)
	// - Destination: 24x21 8bpp
	const int stride_diff = img->stride() - img->width();
	uint8_t *p_dest = static_cast<uint8_t*>(img->bits());
	for (unsigned int y = 21; y > 0; y--) {
		for (unsigned int x = 12; x > 0; x -= 4, img_buf++) {
			// NOTE: Left pixel is the MSB.
			uint8_t src = *img_buf;
			for (unsigned int bit = 0; bit < 4; bit++, src <<= 2, p_dest += 2) {
				uint8_t px = (src & 0xC0) >> 6;
				p_dest[0] = px;
				p_dest[1] = px;
			}
		}
		p_dest += stride_diff;
	}

	// Set the sBIT metadata.
	// TODO: Use grayscale instead of RGB.
	static const rp_image::sBIT_t sBIT = {2,2,2,0,0};
	img->set_sBIT(sBIT);

	// Image has been converted.
	return img;
}

} }

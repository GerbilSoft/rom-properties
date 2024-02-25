/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_C64.cpp: Image decoding functions: Commodore 64            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_C64.hpp"

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Commodore 64 single-color sprite (24x21) to rp_image.
 * A default monochrome palette is used.
 * @param img_buf CI4 image buffer
 * @param img_siz Size of image data [must be >= 24*21/8 (63)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromC64_SingleColor_Sprite(
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(img_siz >= 24*21/8);
	if (!img_buf || img_siz < 24*21/8) {
		return nullptr;
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(24, 21, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Set up a standard palette.
	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.
	// NOTE: Not marking color 0 as transparent, since it would
	// usually result in the icon being unreadable.
	uint32_t *const palette = img->palette();
	palette[0] = 0xFFFFFFFF;	// background (normally "transparent")
	palette[1] = 0xFF000000;	// foreground

	// Convert the sprite image data.
	// - Source: 24x21 monochrome (3 bytes per line)
	// - Destination: 24x21 8bpp
	const int stride_diff = img->stride() - img->width();
	uint8_t *p_dest = static_cast<uint8_t*>(img->bits());
	for (unsigned int y = 21; y > 0; y--) {
		for (unsigned int x = 24; x > 0; x -= 8, img_buf++) {
			// NOTE: Left pixel is the MSB.
			uint8_t src = *img_buf;
			for (unsigned int bit = 0; bit < 8; bit++, src <<= 1, p_dest++) {
				*p_dest = ((src & 0x80) ? 1 : 0);
			}
		}
		p_dest += stride_diff;
	}

	// Set the sBIT metadata.
	// TODO: Use grayscale instead of RGB.
	static const rp_image::sBIT_t sBIT = {1,1,1,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

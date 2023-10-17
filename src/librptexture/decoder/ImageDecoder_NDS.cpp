/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_GCN.cpp: Image decoding functions: Nintendo DS             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_NDS.hpp"
#include "ImageDecoder_p.hpp"

#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Nintendo DS CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromNDS_CI4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const uint16_t *RESTRICT pal_buf, size_t pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) / 2));
	assert(pal_siz >= 16*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) / 2) || pal_siz < 16*2)
	{
		return nullptr;
	}

	// NDS CI4 uses 8x8 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Convert the palette.
	uint32_t *const palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		return nullptr;
	}

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.
	for (unsigned int i = 0; i < 16; i += 2) {
		// NDS color format is BGR555.
		palette[i+0] = BGR555_to_ARGB32(le16_to_cpu(pal_buf[i+0]));
		palette[i+1] = BGR555_to_ARGB32(le16_to_cpu(pal_buf[i+1]));
	}
	// Color 0 is always transparent.
	// NOTE: Not special-casing color 0 in order to prevent an off-by-one.
	palette[0] = 0;
	img->set_tr_idx(0);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 8);
	const unsigned int tilesY = static_cast<unsigned int>(height / 8);

	// Tile pointer.
	const array<uint8_t, 8*8/2> *pTileBuf = reinterpret_cast<const array<uint8_t, 8*8/2>*>(img_buf);

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++) {
			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile_CI4_LeftLSN<8, 8>(img.get(), *pTileBuf, x, y);
			pTileBuf++;
		}
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {5,5,5,0,1};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

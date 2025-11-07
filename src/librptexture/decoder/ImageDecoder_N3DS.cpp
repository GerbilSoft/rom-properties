/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_GCN.cpp: Image decoding functions: Nintendo 3DS            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_N3DS.hpp"
#include "ImageDecoder_p.hpp"

#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

// N3DS uses 3-level Z-ordered tiling.
// References:
// - https://github.com/devkitPro/3dstools/blob/master/src/smdhtool.cpp
// - https://en.wikipedia.org/wiki/Z-order_curve
static constexpr array<uint8_t, 64> N3DS_tile_order = {{
	 0,  1,  8,  9,  2,  3, 10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
	 4,  5, 12, 13,  6,  7, 14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
	32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
	36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63
}};

/**
 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromN3DSTiledRGB565(int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (static_cast<size_t>(width) * static_cast<size_t>(height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (static_cast<size_t>(width) * static_cast<size_t>(height) * 2))
	{
		return nullptr;
	}

	// N3DS tiled images use 8x8 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 8);
	const unsigned int tilesY = static_cast<unsigned int>(height / 8);

	// Temporary tile buffer.
	array<uint32_t, 8*8> tileBuf;

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++) {
			// Convert each tile to ARGB32 manually.
			for (size_t i = 0; i < tileBuf.size(); i += 2, img_buf += 2) {
				tileBuf[N3DS_tile_order[i+0]] = RGB565_to_ARGB32(le16_to_cpu(img_buf[0]));
				tileBuf[N3DS_tile_order[i+1]] = RGB565_to_ARGB32(le16_to_cpu(img_buf[1]));
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 8, 8>(img.get(), tileBuf, x, y);
		}
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {5,6,5,0,0};
	img->set_sBIT(sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a Nintendo 3DS RGB565+A4 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param alpha_buf A4 tiled alpha buffer.
 * @param alpha_siz Size of alpha data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromN3DSTiledRGB565_A4(int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz,
	const uint8_t *RESTRICT alpha_buf, size_t alpha_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(alpha_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (static_cast<size_t>(width) * static_cast<size_t>(height) * 2));
	assert(alpha_siz >= (static_cast<size_t>(width) * static_cast<size_t>(height) / 2));
	if (!img_buf || !alpha_buf || width <= 0 || height <= 0 ||
	    img_siz < (static_cast<size_t>(width) * static_cast<size_t>(height) * 2) ||
	    alpha_siz < (static_cast<size_t>(width) * static_cast<size_t>(height) / 2))
	{
		return nullptr;
	}

	// N3DS tiled images use 8x8 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 8);
	const unsigned int tilesY = static_cast<unsigned int>(height / 8);

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Temporary tile buffer.
	array<uint32_t, 8*8> tileBuf;

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++) {
			// Convert each tile to ARGB32 manually.
			// FIXME: Nybble ordering for A4?
			// Assuming LeftLSN, same as NDS CI4.
			for (size_t i = 0; i < tileBuf.size(); i += 2, img_buf += 2, alpha_buf++) {
				tileBuf[N3DS_tile_order[i+0]] = RGB565_A4_to_ARGB32(
					le16_to_cpu(img_buf[0]), *alpha_buf & 0x0F);
				tileBuf[N3DS_tile_order[i+1]] = RGB565_A4_to_ARGB32(
					le16_to_cpu(img_buf[1]), *alpha_buf >> 4);
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 8, 8>(img.get(), tileBuf, x, y);
		}
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {5,6,5,0,4};
	img->set_sBIT(sBIT);

	// Image has been converted.
	return img;
}

} }

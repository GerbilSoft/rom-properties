/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_ASTC.cpp: Image decoding functions. (ASTC)                 *
 *                                                                         *
 * Copyright (c) 2019-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

#include "ImageSizeCalc.hpp"
#include "basisu_astc_decomp.h"

// C++ STL classes.
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert an ASTC 2D image to rp_image.
 * @param width Image width
 * @param height Image height
 * @param img_buf PVRTC image buffer
 * @param img_siz Size of image data
 * @param block_x ASTC block size, X value
 * @param block_y ASTC block size, Y value
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image *fromASTC(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	uint8_t block_x, uint8_t block_y)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// TODO: Validate combinations.
	if (!ImageSizeCalc::validateBlockSizeASTC(block_x, block_y)) {
		return nullptr;
	}

	// Get the expected size.
	const unsigned int expected_size_in = ImageSizeCalc::calcImageSizeASTC(width, height, block_x, block_y);
	assert(img_siz >= static_cast<int>(expected_size_in));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < static_cast<int>(expected_size_in))
	{
		return nullptr;
	}

	// Align the image size.
	// TODO: Combine with calcImageSizeASTC()?
	int physWidth = width;
	int physHeight = height;
	ImageSizeCalc::alignImageSizeASTC(physWidth, physHeight, block_x, block_y);

	// Create an rp_image.
	rp_image *const img = new rp_image(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		img->unref();
		return nullptr;
	}

	// Basis Universal's ASTC decoder handles one block at a time,
	// so we'll need to use a tiled decode loop.

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(physWidth / block_x);
	const unsigned int tilesY = static_cast<unsigned int>(physHeight / block_y);

	// Temporary tile buffer.
	// NOTE: Largest ASTC format is 12x12.
	array<uint32_t, 12*12> tileBuf;
	const int stride_px = img->stride() / sizeof(uint32_t);
	uint32_t *const pImgBits = static_cast<uint32_t*>(img->bits());

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++, img_buf += 16) {
			// Decode the tile from ASTC.
			bool bRet = basisu_astc::astc::decompress(
				reinterpret_cast<uint8_t*>(tileBuf.data()), img_buf,
				false,	// TODO: sRGB scaling?
				block_x, block_y);
			if (!bRet) {
				// ASTC decompression error.
				img->unref();
				return nullptr;
			}

			// Blit the tile to the main image buffer.
			// NOTE: Not using BlitTile because ASTC has lots of different tile sizes.

			// Go to the first pixel for this tile.
			uint32_t *pImgBuf = pImgBits;
			pImgBuf += (y * block_y) * stride_px;
			pImgBuf += (x * block_x);

			const uint32_t *pTileBuf = tileBuf.data();
			for (unsigned int ty = block_y; ty > 0; ty--) {
				memcpy(pImgBuf, pTileBuf, (block_x * sizeof(uint32_t)));
				pImgBuf += stride_px;
				pTileBuf += block_x;
			}
		}
	}

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	// NOTE: Assuming ASTC always has alpha for now.
	static const rp_image::sBIT_t sBIT  = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

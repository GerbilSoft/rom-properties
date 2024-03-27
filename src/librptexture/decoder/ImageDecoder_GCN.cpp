/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_GCN.cpp: Image decoding functions: GameCube                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageDecoder_GCN.hpp"
#include "ImageDecoder_p.hpp"

#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// C++ STL classes
using std::array;

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a GameCube 16-bit image to rp_image.
 * @param px_format 16-bit pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB5A3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromGcn16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) * 2))
	{
		return nullptr;
	}

	// GameCube RGB5A3 uses 4x4 tiles.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 4);
	const unsigned int tilesY = static_cast<unsigned int>(height / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

#define GCN_16(pxfmt, pxfunc, sBIT_val) \
		case (pxfmt): { \
			for (unsigned int y = 0; y < tilesY; y++) { \
				for (unsigned int x = 0; x < tilesX; x++) { \
					/* Convert each tile to ARGB32 manually. */ \
					/* TODO: Optimize using pointers instead of indexes? */ \
					for (unsigned int i = 0; i < 4*4; i += 2, img_buf += 2) { \
						tileBuf[i+0] = pxfunc(be16_to_cpu(img_buf[0])); \
						tileBuf[i+1] = pxfunc(be16_to_cpu(img_buf[1])); \
					} \
					/* Blit the tile to the main image buffer. */ \
					ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y); \
				} \
			} \
			/* Set the sBIT metadata. */ \
			img->set_sBIT(sBIT_val); \
			break; \
		}
	// NOTE: For RGB5A3, pixels may be RGB555 or ARGB4444.
	// We'll use 555 for RGB, and 4 for alpha.
	// TODO: Set alpha to 0 if no translucent pixels were found.
	static const rp_image::sBIT_t sBIT_5A3 = {5,5,5,0,4};
	static const rp_image::sBIT_t sBIT_565 = {5,6,5,0,0};

	// NOTE: For IA8, setting the grayscale value, though we're
	// not saving grayscale PNGs at the moment.
	static const rp_image::sBIT_t sBIT_IA8 = {8,8,8,8,8};

	switch (px_format) {
		GCN_16(PixelFormat::RGB5A3, RGB5A3_to_ARGB32, &sBIT_5A3)
		GCN_16(PixelFormat::RGB565, RGB565_to_ARGB32, &sBIT_565)
		GCN_16(PixelFormat::IA8,    IA8_to_ARGB32,    &sBIT_IA8)

		default:
			assert(!"Invalid pixel format for this function.");
			return nullptr;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube CI8 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromGcnCI8(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const uint16_t *RESTRICT pal_buf, size_t pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((size_t)width * (size_t)height));
	assert(pal_siz >= 256*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < ((size_t)width * (size_t)height) || pal_siz < 256*2)
	{
		return nullptr;
	}

	// GameCube CI8 uses 8x4 tiles.
	assert(width % 8 == 0);
	assert(height % 4 == 0);
	if (width % 8 != 0 || height % 4 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *const palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		return nullptr;
	}

	int tr_idx = -1;
	for (unsigned int i = 0; i < 256; i += 2) {
		// GCN color format is RGB5A3.
		palette[i] = RGB5A3_to_ARGB32(be16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = static_cast<int>(i);
		}
		palette[i+1] = RGB5A3_to_ARGB32(be16_to_cpu(pal_buf[i+1]));
		if (tr_idx < 0 && ((palette[i+1] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = static_cast<int>(i+1);
		}
	}
	img->set_tr_idx(tr_idx);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 8);
	const unsigned int tilesY = static_cast<unsigned int>(height / 4);

	// Tile pointer
	const array<uint8_t, 8*4> *pTileBuf = reinterpret_cast<const array<uint8_t, 8*4>*>(img_buf);

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++) {
			// Decode the current tile.
			ImageDecoderPrivate::BlitTile<uint8_t, 8, 4>(img.get(), *pTileBuf, x, y);
			pTileBuf++;
		}
	}

	// Set the sBIT metadata.
	// NOTE: Pixels may be RGB555 or ARGB4444.
	// We'll use 555 for RGB, and 4 for alpha.
	// TODO: Set alpha to 0 if no translucent pixels were found.
	static const rp_image::sBIT_t sBIT = {5,5,5,0,4};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube I8 image to rp_image.
 * NOTE: Uses a grayscale palette.
 * FIXME: Needs verification.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf I8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromGcnI8(int width, int height,
	const uint8_t *img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((size_t)width * (size_t)height));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((size_t)width * (size_t)height))
	{
		return nullptr;
	}

	// GameCube I8 uses 8x4 tiles.
	// FIXME: Verify!
	assert(width % 8 == 0);
	assert(height % 4 == 0);
	if (width % 8 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 8);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Initialize a grayscale palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *const palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		return nullptr;
	}

	uint32_t gray = 0xFF000000U;
	for (unsigned int i = 0; i < 256; i++, gray += 0x010101U) {
		palette[i] = gray;
	}
	// No transparency here.
	img->set_tr_idx(-1);

	// Tile pointer.
	const array<uint8_t, 8*4> *pTileBuf = reinterpret_cast<const array<uint8_t, 8*4>*>(img_buf);

	for (unsigned int y = 0; y < tilesY; y++) {
		for (unsigned int x = 0; x < tilesX; x++) {
			// Decode the current tile.
			ImageDecoderPrivate::BlitTile<uint8_t, 8, 4>(img.get(), *pTileBuf, x, y);
			pTileBuf++;
		}
	}

	// Set the sBIT metadata.
	// TODO: Use grayscale instead of RGB.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromGcnCI4(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	assert(pal_siz >= 16*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2) || pal_siz < 16*2)
	{
		return nullptr;
	}

	// GameCube CI4 uses 8x8 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 8);
	const int tilesY = (height / 8);

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 16; i++) {
		// GCN color format is RGB5A3.
		palette[i] = RGB5A3_to_ARGB32(be16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = i;
		}
	}
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Tile pointer
	const array<uint8_t, 8*8/2> *pTileBuf = reinterpret_cast<const array<uint8_t, 8*8/2>*>(img_buf);

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Decode the current tile.
			ImageDecoderPrivate::BlitTile_CI4_LeftMSN<8, 8>(img.get(), *pTileBuf, x, y);
			pTileBuf++;
		}
	}

	// Image has been converted.
	return img;
}

} }

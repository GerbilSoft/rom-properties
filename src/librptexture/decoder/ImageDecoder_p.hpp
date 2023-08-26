/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_p.hpp: Image decoding functions. (PRIVATE NAMESPACE)       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "../img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <array>

namespace LibRpTexture { namespace ImageDecoderPrivate {

/**
 * Blit a tile to an rp_image. (pixel*)
 * NOTE: No bounds checking is done.
 * @tparam pixel	[in] Pixel type
 * @tparam tileW	[in] Tile width
 * @tparam tileH	[in] Tile height
 * @param img		[out] rp_image
 * @param tileBuf	[in] Tile buffer (pixel*)
 * @param tileX		[in] Horizontal tile number
 * @param tileY		[in] Vertical tile number
 */
template<typename pixel, unsigned int tileW, unsigned int tileH>
static inline void BlitTile(
	rp_image *RESTRICT img, const pixel *tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	switch (sizeof(pixel)) {
		case 4:
			assert(img->format() == rp_image::Format::ARGB32);
			break;
		case 1:
			assert(img->format() == rp_image::Format::CI8);
			break;
		default:
			assert(!"Unsupported sizeof(pixel).");
			return;
	}

	// Go to the first pixel for this tile.
	const int stride_px = img->stride() / sizeof(pixel);
	pixel *pImgBuf = static_cast<pixel*>(img->bits());
	pImgBuf += (tileY * tileH) * stride_px;
	pImgBuf += (tileX * tileW);

	const pixel *pTileBuf = tileBuf;
	for (unsigned int y = tileH; y > 0; y--) {
		memcpy(pImgBuf, pTileBuf, (tileW * sizeof(pixel)));
		pImgBuf += stride_px;
		pTileBuf += tileW;
	}
}

/**
 * Blit a tile to an rp_image. (std::array<pixel>)
 * NOTE: No bounds checking is done.
 * @tparam pixel	[in] Pixel type
 * @tparam tileW	[in] Tile width
 * @tparam tileH	[in] Tile height
 * @param img		[out] rp_image
 * @param tileBuf	[in] Tile buffer (std::array<pixel>)
 * @param tileX		[in] Horizontal tile number
 * @param tileY		[in] Vertical tile number
 */
template<typename pixel, unsigned int tileW, unsigned int tileH>
static inline void BlitTile(
	rp_image *RESTRICT img, const std::array<pixel, tileW*tileH> &tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	BlitTile<pixel, tileW, tileH>(img, tileBuf.data(), tileX, tileY);
}

/**
 * Blit a tile to an rp_image. (std::array<argb32_t>)
 * NOTE: No bounds checking is done.
 * @tparam pixel	[in] Pixel type
 * @tparam tileW	[in] Tile width
 * @tparam tileH	[in] Tile height
 * @param img		[out] rp_image
 * @param tileBuf	[in] Tile buffer (std::array<argb32_t>)
 * @param tileX		[in] Horizontal tile number
 * @param tileY		[in] Vertical tile number
 */
template<typename pixel, unsigned int tileW, unsigned int tileH>
static inline void BlitTile(
	rp_image *RESTRICT img, const std::array<argb32_t, tileW*tileH> &tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	static_assert(sizeof(pixel) == sizeof(argb32_t), "BlitTile with argb32_t tiles requires 32-bit pixels.");
	BlitTile<pixel, tileW, tileH>(img, reinterpret_cast<const uint32_t*>(tileBuf.data()), tileX, tileY);
}

/**
 * Blit a CI4 tile to a CI8 rp_image.
 * NOTE: Left pixel is the least significant nybble.
 * NOTE: No bounds checking is done.
 * @tparam tileW	[in] Tile width
 * @tparam tileH	[in] Tile height
 * @param img		[out] rp_image
 * @param tileBuf	[in] Tile buffer
 * @param tileX		[in] Horizontal tile number
 * @param tileY		[in] Vertical tile number
 */
template<unsigned int tileW, unsigned int tileH>
static inline void BlitTile_CI4_LeftLSN(
	rp_image *RESTRICT img, const std::array<uint8_t, tileW*tileH/2> &tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	static_assert(tileW % 2 == 0, "Tile width must be a multiple of 2.");
	assert(img->format() == rp_image::Format::CI8);
	assert(img->width() % 2 == 0);

	// Go to the first pixel for this tile.
	const int stride_px = img->stride();
	uint8_t *pImgBuf = static_cast<uint8_t*>(img->bits());
	pImgBuf += (tileY * tileH) * stride_px;
	pImgBuf += (tileX * tileW);

	const uint8_t *pTileBuf = tileBuf.data();
	const int stride_px_adj = stride_px - tileW;
	for (unsigned int y = tileH; y > 0; y--) {
		// Expand CI4 pixels to CI8 before writing.
		for (unsigned int x = tileW; x > 0; x -= 2) {
			pImgBuf[0] = (*pTileBuf & 0x0F);
			pImgBuf[1] = (*pTileBuf >> 4);
			pImgBuf += 2;
			pTileBuf++;
		}

		// Next line.
		pImgBuf += stride_px_adj;
	}
}

/**
 * Blit a CI4 tile to a CI8 rp_image.
 * NOTE: Left pixel is the most significant nybble.
 * NOTE: No bounds checking is done.
 * @tparam tileW	[in] Tile width
 * @tparam tileH	[in] Tile height
 * @param img		[out] rp_image
 * @param tileBuf	[in] Tile buffer
 * @param tileX		[in] Horizontal tile number
 * @param tileY		[in] Vertical tile number
 */
template<unsigned int tileW, unsigned int tileH>
static inline void BlitTile_CI4_LeftMSN(
	rp_image *RESTRICT img, const std::array<uint8_t, tileW*tileH/2> &tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	static_assert(tileW % 2 == 0, "Tile width must be a multiple of 2.");
	assert(img->format() == rp_image::Format::CI8);
	assert(img->width() % 2 == 0);

	// Go to the first pixel for this tile.
	const int stride_px = img->stride();
	uint8_t *pImgBuf = static_cast<uint8_t*>(img->bits());
	pImgBuf += (tileY * tileH) * stride_px;
	pImgBuf += (tileX * tileW);

	const uint8_t *pTileBuf = tileBuf.data();
	const int stride_px_adj = stride_px - tileW;
	for (unsigned int y = tileH; y > 0; y--) {
		// Expand CI4 pixels to CI8 before writing.
		for (unsigned int x = tileW; x > 0; x -= 2) {
			pImgBuf[0] = (*pTileBuf >> 4);
			pImgBuf[1] = (*pTileBuf & 0x0F);
			pImgBuf += 2;
			pTileBuf++;
		}

		// Next line.
		pImgBuf += stride_px_adj;
	}
}

} }

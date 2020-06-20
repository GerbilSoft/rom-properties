/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_p.hpp: Image decoding functions. (PRIVATE CLASS)           *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_P_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_P_HPP__

#include "common.h"
#include "byteswap.h"
#include "../img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

namespace LibRpTexture {

class ImageDecoderPrivate
{
	private:
		// ImageDecoderPrivate is a static class.
		ImageDecoderPrivate();
		~ImageDecoderPrivate();
		RP_DISABLE_COPY(ImageDecoderPrivate)

	public:
		/**
		 * Blit a tile to an rp_image.
		 * NOTE: No bounds checking is done.
		 * @tparam pixel	[in] Pixel type.
		 * @tparam tileW	[in] Tile width.
		 * @tparam tileH	[in] Tile height.
		 * @param img		[out] rp_image.
		 * @param tileBuf	[in] Tile buffer.
		 * @param tileX		[in] Horizontal tile number.
		 * @param tileY		[in] Vertical tile number.
		 */
		template<typename pixel, unsigned int tileW, unsigned int tileH>
		static inline void BlitTile(
			rp_image *RESTRICT img, const pixel *RESTRICT tileBuf,
			unsigned int tileX, unsigned int tileY);

		/**
		 * Blit a CI4 tile to a CI8 rp_image.
		 * NOTE: Left pixel is the least significant nybble.
		 * NOTE: No bounds checking is done.
		 * @tparam tileW	[in] Tile width.
		 * @tparam tileH	[in] Tile height.
		 * @param img		[out] rp_image.
		 * @param tileBuf	[in] Tile buffer.
		 * @param tileX		[in] Horizontal tile number.
		 * @param tileY		[in] Vertical tile number.
		 */
		template<unsigned int tileW, unsigned int tileH>
		static inline void BlitTile_CI4_LeftLSN(
			rp_image *RESTRICT img, const uint8_t *RESTRICT tileBuf,
			unsigned int tileX, unsigned int tileY);
};

/**
 * Blit a tile to an rp_image.
 * NOTE: No bounds checking is done.
 * @tparam pixel	[in] Pixel type.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param img		[out] rp_image.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<typename pixel, unsigned int tileW, unsigned int tileH>
inline void ImageDecoderPrivate::BlitTile(
	rp_image *RESTRICT img, const pixel *RESTRICT tileBuf,
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
	pixel *imgBuf = static_cast<pixel*>(img->bits());
	imgBuf += (tileY * tileH) * stride_px;
	imgBuf += (tileX * tileW);

	for (unsigned int y = tileH; y > 0; y--) {
		memcpy(imgBuf, tileBuf, (tileW * sizeof(pixel)));
		imgBuf += stride_px;
		tileBuf += tileW;
	}
}

/**
 * Blit a CI4 tile to a CI8 rp_image.
 * NOTE: Left pixel is the least significant nybble.
 * NOTE: No bounds checking is done.
 * @tparam tileW	[in] Tile width.
 * @tparam tileH	[in] Tile height.
 * @param img		[out] rp_image.
 * @param tileBuf	[in] Tile buffer.
 * @param tileX		[in] Horizontal tile number.
 * @param tileY		[in] Vertical tile number.
 */
template<unsigned int tileW, unsigned int tileH>
inline void ImageDecoderPrivate::BlitTile_CI4_LeftLSN(
	rp_image *RESTRICT img, const uint8_t *RESTRICT tileBuf,
	unsigned int tileX, unsigned int tileY)
{
	static_assert(tileW % 2 == 0, "Tile width must be a multiple of 2.");
	assert(img->format() == rp_image::Format::CI8);
	assert(img->width() % 2 == 0);

	// Go to the first pixel for this tile.
	const int stride_px = img->stride();
	uint8_t *imgBuf = static_cast<uint8_t*>(img->bits());
	imgBuf += (tileY * tileH) * stride_px;
	imgBuf += (tileX * tileW);

	const int stride_px_adj = stride_px - tileW;
	for (unsigned int y = tileH; y > 0; y--) {
		// Expand CI4 pixels to CI8 before writing.
		for (unsigned int x = tileW; x > 0; x -= 2) {
			imgBuf[0] = (*tileBuf & 0x0F);
			imgBuf[1] = (*tileBuf >> 4);
			imgBuf += 2;
			tileBuf++;
		}

		// Next line.
		imgBuf += stride_px_adj;
	}
}

}

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_P_HPP__ */

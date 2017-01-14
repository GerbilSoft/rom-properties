/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ImageDecoder.cpp: Image decoding functions.                             *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "ImageDecoder.hpp"
#include "img/rp_image.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

namespace LibRomData {

class ImageDecoderPrivate
{
	private:
		// ImageDecoderPrivate is a static class.
		ImageDecoderPrivate();
		~ImageDecoderPrivate();
		ImageDecoderPrivate(const ImageDecoderPrivate &other);
		ImageDecoderPrivate &operator=(const ImageDecoderPrivate &other);

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
		template<typename pixel, int tileW, int tileH>
		static inline void BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY);

		/**
		 * Convert a BGR555 pixel to ARGB32.
		 * @param px16 BGR555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGR555_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
		 * @param px16 RGB5A3 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB5A3_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ARGB4444 pixel to ARGB32. (Dreamcast)
		 * @param px16 ARGB4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ARGB4444_to_ARGB32(uint16_t px16);
};

/** ImageDecoderPrivate **/

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
template<typename pixel, int tileW, int tileH>
inline void BlitTile(rp_image *img, const pixel *tileBuf, int tileX, int tileY)
{
	switch (sizeof(pixel)) {
		case 4:
			assert(img->format() == rp_image::FORMAT_ARGB32);
			break;
		case 1:
			assert(img->format() == rp_image::FORMAT_CI8);
			break;
		default:
			assert(!"Unsupported sizeof(pixel).");
			return;
	}

	// Go to the first pixel for this tile.
	const int stride_px = img->stride() / sizeof(pixel);
	pixel *imgBuf = static_cast<pixel*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	for (int y = tileH; y > 0; y--) {
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
template<int tileW, int tileH>
inline void BlitTile_CI4_LeftLSN(rp_image *img, const uint8_t *tileBuf, int tileX, int tileY)
{
	assert(img->format() == rp_image::FORMAT_CI8);
	assert(img->width() % 2 == 0);
	assert(tileW % 2 == 0);

	// Go to the first pixel for this tile.
	uint8_t *imgBuf = static_cast<uint8_t*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	const int stride_px_adj = img->stride() - tileW;
	for (int y = tileH; y > 0; y--) {
		// Expand CI4 pixels to CI8 before writing.
		for (int x = tileW; x > 0; x -= 2) {
			imgBuf[0] = (*tileBuf & 0x0F);
			imgBuf[1] = (*tileBuf >> 4);
			imgBuf += 2;
			tileBuf++;
		}

		// Next line.
		imgBuf += stride_px_adj;
	}
}

/**
 * Convert a BGR555 pixel to ARGB32.
 * @param px16 BGR555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGR555_to_ARGB32(uint16_t px16)
{
	// NOTE: px16 has already been byteswapped.
	uint32_t px32;

	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	px32 = ((((px16 << 19) & 0xF80000) | ((px16 << 17) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue

	// No alpha channel.
	px32 |= 0xFF000000U;
	return px32;
}

/**
 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
 * @param px16 RGB5A3 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB5A3_to_ARGB32(uint16_t px16)
{
	uint32_t px32 = 0;

	if (px16 & 0x8000) {
		// BGR555: xRRRRRGG GGGBBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32 |= (((px16 << 3) & 0x0000F8) | ((px16 >> 2) & 0x000007));	// B
		px32 |= (((px16 << 6) & 0x00F800) | ((px16 << 1) & 0x000700));	// G
		px32 |= (((px16 << 9) & 0xF80000) | ((px16 << 4) & 0x070000));	// R
		px32 |= 0xFF000000U; // no alpha channel
	} else {
		// RGB4A3
		px32  =  (px16 & 0x000F);	// B
		px32 |= ((px16 & 0x00F0) << 4);	// G
		px32 |= ((px16 & 0x0F00) << 8);	// R
		px32 |= (px32 << 4);		// Copy to the top nybble.

		// Calculate the alpha channel.
		uint8_t a = ((px16 >> 7) & 0xE0);
		a |= (a >> 3);
		a |= (a >> 3);

		// Apply the alpha channel.
		px32 |= (a << 24);
	}

	return px32;
}

/**
 * Convert an ARGB4444 pixel to ARGB32. (Dreamcast)
 * @param px16 ARGB4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ARGB4444_to_ARGB32(uint16_t px16)
{
	uint32_t px32;
	px32  =  (px16 & 0x000F);		// B
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) << 8);		// R
	px32 |= ((px16 & 0xF000) << 12);	// A
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/** ImageDecoder **/

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
rp_image *ImageDecoder::fromNDS_CI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) / 2) || pal_siz < 0x20)
		return nullptr;

	// NDS CI4 uses 8x8 tiles.
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 8);
	const int tilesY = (height / 8);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	palette[0] = 0; // Color 0 is always transparent.
	img->set_tr_idx(0);
	for (int i = 1; i < 16; i++) {
		// NDS color format is BGR555.
		palette[i] = ImageDecoderPrivate::BGR555_to_ARGB32(le16_to_cpu(pal_buf[i]));
	}

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Blit the tile to the main image buffer.
			BlitTile_CI4_LeftLSN<8, 8>(img, img_buf, x, y);
			img_buf += ((8 * 8) / 2);
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a GameCube RGB5A3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB5A3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromGcnRGB5A3(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	if (!img_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) * 2))
		return nullptr;

	// GameCube RGB5A3 uses 4x4 tiles.
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 4);
	const int tilesY = (height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Convert each tile to ARGB32 manually.
			// TODO: Optimize using pointers instead of indexes?
			for (int i = 0; i < 4*4; i++, img_buf++) {
				tileBuf[i] = ImageDecoderPrivate::RGB5A3_to_ARGB32(be16_to_cpu(*img_buf));
			}

			// Blit the tile to the main image buffer.
			BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
		}
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
rp_image *ImageDecoder::fromGcnCI8(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < (width * height) || pal_siz < 256*2)
		return nullptr;

	// GameCube CI8 uses 8x4 tiles.
	if (width % 8 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const int tilesX = (width / 8);
	const int tilesY = (height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 256; i++) {
		// GCN color format is RGB5A3.
		palette[i] = ImageDecoderPrivate::RGB5A3_to_ARGB32(be16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = i;
		}
	}
	img->set_tr_idx(tr_idx);

	// Tile pointer.
	const uint8_t *tileBuf = img_buf;

	for (int y = 0; y < tilesY; y++) {
		for (int x = 0; x < tilesX; x++) {
			// Decode the current tile.
			BlitTile<uint8_t, 8, 4>(img, tileBuf, x, y);
			tileBuf += (8 * 4);
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a Dreamcast CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDreamcastCI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) / 2) || pal_siz < 0x20)
		return nullptr;

	// CI4 width must be a multiple of two.
	if (width % 2 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 16; i++) {
		// Dreamcast color format is ARGB4444.
		palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = i;
		}
	}
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (CI4 -> CI8)
	for (int y = 0; y < height; y++) {
		uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 2) {
			px_dest[0] = (*img_buf >> 4);
			px_dest[1] = (*img_buf & 0x0F);
			img_buf++;
			px_dest += 2;
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a Dreamcast CI8 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDreamcastCI8(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < (width * height) || pal_siz < 256*2)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 256; i++) {
		// Dreamcast color format is ARGB4444.
		palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
		if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
			// Found the transparent color.
			tr_idx = i;
		}
	}
	img->set_tr_idx(tr_idx);

	// Copy one line at a time. (CI8 -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	const int stride = img->stride();
	for (int y = height; y > 0; y--) {
		memcpy(px_dest, img_buf, width);
		px_dest += stride;
		img_buf += width;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a Dreamcast ARGB4444 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ARGB4444 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDreamcastARGB4444(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	if (!img_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) * 2))
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Convert one line at a time. (ARGB4444 -> ARGB32)
	for (int y = 0; y < height; y++) {
		uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
		for (int x = width; x > 0; x--) {
			*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(*px_dest);
			img_buf++;
			px_dest++;
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a Dreamcast monochrome image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf Monochrome image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/8]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDreamcastMono(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	if (!img_buf)
		return nullptr;
	else if (width < 0 || height < 0)
		return nullptr;
	else if (img_siz < ((width * height) / 8))
		return nullptr;

	// Monochrome width must be a multiple of eight.
	if (width % 8 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Set a default monochrome palette.
	uint32_t *palette = img->palette();
	palette[0] = 0xFFFFFFFF;	// white
	palette[1] = 0xFF000000;	// black
	img->set_tr_idx(-1);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (monochrome -> CI8)
	for (int y = 0; y < height; y++) {
		uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 8) {
			uint8_t pxMono = *img_buf++;
			// TODO: Unroll this loop?
			for (int bit = 8; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel.
				*px_dest = (pxMono >> 7);
				pxMono <<= 1;
			}
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a PlayStation CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromPS1_CI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	if (!img_buf || !pal_buf)
		return nullptr;
	else if (width < 0 || height < 0 || (width % 2) != 0)
		return nullptr;
	else if (img_siz < ((width * height) / 2) || pal_siz < 16*2)
		return nullptr;

	// PS1 CI4 is linear. No tiles.

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	for (int i = 0; i < 16; i++) {
		// PS1 color format is BGR555.
		// NOTE: If the color value is $0000, it's transparent.
		const uint16_t px16 = le16_to_cpu(pal_buf[i]);
		if (px16 == 0) {
			// Transparent color.
			palette[i] = 0;
			if (tr_idx < 0) {
				tr_idx = i;
			}
		} else {
			// Non-transparent color.
			palette[i] = ImageDecoderPrivate::BGR555_to_ARGB32(px16);
		}
	}
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert from CI4 to CI8.
	for (int y = 0; y < height; y++) {
		uint8_t *dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 2, dest += 2, img_buf++) {
			dest[0] = (*img_buf & 0x0F);
			dest[1] = (*img_buf >> 4);
		}
	}

	// Image has been converted.
	return img;
}

}

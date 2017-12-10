/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_p.hpp: Image decoding functions. (PRIVATE CLASS)           *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "common.h"
#include "img/rp_image.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

namespace LibRpBase {

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

		/** Color conversion functions. **/

		// 2-bit alpha lookup table.
		// NOTE: Implementation is in ImageDecoder_Linear.cpp.
		static const uint32_t a2_lookup[4];

		// 2-bit color lookup table.
		// NOTE: Implementation is in ImageDecoder_Linear.cpp.
		static const uint8_t c2_lookup[4];

		// 3-bit color lookup table.
		// NOTE: Implementation is in ImageDecoder_Linear.cpp.
		static const uint8_t c3_lookup[8];

		// 16-bit RGB

		/**
		 * Convert an RGB565 pixel to ARGB32.
		 * @param px16 RGB565 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB565_to_ARGB32(uint16_t px16);

		/**
		 * Convert a BGR565 pixel to ARGB32.
		 * @param px16 BGR565 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGR565_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ARGB1555 pixel to ARGB32. (Dreamcast)
		 * @param px16 ARGB1555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ARGB1555_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ARGB4444 pixel to ARGB32. (Dreamcast)
		 * @param px16 ARGB4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ARGB4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ABGR4444 pixel to ARGB32.
		 * @param px16 ABGR4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ABGR4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGBA4444 pixel to ARGB32.
		 * @param px16 RGBA4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGBA4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert a BGRA4444 pixel to ARGB32.
		 * @param px16 BGRA4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGRA4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an xRGB4444 pixel to ARGB32. (Dreamcast)
		 * @param px16 xRGB4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t xRGB4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an xBGR4444 pixel to ARGB32.
		 * @param px16 xBGR4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t xBGR4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGBx4444 pixel to ARGB32.
		 * @param px16 RGBx4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGBx4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert a BGRx4444 pixel to ARGB32.
		 * @param px16 BGRx4444 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGRx4444_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ABGR1555 pixel to ARGB32.
		 * @param px16 ABGR1555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ABGR1555_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RGBA5551 pixel to ARGB32.
		 * @param px16 RGBA5551 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGBA5551_to_ARGB32(uint16_t px16);

		/**
		 * Convert a BGRA5551 pixel to ARGB32.
		 * @param px16 BGRA5551 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGRA5551_to_ARGB32(uint16_t px16);

		/**
		 * Convert an ARGB8332 pixel to ARGB32.
		 * @param px16 ARGB8332 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t ARGB8332_to_ARGB32(uint16_t px16);

		/**
		 * Convert an RG88 pixel to ARGB32.
		 * @param px16 RG88 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RG88_to_ARGB32(uint16_t px16);

		/**
		 * Convert a GR88 pixel to ARGB32.
		 * @param px16 GR88 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t GR88_to_ARGB32(uint16_t px16);

		// GameCube-specific 16-bit RGB

		/**
		 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
		 * @param px16 RGB5A3 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB5A3_to_ARGB32(uint16_t px16);

		/**
		 * Convert an IA8 pixel to ARGB32. (GameCube/Wii)
		 * NOTE: Uses a grayscale palette.
		 * @param px16 IA8 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t IA8_to_ARGB32(uint16_t px16);

		// Nintendo 3DS-specific 16-bit RGB

		/**
		 * Convert an RGB565+A4 pixel to ARGB32.
		 * @param px16 RGB565 pixel.
		 * @param a4 A4 value.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB565_A4_to_ARGB32(uint16_t px16, uint8_t a4);

		// 15-bit RGB

		/**
		 * Convert an RGB555 pixel to ARGB32.
		 * @param px16 RGB555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t RGB555_to_ARGB32(uint16_t px16);

		/**
		 * Convert a BGR555 pixel to ARGB32.
		 * @param px16 BGR555 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t BGR555_to_ARGB32(uint16_t px16);

		// 32-bit RGB

		/**
		 * Convert a G16R16 pixel to ARGB32.
		 * @param px32 G16R16 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t G16R16_to_ARGB32(uint32_t px32);

		/**
		 * Convert an A2R10G10B10 pixel to ARGB32.
		 * @param px32 A2R10G10B10 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t A2R10G10B10_to_ARGB32(uint32_t px32);

		/**
		 * Convert an A2B10G10R10 pixel to ARGB32.
		 * @param px32 A2B10G10R10 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t A2B10G10R10_to_ARGB32(uint32_t px32);

		// Luminance

		/**
		 * Convert an L8 pixel to ARGB32.
		 * NOTE: Uses a grayscale palette.
		 * @param px8 L8 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t L8_to_ARGB32(uint8_t px8);

		/**
		 * Convert an A4L4 pixel to ARGB32.
		 * NOTE: Uses a grayscale palette.
		 * @param px8 A4L4 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t A4L4_to_ARGB32(uint8_t px8);

		/**
		 * Convert an L16 pixel to ARGB32.
		 * NOTE: Uses a grayscale palette.
		 * @param px16 L16 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t L16_to_ARGB32(uint16_t px16);

		/**
		 * Convert an A8L8 pixel to ARGB32.
		 * NOTE: Uses a grayscale palette.
		 * @param px16 A8L8 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t A8L8_to_ARGB32(uint16_t px16);

		// Alpha

		/**
		 * Convert an A8 pixel to ARGB32.
		 * NOTE: Uses a black background.
		 * @param px8 A8 pixel.
		 * @return ARGB32 pixel.
		 */
		static inline uint32_t A8_to_ARGB32(uint8_t px8);
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
	pixel *imgBuf = static_cast<pixel*>(img->scanLine((int)(tileY * tileH)));
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
	assert(img->format() == rp_image::FORMAT_CI8);
	assert(img->width() % 2 == 0);
	assert(tileW % 2 == 0);

	// Go to the first pixel for this tile.
	uint8_t *imgBuf = static_cast<uint8_t*>(img->scanLine(tileY * tileH));
	imgBuf += (tileX * tileW);

	const int stride_px_adj = img->stride() - tileW;
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

/** Color conversion functions. **/
// NOTE: px16 and px32 are always in host-endian.

// 16-bit RGB

/**
 * Convert an RGB565 pixel to ARGB32.
 * @param px16 RGB565 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB565_to_ARGB32(uint16_t px16)
{
	// RGB565: RRRRRGGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((((px16 <<  8) & 0xF80000) | ((px16 <<  3) & 0x070000))) |	// Red
		((((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300))) |	// Green
		((((px16 <<  3) & 0x0000F8) | ((px16 >>  2) & 0x000007)));	// Blue
	return px32;
}

/**
 * Convert a BGR565 pixel to ARGB32.
 * @param px16 BGR565 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGR565_to_ARGB32(uint16_t px16)
{
	// RGB565: BBBBBGGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((((px16 << 19) & 0xF80000) | ((px16 << 14) & 0x070000))) |	// Red
		((((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300))) |	// Green
		((((px16 >>  8) & 0x0000F8) | ((px16 >> 13) & 0x000007)));	// Blue
	return px32;
}

/**
 * Convert an ARGB1555 pixel to ARGB32.
 * @param px16 ARGB1555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ARGB1555_to_ARGB32(uint16_t px16)
{
	// ARGB1555: ARRRRRGG GGGBBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32 = ((((px16 <<  9) & 0xF80000) | ((px16 <<  4) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 <<  3) & 0x0000F8) | ((px16 >>  2) & 0x000007)));	// Blue
	// Alpha channel.
	if (px16 & 0x8000) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an ABGR1555 pixel to ARGB32.
 * @param px16 ABGR1555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ABGR1555_to_ARGB32(uint16_t px16)
{
	// ABGR1555: ABBBBBGG GGGRRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32 = ((((px16 << 19) & 0xF80000) | ((px16 << 14) & 0x070000))) |	// Red
	       ((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
	       ((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue
	// Alpha channel.
	if (px16 & 0x8000) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an RGBA5551 pixel to ARGB32.
 * @param px16 RGBA5551 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGBA5551_to_ARGB32(uint16_t px16)
{
	// RGBA5551: RRRRRGGG GGBBBBBA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32 = ((((px16 <<  8) & 0xF80000) | ((px16 <<  3) & 0x070000))) |	// Red
	       ((((px16 <<  5) & 0x00F800) | ((px16      ) & 0x000700))) |	// Green
	       ((((px16 <<  2) & 0x0000F8) | ((px16 >>  3) & 0x000007)));	// Blue
	// Alpha channel.
	if (px16 & 0x0001) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert a BGRA5551 pixel to ARGB32.
 * @param px16 BGRA5551 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGRA5551_to_ARGB32(uint16_t px16)
{
	// BGRA5551: BBBBBGGG GGRRRRRA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32 = ((((px16 << 18) & 0xF80000) | ((px16 << 13) & 0x070000))) |	// Red
	       ((((px16 <<  5) & 0x00F800) | ((px16      ) & 0x000700))) |	// Green
	       ((((px16 >>  8) & 0x0000F8) | ((px16 >> 13) & 0x000007)));	// Blue
	// Alpha channel.
	if (px16 & 0x0001) {
		px32 |= 0xFF000000U;
	}
	return px32;
}

/**
 * Convert an ARGB4444 pixel to ARGB32.
 * @param px16 ARGB4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ARGB4444_to_ARGB32(uint16_t px16)
{
	// ARGB4444: AAAARRRR GGGGBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32  =  (px16 & 0x000F);		// B
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) << 8);		// R
	px32 |= ((px16 & 0xF000) << 12);	// A
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an ABGR4444 pixel to ARGB32.
 * @param px16 ABGR4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ABGR4444_to_ARGB32(uint16_t px16)
{
	// ARGB4444: AAAABBBB GGGGRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32  = ((px16 & 0x000F) << 16);	// R
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) >> 8);		// B
	px32 |= ((px16 & 0xF000) << 12);	// A
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an RGBA4444 pixel to ARGB32.
 * @param px16 RGBA4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGBA4444_to_ARGB32(uint16_t px16)
{
	// RGBA4444: RRRRGGGG BBBBAAAA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32  = ((px16 & 0x000F) << 24);	// A
	px32 |= ((px16 & 0x00F0) >> 4);		// B
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) << 4);		// R
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert a BGRA4444 pixel to ARGB32.
 * @param px16 BGRA4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGRA4444_to_ARGB32(uint16_t px16)
{
	// RGBA4444: BBBBGGGG RRRRAAAA
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32  = ((px16 & 0x000F) << 24);	// A
	px32 |= ((px16 & 0x00F0) << 12);	// R
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) >> 12);	// B
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an xRGB4444 pixel to ARGB32.
 * @param px16 xRGB4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::xRGB4444_to_ARGB32(uint16_t px16)
{
	// xRGB4444: xxxxRRRR GGGGBBBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |=  (px16 & 0x000F);		// B
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) << 8);		// R
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an xBGR4444 pixel to ARGB32.
 * @param px16 xBGR4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::xBGR4444_to_ARGB32(uint16_t px16)
{
	// xRGB4444: xxxxBBBB GGGGRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x000F) << 16);	// R
	px32 |= ((px16 & 0x00F0) << 4);		// G
	px32 |= ((px16 & 0x0F00) >> 8);		// B
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an RGBx4444 pixel to ARGB32.
 * @param px16 RGBx4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGBx4444_to_ARGB32(uint16_t px16)
{
	// RGBx4444: RRRRGGGG BBBBxxxx
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x00F0) >> 4);		// B
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) << 4);		// R
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert a BGRx4444 pixel to ARGB32.
 * @param px16 BGRx4444 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGRx4444_to_ARGB32(uint16_t px16)
{
	// RGBx4444: BBBBGGGG RRRRxxxx
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((px16 & 0x00F0) << 12);	// R
	px32 |=  (px16 & 0x0F00);		// G
	px32 |= ((px16 & 0xF000) >> 12);	// B
	px32 |=  (px32 << 4);			// Copy to the top nybble.
	return px32;
}

/**
 * Convert an ARGB8332 pixel to ARGB32.
 * @param px16 ARGB8332 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::ARGB8332_to_ARGB32(uint16_t px16)
{
	// ARGB8332: AAAAAAAA RRRGGGBB
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32;
	px32 = (c3_lookup[(px16 >> 5) & 7] << 16) |	// Red
	       (c3_lookup[(px16 >> 2) & 7] <<  8) |	// Green
	       (c2_lookup[ px16       & 3]      ) |	// Blue
	       ((px16 << 16) & 0xFF000000);		// Alpha
	return px32;
}

/**
 * Convert an RG88 pixel to ARGB32.
 * @param px16 RG88 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RG88_to_ARGB32(uint16_t px16)
{
	// RG88:     RRRRRRRR GGGGGGGG
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000 | ((uint32_t)px16 << 8);
}

/**
 * Convert a GR88 pixel to ARGB32.
 * @param px16 GR88 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::GR88_to_ARGB32(uint16_t px16)
{
	// GR88:     GGGGGGGG RRRRRRRR
	// ARGB32:   AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return 0xFF000000 | ((uint32_t)__swab16(px16) << 8);
}

// GameCube-specific 16-bit RGB

/**
 * Convert an RGB5A3 pixel to ARGB32. (GameCube/Wii)
 * @param px16 RGB5A3 pixel. (Must be host-endian.)
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB5A3_to_ARGB32(uint16_t px16)
{
	uint32_t px32;

	if (px16 & 0x8000) {
		// BGR555: xRRRRRGG GGGBBBBB
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  = 0xFF000000U;	// no alpha channel
		px32 |= (((px16 << 3) & 0x0000F8) | ((px16 >> 2) & 0x000007));	// B
		px32 |= (((px16 << 6) & 0x00F800) | ((px16 << 1) & 0x000700));	// G
		px32 |= (((px16 << 9) & 0xF80000) | ((px16 << 4) & 0x070000));	// R
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
 * Convert an IA8 pixel to ARGB32. (GameCube/Wii)
 * NOTE: Uses a grayscale palette.
 * @param px16 IA8 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::IA8_to_ARGB32(uint16_t px16)
{
	// FIXME: What's the component order of IA8?
	// Assuming I=MSB, A=LSB...

	// IA8:    IIIIIIII AAAAAAAA
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return ((px16 & 0xFF) << 16) | ((px16 & 0xFF00) << 8) | (px16 & 0xFF00) | ((px16 >> 8) & 0xFF);
}

// Nintendo 3DS-specific 16-bit RGB

/**
 * Convert an RGB565+A4 pixel to ARGB32.
 * @param px16 RGB565 pixel.
 * @param a4 A4 value.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB565_A4_to_ARGB32(uint16_t px16, uint8_t a4)
{
	// RGB565: RRRRRGGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	a4 &= 0x0F;
	uint32_t px32 = (a4 << 24) | (a4 << 28);				// Alpha
	px32 |= ((((px16 <<  8) & 0xF80000) | ((px16 <<  3) & 0x070000))) |	// Red
		((((px16 <<  5) & 0x00FC00) | ((px16 >>  1) & 0x000300))) |	// Green
		((((px16 <<  3) & 0x0000F8) | ((px16 >>  2) & 0x000007)));	// Blue
	return px32;
}

// 15-bit RGB

/**
 * Convert an RGB555 pixel to ARGB32.
 * @param px16 RGB555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::RGB555_to_ARGB32(uint16_t px16)
{
	// RGB555: xRRRRRGG GGGBBBBB
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((((px16 <<  9) & 0xF80000) | ((px16 <<  4) & 0x070000))) |	// Red
		((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
		((((px16 <<  3) & 0x0000F8) | ((px16 >>  2) & 0x000007)));	// Blue
	return px32;
}

/**
 * Convert a BGR555 pixel to ARGB32.
 * @param px16 BGR555 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::BGR555_to_ARGB32(uint16_t px16)
{
	// BGR555: xBBBBBGG GGGRRRRR
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t px32 = 0xFF000000U;
	px32 |= ((((px16 << 19) & 0xF80000) | ((px16 << 14) & 0x070000))) |	// Red
		((((px16 <<  6) & 0x00F800) | ((px16 <<  1) & 0x000700))) |	// Green
		((((px16 >>  7) & 0x0000F8) | ((px16 >> 12) & 0x000007)));	// Blue
	return px32;
}

// 32-bit RGB

/**
 * Convert a G16R16 pixel to ARGB32.
 * @param px32 G16R16 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::G16R16_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// G16R16: GGGGGGGG gggggggg RRRRRRRR rrrrrrrr
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb = 0xFF000000U;
	argb |= ((px32 <<  8) & 0x00FF0000) |
		((px32 >> 16) & 0x0000FF00);
	return argb;
}

/**
 * Convert an A2R10G10B10 pixel to ARGB32.
 * @param px32 A2R10G10B10 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::A2R10G10B10_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// A2R10G10B10: AARRRRRR RRrrGGGG GGGGggBB BBBBBBbb
	//      ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb;
	argb = ((px32 >> 6) & 0xFF0000) |	// Red
	       ((px32 >> 4) & 0x00FF00) |	// Green
	       ((px32 >> 2) & 0x0000FF) |	// Blue
	       a2_lookup[px32 >> 30];		// Alpha
	return argb;
}

/**
 * Convert an A2B10G10R10 pixel to ARGB32.
 * @param px32 A2B10G10R10 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::A2B10G10R10_to_ARGB32(uint32_t px32)
{
	// NOTE: This will truncate the color channels.
	// TODO: Add ARGB64 support?

	// A2B10G10R10: AABBBBBB BBbbGGGG GGGGggRR RRRRRRrr
	//      ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb;
	argb = ((px32 << 14) & 0xFF0000) |	// Red
	       ((px32 >>  4) & 0x00FF00) |	// Green
	       ((px32 >> 22) & 0x0000FF) |	// Blue
	       a2_lookup[px32 >> 30];		// Alpha
	return argb;
}

/**
 * Convert an L8 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px8 L8 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::L8_to_ARGB32(uint8_t px8)
{
	//     L8: LLLLLLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb = 0xFF000000U;
	argb |= px8 | (px8 << 8) | (px8 << 16);
	return argb;
}

/**
 * Convert an A4L4 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px8 A4L4 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::A4L4_to_ARGB32(uint8_t px8)
{
	//   A4L4: AAAALLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb;
	argb = ((px8 & 0xF0) << 20) | (px8 & 0x0F);	// Low nybble of A and B.
	argb |= (argb << 4);				// Copy to high nybble.
	argb |= (argb & 0xFF) <<  8;			// Copy B to G.
	argb |= (argb & 0xFF) << 16;			// Copy B to R.
	return argb;
}

/**
 * Convert an L16 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px16 L16 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::L16_to_ARGB32(uint16_t px16)
{
	// NOTE: This will truncate the luminance.
	// TODO: Add ARGB64 support?
	return L8_to_ARGB32(px16 >> 8);
}

/**
 * Convert an A8L8 pixel to ARGB32.
 * NOTE: Uses a grayscale palette.
 * @param px16 A8L8 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::A8L8_to_ARGB32(uint16_t px16)
{
	//   A8L8: AAAAAAAA LLLLLLLL
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	uint32_t argb;
	argb =  (px16 & 0xFF) |			// Red
	       ((px16 & 0xFF) << 8) |		// Green
	       ((px16 & 0xFF) << 16) |		// Blue
	       ((px16 << 16) & 0xFF000000);	// Alpha
	return argb;
}

// Alpha

/**
 * Convert an A8 pixel to ARGB32.
 * NOTE: Uses a black background.
 * @param px8 A8 pixel.
 * @return ARGB32 pixel.
 */
inline uint32_t ImageDecoderPrivate::A8_to_ARGB32(uint8_t px8)
{
	//     A8: AAAAAAAA
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	return (px8 << 24);
}

}

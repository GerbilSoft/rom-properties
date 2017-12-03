/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_ETC1.cpp: Image decoding functions. (ETC1)                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

// References:
// - https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
// - https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.html#ETC1

namespace LibRpBase {

// ETC1 block format.
// NOTE: Blocks are stored in big-endian.
union etc1_block {
	struct {
		// Low DWORD is the same regardless of diffbit.
		uint16_t lsb;
		uint16_t msb;

		// High DWORD is different depending on diffbit.

		// Common byte.
		struct {
			uint8_t flip	:1;
			uint8_t diff	:1;
			uint8_t tbl_cw2	:3;
			uint8_t tbl_cw1	:3;
		} common;

		union {
			// diffbit == 0
			struct {
				uint8_t base_B2	:4;
				uint8_t base_B1	:4;
				uint8_t base_G2	:4;
				uint8_t base_G1	:4;
				uint8_t base_R2	:4;
				uint8_t base_R1	:4;
			} indiv;

			// diffbit == 1
			struct {
				int8_t  dcol_B2	:3;
				uint8_t base_B1	:5;
				int8_t  dcol_G2	:3;
				uint8_t base_G1	:5;
				int8_t  dcol_R2	:3;
				uint8_t base_R1	:5;
			} diff;
		};
	};

	// 64-bit data. (big-endian)
	uint64_t u64;
};
ASSERT_STRUCT(etc1_block, 8);

/**
 * Pixel index values:
 * msb lsb
 *  1   1  == 3: -b (large negative value)
 *  1   0  == 2: -a (small negative value)
 *  0   0  == 0:  a (small positive value)
 *  0   1  == 1:  b (large positive value)
 *
 * Rearranged in ascending two-bit value order:
 *  0   0  == 0:  a (small positive value)
 *  0   1  == 1:  b (large positive value)
 *  1   0  == 2: -a (small negative value)
 *  1   1  == 3: -b (large negative value)
 */

/**
 * Intensity modifier sets.
 * Index 0 is the table codeword.
 * Index 1 is the pixel index value.
 *
 * NOTE: This table was rearranged to match the pixel
 * index values in ascending two-bit value order as
 * listed above instead of mapping to ETC1 table 3.17.2.
 */
static const int16_t etc1_intensity[8][4] = {
	{ 2,   8,  -2,   -8},
	{ 5,  17,  -5,  -17},
	{ 9,  29,  -9,  -29},
	{13,  42, -13,  -42},
	{18,  60, -18,  -60},
	{24,  80, -24,  -80},
	{33, 106, -33, -106},
	{47, 183, -47, -183},
};

// ETC1 arranges pixels by column, then by row.
// This table maps it back to linear.
static const uint8_t etc1_mapping[16] = {
	0, 4,  8, 12,
	1, 5,  9, 13,
	2, 6, 10, 14,
	3, 7, 11, 15,
};

/**
 * Extend a 4-bit color component to 8-bit color.
 * @param value 4-bit color component.
 * @return 8-bit color value.
 */
static inline uint8_t extend_4to8bits(uint8_t value)
{
	return (value << 4) | value;
}

/**
 * Extend a 5-bit color component to 8-bit color.
 * @param value 5-bit color component.
 * @return 8-bit color value.
 */
static inline uint8_t extend_5to8bits(uint8_t value)
{
	return (value << 5) | (value >> 2);
}

// Temporary RGB structure that allows us to clamp it later.
// TODO: Use SSE2?
struct ColorRGB {
	int R;
	int G;
	int B;
};

/**
 * Clamp a ColorRGB struct and convert it to xRGB32.
 * @param color ColorRGB struct.
 * @return xRGB32 value. (Alpha channel set to 0xFF)
 */
static inline uint32_t clamp_ColorRGB(const ColorRGB &color)
{
	uint32_t xrgb32;
	if (color.B > 255) {
		xrgb32 = 255;
	} else if (color.B > 0) {
		xrgb32 = color.B;
	}
	if (color.G > 255) {
		xrgb32 |= (255 << 8);
	} else if (color.G > 0) {
		xrgb32 |= (color.G << 8);
	}
	if (color.R > 255) {
		xrgb32 |= (255 << 16);
	} else if (color.R > 0) {
		xrgb32 |= (color.R << 16);
	}
	return xrgb32 | 0xFF000000;
}

/**
 * Convert an ETC1 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromETC1(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2))
	{
		return nullptr;
	}

	// ETC1 uses 4x4 tiles.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	const etc1_block *etc1_src = reinterpret_cast<const etc1_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, etc1_src++) {
		// Byteswap the block first.
		etc1_block block;
		block.u64 = be64_to_cpu(etc1_src->u64);

		// Determine the base colors for the two subblocks.
		// NOTE: These are kept as separate RGB components,
		// since we have to manipulate and clamp them manually.
		ColorRGB base_color[2];
		if (!block.common.diff) {
			// Individual mode.
			base_color[0].R = extend_4to8bits(block.indiv.base_R1);
			base_color[0].G = extend_4to8bits(block.indiv.base_G1);
			base_color[0].B = extend_4to8bits(block.indiv.base_B1);
			base_color[1].R = extend_4to8bits(block.indiv.base_R2);
			base_color[1].G = extend_4to8bits(block.indiv.base_G2);
			base_color[1].B = extend_4to8bits(block.indiv.base_B2);
		} else {
			// Differential mode.
			base_color[0].R = extend_5to8bits(block.diff.base_R1);
			base_color[0].G = extend_5to8bits(block.diff.base_G1);
			base_color[0].B = extend_5to8bits(block.diff.base_B1);
			// Differential colors are 3-bit two's complement.
			// TODO: Verify that signed bitfields work.
			base_color[1].R = extend_5to8bits(block.diff.base_R1 + block.diff.dcol_R2);
			base_color[1].G = extend_5to8bits(block.diff.base_G1 + block.diff.dcol_G2);
			base_color[1].B = extend_5to8bits(block.diff.base_B1 + block.diff.dcol_B2);
		}

		// Tile arrangement:
		// flip == 0        flip == 1
		// a e | i m        a e   i m
		// b f | j n        b f   j n
		//     |            ---------
		// c g | k o        c g   k o
		// d h | l p        d h   l p

		// Intensities for the table codewords.
		const int16_t *const tbl[2] = {
			etc1_intensity[block.common.tbl_cw1],
			etc1_intensity[block.common.tbl_cw2]
		};

		// Process the 16 pixel indexes.
		// TODO: Use SSE2 for saturated arithmetic?
		uint16_t px_msb = block.msb;
		uint16_t px_lsb = block.lsb;
		for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1) {
			ColorRGB color;
			unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

			// TODO: Select tbl[0] or tbl[1] using subblocks.
			// For now, assuming subblock 0.
			const int adj = tbl[0][px_idx];
			color = base_color[0];
			color.R += adj;
			color.G += adj;
			color.B += adj;

			// Clamp the color components and save it to the tile buffer.
			tileBuf[etc1_mapping[i]] = clamp_ColorRGB(color);
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

}

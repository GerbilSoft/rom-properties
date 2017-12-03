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
// - https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.html#ETC2

namespace LibRpBase {

// ETC1 block format.
// NOTE: Layout maps to on-disk format, which is big-endian.
struct etc1_block {
	// Base colors
	// Byte layout:
	// - diffbit == 0: 4 MSB == base 1, 4 LSB == base 2
	// - diffbit == 1: 5 MSB == base, 3 LSB == differential
	union {
		// Indiv/Diff
		struct {
			uint8_t R;
			uint8_t G;
			uint8_t B;
		} id;

		// ETC2 'T' mode
		struct {
			uint8_t R1;
			uint8_t G1B1;
			uint8_t R2G2;
			// B2 is in `control`.
		} t;

		// ETC2 'H' mode
		struct {
			uint8_t R1G1a;
			uint8_t G1bB1aB1b;
			uint8_t B1bR2G2;
			// Part of G2 is in `control`.
			// B2 is in `control`.
		} h;
	};

	// Control byte: [ETC1]
	// - 3 MSB:  table code word 1
	// - 3 next: table code word 2
	// - 1 bit:  diff bit
	// - 1 LSB:  flip bit
	uint8_t control;

	// Pixel index bits. (big-endian)
	uint16_t msb;
	uint16_t lsb;
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

// ETC1 subblock mapping.
// Index: flip bit
// Value: 16-bit bitfield; bit 0 == ETC1-arranged pixel 0.
static const uint16_t etc1_subblock_mapping[2] = {
	// flip == 0: 2x4
	0xFF00,

	// flip == 1: 4x2
	0xCCCC,
};

// 3-bit 2's complement lookup table.
static const int8_t etc1_3bit_diff_tbl[8] = {
	0, 1, 2, 3, -4, -3, -2, -1
};

// ETC2 mode.
enum etc2_mode {
	ETC2_MODE_ETC1,		// ETC1-compatible mode (indiv, diff)
	ETC2_MODE_TH,		// ETC2 'T' or 'H' mode
	ETC2_MODE_PLANAR,	// ETC2 'Planar' mode
};

// ETC2 distance table for 'T' and 'H' modes.
static const uint8_t etc2_dist_tbl[8] = {
	 3,  6, 11, 16,
	23, 32, 41, 64,
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
	return (value << 3) | (value >> 2);
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
	uint32_t xrgb32 = 0;
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
		// Determine the base colors for the two subblocks.
		// NOTE: These are kept as separate RGB components,
		// since we have to manipulate and clamp them manually.
		ColorRGB base_color[2];
		// control, bit 1: diffbit
		if (!(etc1_src->control & 0x02)) {
			// Individual mode.
			// TODO: Optimize the extend function by assuming the value is MSB-aligned.
			base_color[0].R = extend_4to8bits(etc1_src->id.R >> 4);
			base_color[0].G = extend_4to8bits(etc1_src->id.G >> 4);
			base_color[0].B = extend_4to8bits(etc1_src->id.B >> 4);
			base_color[1].R = extend_4to8bits(etc1_src->id.R & 0x0F);
			base_color[1].G = extend_4to8bits(etc1_src->id.G & 0x0F);
			base_color[1].B = extend_4to8bits(etc1_src->id.B & 0x0F);
		} else {
			// Differential mode.
			// TODO: Optimize the extend function by assuming the value is MSB-aligned.
			base_color[0].R = extend_5to8bits(etc1_src->id.R >> 3);
			base_color[0].G = extend_5to8bits(etc1_src->id.G >> 3);
			base_color[0].B = extend_5to8bits(etc1_src->id.B >> 3);

			// Differential colors are 3-bit two's complement.
			int8_t dR2 = etc1_3bit_diff_tbl[etc1_src->id.R & 0x07];
			int8_t dG2 = etc1_3bit_diff_tbl[etc1_src->id.G & 0x07];
			int8_t dB2 = etc1_3bit_diff_tbl[etc1_src->id.B & 0x07];
			base_color[1].R = extend_5to8bits((etc1_src->id.R >> 3) + dR2);
			base_color[1].G = extend_5to8bits((etc1_src->id.G >> 3) + dG2);
			base_color[1].B = extend_5to8bits((etc1_src->id.B >> 3) + dB2);
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
			etc1_intensity[etc1_src->control >> 5],
			etc1_intensity[(etc1_src->control >> 2) & 0x07]
		};

		// Process the 16 pixel indexes.
		// TODO: Use SSE2 for saturated arithmetic?
		uint16_t px_msb = be16_to_cpu(etc1_src->msb);
		uint16_t px_lsb = be16_to_cpu(etc1_src->lsb);
		// control, bit 0: flip
		uint16_t subblock = etc1_subblock_mapping[etc1_src->control & 0x01];
		for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1, subblock >>= 1) {
			ColorRGB color;
			unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

			// Select the table codeword based on the current subblock.
			const uint8_t cur_sub = subblock & 1;
			const int adj = tbl[cur_sub][px_idx];
			color = base_color[cur_sub];
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

/**
 * Convert an ETC2 RGB image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromETC2_RGB(int width, int height,
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

	// ETC2 uses 4x4 tiles.
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
		// Determine the base colors for the two subblocks.
		// NOTE: These are kept as separate RGB components,
		// since we have to manipulate and clamp them manually.
		ColorRGB base_color[2];

		// 'T','H' modes: Paint colors are used instead of base colors.
		ColorRGB paint_color[4];
		etc2_mode mode;

		// control, bit 1: diffbit
		if (!(etc1_src->control & 0x02)) {
			// Individual mode.
			// TODO: Optimize the extend function by assuming the value is MSB-aligned.
			mode = ETC2_MODE_ETC1;
			base_color[0].R = extend_4to8bits(etc1_src->id.R >> 4);
			base_color[0].G = extend_4to8bits(etc1_src->id.G >> 4);
			base_color[0].B = extend_4to8bits(etc1_src->id.B >> 4);
			base_color[1].R = extend_4to8bits(etc1_src->id.R & 0x0F);
			base_color[1].G = extend_4to8bits(etc1_src->id.G & 0x0F);
			base_color[1].B = extend_4to8bits(etc1_src->id.B & 0x0F);
		} else {
			// Other mode.
			// TODO: Optimize the extend function by assuming the value is MSB-aligned.

			// Differential colors are 3-bit two's complement.
			int8_t dR2 = etc1_3bit_diff_tbl[etc1_src->id.R & 0x07];
			int8_t dG2 = etc1_3bit_diff_tbl[etc1_src->id.G & 0x07];
			int8_t dB2 = etc1_3bit_diff_tbl[etc1_src->id.B & 0x07];

			// Sums of R+dR2, G+dG2, and B+dB2 are used to determine the mode.
			// If all of the sums are within [0,31], ETC1 differential mode is used.
			// Otherwise, a new ETC2 mode is used, which may discard some of the above values.
			int sR = (etc1_src->id.R >> 3) + dR2;
			int sG = (etc1_src->id.G >> 3) + dG2;
			int sB = (etc1_src->id.B >> 3) + dB2;
			if ((sR & ~0x1F) != 0) {
				// 'T' mode.
				// Base colors are arranged differently compared to ETC1,
				// and R1 is calculated differently.
				// Note that G and B are arranged slightly differently.
				mode = ETC2_MODE_TH;
				base_color[0].R = extend_4to8bits(((etc1_src->t.R1 & 0x18) >> 1) |
								   (etc1_src->t.R1 & 0x03));
				base_color[0].G = extend_4to8bits(etc1_src->t.G1B1 >> 4);
				base_color[0].B = extend_4to8bits(etc1_src->t.G1B1 & 0x0F);
				base_color[1].R = extend_4to8bits(etc1_src->t.R2G2 >> 4);
				base_color[1].G = extend_4to8bits(etc1_src->t.R2G2 & 0x0F);
				base_color[1].B = extend_4to8bits(etc1_src->control >> 4);

				// Determine the paint colors.
				paint_color[0] = base_color[0];
				paint_color[2] = base_color[1];

				// Paint colors 1 and 3 are adjusted using the distance table.
				const uint8_t d = etc2_dist_tbl[((etc1_src->control & 0x0C) >> 1) |
								 (etc1_src->control & 0x01)];
				paint_color[1].R = base_color[0].R + d;
				paint_color[1].G = base_color[0].G + d;
				paint_color[1].B = base_color[0].B + d;
				paint_color[3].R = base_color[1].R + d;
				paint_color[3].G = base_color[1].G + d;
				paint_color[3].B = base_color[1].B + d;
			} else if ((sG & ~0x1F) != 0) {
				// 'H' mode.
				// Base colors are arranged differently compared to ETC1,
				// and G1 and B1 are calculated differently.
				mode = ETC2_MODE_TH;
				base_color[0].R = extend_4to8bits(etc1_src->h.R1G1a >> 3);
				base_color[0].G = extend_4to8bits(((etc1_src->h.R1G1a & 0x07) << 1) |
								  ((etc1_src->h.G1bB1aB1b >> 4) & 0x01));
				base_color[0].B = extend_4to8bits( (etc1_src->h.G1bB1aB1b & 0x08) |
								  ((etc1_src->h.G1bB1aB1b & 0x03) << 1) |
								   (etc1_src->h.B1bR2G2 >> 7));
				base_color[1].R = extend_4to8bits(etc1_src->h.B1bR2G2 >> 3);
				base_color[1].G = extend_4to8bits(((etc1_src->h.B1bR2G2 & 0x07) << 1) |
								  (etc1_src->control >> 7));
				base_color[1].B = extend_4to8bits((etc1_src->control >> 3) & 0x0F);

				// Determine the paint colors.
				// All paint colors in 'H' mode are adjusted using the distance table.
				uint8_t d_idx = (etc1_src->control & 0x04) | ((etc1_src->control & 0x01) << 1);
				// d_idx LSB is determined by comparing the base colors in xRGB32 format.
				d_idx |= (clamp_ColorRGB(base_color[0]) >= clamp_ColorRGB(base_color[1]));

				const uint8_t d = etc2_dist_tbl[d_idx];
				paint_color[0].R = base_color[0].R + d;
				paint_color[0].G = base_color[0].G + d;
				paint_color[0].B = base_color[0].B + d;
				paint_color[1].R = base_color[0].R - d;
				paint_color[1].G = base_color[0].G - d;
				paint_color[1].B = base_color[0].B - d;
				paint_color[2].R = base_color[1].R + d;
				paint_color[2].G = base_color[1].G + d;
				paint_color[2].B = base_color[1].B + d;
				paint_color[3].R = base_color[1].R - d;
				paint_color[3].G = base_color[1].G - d;
				paint_color[3].B = base_color[1].B - d;
			} else if ((sB & ~0x1F) != 0) {
				// 'Planar' mode. [TODO]
				mode = ETC2_MODE_PLANAR;
				memset(paint_color, 0, sizeof(paint_color));
			} else {
				// ETC1 differential mode.
				mode = ETC2_MODE_ETC1;
				base_color[0].R = extend_5to8bits(etc1_src->id.R >> 3);
				base_color[0].G = extend_5to8bits(etc1_src->id.G >> 3);
				base_color[0].B = extend_5to8bits(etc1_src->id.B >> 3);
				base_color[1].R = extend_5to8bits(sR);
				base_color[1].G = extend_5to8bits(sG);
				base_color[1].B = extend_5to8bits(sB);
			}
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
			etc1_intensity[etc1_src->control >> 5],
			etc1_intensity[(etc1_src->control >> 2) & 0x07]
		};

		// Process the 16 pixel indexes.
		// TODO: Use SSE2 for saturated arithmetic?
		uint16_t px_msb = be16_to_cpu(etc1_src->msb);
		uint16_t px_lsb = be16_to_cpu(etc1_src->lsb);
		switch (mode) {
			default:
				assert(!"Invalid ETC2 mode.");
				delete img;
				return nullptr;

			case ETC2_MODE_ETC1: {
				// ETC1 mode.
				// control, bit 0: flip
				uint16_t subblock = etc1_subblock_mapping[etc1_src->control & 0x01];
				for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1, subblock >>= 1) {
					ColorRGB color;
					unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

					// Select the table codeword based on the current subblock.
					const uint8_t cur_sub = subblock & 1;
					const int adj = tbl[cur_sub][px_idx];
					color = base_color[cur_sub];
					color.R += adj;
					color.G += adj;
					color.B += adj;

					// Clamp the color components and save it to the tile buffer.
					tileBuf[etc1_mapping[i]] = clamp_ColorRGB(color);
				}
				break;
			}

			case ETC2_MODE_TH: {
				// ETC2 'T' or 'H' mode.
				for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1) {
					ColorRGB color;
					unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

					// Pixel index indicates the paint color to use.
					color = paint_color[px_idx];

					// Clamp the color components and save it to the tile buffer.
					// TODO: If the intensity table isn't used, optimize this by
					// storing the paint color as xRGB32.
					tileBuf[etc1_mapping[i]] = clamp_ColorRGB(color);
				}
				break;
			}

			case ETC2_MODE_PLANAR: {
				// ETC2 'Planar' mode.
				memset(tileBuf, 0, sizeof(tileBuf));
				break;
			}
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

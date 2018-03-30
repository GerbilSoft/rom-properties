/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_ETC1.cpp: Image decoding functions. (ETC1)                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
union etc1_block {
	struct {
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

	struct {
		// Planar mode has 3 colors in RGB676 format.
		// Colors are labelled 'O', 'H', and 'V'.
		uint8_t RO_GO1;		// 6-1: RO;     0: GO1
		uint8_t GO2_BO1;	// 6-1: GO2;    0: BO1
		uint8_t BO2_BO3;	// 4-3: BO2;  1-0: BO3a
		uint8_t BO3_RH;		//   7: BO3b; 6-2: RH1; 0: RH2
		uint8_t GH_BH;		// 7-1: GH;     0: BH
		uint8_t BH_RV;		// 7-3: BH;   2-0: RV
		uint8_t RV_GV;		// 7-5: RV;   4-0: GV
		uint8_t GV_BV;		// 7-6: GV;   5-0: BV
	} planar;
};
ASSERT_STRUCT(etc1_block, 8);

// ETC2 alpha block format.
// NOTE: Layout maps to on-disk format, which is big-endian.
union etc2_alpha {
	struct {
		uint8_t base_codeword;	// Base codeword.
		uint8_t mult_tbl_idx;	// Multiplier (high 4); table index (low 4)
		uint8_t values[6];	// Alpha values. (48-bit unsigned; 3-bit per pixel)
	};
	uint64_t u64;				// Access the 48-bit alpha value directly. (Requires shifting.)
};
ASSERT_STRUCT(etc2_alpha, 8);

// ETC2 RGBA block format.
// NOTE: Layout maps to on-disk format, which is big-endian.
struct etc2_rgba_block {
	etc2_alpha alpha;
	etc1_block etc1;
};
ASSERT_STRUCT(etc2_rgba_block, 16);

/**
 * Extract the 48-bit code value from etc2_alpha.
 * @param data etc2_alpha.
 * @return 48-bit code value.
 */
static FORCEINLINE uint64_t extract48(const etc2_alpha *RESTRICT data)
{
	// values[6] starts at 0x02 within etc2_alpha.
	// Hence, we need to mask it after byteswapping.
	// TODO: constexpr?
	// TODO: Verify on big-endian.
	return be64_to_cpu(data->u64) & 0x0000FFFFFFFFFFFFULL;
}

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

/**
 * Intensity modifier sets. (ETC2 with punchthrough alpha if opaque == 0)
 * Index 0 is the table codeword.
 * Index 1 is the pixel index value.
 *
 * NOTE: This table was rearranged to match the pixel
 * index values in ascending two-bit value order as
 * listed above instead of mapping to ETC1 table 3.17.2.
 */
static const int16_t etc2_intensity_a1[8][4] = {
	{0,   8, 0,   -8},
	{0,  17, 0,  -17},
	{0,  29, 0,  -29},
	{0,  42, 0,  -42},
	{0,  60, 0,  -60},
	{0,  80, 0,  -80},
	{0, 106, 0, -106},
	{0, 183, 0, -183},
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

// ETC2 block mode.
enum etc2_block_mode {
	ETC2_BLOCK_MODE_UNKNOWN = 0,
	ETC2_BLOCK_MODE_ETC1,		// ETC1-compatible mode (indiv, diff)
	ETC2_BLOCK_MODE_TH,		// ETC2 'T' or 'H' mode
	ETC2_BLOCK_MODE_PLANAR,		// ETC2 'Planar' mode
};

// ETC2 distance table for 'T' and 'H' modes.
static const uint8_t etc2_dist_tbl[8] = {
	 3,  6, 11, 16,
	23, 32, 41, 64,
};

// ETC2 alpha modifiers table.
static const int8_t etc2_alpha_tbl[16][8] = {
	{-3, -6,  -9, -15, 2, 5, 8, 14},
	{-3, -7, -10, -13, 2, 6, 9, 12},
	{-2, -5,  -8, -13, 1, 4, 7, 12},
	{-2, -4,  -6, -13, 1, 3, 5, 12},
	{-3, -6,  -8, -12, 2, 5, 7, 11},
	{-3, -7,  -9, -11, 2, 6, 8, 10},
	{-4, -7,  -8, -11, 3, 6, 7, 10},
	{-3, -5,  -8, -11, 2, 4, 7, 10},
	{-2, -6,  -8, -10, 1, 5, 7,  9},
	{-2, -5,  -8, -10, 1, 4, 7,  9},
	{-2, -4,  -8, -10, 1, 3, 7,  9},
	{-2, -5,  -7, -10, 1, 4, 6,  9},
	{-3, -4,  -7, -10, 2, 3, 6,  9},
	{-1, -2,  -3, -10, 0, 1, 2,  9},
	{-4, -6,  -8,  -9, 3, 5, 7,  8},
	{-3, -5,  -7,  -9, 2, 4, 6,  8},
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

/**
 * Extend a 6-bit color component to 8-bit color.
 * @param value 6-bit color component.
 * @return 8-bit color value.
 */
static inline uint8_t extend_6to8bits(uint8_t value)
{
	return (value << 2) | (value >> 4);
}

/**
 * Extend a 7-bit color component to 8-bit color.
 * @param value 7-bit color component.
 * @return 7-bit color value.
 */
static inline uint8_t extend_7to8bits(uint8_t value)
{
	return (value << 1) | (value >> 6);
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

// ETC decoding mode.
enum ETC_Decoding_Mode {
	// Bit 0: ETC1 vs. ETC2
	ETC_DM_ETC1 = (0 << 0),	// ETC1
	ETC_DM_ETC2 = (1 << 0),	// ETC2
	ETC_DM_MASK12 = (1 << 0),

	// Bit 1: ETC2 punchthrough alpha
	ETC2_DM_A1 = (1 << 1),
};

/**
 * Decode an ETC1/ETC2 RGB block.
 * @param mode          [in] Mode flags.
 * @param tileBuf	[out] Destination tile buffer.
 * @param src		[in] Source RGB block.
 */
template</* ETC_Decoding_Mode */ unsigned int mode>
static void decodeBlock_ETC_RGB(uint32_t tileBuf[4*4], const etc1_block *etc1_src)
{
	// Prevent invalid combinations from being used.
	static_assert(mode != (ETC_DM_ETC1 | ETC2_DM_A1), "Cannot use ETC1 with punchthrough alpha.");

	// Base colors.
	// For ETC1 mode, these are used as base colors for the two subblocks.
	// For 'T' and 'H' mode, these are used to calculate the paint colors.
	// For 'Planar' mode, three colors are used as 'O', 'H', and 'V'.
	ColorRGB base_color[3];

	// 'T', 'H' modes: Paint colors are used instead of base colors.
	// Intensity modifications are not supported, so we'll store the
	// final xRGB32 values instead of ColorRGB.
	uint32_t paint_color[4];

	// ETC2 block mode.
	etc2_block_mode block_mode = ETC2_BLOCK_MODE_UNKNOWN;

	// TODO: Optimize the extend function by assuming the value is MSB-aligned.

	// control, bit 1: diffbit
	// NOTE: If using punchthrough alpha, this is repurposed as the opaque bit.
	// Hence, individual mode is unavailable.
	if (!(mode & ETC2_DM_A1) && !(etc1_src->control & 0x02)) {
		// Individual mode.
		block_mode = ETC2_BLOCK_MODE_ETC1;
		base_color[0].R = extend_4to8bits(etc1_src->id.R >> 4);
		base_color[0].G = extend_4to8bits(etc1_src->id.G >> 4);
		base_color[0].B = extend_4to8bits(etc1_src->id.B >> 4);
		base_color[1].R = extend_4to8bits(etc1_src->id.R & 0x0F);
		base_color[1].G = extend_4to8bits(etc1_src->id.G & 0x0F);
		base_color[1].B = extend_4to8bits(etc1_src->id.B & 0x0F);
	} else {
		// Other mode.

		// Differential colors are 3-bit two's complement.
		const int8_t dR2 = etc1_3bit_diff_tbl[etc1_src->id.R & 0x07];
		const int8_t dG2 = etc1_3bit_diff_tbl[etc1_src->id.G & 0x07];
		const int8_t dB2 = etc1_3bit_diff_tbl[etc1_src->id.B & 0x07];

		// Sums of R+dR2, G+dG2, and B+dB2 are used to determine the mode.
		// If all of the sums are within [0,31], ETC1 differential mode is used.
		// Otherwise, a new ETC2 mode is used, which may discard some of the above values.
		const int sR = (etc1_src->id.R >> 3) + dR2;
		const int sG = (etc1_src->id.G >> 3) + dG2;
		const int sB = (etc1_src->id.B >> 3) + dB2;

		if ((mode & ETC_DM_MASK12) == ETC_DM_ETC2) {
			// ETC2 block modes are available.
			if ((sR & ~0x1F) != 0) {
				// 'T' mode.
				// Base colors are arranged differently compared to ETC1,
				// and R1 is calculated differently.
				// Note that G and B are arranged slightly differently.
				block_mode = ETC2_BLOCK_MODE_TH;
				base_color[0].R = extend_4to8bits(((etc1_src->t.R1 & 0x18) >> 1) |
								   (etc1_src->t.R1 & 0x03));
				base_color[0].G = extend_4to8bits(etc1_src->t.G1B1 >> 4);
				base_color[0].B = extend_4to8bits(etc1_src->t.G1B1 & 0x0F);
				base_color[1].R = extend_4to8bits(etc1_src->t.R2G2 >> 4);
				base_color[1].G = extend_4to8bits(etc1_src->t.R2G2 & 0x0F);
				base_color[1].B = extend_4to8bits(etc1_src->control >> 4);

				// Determine the paint colors.
				paint_color[0] = clamp_ColorRGB(base_color[0]);
				paint_color[2] = clamp_ColorRGB(base_color[1]);

				// Paint colors 1 and 3 are adjusted using the distance table.
				const uint8_t d = etc2_dist_tbl[((etc1_src->control & 0x0C) >> 1) |
								 (etc1_src->control & 0x01)];
				ColorRGB tmp;
				tmp.R = base_color[1].R + d;
				tmp.G = base_color[1].G + d;
				tmp.B = base_color[1].B + d;
				paint_color[1] = clamp_ColorRGB(tmp);
				tmp.R = base_color[1].R - d;
				tmp.G = base_color[1].G - d;
				tmp.B = base_color[1].B - d;
				paint_color[3] = clamp_ColorRGB(tmp);
			} else if ((sG & ~0x1F) != 0) {
				// 'H' mode.
				// Base colors are arranged differently compared to ETC1,
				// and G1 and B1 are calculated differently.
				block_mode = ETC2_BLOCK_MODE_TH;
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
				ColorRGB tmp;
				tmp.R = base_color[0].R + d;
				tmp.G = base_color[0].G + d;
				tmp.B = base_color[0].B + d;
				paint_color[0] = clamp_ColorRGB(tmp);
				tmp.R = base_color[0].R - d;
				tmp.G = base_color[0].G - d;
				tmp.B = base_color[0].B - d;
				paint_color[1] = clamp_ColorRGB(tmp);
				tmp.R = base_color[1].R + d;
				tmp.G = base_color[1].G + d;
				tmp.B = base_color[1].B + d;
				paint_color[2] = clamp_ColorRGB(tmp);
				tmp.R = base_color[1].R - d;
				tmp.G = base_color[1].G - d;
				tmp.B = base_color[1].B - d;
				paint_color[3] = clamp_ColorRGB(tmp);
			} else if ((sB & ~0x1F) != 0) {
				// 'Planar' mode.
				// TODO: Needs testing - I don't have a sample file with 'Planar' encoding.
				block_mode = ETC2_BLOCK_MODE_PLANAR;

				// 'O' color.
				base_color[0].R = extend_6to8bits((etc1_src->planar.RO_GO1 >> 1) & 0x3F);
				base_color[0].G = extend_7to8bits(((etc1_src->planar.RO_GO1 << 6) & 0x40) |
								  ((etc1_src->planar.GO2_BO1 >> 1) & 0x3F));
				base_color[0].B = extend_6to8bits(((etc1_src->planar.GO2_BO1 << 5) & 0x20) |
								   (etc1_src->planar.BO2_BO3 & 0x18) |
								  ((etc1_src->planar.BO2_BO3 << 1) & 0x06) |
								   (etc1_src->planar.BO3_RH >> 7));

				// 'H' color.
				base_color[1].R = extend_6to8bits(((etc1_src->planar.BO3_RH >> 1) & 0x3C) |
								   (etc1_src->planar.BO3_RH & 0x01));
				base_color[1].G = extend_7to8bits(etc1_src->planar.GH_BH >> 1);
				base_color[1].B = extend_6to8bits(((etc1_src->planar.GH_BH << 5) & 0x20) |
								   (etc1_src->planar.BH_RV >> 3));

				// 'V' color.
				base_color[2].R = extend_6to8bits(((etc1_src->planar.BH_RV << 3) & 0x38) |
								   (etc1_src->planar.RV_GV >> 5));
				base_color[2].G = extend_7to8bits(((etc1_src->planar.RV_GV << 2) & 0x7C) |
								   (etc1_src->planar.GV_BV >> 6));
				base_color[2].B = extend_6to8bits(etc1_src->planar.GV_BV & 0x3F);
			}
		}

		if ((mode & ETC_DM_MASK12) == ETC_DM_ETC1 ||
		    block_mode == ETC2_BLOCK_MODE_UNKNOWN)
		{
			// ETC1 differential mode.
			block_mode = ETC2_BLOCK_MODE_ETC1;
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

	// Process the 16 pixel indexes.
	// TODO: Use SSE2 for saturated arithmetic?
	uint16_t px_msb = be16_to_cpu(etc1_src->msb);
	uint16_t px_lsb = be16_to_cpu(etc1_src->lsb);
	switch (block_mode) {
		default:
			// TODO: Return an error code?
			assert(!"Invalid ETC2 block mode.");
			memset(tileBuf, 0, 4*4*sizeof(uint32_t));
			break;

		case ETC2_BLOCK_MODE_ETC1: {
			// ETC1 block mode.

			// Intensities for the table codewords.
			const int16_t *tbl[2];
			if ((mode & ETC2_DM_A1) && !(etc1_src->control & 0x02)) {
				// ETC2, punchthrough alpha: Opaque bit is unset.
				tbl[0] = etc2_intensity_a1[ etc1_src->control >> 5];
				tbl[1] = etc2_intensity_a1[(etc1_src->control >> 2) & 0x07];
			} else {
				// All other versions.
				tbl[0] = etc1_intensity[ etc1_src->control >> 5];
				tbl[1] = etc1_intensity[(etc1_src->control >> 2) & 0x07];
			}

			// control, bit 0: flip
			uint16_t subblock = etc1_subblock_mapping[etc1_src->control & 0x01];
			for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1, subblock >>= 1) {
				uint32_t *const p = &tileBuf[etc1_mapping[i]];
				const unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

				if ((mode & ETC2_DM_A1) && !(etc1_src->control & 0x02)) {
					// ETC2 punchthrough alpha: opaque bit is 0.
					if (px_idx == 2) {
						// Pixel is completely transparent.
						*p = 0;
						continue;
					}
				}

				// Select the table codeword based on the current subblock.
				const uint8_t cur_sub = subblock & 1;
				const int adj = tbl[cur_sub][px_idx];
				ColorRGB color = base_color[cur_sub];
				color.R += adj;
				color.G += adj;
				color.B += adj;

				// Clamp the color components and save it to the tile buffer.
				*p = clamp_ColorRGB(color);
			}
			break;
		}

		case ETC2_BLOCK_MODE_TH: {
			// ETC2 'T' or 'H' mode.
			for (unsigned int i = 0; i < 16; i++, px_msb >>= 1, px_lsb >>= 1) {
				uint32_t *const p = &tileBuf[etc1_mapping[i]];
				const unsigned int px_idx = ((px_msb & 1) << 1) | (px_lsb & 1);

				if ((mode & ETC2_DM_A1) && !(etc1_src->control & 0x02)) {
					// ETC2 punchthrough alpha: opaque bit is 0.
					if (px_idx == 2) {
						// Pixel is completely transparent.
						*p = 0;
						continue;
					}
				}

				// Pixel index indicates the paint color to use.
				*p = paint_color[px_idx];
			}
			break;
		}

		case ETC2_BLOCK_MODE_PLANAR: {
			// ETC2 'Planar' mode.
			// Each pixel is interpolated using the three RGB676 colors.
			for (unsigned int i = 0; i < 16; i++) {
				// NOTE: Using ETC1 pixel arrangement.
				// Rows first, then columns.
				const int pX = i / 4;
				const int pY = i % 4;

				// Color order: 0, 1, 2 => 'O', 'H', 'V'
				// TODO: SIMD optimization?
				ColorRGB tmp;
				tmp.R = ((pX * (base_color[1].R - base_color[0].R)) +
					 (pY * (base_color[2].R - base_color[0].R)) +
					  (4 *  base_color[0].R) + 2) >> 2;
				tmp.G = ((pX * (base_color[1].G - base_color[0].G)) +
					 (pY * (base_color[2].G - base_color[0].G)) +
					  (4 *  base_color[0].G) + 2) >> 2;
				tmp.B = ((pX * (base_color[1].B - base_color[0].B)) +
					 (pY * (base_color[2].B - base_color[0].B)) +
					  (4 *  base_color[0].B) + 2) >> 2;

				// Clamp the color components and save it to the tile buffer.
				tileBuf[etc1_mapping[i]] = clamp_ColorRGB(tmp);
			}
			break;
		}
	}
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
		// Decode the ETC1 RGB block.
		decodeBlock_ETC_RGB<ETC_DM_ETC1>(tileBuf, etc1_src);

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
		// Decode the ETC2 RGB block.
		decodeBlock_ETC_RGB<ETC_DM_ETC2>(tileBuf, etc1_src);

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
 * Decode an ETC2 alpha block.
 * @param tileBuf	[out] Destination tile buffer.
 * @param src		[in] Source alpha block.
 */
static void decodeBlock_ETC2_alpha(uint32_t tileBuf[4*4], const etc2_alpha *alpha)
{
	// argb32_t for alpha channel handling.
	argb32_t *const pArgb = reinterpret_cast<argb32_t*>(tileBuf);

	// Get the base codeword and multiplier.
	// NOTE: mult == 0 is not allowed to be used by the encoder,
	// but the specification requires decoders to handle it.
	const uint8_t base = alpha->base_codeword;
	const uint8_t mult = (alpha->mult_tbl_idx >> 4);

	// Table pointer.
	const int8_t *const tbl = etc2_alpha_tbl[alpha->mult_tbl_idx & 0x0F];

	// TODO: Zero out the alpha channel in the entire tile using SIMD.

	// Pixel index.
	uint64_t alpha48 = extract48(alpha);

	for (unsigned int i = 0; i < 16; i++, alpha48 >>= 3) {
		// Calculate the alpha value for this pixel.
		int A = base + (tbl[alpha48 & 0x07] * mult);
		if (A > 255) {
			A = 255;
		} else if (A < 0) {
			A = 0;
		}

		// Set the new alpha value.
		pArgb[etc1_mapping[i]].a = (uint8_t)A;
	}
}

/**
 * Convert an ETC2 RGBA image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGBA image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromETC2_RGBA(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (width * height));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height))
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

	const etc2_rgba_block *etc2_src = reinterpret_cast<const etc2_rgba_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, etc2_src++) {
		// Decode the ETC2 RGB block.
		decodeBlock_ETC_RGB<ETC_DM_ETC2>(tileBuf, &etc2_src->etc1);

		// Decode the ETC2 alpha block.
		// TODO: Don't fill in the alpha channel in decodeBlock_ETC2_RGB()?
		decodeBlock_ETC2_alpha(tileBuf, &etc2_src->alpha);

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert an ETC2 RGB+A1 (punchthrough alpha) image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB+A1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromETC2_RGB_A1(int width, int height,
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
		// Decode the ETC2 RGB block.
		decodeBlock_ETC_RGB<ETC_DM_ETC2 | ETC2_DM_A1>(tileBuf, etc1_src);

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,1};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_BC7.cpp: Image decoding functions. (BC7)                   *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

// C++ includes.
#include <algorithm>	// std::swap() until C++11
#include <utility>	// std::swap() since C++11

// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/hh308953(v=vs.85).aspx
// - https://msdn.microsoft.com/en-us/library/windows/desktop/hh308954(v=vs.85).aspx

namespace LibRpBase {

// Interpolation values.
static const uint8_t aWeight2[] = {0, 21, 43, 64};
static const uint8_t aWeight3[] = {0, 9, 18, 27, 37, 46, 55, 64};
static const uint8_t aWeight4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

/**
 * Interpolate a color component.
 * @tparam bits Index precision, in number of bits.
 * @param index Color/alpha index.
 * @param e0 Endpoint 0 component.
 * @param e1 Endpoint 1 component.
 * @return Interpolated color component.
 */
template<int bits>
static FORCEINLINE uint8_t interpolate_component(unsigned int index, uint8_t e0, uint8_t e1)
{
	static_assert(bits == 2 || bits == 3 || bits == 4, "Unsupported `bits` value.");

	// Shortcut for no-interpolation cases.
	// TODO: Does this actually improve performance?
	if (index == 0) {
		return e0;
	} else if (index == (1<<bits)-1) {
		return e1;
	}

	uint8_t weight;
	switch (bits) {
		case 2:
			weight = aWeight2[index];
			break;
		case 3:
			weight = aWeight3[index];
			break;
		case 4:
			weight = aWeight4[index];
			break;
		default:
			// Should not happen...
			return 0;
	}

	return (uint8_t)((((64 - weight) * (unsigned int)e0) +
			       ((weight  * (unsigned int)e1) + 32)) >> 6);
}

/**
 * Interpolate color values.
 * @tparam bits Index precision, in number of bits.
 * @tparam alpha If true, interpolate the alpha channel.
 * @param index Color/alpha index.
 * @param colors Color endpoints. (2-element array)
 * @return Interpolated ARGB32 color.
 */
template<int bits, bool alpha>
static inline uint32_t interpolate_color(unsigned int index, const argb32_t colors[2])
{
	argb32_t argb;
	argb.r = interpolate_component<bits>(index, colors[0].r, colors[1].r);
	argb.g = interpolate_component<bits>(index, colors[0].g, colors[1].g);
	argb.b = interpolate_component<bits>(index, colors[0].b, colors[1].b);
	if (alpha) {
		argb.a = interpolate_component<bits>(index, colors[0].a, colors[1].a);
	} else {
		argb.a = 255;
	}
	return argb.u32;
}

/**
 * Convert a BC7 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC7 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromBC7(int width, int height,
	const uint8_t *img_buf, int img_siz)
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

	// BC7 uses 4x4 tiles.
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

	// BC7 has eight block modes with varying properties, including
	// bitfields of different lengths. As such, the only guaranteed
	// block format we have is 128-bit little-endian, which will be
	// represented as four uint32_t values.
	const uint32_t *bc7_src = reinterpret_cast<const uint32_t*>(img_buf);

	// Temporary tile buffer.
	argb32_t tileBuf[4*4];

	// Temporary ARGB and alpha values.
	argb32_t colors[6];
	memset(&colors, 0, sizeof(colors));
	uint8_t alpha[2] = {0, 0};

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, bc7_src += 4) {
		// FIXME: Proper support on big-endian.
		// Not doing byteswapping right now...

		bool done = true;
		uint32_t cx;	// Temporary color value.

		// P-bits represent the LSB of the various color components.

		// Rotation bits:
		// - 00: ARGB - no swapping
		// - 01: RAGB - swap A and R
		// - 10: GRAB - swap A and G
		// - 11: BRGA - swap A and B

		// Check the block mode.
		const uint8_t mode = le32_to_cpu(bc7_src[0]) & 0xFF;
		if (mode & 0x01) {
			/**
			 * Mode 0:
			 * - Color components only (no alpha)
			 * - 3 subsets per block
			 * - RGBP 4.4.4.1 endpoints with a unique P-bit per endpoint
			 * - 3-bit indexes
			 * - 16 partitions
			 *
			 * LSB: [1]
			 *      [4-bit partition]
			 *      [4-bit R0][4-bit R1][4-bit R2][4-bit R3][4-bit R4][4-bit R5]
			 *      [4-bit G0][4-bit G1][4-bit G2][4-bit G3][4-bit G4][4-bit G5]
			 *      [4-bit B0][4-bit B1][4-bit B2][4-bit B3][4-bit B4][4-bit B5]
			 *      [1-bit P0][1-bit P1][1-bit P2][1-bit P3][1-bit P4][1-bit P5]
			 *      [45-bit index] MSB
			 */
			done = false;
			cx = 0xFF000000;
		} else if ((mode & 0x03) == 0x02) {
			/**
			 * Mode 1:
			 * - Color components only (no alpha)
			 * - 2 subsets per block
			 * - RGBP 6.6.6.1 endpoints with a shared P-bit per subset
			 * - 3-bit indexes
			 * - 64 partitions
			 *
			 * LSB: [01]
			 *      [6-bit partition]
			 *      [6-bit R0][6-bit R1][6-bit R2][6-bit R3]
			 *      [6-bit G0][6-bit G1][6-bit G2][6-bit G3]
			 *      [6-bit B0][6-bit B1][6-bit B2][6-bit B3]
			 *      [1-bit P0][1-bit P1]
			 *      [46-bit index] MSB
			 */
			done = false;
			cx = 0xFF0000FF;
		} else if ((mode & 0x07) == 0x04) {
			/**
			 * Mode 2:
			 * - Color components only (no alpha)
			 * - 3 subsets per block
			 * - RGB 5.5.5 endpoints
			 * - 2-bit indexes
			 * - 64 partitions
			 *
			 * LSB: [001]
			 *      [6-bit partition]
			 *      [5-bit R0][5-bit R1][5-bit R2][5-bit R3][5-bit R4][5-bit R5]
			 *      [5-bit G0][5-bit G1][5-bit G2][5-bit G3][5-bit G4][5-bit G5]
			 *      [5-bit B0][5-bit B1][5-bit B2][5-bit B3][5-bit B4][5-bit B5]
			 *      [29-bit index] MSB
			 */
			done = false;
			cx = 0xFF00FF00;
		} else if ((mode & 0x0F) == 0x08) {
			/**
			 * Mode 3:
			 * - Color components only (no alpha)
			 * - 2 subsets per block
			 * - RGBP 7.7.7.1 endpoints with a unqiue P-bit per subset
			 * - 2-bit indexes
			 * - 64 partitions
			 *
			 * LSB: [0001]
			 *      [6-bit partition]
			 *      [7-bit R0][7-bit R1][7-bit R2][7-bit R3]
			 *      [7-bit G0][7-bit G1][7-bit G2][7-bit G3]
			 *      [7-bit B0][7-bit B1][7-bit B2][7-bit B3]
			 *      [1-bit P0][1-bit P1][1-bit P2][1-bit P3]
			 *      [30-bit index] MSB
			 */
			done = false;
			cx = 0xFF00FFFF;
		} else if ((mode & 0x1F) == 0x10) {
			/**
			 * Mode 4:
			 * - Color components with separate alpha component
			 * - 1 subset per block
			 * - RGB 5.5.5 color endpoints
			 * - 6-bit alpha endpoints
			 * - 16 x 2-bit indexes
			 * - 16 x 3-bit indexes
			 * - 2-bit component rotation
			 * - 1-bit index selector (whether the 2- or 3-bit indexes are used)
			 *
			 * LSB: [00001]
			 *      [2-bit rotation]
			 *      [1-bit idxMode]
			 *      [5-bit R0][5-bit R1]
			 *      [5-bit G0][5-bit G1]
			 *      [5-bit B0][5-bit B1]
			 *      [6-bit A0][6-bit A1]
			 *      [31-bit index] MSB
			 *      [47-bit index] MSB
			 */
			done = false;
			cx = 0xFFFF0000;
		} else if ((mode & 0x3F) == 0x20) {
			/**
			 * Mode 5:
			 * - Color components with separate alpha component
			 * - 1 subset per block
			 * - RGB 7.7.7 color endpoints
			 * - 6-bit alpha endpoints
			 * - 16 x 2-bit color indexes
			 * - 16 x 2-bit alpha indexes
			 * - 2-bit component rotation
			 *
			 * LSB: [000001]
			 *      [2-bit rotation]
			 *      [7-bit R0][7-bit R1]
			 *      [7-bit G0][7-bit G1]
			 *      [7-bit B0][7-bit B1]
			 *      [8-bit A0][8-bit A1]
			 *      [31-bit color index data] MSB
			 *      [31-bit alpha index data] MSB
			 */

			// Extract the RGB and alpha values.
			// 7-bit RGB; duplicate the MSB in the LSB.
			colors[0].r = (bc7_src[0] >>  7) & 0xFE; colors[0].r |= (colors[0].r >> 7);
			colors[1].r = (bc7_src[0] >> 14) & 0xFE; colors[1].r |= (colors[1].r >> 7);

			colors[0].g = (bc7_src[0] >> 21) & 0xFE; colors[0].g |= (colors[0].g >> 7);
			colors[1].g = ((bc7_src[0] >> 28) & 0x0E) | ((bc7_src[1] << 4) & 0xF0);
				colors[1].g |= (colors[1].g >> 7);

			colors[0].b = (bc7_src[1] >>  3) & 0xFE; colors[0].b |= (colors[0].b >> 7);
			colors[1].b = (bc7_src[1] >> 10) & 0xFE; colors[0].b |= (colors[1].b >> 7);

			alpha[0] = (bc7_src[1] >> 18);
			alpha[1] = ((bc7_src[1] >> 26) & 0x3F) | ((bc7_src[2] << 6) & 0xC0);

			// Rotation bits.
			const uint8_t rot_bits = (bc7_src[1] >> 6) & 3;

			// Get the indexes.
			// NOTE: Unspecified index bits are fixed at 0.
			uint32_t index_color = (bc7_src[2] >> 2) | ((bc7_src[3] & 1) << 31);
			uint32_t index_alpha = bc7_src[3] >> 1;

			// Each index is two bits, which represents one of the two endpoints.
			for (unsigned int i = 0; i < 16; i++, index_color >>= 2, index_alpha >>= 2) {
				tileBuf[i].u32 = interpolate_color<2,false>(index_color & 3, colors);
				tileBuf[i].a = interpolate_component<2>(index_alpha & 3, alpha[0], alpha[1]);

				// Check for rotation bits.
				// TODO: Optimize using u8 indexing?
				switch (rot_bits & 3) {
					case 0:
						// Nothing to do here.
						break;
					case 1:
						// Swap A and R.
						std::swap(tileBuf[i].a, tileBuf[i].r);
						break;
					case 2:
						// Swap A and G.
						std::swap(tileBuf[i].a, tileBuf[i].g);
						break;
					case 3:
						// Swap A and B.
						std::swap(tileBuf[i].a, tileBuf[i].b);
						break;
					default:
						break;
				}
			}
		} else if ((mode & 0x7F) == 0x40) {
			/**
			 * Mode 6:
			 * - Combined color and alpha components
			 * - 1 subset per block
			 * - RGBAP 7.7.7.7.1 color (and alpha) endpoints (unqiue P-bit per endpoint)
			 * - 16 x 4-bit indexes
			 *
			 * LSB: [0000001]
			 *      [7-bit R0][7-bit R1]
			 *      [7-bit G0][7-bit G1]
			 *      [7-bit B0][7-bit B1]
			 *      [7-bit A0][7-bit A1]
			 *      [1-bit P0][1-bit P1]
			 *      [63-bit index data] MSB
			 */
			done = false;
			cx = 0xFFFFFF00;
		} else if ((mode & 0xFF) == 0x80) {
			/**
			 * Mode 7:
			 * - Combined color and alpha components
			 * - 2 subsets per block
			 * - RGBAP 5.5.5.5.1 color (and alpha) endpoints (unqiue P-bit per endpoint)
			 * - 2-bit indexes
			 * - 64-bit partitions
			 *
			 * LSB: [00000001]
			 *      [5-bit R0][5-bit R1][5-bit R2][5-bit R3]
			 *      [5-bit G0][5-bit G1][5-bit G2][5-bit G3]
			 *      [5-bit B0][5-bit B1][5-bit B2][5-bit B3]
			 *      [5-bit A0][5-bit A1][5-bit A2][5-bit A3]
			 *      [1-bit P0][1-bit P1][1-bit P2][1-bit P3]
			 *      [30-bit index data] MSB
			 */
			done = false;
			cx = 0xFFFFFFFF;
		} else {
			// Invalid mode.
			done = false;
			cx = 0xFFFFFFFF;
		}

		if (!done) {
			// Not implemented yet. Use the fake color.
			for (unsigned int i = 0; i < 16; i++) {
				// TODO: decode
				tileBuf[i].u32 = cx;
			}
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img,
			reinterpret_cast<const uint32_t*>(&tileBuf[0]), x, y);
	} }

	// Image has been converted.
	return img;
}

}

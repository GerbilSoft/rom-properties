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

// Partition definitions for modes with 2 subsets.
// References:
// - https://rockets2000.wordpress.com/2015/05/19/bc7-partitions-subsets/
// - https://github.com/hglm/detex/blob/master/bptc-tables.c
// TODO: Optimize into bitfields.
static uint8_t bc7_2sub[64][16] = {
	// 0
	{0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1},
	{0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
	{0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1},
	{0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1},
	{0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1},
	{0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1},

	// 8
	{0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1},
	{0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1},
	{0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
	{0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1},

	// 16
	{0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1},
	{0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0},
	{0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0},
	{0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0},
	{0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0},
	{0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1},

	// 24
	{0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0},
	{0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0},
	{0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0},
	{0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0},
	{0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0},
	{0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0},
	{0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0},
	{0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0},

	// 32
	{0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1},
	{0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1},
	{0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0},
	{0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0},
	{0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0},
	{0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0},
	{0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1},
	{0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1},

	// 40
	{0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0},
	{0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0},
	{0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0},
	{0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0},
	{0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0},
	{0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1},
	{0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1},
	{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},

	// 48
	{0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0},
	{0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0},
	{0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0},
	{0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0},
	{0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1},
	{0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1},
	{0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0},
	{0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0},

	// 56
	{0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1},
	{0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1},
	{0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1},
	{0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1},
	{0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1},
	{0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1},
	{0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0},
	{0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0},
	{0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1},
};

// Partition definitions for modes with 2 subsets.
// References:
// - https://rockets2000.wordpress.com/2015/05/19/bc7-partitions-subsets/
// - https://github.com/hglm/detex/blob/master/bptc-tables.c
// TODO: Optimize into bitfields.
static uint8_t bc7_3sub[64][16] = {
	// 0
	{0,0,1,1,0,0,1,1,0,2,2,1,2,2,2,2},
	{0,0,0,1,0,0,1,1,2,2,1,1,2,2,2,1},
	{0,0,0,0,2,0,0,1,2,2,1,1,2,2,1,1},
	{0,2,2,2,0,0,2,2,0,0,1,1,0,1,1,1},
	{0,0,0,0,0,0,0,0,1,1,2,2,1,1,2,2},
	{0,0,1,1,0,0,1,1,0,0,2,2,0,0,2,2},
	{0,0,2,2,0,0,2,2,1,1,1,1,1,1,1,1},
	{0,0,1,1,0,0,1,1,2,2,1,1,2,2,1,1},

	// 8
	{0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2},
	{0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2},
	{0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2},
	{0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2},
	{0,1,1,2,0,1,1,2,0,1,1,2,0,1,1,2},
	{0,1,2,2,0,1,2,2,0,1,2,2,0,1,2,2},
	{0,0,1,1,0,1,1,2,1,1,2,2,1,2,2,2},
	{0,0,1,1,2,0,0,1,2,2,0,0,2,2,2,0},

	// 16
	{0,0,0,1,0,0,1,1,0,1,1,2,1,1,2,2},
	{0,1,1,1,0,0,1,1,2,0,0,1,2,2,0,0},
	{0,0,0,0,1,1,2,2,1,1,2,2,1,1,2,2},
	{0,0,2,2,0,0,2,2,0,0,2,2,1,1,1,1},
	{0,1,1,1,0,1,1,1,0,2,2,2,0,2,2,2},
	{0,0,0,1,0,0,0,1,2,2,2,1,2,2,2,1},
	{0,0,0,0,0,0,1,1,0,1,2,2,0,1,2,2},
	{0,0,0,0,1,1,0,0,2,2,1,0,2,2,1,0},

	// 24
	{0,1,2,2,0,1,2,2,0,0,1,1,0,0,0,0},
	{0,0,1,2,0,0,1,2,1,1,2,2,2,2,2,2},
	{0,1,1,0,1,2,2,1,1,2,2,1,0,1,1,0},
	{0,0,0,0,0,1,1,0,1,2,2,1,1,2,2,1},
	{0,0,2,2,1,1,0,2,1,1,0,2,0,0,2,2},
	{0,1,1,0,0,1,1,0,2,0,0,2,2,2,2,2},
	{0,0,1,1,0,1,2,2,0,1,2,2,0,0,1,1},
	{0,0,0,0,2,0,0,0,2,2,1,1,2,2,2,1},

	// 32
	{0,0,0,0,0,0,0,2,1,1,2,2,1,2,2,2},
	{0,2,2,2,0,0,2,2,0,0,1,2,0,0,1,1},
	{0,0,1,1,0,0,1,2,0,0,2,2,0,2,2,2},
	{0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0},
	{0,0,0,0,1,1,1,1,2,2,2,2,0,0,0,0},
	{0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0},
	{0,1,2,0,2,0,1,2,1,2,0,1,0,1,2,0},
	{0,0,1,1,2,2,0,0,1,1,2,2,0,0,1,1},

	// 40
	{0,0,1,1,1,1,2,2,2,2,0,0,0,0,1,1},
	{0,1,0,1,0,1,0,1,2,2,2,2,2,2,2,2},
	{0,0,0,0,0,0,0,0,2,1,2,1,2,1,2,1},
	{0,0,2,2,1,1,2,2,0,0,2,2,1,1,2,2},
	{0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1},
	{0,2,2,0,1,2,2,1,0,2,2,0,1,2,2,1},
	{0,1,0,1,2,2,2,2,2,2,2,2,0,1,0,1},
	{0,0,0,0,2,1,2,1,2,1,2,1,2,1,2,1},

	// 48
	{0,1,0,1,0,1,0,1,0,1,0,1,2,2,2,2},
	{0,2,2,2,0,1,1,1,0,2,2,2,0,1,1,1},
	{0,0,0,2,1,1,1,2,0,0,0,2,1,1,1,2},
	{0,0,0,0,2,1,1,2,2,1,1,2,2,1,1,2},
	{0,2,2,2,0,1,1,1,0,1,1,1,0,2,2,2},
	{0,0,0,2,1,1,1,2,1,1,1,2,0,0,0,2},
	{0,1,1,0,0,1,1,0,0,1,1,0,2,2,2,2},
	{0,0,0,0,0,0,0,0,2,1,1,2,2,1,1,2},

	// 56
	{0,1,1,0,0,1,1,0,2,2,2,2,2,2,2,2},
	{0,0,2,2,0,0,1,1,0,0,1,1,0,0,2,2},
	{0,0,2,2,1,1,2,2,1,1,2,2,0,0,2,2},
	{0,0,0,0,0,0,0,0,0,0,0,0,2,1,1,2},
	{0,0,0,2,0,0,0,1,0,0,0,2,0,0,0,1},
	{0,2,2,2,1,2,2,2,0,2,2,2,1,2,2,2},
	{0,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2},
	{0,1,1,1,2,0,1,1,2,2,0,1,2,2,2,0},
};

/**
 * Interpolate a color component.
 * @tparam bits Index precision, in number of bits.
 * @param index Color/alpha index.
 * @param e0 Endpoint 0 component.
 * @param e1 Endpoint 1 component.
 * @return Interpolated color component.
 */
static uint8_t interpolate_component(unsigned int bits, unsigned int index, uint8_t e0, uint8_t e1)
{
	assert(bits >= 2 && bits <= 4);
	assert(index < (1U << bits));

	// Shortcut for no-interpolation cases.
	// TODO: Does this actually improve performance?
	if (index == 0) {
		return e0;
	} else if (index == (1U<<bits)-1) {
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
 * Get the mode number.
 * @param dword0 LSB DWORD.
 * @return Mode number.
 */
static inline int get_mode(uint32_t dword0)
{
	// TODO: ctz/_BitScanForward?
	for (int i = 0; i < 8; i++, dword0 >>= 1) {
		if (dword0 & 1) {
			// Found the mode number.
			return i;
		}
	}

	// Invalid mode.
	assert(!"BC7 block has an invalid mode.");
	return -1;
}

/**
 * Right-shift two 64-bit values as if it's a single 128-bit value.
 * @param msb	[in/out] MSB QWORD
 * @param lsb	[in/out] LSB QWORD
 * @param shamt [in] Shift amount
 */
static inline void rshift128(uint64_t &msb, uint64_t &lsb, unsigned int shamt)
{
	if (shamt > 128) {
		// Shifting more than 128 bits.
		lsb = 0;
		msb = 0;
	} else if (shamt > 64) {
		// Shifting more than 64 bits.
		lsb = msb >> (shamt - 64);
		msb = 0;
	} else {
		// Shift LSB first.
		lsb >>= (shamt - 64);
		// Copy from MSB to LSB.
		lsb |= (msb << (64 - shamt));
		// Shift MSB next.
		msb >>= shamt;
	}
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
	// represented as two uint64_t values, which will be shifted
	// as each component is processed.
	// TODO: Optimize by using fewer shifts?
	const uint64_t *bc7_src = reinterpret_cast<const uint64_t*>(img_buf);

	// Temporary tile buffer.
	argb32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, bc7_src += 2) {
		/** BEGIN: Temporary values. **/

		// Endpoints.
		// - [6]: Individual endpoints.
		// - [3]: RGB components.
		uint8_t endpoints[6][3];

		// Alpha components.
		// If the selected mode doesn't have alpha,
		// this will be set to {255, 255}.
		uint8_t alpha[2];

		/** END: Temporary values. **/

		// TODO: Make sure this is correct on big-endian.
		uint64_t lsb = le64_to_cpu(bc7_src[0]);
		uint64_t msb = le64_to_cpu(bc7_src[1]);

		// Check the block mode.
		const int mode = get_mode(static_cast<uint32_t>(lsb));
		if (mode < 0) {
			// Invalid mode.
			delete img;
			return nullptr;
		}
		rshift128(msb, lsb, mode+1);

		// Rotation mode.
		// Only present in modes 4 and 5.
		// For all other modes, this is assumed to be 00.
		// - 00: ARGB - no swapping
		// - 01: RAGB - swap A and R
		// - 10: GRAB - swap A and G
		// - 11: BRGA - swap A and B
		uint8_t rotation_mode;
		if (mode == 4 || mode == 5) {
			rotation_mode = lsb & 3;
			rshift128(msb, lsb, 2);
		} else {
			// No rotation.
			rotation_mode = 0;
		}

		// Bits per index. (either 2 or 3)
		// NOTE: Most modes don't have the full 32-bit or 48-bit
		// index table. Missing bits are assumed to be 0.
		static const uint8_t IndexBits[8] = {3, 3, 2, 2, 0, 2, 4, 2};
		unsigned int index_bits = IndexBits[mode];
		if (index_bits == 0) {
			// Mode 4: Selectable between 2 and 3.
			index_bits = (lsb & 1 ? 3 : 2);
			rshift128(msb, lsb, 1);
		}

		// Subset/partition.
		static const uint8_t SubsetCount[8] = {3, 2, 3, 2, 1, 1, 1, 2};
		static const uint8_t PartitionBits[8] = {4, 6, 6, 6, 0, 0, 0, 6};
		const uint8_t *subset;
		if (PartitionBits[mode] != 0) {
			unsigned int partition = lsb & ((1U << PartitionBits[mode]) - 1);
			rshift128(msb, lsb, PartitionBits[mode]);

			// Determine the subset to use.
			switch (SubsetCount[mode]) {
				default:
				case 1:
					// One subset.
					subset = nullptr;
					break;
				case 2:
					// Two subsets.
					subset = &bc7_2sub[partition][0];
					break;
				case 3:
					// Three subsets.
					subset = &bc7_3sub[partition][0];
					break;
			}
		} else {
			// No subsets/partitions.
			subset = nullptr;
		}

		// P-bits.
		// NOTE: These are applied per subset.
		// The P-bit count is needed here in order to determine the
		// shift amount for the endpoints and alpha values.
		static const uint8_t PBitCount[8] = {1, 1, 0, 1, 0, 0, 1, 1};

		// Number of endpoints.
		static const uint8_t EndpointCount[8] = {6, 4, 6, 4, 2, 2, 2, 4};
		// Bits per endpoint component.
		static const uint8_t EndpointBits[8] = {4, 6, 5, 7, 5, 7, 7, 5};

		// Extract and extend the components.
		// NOTE: Components are stored in RRRR/GGGG/BBBB/AAAA order.
		// Needs to be shuffled for RGBA.
		uint8_t endpoint_bits = EndpointBits[mode];
		uint8_t endpoint_count = EndpointCount[mode];
		const uint8_t endpoint_mask = (1U << endpoint_bits) - 1;
		const uint8_t endpoint_shamt = (8 - endpoint_bits);
		const unsigned int component_count = endpoint_count * 3;
		uint8_t ep_idx = 0, comp_idx = 0;
		for (unsigned int i = 0; i < component_count; i++) {
			endpoints[ep_idx][comp_idx] = (lsb & endpoint_mask) << endpoint_shamt;
			ep_idx++;
			if (ep_idx == endpoint_count) {
				// Next component.
				comp_idx++;
				ep_idx = 0;
			}

			// Shift the data over.
			rshift128(msb, lsb, endpoint_bits);
		}

		// Do we have alpha components?
		static const uint8_t AlphaBits[8] = {0, 0, 0, 0, 6, 8, 7, 5};
		uint8_t alpha_bits = AlphaBits[mode];
		if (alpha_bits != 0) {
			// We have alpha components.
			// TODO: Optimize shifting?
			const uint8_t alpha_mask = (1U << alpha_bits) - 1;
			const uint8_t alpha_shamt = (8 - alpha_bits);
			alpha[0] = (lsb & alpha_mask) << alpha_shamt;
			alpha[1] = ((lsb >> alpha_bits) & alpha_mask) << alpha_shamt;
			rshift128(msb, lsb, alpha_bits * 2);
		} else {
			// No alpha. Use 255.
			alpha[0] = 255;
			alpha[1] = 255;
		}

		// P-bits.
		if (PBitCount[mode] != 0) {
			// Optimization to avoid having to shift the
			// whole 64-bit and/or 128-bit value multiple times.
			unsigned int lsb8 = (lsb & 0xFF);
			if (mode == 1) {
				// Mode 1: Two P-bits for four subsets.
				const uint8_t p_bit0 = (lsb8 & 1) << 1;
				const uint8_t p_bit1 = (lsb8 & 2);

				// Subset 0
				endpoints[0][0] |= p_bit0;
				endpoints[0][1] |= p_bit0;
				endpoints[0][2] |= p_bit0;
				endpoints[1][0] |= p_bit0;
				endpoints[1][1] |= p_bit0;
				endpoints[1][2] |= p_bit0;

				// Subset 1
				endpoints[2][0] |= p_bit1;
				endpoints[2][1] |= p_bit1;
				endpoints[2][2] |= p_bit1;
				endpoints[3][0] |= p_bit1;
				endpoints[3][1] |= p_bit1;
				endpoints[3][2] |= p_bit1;

				rshift128(msb, lsb, 2);
			} else {
				// Other modes: Unique P-bit for each subset.
				const uint8_t p_shamt = 7 - endpoint_bits;
				for (unsigned int i = 0; i < endpoint_count; i++, lsb8 >>= 1) {
					const uint8_t p_bit = (lsb8 & 1) << p_shamt;
					endpoints[i][0] |= p_bit;
					endpoints[i][1] |= p_bit;
					endpoints[i][2] |= p_bit;
				}
				rshift128(msb, lsb, EndpointCount[mode]);
			}

			// Increase the endpoint bits to indicate how many bits
			// need to be copied when expanding the color value.
			endpoint_bits++;
		}

		// At this point, the only remaining data is indexes,
		// which fits entirely into LSB. Hence, we can stop
		// using rshift128().

		// Process the index data.
		// (Mode 5: Color index data.)
		const unsigned int index_mask = (1U << index_bits) - 1;
		for (unsigned int i = 0; i < 16; i++, lsb >>= index_bits) {
			const unsigned int data_idx = lsb & index_mask;
			const unsigned int ep_idx = (subset ? subset[i] * 2 : 0);

			tileBuf[i].r = interpolate_component(index_bits, data_idx, endpoints[ep_idx][0], endpoints[ep_idx+1][0]);
			tileBuf[i].g = interpolate_component(index_bits, data_idx, endpoints[ep_idx][1], endpoints[ep_idx+1][1]);
			tileBuf[i].b = interpolate_component(index_bits, data_idx, endpoints[ep_idx][2], endpoints[ep_idx+1][2]);
		}

		// TODO: Alpha handling.
		// For now, set it to 255.
		for (unsigned int i = 0; i < 16; i++) {
			tileBuf[i].a = 255;
		}

		// Component rotation?
		// TODO: Optimize with SSSE3.
		switch (rotation_mode) {
			case 0:
				// ARGB: No rotation.
				break;
			case 1:
				// RAGB: Swap A and R.
				for (unsigned int i = 0; i < 16; i++) {
					std::swap(tileBuf[i].a, tileBuf[i].r);
				}
				break;
			case 2:
				// GRAB: Swap A and G.
				for (unsigned int i = 0; i < 16; i++) {
					std::swap(tileBuf[i].a, tileBuf[i].g);
				}
				break;
			case 3:
				// BRGA: Swap A and B.
				for (unsigned int i = 0; i < 16; i++) {
					std::swap(tileBuf[i].a, tileBuf[i].b);
				}
				break;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img,
			reinterpret_cast<const uint32_t*>(&tileBuf[0]), x, y);
	} }

	// Image has been converted.
	return img;
}

}

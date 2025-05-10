/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_BC7.cpp: Image decoding functions: BC7                     *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "ImageDecoder_ASTC.hpp"
#include "ImageDecoder_p.hpp"

// C++ STL classes
using std::array;

// References:
// - https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format
// - https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format-mode-reference

namespace LibRpTexture { namespace ImageDecoder {

// Interpolation values.
static constexpr array<uint8_t,  4> aWeight2 = {{0, 21, 43, 64}};
static constexpr array<uint8_t,  8> aWeight3 = {{0, 9, 18, 27, 37, 46, 55, 64}};
static constexpr array<uint8_t, 16> aWeight4 = {{0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64}};

/** Partition definitions. **/

// Each 32-bit value defines a partition with 2 or 3 subsets.
// For 2-subset modes, every two bits can be either 00 or 01.
// For 3-subset modes, every two bits can be 00, 01, or 10.

// Partition definitions for modes with 2 subsets.
// References:
// - https://rockets2000.wordpress.com/2015/05/19/bc7-partitions-subsets/
// - https://github.com/hglm/detex/blob/master/bptc-tables.c
static constexpr array<uint32_t, 64> bc7_2sub = {{
	0x50505050, 0x40404040, 0x54545454, 0x54505040,
	0x50404000, 0x55545450, 0x55545040, 0x54504000,
	0x50400000, 0x55555450, 0x55544000, 0x54400000,
	0x55555440, 0x55550000, 0x55555500, 0x55000000,
	0x55150100, 0x00004054, 0x15010000, 0x00405054,
	0x00004050, 0x15050100, 0x05010000, 0x40505054,
	0x00404050, 0x05010100, 0x14141414, 0x05141450,
	0x01155440, 0x00555500, 0x15014054, 0x05414150,
	0x44444444, 0x55005500, 0x11441144, 0x05055050,
	0x05500550, 0x11114444, 0x41144114, 0x44111144,
	0x15055054, 0x01055040, 0x05041050, 0x05455150,
	0x14414114, 0x50050550, 0x41411414, 0x00141400,
	0x00041504, 0x00105410, 0x10541000, 0x04150400,
	0x50410514, 0x41051450, 0x05415014, 0x14054150,
	0x41050514, 0x41505014, 0x40011554, 0x54150140,
	0x50505500, 0x00555050, 0x15151010, 0x54540404
}};

// Partition definitions for modes with 3 subsets.
// References:
// - https://rockets2000.wordpress.com/2015/05/19/bc7-partitions-subsets/
// - https://github.com/hglm/detex/blob/master/bptc-tables.c
static constexpr array<uint32_t, 64> bc7_3sub = {{
	0xAA685050, 0x6A5A5040, 0x5A5A4200, 0x5450A0A8,
	0xA5A50000, 0xA0A05050, 0x5555A0A0, 0x5A5A5050,
	0xAA550000, 0xAA555500, 0xAAAA5500, 0x90909090,
	0x94949494, 0xA4A4A4A4, 0xA9A59450, 0x2A0A4250,
	0xA5945040, 0x0A425054, 0xA5A5A500, 0x55A0A0A0,
	0xA8A85454, 0x6A6A4040, 0xA4A45000, 0x1A1A0500,
	0x0050A4A4, 0xAAA59090, 0x14696914, 0x69691400,
	0xA08585A0, 0xAA821414, 0x50A4A450, 0x6A5A0200,
	0xA9A58000, 0x5090A0A8, 0xA8A09050, 0x24242424,
	0x00AA5500, 0x24924924, 0x24499224, 0x50A50A50,
	0x500AA550, 0xAAAA4444, 0x66660000, 0xA5A0A5A0,
	0x50A050A0, 0x69286928, 0x44AAAA44, 0x66666600,
	0xAA444444, 0x54A854A8, 0x95809580, 0x96969600,
	0xA85454A8, 0x80959580, 0xAA141414, 0x96960000,
	0xAAAA1414, 0xA05050A0, 0xA0A5A5A0, 0x96000000,
	0x40804080, 0xA9A8A9A8, 0xAAAAAA44, 0x2A4A5254
}};

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
#ifdef _MSC_VER
// MSVC doesn't like using constexpr with _BitScanForward().
static inline int get_mode(uint32_t dword0)
#else /* !_MSC_VER */
static inline constexpr int get_mode(uint32_t dword0)
#endif /* _MSC_VER */
{
#ifdef _MSC_VER
	unsigned long index;
	if (!_BitScanForward(&index, dword0)) {
		assert(!"BC7 block has an invalid mode.");
		return -1;
	}
	return index;
#else /* !_MSC_VER */
	if (!dword0) {
		assert(!"BC7 block has an invalid mode.");
		return -1;
	}
	return __builtin_ctz(dword0);
#endif
}

// Anchor indexes for the second subset (idx == 1) in 2-subset modes.
static constexpr array<uint8_t, 64> anchorIndexes_subset2of2 = {{
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15,  2,  8,  2,  2,  8,  8, 15,
	 2,  8,  2,  2,  8,  8,  2,  2,
	15, 15,  6,  8,  2,  8, 15, 15,
	 2,  8,  2,  2,  2, 15, 15,  6,
	 6,  2,  6,  8, 15, 15,  2,  2,
	15, 15, 15, 15, 15,  2,  2, 15,
}};

// Anchor indexes for the second subset (idx == 1) in 3-subset modes.
static constexpr array<uint8_t, 64> anchorIndexes_subset2of3 = {{
	 3,  3, 15, 15,  8,  3, 15, 15,
	 8,  8,  6,  6,  6,  5,  3,  3,
	 3,  3,  8, 15,  3,  3,  6, 10,
	 5,  8,  8,  6,  8,  5, 15, 15,
	 8, 15,  3,  5,  6, 10,  8, 15,
	15,  3, 15,  5, 15, 15, 15, 15,
	 3, 15,  5,  5,  5,  8,  5, 10,
	 5, 10,  8, 13, 15, 12,  3,  3,
}};

// Anchor indexes for the third subset (idx == 2) in 3-subset modes.
static constexpr array<uint8_t, 64> anchorIndexes_subset3of3 = {{
	15,  8,  8,  3, 15, 15,  3,  8,
	15, 15, 15, 15, 15, 15, 15,  8,
	15,  8, 15,  3, 15,  8, 15,  8,
	 3, 15,  6, 10, 15, 15, 10,  8,
	15,  3, 15, 10, 10,  8,  9, 10,
	 6, 15,  8, 15,  3,  6,  6,  8,
	15,  3, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15,  3, 15, 15,  8,
}};

/**
 * Get the index of the "anchor" bits for implied index bits.
 * @param partition Partition number.
 * @param subset Subset number.
 * @param subsetCount Total number of subsets. (1, 2, 3)
 */
static uint8_t getAnchorIndex(uint8_t partition, uint8_t subset, uint8_t subsetCount)
{
	if (subset == 0) {
		// Subset 0 always has an anchor index of 0.
		return 0;
	}

	uint8_t idx;
	switch (subsetCount) {
		default:
			assert(!"Invalid subset count.");
			idx = 0;
			break;
		case 1:
			// Should've been handled above but okay.
			idx = 0;
			break;
		case 2:
			// Two subsets.
			// Assume this is the second subset.
			idx = anchorIndexes_subset2of2[partition];
			break;
		case 3:
			// Three subsets.
			// Subset is either 1 or 2, since subset can't be 0.
			assert(subset != 0);
			idx = (subset == 1
				? anchorIndexes_subset2of3[partition]
				: anchorIndexes_subset3of3[partition]
				);
			break;
	}

	return idx;
}

/**
 * BC7 block struct.
 */
struct bc7_block {
	uint64_t lsb;
	uint64_t msb;

	explicit bc7_block(const uint64_t *src)
	{
		// TODO: Make sure this is correct on big-endian.
		lsb = le64_to_cpu(src[0]);
		msb = le64_to_cpu(src[1]);
	}

	/**
	 * Right-shift the two 64-bit values as if it's a single 128-bit value.
	 * NOTE: Assuming `shamt` is always less than 64.
	 * @param shamt	[in] Shift amount
	 */
	FORCEINLINE void rshift128(unsigned int shamt)
	{
		assert(shamt < 64);
		if (shamt == 0) {
			// Nothing to do here...
			return;
		}

		// Shift LSB first.
		lsb >>= shamt;
		// Copy from MSB to LSB.
		lsb |= (msb << (64 - shamt));
		// Shift MSB next.
		msb >>= shamt;
	}
};

/**
 * Decode a BC7 block.
 * @param tileBuf	[out] Tile buffer
 * @param bc7_src	[in] BC7 source data
 * @return 0 on success; negative POSIX error code on error.
 */
static int decodeBC7Block(array<argb32_t, 4*4> &tileBuf, const uint64_t *bc7_src)
{
	/** BEGIN: Temporary values. **/

	// Endpoints.
	// - [8]: Individual endpoints.
	// - [4]: RGBx components. (idx3 is unused)
	// NOTE: Endpoints 6 and 7 are never used.
	// They're kept here because the subset index is 2-bit.
	union {
		uint8_t   u8[8][4];
		uint32_t u32[8];
	} endpoints;

	// Alpha components.
	// If no alpha is present, this will be 255.
	// For modes with alpha components, there is always
	// one alpha channel per endpoint.
	array<uint8_t, 4> alpha;

	// Anchor indexes.
	// Subset 0 is always anchored at 0.
	// Other subsets depend on subset count and partition number.
	// NOTE: Index 3 is invalid. It's present here for alignment
	// and because the subset index is 2-bit.
	array<uint8_t, 4> anchor_index;
	anchor_index[0] = 0;

	/** END: Temporary values. **/

	// Current block.
	bc7_block block(bc7_src);

	// Check the block mode.
	const int mode = get_mode(static_cast<uint32_t>(block.lsb));
	assert(mode >= 0);
	assert(mode < 8);
	if (mode < 0 || mode >= 8) {
		// Invalid mode.
		return -EIO;
	}
	block.rshift128(mode+1);

	// Rotation mode.
	// Only present in modes 4 and 5.
	// For all other modes, this is assumed to be 00.
	// - 00: ARGB - no swapping
	// - 01: RAGB - swap A and R
	// - 10: GRAB - swap A and G
	// - 11: BRGA - swap A and B
	uint8_t rotation_mode;
	if (mode == 4 || mode == 5) {
		rotation_mode = block.lsb & 3;
		block.rshift128(2);
	} else {
		// No rotation.
		rotation_mode = 0;
	}

	// Index mode selector. (Mode 4 only)
	uint8_t idxMode_m4 = 0;
	if (mode == 4) {
		// Mode 4 has both 2-bit and 3-bit selectors.
		// The index selection bit determines which is used for
		// color data and which is used for alpha data:
		// - idxMode_m4 == 0: Color == 2-bit, Alpha == 3-bit
		// - idxMode_m4 == 1: Color == 3-bit, Alpha == 2-bit
		idxMode_m4 = block.lsb & 1;
		block.rshift128(1);
	}

	// Subset/partition.
	static constexpr array<uint8_t, 8> SubsetCount   = {{3, 2, 3, 2, 1, 1, 1, 2}};
	static constexpr array<uint8_t, 8> PartitionBits = {{4, 6, 6, 6, 0, 0, 0, 6}};
	uint32_t subset = 0;
	uint8_t partition = 0;
	if (PartitionBits[mode] != 0) {
		partition = block.lsb & ((1U << PartitionBits[mode]) - 1);
		block.rshift128(PartitionBits[mode]);

		// Determine the subset to use.
		switch (SubsetCount[mode]) {
			default:
			case 1:
				// One subset.
				subset = 0;
				break;
			case 2:
				// Two subsets.
				subset = bc7_2sub[partition];
				break;
			case 3:
				// Three subsets.
				subset = bc7_3sub[partition];
				break;
		}
	} else {
		// No subsets/partitions.
		subset = 0;
	}

	// Number of endpoints.
	static constexpr array<uint8_t, 8> EndpointCount = {{6, 4, 6, 4, 2, 2, 2, 4}};
	// Bits per endpoint component.
	static constexpr array<uint8_t, 8> EndpointBits  = {{4, 6, 5, 7, 5, 7, 7, 5}};

	// Extract and extend the components.
	// NOTE: Components are stored in RRRR/GGGG/BBBB/AAAA order.
	// Needs to be shuffled for RGBA.
	uint8_t endpoint_bits = EndpointBits[mode];
	const uint8_t endpoint_count = EndpointCount[mode];
	const uint8_t endpoint_mask = (1U << endpoint_bits) - 1;
	const uint8_t endpoint_shamt = 8U - endpoint_bits;
	const unsigned int component_count = endpoint_count * 3;
	uint8_t ep_idx = 0, comp_idx = 0;
	for (unsigned int i = 0; i < component_count; i++) {
		endpoints.u8[ep_idx][comp_idx] = (block.lsb & endpoint_mask) << endpoint_shamt;
		ep_idx++;
		if (ep_idx == endpoint_count) {
			// Next component.
			comp_idx++;
			ep_idx = 0;
		}

		// Shift the data over.
		block.rshift128(endpoint_bits);
	}

	// Do we have alpha components?
	static constexpr array<uint8_t, 8> AlphaBits = {{0, 0, 0, 0, 6, 8, 7, 5}};
	uint8_t alpha_bits = AlphaBits[mode];
	if (alpha_bits != 0) {
		// We have alpha components.
		// TODO: Might not actually be alpha if rotation is enabled...
		// TODO: Or, rotation might enable alpha...
		const uint8_t alpha_mask = (1U << alpha_bits) - 1;
		const uint8_t alpha_shamt = 8U - alpha_bits;
		for (unsigned int i = 0; i < endpoint_count; i++) {
			alpha[i] = (block.lsb & alpha_mask) << alpha_shamt;
			block.rshift128(alpha_bits);
		}
	} else {
		// No alpha. Use 255.
		alpha[0] = 255;
		alpha[1] = 255;
		alpha[2] = 255;
		alpha[3] = 255;
	}

	// P-bits.
	// NOTE: These are applied per subset.
	// The P-bit count is needed here in order to determine the
	// shift amount for the endpoints and alpha values.
	static constexpr array<uint8_t, 8> PBitCount = {{1, 1, 0, 1, 0, 0, 1, 1}};
	if (PBitCount[mode] != 0) {
		// Optimization to avoid having to shift the
		// whole 64-bit and/or 128-bit value multiple times.
		unsigned int lsb8 = (block.lsb & 0xFF);
		if (mode == 1) {
			// Mode 1: Two P-bits for four endpoints.

			// Subset 0
			if (block.lsb & 1) {
				endpoints.u32[0] |= 0x02020202;
				endpoints.u32[1] |= 0x02020202;
			}

			// Subset 1
			if (block.lsb & 2) {
				endpoints.u32[2] |= 0x02020202;
				endpoints.u32[3] |= 0x02020202;
			}

			block.rshift128(2);
		} else {
			// Other modes: Unique P-bit for each endpoint.
			const uint8_t p_ep_shamt = 7 - endpoint_bits;
			for (unsigned int i = 0; i < endpoint_count; i++, lsb8 >>= 1) {
				if (lsb8 & 1) {
					endpoints.u32[i] |= (0x01010101 << p_ep_shamt);
				}
			}

			if (alpha_bits > 0) {
				// Apply P-bits to the alpha components.
				assert(endpoint_count <= ARRAY_SIZE(alpha));
				const uint8_t p_a_shamt = 7 - alpha_bits;
				lsb8 = (block.lsb & 0xFF);
				for (unsigned int i = 0; i < endpoint_count; i++, lsb8 >>= 1) {
					alpha[i] |= (lsb8 & 1) << p_a_shamt;
				}

				// Increment the alpha bits to indicate how many bits
				// need to be copied when expanding the color value.
				alpha_bits++;
			}

			block.rshift128(endpoint_count);
		}

		// Increment the endpoint bits to indicate how many bits
		// need to be copied when expanding the color value.
		endpoint_bits++;
	}

	// Expand the endpoints and alpha components.
	if (endpoint_bits < 8) {
		for (unsigned int i = 0; i < endpoint_count; i++) {
			endpoints.u8[i][0] = endpoints.u8[i][0] | (endpoints.u8[i][0] >> endpoint_bits);
			endpoints.u8[i][1] = endpoints.u8[i][1] | (endpoints.u8[i][1] >> endpoint_bits);
			endpoints.u8[i][2] = endpoints.u8[i][2] | (endpoints.u8[i][2] >> endpoint_bits);
		}
	}
	if (alpha_bits != 0 && alpha_bits < 8) {
		for (unsigned int i = 0; i < endpoint_count; i++) {
			alpha[i] = alpha[i] | (alpha[i] >> alpha_bits);
		}
	}

	// Bits per index. (either 2 or 3)
	// NOTE: Most modes don't have the full 32-bit or 48-bit
	// index table. Missing bits are assumed to be 0.
	static constexpr array<uint8_t, 8> IndexBits = {{3, 3, 2, 2, 0, 2, 4, 2}};
	unsigned int index_bits = IndexBits[mode];

	// At this point, the only remaining data is indexes,
	// which fits entirely into LSB. Hence, we can stop
	// using rshift128().

	// EXCEPTION: Mode 4 has both 2-bit *and* 3-bit indexes.
	// Depending on idxMode_m4, we have to use one or the other.
	uint64_t idxData;
	uint8_t index_mask;
	if (mode == 4) {
		// Load the color indexes.
		if (idxMode_m4) {
			// idxMode is set: Color data uses the 3-bit indexes.
			// NOTE: We've already shifted by 50 bits by now, so the
			// MSB contains the high 14 bits of the index data, and
			// the LSB contains the low 33 bits of the index data.
			idxData = (block.msb << 33) | (block.lsb >> 31);
			index_bits = 3;
			index_mask = (1U << 3) - 1;
		} else {
			// idxMode is not set: Color data uses the 2-bit indexes.
			idxData = block.lsb & ((1U << 31) - 1);
			index_bits = 2;
			index_mask = (1U << 2) - 1;
		}
	} else {
		// Use the LSB indexes as-is.
		idxData = block.lsb;
		index_mask = (1U << index_bits) - 1;
	}

	// Get the anchor indexes.
	const uint8_t subset_count = SubsetCount[mode];
	for (unsigned int i = 1; i < subset_count; i++) {
		anchor_index[i] = getAnchorIndex(partition, i, subset_count);
	}

	// Process the index data for the color components.
	uint32_t subsetData = subset;
	for (unsigned int i = 0; i < 16; i++, subsetData >>= 2) {
		const uint8_t subset_idx = subsetData & 3;
		assert(subset_idx != 3);
		uint8_t data_idx;
		if (i == anchor_index[subset_idx]) {
			// This is an anchor index.
			// Highest bit is 0.
			data_idx = idxData & (index_mask >> 1);
			idxData >>= (index_bits - 1);
		} else {
			// Regular index.
			data_idx = idxData & index_mask;
			idxData >>= index_bits;
		}

		const uint8_t ep_idx = subset_idx * 2;
		tileBuf[i].r = interpolate_component(index_bits, data_idx, endpoints.u8[ep_idx][0], endpoints.u8[ep_idx+1][0]);
		tileBuf[i].g = interpolate_component(index_bits, data_idx, endpoints.u8[ep_idx][1], endpoints.u8[ep_idx+1][1]);
		tileBuf[i].b = interpolate_component(index_bits, data_idx, endpoints.u8[ep_idx][2], endpoints.u8[ep_idx+1][2]);
	}

	// Alpha handling.
	if (mode == 4) {
		// Mode 4: Alpha indexes are present.
		// Load the appropriate indexes based on idxMode.
		uint8_t index_bits, index_mask;
		if (idxMode_m4) {
			// idxMode is set: Alpha data uses the 2-bit indexes.
			idxData = block.lsb & ((1U << 31) - 1);
			index_bits = 2;
			index_mask = (1U << 2) - 1;
		} else {
			// idxMode is not set: Alpha data uses the 3-bit indexes.
			// NOTE: We've already shifted by 50 bits by now, so the
			// MSB contains the high 14 bits of the index data, and
			// the LSB contains the low 33 bits of the index data.
			idxData = (block.msb << 33) | (block.lsb >> 31);
			index_bits = 3;
			index_mask = (1U << 3) - 1;
		}

		subsetData = subset;
		for (unsigned int i = 0; i < 16; i++, subsetData >>= 2) {
			const uint8_t subset_idx = subsetData & 3;
			uint8_t data_idx;
			if (i == anchor_index[subset_idx]) {
				// This is an anchor index.
				// Highest bit is 0.
				data_idx = idxData & (index_mask >> 1);
				idxData >>= (index_bits - 1);
			} else {
				// Regular index.
				data_idx = idxData & index_mask;
				idxData >>= index_bits;
			}

			tileBuf[i].a = interpolate_component(index_bits, data_idx, alpha[0], alpha[1]);
		}
	} else if (alpha_bits == 0) {
		// No alpha. Assume 255.
		for (unsigned int i = 0; i < 16; i++) {
			tileBuf[i].a = 255;
		}
	} else {
		// Process alpha using the index data.
		if (mode == 5) {
			// Mode 5: Separate alpha indexes, stored after the color indexes.
			idxData = block.lsb >> 31;
		} else {
			// Other modes: Same indexes as color data.
			idxData = block.lsb;
		}
		subsetData = subset;
		for (unsigned int i = 0; i < 16; i++, subsetData >>= 2) {
			const uint8_t subset_idx = subsetData & 3;
			uint8_t data_idx;
			if (i == anchor_index[subset_idx]) {
				// This is an anchor index.
				// Highest bit is 0.
				data_idx = idxData & (index_mask >> 1);
				idxData >>= (index_bits - 1);
			} else {
				// Regular index.
				data_idx = idxData & index_mask;
				idxData >>= index_bits;
			}

			const uint8_t ep_idx = subset_idx * 2;
			tileBuf[i].a = interpolate_component(index_bits, data_idx, alpha[ep_idx], alpha[ep_idx+1]);
		}
	}

	// Component rotation.
	switch (rotation_mode & 3) {
		case 0:
			// ARGB: No rotation.
			break;
		case 1:
			// RAGB: Swap A and R.
			for (argb32_t &pixel : tileBuf) {
				std::swap(pixel.a, pixel.r);
			}
			break;
		case 2:
			// GRAB: Swap A and G.
			for (argb32_t &pixel : tileBuf) {
				std::swap(pixel.a, pixel.g);
			}
			break;
		case 3:
			// BRGA: Swap A and B.
			for (argb32_t &pixel : tileBuf) {
				std::swap(pixel.a, pixel.b);
			}
			break;
	}

	// Block decoded.
	return 0;
}

/**
 * Convert a BC7 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC7 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromBC7(int width, int height,
	const uint8_t *img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// BC7 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= (static_cast<size_t>(physWidth) * static_cast<size_t>(physHeight)));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (static_cast<size_t>(physWidth) * static_cast<size_t>(physHeight)))
	{
		return nullptr;
	}

	// Calculate the total number of tiles.
	const int tilesX = physWidth / 4;
	const int tilesY = physHeight / 4;
	const unsigned int bytesPerTileRow = tilesX * sizeof(bc7_block);	// for OpenMP

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// sBIT metadata.
	// TODO: Dynamically determine if we have alpha?
	// Rotation bits makes this difficult...
	static const rp_image::sBIT_t sBIT = {8,8,8,0,8};

#ifdef _OPENMP
	bool bErr = false;
#endif /* _OPENMP */

#pragma omp parallel for default(none) shared(img_buf, img, bErr) firstprivate(tilesX, tilesY, bytesPerTileRow)
	for (int y = 0; y < tilesY; y++) {
		// BC7 has eight block modes with varying properties, including
		// bitfields of different lengths. As such, the only guaranteed
		// block format we have is 128-bit little-endian, which will be
		// represented as two uint64_t values, which will be shifted
		// as each component is processed.
		// TODO: Optimize by using fewer shifts?
		const uint64_t *bc7_src = reinterpret_cast<const uint64_t*>(
			&img_buf[y * bytesPerTileRow]);
		for (int x = 0; x < tilesX; x++, bc7_src += 2) {
			// Temporary tile buffer
			array<argb32_t, 4*4> tileBuf;

			// Decode the block.
			int ret = decodeBC7Block(tileBuf, bc7_src);
			if (ret != 0) {
				// BC7 decoding error.
#ifdef _OPENMP
				// Cannot return when using OpenMP,
				// so set an error value and continue.
				bErr = true;
				break;
#else /* !_OPENMP */
				// Not using OpenMP, so return immediately.
				return nullptr;
#endif /* _OPENMP */
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
		}
	}

#ifdef _OPENMP
	if (bErr) {
		// A decoding error occurred.
		return nullptr;
	}
#endif /* _OPENMP */

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageSizeCalc.hpp: Image size calculation functions.                    *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ImageSizeCalc.hpp"

namespace LibRpTexture { namespace ImageSizeCalc {

/**
 * Calculate an image size using the specified format opcode table.
 * @param op_tbl Opcode table
 * @param tbl_size Size of op_tbl (number of entries)
 * @param format Image format ID
 * @param width Image width
 * @param height Image height
 * @return Image size, in bytes
 */
unsigned int calcImageSize_tbl(
	const OpCode *op_tbl, size_t tbl_size,
	unsigned int format, unsigned int width, unsigned int height)
{
	assert(format < tbl_size);
	if (format >= tbl_size) {
		// Invalid format.
		return 0;
	}

	unsigned int image_size = 0;
	if (op_tbl[format] < OpCode::Align4Divide2) {
		image_size = width * height;
	}

	switch (op_tbl[format]) {
		default:
		case OpCode::Unknown:
			// Invalid opcode.
			return 0;

		case OpCode::None:					break;
		case OpCode::Multiply2:		image_size *= 2;	break;
		case OpCode::Multiply3:		image_size *= 3;	break;
		case OpCode::Multiply4:		image_size *= 4;	break;
		case OpCode::Multiply6:		image_size *= 6;	break;
		case OpCode::Multiply8:		image_size *= 8;	break;
		case OpCode::Multiply12:	image_size *= 12;	break;
		case OpCode::Multiply16:	image_size *= 16;	break;
		case OpCode::Divide2:		image_size /= 2;	break;
		case OpCode::Divide4:		image_size /= 4;	break;

		case OpCode::Align4Divide2:
			image_size = ALIGN_BYTES(4, width) * ALIGN_BYTES(4, height) / 2;
			break;

		case OpCode::Align4:
			image_size = ALIGN_BYTES(4, width) * ALIGN_BYTES(4, height);
			break;

		case OpCode::Align8Divide4:
			image_size = ALIGN_BYTES(8, width) * ALIGN_BYTES(8, height) / 4;
			break;
	}

	return image_size;
}

/**
 * Calculate the expected size of a PVRTC-I compressed 2D image.
 * PVRTC-I requires power-of-2 dimensions.
 * @tparam is2bpp True for 2bpp; false for 4bpp.
 * @param width Image width
 * @param height Image height
 * @return Expected size, in bytes
 */
template<bool is2bpp>
unsigned int T_calcImageSizePVRTC_PoT(int width, int height)
{
	static constexpr int min_width = (is2bpp ? 8 : 4);
	if (width < min_width) {
		width = min_width;
	} else if (!isPow2(width)) {
		width = nextPow2(width);
	}

	if (height < 4) {
		height = 4;
	} else if (!isPow2(height)) {
		height = nextPow2(height);
	}

	return (width * height / (is2bpp ? 4 : 2));
}

// Explicit instantiation
template unsigned int T_calcImageSizePVRTC_PoT<true>(int width, int height);
template unsigned int T_calcImageSizePVRTC_PoT<false>(int width, int height);

/**
 * Calculate the expected size of an ASTC-compressed 2D image.
 * @param width Image width
 * @param height Image height
 * @param block_x Block width
 * @param block_y Block height
 * @return Expected size, in bytes
 */
unsigned int calcImageSizeASTC(int width, int height, uint8_t block_x, uint8_t block_y)
{
	if (!validateBlockSizeASTC(block_x, block_y)) {
		// Invalid block size.
		return 0;
	}

	// ASTC block size is always 128-bit.
	const unsigned int texelsInBlock =
		static_cast<unsigned int>(block_x) * static_cast<unsigned int>(block_y);

	// Based on the X and Y parameters, calculate the expected
	// total compressed image size.
	// NOTE: Physical image size must be aligned to the block size.
	alignImageSizeASTC(width, height, block_x, block_y);

	const unsigned int texels = width * height;
	unsigned int blocks_req = texels / texelsInBlock;
	if (texels % texelsInBlock != 0) {
		blocks_req++;
	}

	// Each block is 128 bits (16 bytes).
	return blocks_req * 16;
}

} }

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageSizeCalc.hpp: Image size calculation functions.                    *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGESIZECALC_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGESIZECALC_HPP__

// C includes (C++ namespace)
#include <cassert>

namespace LibRpTexture { namespace ImageSizeCalc {

// OpCode values for calcImageSize().
enum class OpCode : uint8_t {
	Unknown = 0,
	None,
	Multiply2,
	Multiply3,
	Multiply4,
	Multiply6,
	Multiply8,
	Multiply12,
	Multiply16,
	Divide2,
	Divide4,

	// DXTn requires aligned blocks.
	Align4Divide2,
	Align4,

	// ASTC requires aligned blocks.
	// NOTE: This only works for ASTC_8x8.
	// Other block sizes should use calcImageSizeASTC().
	Align8Divide4,

	Max
};

/**
 * Calculate an image size using the specified format opcode table.
 * @param op_tbl Opcode table
 * @param tbl_size Size of op_tbl (number of entries)
 * @param format Image format ID
 * @param width Image width
 * @param height Image height
 * @return Image size, in bytes
 */
unsigned int calcImageSize(
	const OpCode *op_tbl, size_t tbl_size,
	unsigned int format, unsigned int width, unsigned int height);

/**
 * Validate ASTC block size.
 * @param block_x	[in] Block width
 * @param block_y	[in] Block height
 * @return True if valid; false if not.
 */
static inline bool validateBlockSizeASTC(uint8_t block_x, uint8_t block_y)
{
	assert(block_x >= 4);
	assert(block_x <= 12);
	assert(block_y >= 4);
	assert(block_y <= 12);
	assert(block_x >= block_y);
	if (block_x < 4 || block_x > 12 ||
	    block_y < 4 || block_y > 12 ||
	    block_x < block_y)
	{
		// Invalid block size.
		return false;
	}

	// TODO: Validate combinations?
	return true;
}

/**
 * Align width/height for ASTC.
 * @param width		[in,out] Image width
 * @param height	[in,out] Image height
 * @param block_x	[in] Block width
 * @param block_y	[in] Block height
 */
static inline void alignImageSizeASTC(int& width, int& height, uint8_t block_x, uint8_t block_y)
{
	if (width % block_x != 0) {
		width += (block_x - (width % block_x));
	}
	if (height % block_y != 0) {
		height += (block_y - (height % block_y));
	}
}

/**
 * Calculate the expected size of an ASTC-compressed 2D image.
 * @param width Image width
 * @param height Image height
 * @param block_x Block width
 * @param block_y Block height
 * @return Expected size, in bytes
 */
unsigned int calcImageSizeASTC(int width, int height, uint8_t block_x, uint8_t block_y);

} }

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DECODER_PIXELCONVERSION_HPP__ */

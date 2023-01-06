/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_ASTC.hpp: Image decoding functions: ASTC                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

#ifdef ENABLE_ASTC
/**
 * ASTC lookup table.
 * - Index: Matches ordering in DDS (div3), PVR3, KTX, KTX2.
 * - Value 0: block_x
 * - Value 1: block_y
 */
extern const uint8_t astc_lkup_tbl[14][2];

/**
 * Convert an ASTC 2D image to rp_image.
 * @param width Image width
 * @param height Image height
 * @param img_buf PVRTC image buffer
 * @param img_siz Size of image data
 * @param block_x ASTC block size, X value
 * @param block_y ASTC block size, Y value
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image *fromASTC(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	uint8_t block_x, uint8_t block_y);
#endif /* ENABLE_ASTC */

} }

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_NDS.hpp: Image decoding functions: Nintendo DS             *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Nintendo DS CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromNDS_CI4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const uint16_t *RESTRICT pal_buf, size_t pal_siz);

} }

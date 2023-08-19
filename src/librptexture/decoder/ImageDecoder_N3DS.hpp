/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_N3DS.hpp: Image decoding functions: Nintendo 3DS           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
std::shared_ptr<rp_image> fromN3DSTiledRGB565(int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a Nintendo 3DS RGB565+A4 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param alpha_buf A4 tiled alpha buffer.
 * @param alpha_siz Size of alpha data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 5, 6)
std::shared_ptr<rp_image> fromN3DSTiledRGB565_A4(int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz,
	const uint8_t *RESTRICT alpha_buf, size_t alpha_siz);

} }

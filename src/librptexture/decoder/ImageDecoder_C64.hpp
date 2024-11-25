/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_C64.hpp: Image decoding functions: Commodore 64            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a Commodore 64 multi-color sprite (12x21) to rp_image.
 * A default 4-color grayscale palette is used.
 * @param img_buf CI4 image buffer
 * @param img_siz Size of image data [must be >= 12*21/4 (63)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 1, 2)
rp_image_ptr fromC64_MultiColor_Sprite(
	const uint8_t *RESTRICT img_buf, size_t img_siz);

} }

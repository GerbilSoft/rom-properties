/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_ETC1.hpp: Image decoding functions: ETCn                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert an ETC1 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromETC1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert an ETC2 RGB image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromETC2_RGB(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert an ETC2 RGBA image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGBA image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromETC2_RGBA(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert an ETC2 RGB+A1 (punchthrough alpha) image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB+A1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromETC2_RGB_A1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert an EAC R11 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf EAC R11 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromEAC_R11(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert an EAC RG11 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf EAC R11 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromEAC_RG11(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

} }

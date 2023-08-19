/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_S3TC.hpp: Image decoding functions: S3TC                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a GameCube DXT1 image to rp_image.
 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT1_GCN(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as black.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT1_A1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT2 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT2 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT2(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT3(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a DXT5 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromDXT5(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a BC4 (ATI1) image to rp_image.
 * Color component is Red.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromBC4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a BC5 (ATI2) image to rp_image.
 * Color components are Red and Green.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
std::shared_ptr<rp_image> fromBC5(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz);

/**
 * Convert a Red image to Luminance.
 * Use with fromBC4() to decode an LATC1 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
int fromRed8ToL8(const std::shared_ptr<rp_image> &img);

/**
 * Convert a Red+Green image to Luminance+Alpha.
 * Use with fromBC5() to decode an LATC2 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
int fromRG8ToLA8(const std::shared_ptr<rp_image> &img);

} }

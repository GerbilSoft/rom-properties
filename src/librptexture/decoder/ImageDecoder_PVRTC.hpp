/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_PVRTC.hpp: Image decoding functions: PVRTC                 *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

#ifdef ENABLE_PVRTC
/* PVRTC */

enum PVRTC_Mode_e {
	// bpp
	PVRTC_4BPP		= (0U << 0),
	PVRTC_2BPP		= (1U << 0),
	PVRTC_BPP_MASK		= (1U << 0),

	// Alpha channel (PVRTC-I only)
	PVRTC_ALPHA_NONE	= (0U << 1),
	PVRTC_ALPHA_YES		= (1U << 1),
	PVRTC_ALPHA_MASK	= (1U << 1),
};

/**
 * Convert a PVRTC 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromPVRTC(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	uint8_t mode);

/**
 * Convert a PVRTC-II 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromPVRTCII(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	uint8_t mode);
#endif /* ENABLE_PVRTC */

} }

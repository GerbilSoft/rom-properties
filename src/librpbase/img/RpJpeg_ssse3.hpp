/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_ssse3.hpp: JPEG image handler.                                   *
 * SSSE3-optimized version.                                                *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// for RESTRICT
#include "common.h"

// librptexture
#include "librptexture/img/rp_image.hpp"

// libjpeg
#include <jpeglib.h>

namespace LibRpBase { namespace RpJpeg { namespace ssse3 {

/**
 * Decode a 24-bit BGR JPEG to 32-bit ARGB.
 * SSSE3-optimized version.
 * NOTE: This function should ONLY be called from RpJpeg::load().
 * @param img		[in/out] rp_image.
 * @param cinfo		[in/out] JPEG decompression struct.
 * @param buffer 	[in/out] Line buffer. (Must be 16-byte aligned!)
 */
void decodeBGRtoARGB(rp_image *RESTRICT img, jpeg_decompress_struct *RESTRICT cinfo, JSAMPARRAY buffer);

} } }

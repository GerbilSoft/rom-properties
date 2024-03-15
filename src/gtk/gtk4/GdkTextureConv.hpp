/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkTextureConv.hpp: Helper functions to convert from rp_image to GDK4.  *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: GdkTexture doesn't natively support 8bpp, and it doesn't
// allow for raw data access. Because of this, we can't simply
// make a GdkTexture rp_image backend.

#include "librpcpuid/cpu_dispatch.h"
namespace LibRpTexture {
	class rp_image;
}

namespace GdkTextureConv {

/**
 * Convert an rp_image to GdkTexture.
 * @param img	[in] rp_image.
 * @return GdkTexture, or nullptr on error.
 */
GdkTexture *rp_image_to_GdkTexture(const LibRpTexture::rp_image *img);

} //namespace GdkTextureConv

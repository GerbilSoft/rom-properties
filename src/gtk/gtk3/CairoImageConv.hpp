/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CairoImageConv.hpp: Helper functions to convert from rp_image to Cairo. *
 *                                                                         *
 * Copyright (c) 2017-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: Cairo doesn't natively support 8bpp. Because of this,
// we can't simply make a cairo_surface_t rp_image backend.

#include "librptexture/img/rp_image.hpp"

#include <cairo.h>

namespace CairoImageConv {

/**
 * Convert an rp_image to cairo_surface_t.
 * @param img		[in] rp_image
 * @param premultiply	[in] If true, premultiply. Needed for display; NOT needed for PNG.
 * @return cairo_surface_t, or nullptr on error.
 */
cairo_surface_t *rp_image_to_cairo_surface_t(const LibRpTexture::rp_image *img, bool premultiply = true);

/**
 * Convert an rp_image to cairo_surface_t.
 * @param img		[in] rp_image
 * @param premultiply	[in] If true, premultiply. Needed for display; NOT needed for PNG.
 * @return cairo_surface_t, or nullptr on error.
 */
static inline cairo_surface_t *rp_image_to_cairo_surface_t(const LibRpTexture::rp_image_ptr &img, bool premultiply = true)
{
	return rp_image_to_cairo_surface_t(img.get(), premultiply);
}

/**
 * Convert an rp_image to cairo_surface_t.
 * @param img		[in] rp_image
 * @param premultiply	[in] If true, premultiply. Needed for display; NOT needed for PNG.
 * @return cairo_surface_t, or nullptr on error.
 */
static inline cairo_surface_t *rp_image_to_cairo_surface_t(const LibRpTexture::rp_image_const_ptr &img, bool premultiply = true)
{
	return rp_image_to_cairo_surface_t(img.get(), premultiply);
}

} //namespace CairoImageConv

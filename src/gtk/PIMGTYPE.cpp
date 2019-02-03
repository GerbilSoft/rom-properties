/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "PIMGTYPE.hpp"

// C includes. (C++ namespace)
#include <cassert>

#ifdef RP_GTK_USE_CAIRO
/**
 * PIMGTYPE scaling function.
 * @param pImgType PIMGTYPE
 * @param width New width
 * @param height New height
 * @param bilinear If true, use bilinear interpolation.
 * @return Rescaled image. (If unable to rescale, returns a new reference to pImgType.)
 */
PIMGTYPE PIMGTYPE_scale(PIMGTYPE pImgType, int width, int height, bool bilinear)
{
	// TODO: Maintain aspect ratio, and use nearest-neighbor
	// when scaling up from small sizes.
	const int srcWidth = cairo_image_surface_get_width(pImgType);
	const int srcHeight = cairo_image_surface_get_height(pImgType);
	assert(srcWidth > 0 && srcHeight > 0);
	if (unlikely(srcWidth <= 0 || srcHeight <= 0)) {
		return cairo_surface_reference(pImgType);
	}

	cairo_surface_t *const surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	assert(surface != nullptr);
	assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!surface)) {
		return cairo_surface_reference(pImgType);
	} else if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	}

	cairo_t *const cr = cairo_create(surface);
	assert(cr != nullptr);
	assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!cr)) {
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	} else if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	}

	cairo_scale(cr, (double)width / (double)srcWidth, (double)height / (double)srcHeight);
	cairo_set_source_surface(cr, pImgType, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr),
		(bilinear ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST));
	cairo_paint(cr);
	cairo_destroy(cr);
	return surface;
}
#endif /* RP_GTK_USE_CAIRO */

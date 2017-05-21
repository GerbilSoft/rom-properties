/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image_backend.cpp: Image backend and storage classes.                *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "rp_image_backend.hpp"

// C includes (C++ namespace)
#include <cassert>

namespace LibRpBase {

/** rp_image_backend **/

static inline int calc_stride(int width, rp_image::Format format)
{
	assert(width > 0);
	assert(format > rp_image::FORMAT_NONE);
	assert(format <= rp_image::FORMAT_ARGB32);

	if (width <= 0)
		return 0;

	switch (format) {
		case rp_image::FORMAT_CI8:
			return width;
		case rp_image::FORMAT_ARGB32:
			return width * 4;
		default:
			// Invalid image format.
			assert(!"Unsupported rp_image::Format.");
			break;
	}

	return 0;
}

rp_image_backend::rp_image_backend(int width, int height, rp_image::Format format)
	: width(width)
	, height(height)
	, stride(0)
	, format(format)
	, tr_idx(-1)
{
	// Calculate the stride.
	// NOTE: If format == FORMAT_NONE, the subclass is
	// managing width/height/format.
	if (format != rp_image::FORMAT_NONE) {
		stride = calc_stride(width, format);
	}
}

rp_image_backend::~rp_image_backend()
{ }

bool rp_image_backend::isValid(void) const
{
	return (width > 0 && height > 0 && stride > 0 &&
		format != rp_image::FORMAT_NONE &&
		data() && data_len() > 0 &&
		(format != rp_image::FORMAT_CI8 ||
		 (palette() && palette_len() > 0)));
}

/**
 * Clear the width, height, stride, and format properties.
 * Used in error paths.
 * */
void rp_image_backend::clear_properties(void)
{
	this->width = 0;
	this->height = 0;
	this->stride = 0;
	this->format = rp_image::FORMAT_NONE;
}

/**
 * Check if the palette contains alpha values other than 0 and 255.
 * @return True if an alpha value other than 0 and 255 was found; false if not, or if ARGB32.
 */
bool rp_image_backend::has_translucent_palette_entries(void) const
{
	assert(this->format == rp_image::FORMAT_CI8);
	if (this->format != rp_image::FORMAT_CI8)
		return false;

	const uint32_t *palette = this->palette();
	int i = this->palette_len();
	assert(palette != nullptr);
	assert(i > 0);

	for (; i > 1; i -= 2, palette += 2) {
		const uint8_t alpha1 = (palette[0] >> 24);
		const uint8_t alpha2 = (palette[1] >> 24);
		if (alpha1 != 0 && alpha1 != 255) {
			// Found an alpha value other than 0 and 255.
			return true;
		} else if (alpha2 != 0 && alpha2 != 255) {
			// Found an alpha value other than 0 and 255.
			return true;
		}
	}
	if (i == 1) {
		const uint8_t alpha = (*palette >> 24);
		if (alpha != 0 && alpha != 255) {
			// Found an alpha value other than 0 and 255.
			return true;
		}
	}

	// Image does not contain alpha values other than 0 and 255.
	return false;
}

}

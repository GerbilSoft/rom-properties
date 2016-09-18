/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
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

namespace LibRomData {

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
			assert(false);
			break;
	}

	return 0;
}

rp_image_backend::rp_image_backend(int width, int height, rp_image::Format format)
	: width(width)
	, height(height)
	, stride(0)
	, format(format)
	, palette(nullptr)
	, palette_len(0)
	, tr_idx(-1) // COMMIT NOTE: Changing default from 0 to -1.
{
	// Calculate the stride.
	stride = calc_stride(width, format);
}

rp_image_backend::~rp_image_backend()
{ }

bool rp_image_backend::isValid(void) const
{
	return (width > 0 && height > 0 && stride > 0 &&
		format != rp_image::FORMAT_NONE &&
		data() && data_len() > 0 &&
		(format != rp_image::FORMAT_CI8 ||
		 (palette && palette_len > 0)));
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

}

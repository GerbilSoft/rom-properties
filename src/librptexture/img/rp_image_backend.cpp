/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image_backend.cpp: Image backend and storage classes.                *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image_backend.hpp"

namespace LibRpTexture {

/** rp_image_backend **/

static inline int calc_stride(int width, rp_image::Format format)
{
	switch (format) {
		case rp_image::Format::CI8:
			return ALIGN_BYTES(16, width);
		case rp_image::Format::ARGB32:
			return ALIGN_BYTES(16, width * 4);
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
	// Sanity check: Maximum of 32768x32768.
	// Also make sure the format is valid.
	// NOTE: Format::None *is* valid here, and if set, width/height
	// can both be set to 0.
	// These represent empty images. RpGdiplusBackend sets these
	// initially when constructing an rp_image_backend from an
	// existing Gdiplus::Bitmap.
	assert(width > 0 || format == rp_image::Format::None);
	assert(width <= 32768);
	assert(height > 0 || format == rp_image::Format::None);
	assert(height <= 32768);
	assert(format >= rp_image::Format::None);
	assert(format <= rp_image::Format::ARGB32);
	if (width < 0 || width > 32768 ||
	    height < 0 || height > 32768 ||
	    format < rp_image::Format::None || format >= rp_image::Format::Max)
	{
		// Invalid values.
		clear_properties();
		return;
	}

	// Calculate the stride.
	// NOTE: If format == Format::None, the subclass is
	// managing width/height/format.
	if (format != rp_image::Format::None) {
		stride = calc_stride(width, format);
	}
}

bool rp_image_backend::isValid(void) const
{
	return (width > 0 && height > 0 && stride > 0 &&
		format != rp_image::Format::None &&
		data() && data_len() > 0 &&
		(format != rp_image::Format::CI8 ||
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
	this->format = rp_image::Format::None;
}

/**
 * Check if the palette contains alpha values other than 0 and 255.
 * @return True if an alpha value other than 0 and 255 was found; false if not, or if ARGB32.
 */
bool rp_image_backend::has_translucent_palette_entries(void) const
{
	assert(this->format == rp_image::Format::CI8);
	if (this->format != rp_image::Format::CI8)
		return false;

	const uint32_t *palette = this->palette();
	unsigned int i = this->palette_len();
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

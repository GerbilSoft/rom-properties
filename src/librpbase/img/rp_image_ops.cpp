/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image_ops.cpp: Image class. (operations)                             *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpBase {

/** Image operations. **/

/**
 * Duplicate the rp_image.
 * @return New rp_image with a copy of the image data.
 */
rp_image *rp_image::dup(void) const
{
	RP_D(const rp_image);
	const int width = d->backend->width;
	const int height = d->backend->height;
	const rp_image::Format format = d->backend->format;
	assert(width > 0);
	assert(height > 0);

	rp_image *img = new rp_image(width, height, format);
	if (!img->isValid()) {
		// Image is invalid. Return it immediately.
		return img;
	}

	// Copy the image.
	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
	const int row_bytes = img->row_bytes();
	const int dest_stride = img->stride();
	const int src_stride = d->backend->stride;

	if (src_stride == dest_stride) {
		// Copy the entire image all at once.
		size_t len = d->backend->data_len();
		memcpy(dest, src, len);
	} else {
		// Copy one line at a time.
		for (unsigned int y = (unsigned int)height; y > 0; y--) {
			memcpy(dest, src, row_bytes);
			dest += dest_stride;
			src += src_stride;
		}
	}

	// If CI8, copy the palette.
	if (format == rp_image::FORMAT_CI8) {
		int entries = std::min(img->palette_len(), d->backend->palette_len());
		uint32_t *const dest_pal = img->palette();
		memcpy(dest_pal, d->backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	return img;
}

/**
 * Duplicate the rp_image, converting to ARGB32 if necessary.
 * @return New ARGB32 rp_image with a copy of the image data.
 */
rp_image *rp_image::dup_ARGB32(void) const
{
	RP_D(const rp_image);
	if (d->backend->format == FORMAT_ARGB32) {
		// Already in ARGB32.
		// Do a direct dup().
		return this->dup();
	} else if (d->backend->format != FORMAT_CI8) {
		// Only CI8->ARGB32 is supported right now.
		return nullptr;
	}

	const int width = d->backend->width;
	const int height = d->backend->height;
	assert(width > 0);
	assert(height > 0);

	// TODO: Handle palette length smaller than 256.
	assert(d->backend->palette_len() == 256);
	if (d->backend->palette_len() != 256) {
		return nullptr;
	}

	rp_image *img = new rp_image(width, height, FORMAT_ARGB32);
	if (!img->isValid()) {
		// Image is invalid. Something went wrong.
		delete img;
		return nullptr;
	}

	// Copy the image, converting from CI8 to ARGB32.
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
	const uint32_t *pal = d->backend->palette();
	const int dest_adj = (img->stride() / 4) - width;
	const int src_adj = d->backend->stride - width;

	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		// Convert up to 4 pixels per loop iteration.
		unsigned int x;
		for (x = (unsigned int)width; x > 3; x -= 4) {
			dest[0] = pal[src[0]];
			dest[1] = pal[src[1]];
			dest[2] = pal[src[2]];
			dest[3] = pal[src[3]];
			dest += 4;
			src += 4;
		}
		// Remaining pixels.
		for (; x > 0; x--) {
			*dest = pal[*src];
			dest++;
			src++;
		}

		// Next line.
		dest += dest_adj;
		src += src_adj;
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	// Converted to ARGB32.
	return img;
}

/**
 * Square the rp_image.
 *
 * If the width and height don't match, transparent rows
 * and/or columns will be added to "square" the image.
 * Otherwise, this is the same as dup().
 *
 * @return New rp_image with a squared version of the original, or nullptr on error.
 */
rp_image *rp_image::squared(void) const
{
	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	RP_D(const rp_image);
	const int width = d->backend->width;
	const int height = d->backend->height;
	assert(width > 0);
	assert(height > 0);
	if (width <= 0 || height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	rp_image *sq_img = nullptr;
	if (width == height) {
		// Image is already square. dup() it.
		return this->dup();
	} else if (width > height) {
		// Image is wider. Add rows to the top and bottom.
		// TODO: 8bpp support?
		assert(d->backend->format == rp_image::FORMAT_ARGB32);
		if (d->backend->format != rp_image::FORMAT_ARGB32) {
			// Cannot resize this image.
			// Use dup() instead.
			return this->dup();
		}
		sq_img = new rp_image(width, width, rp_image::FORMAT_ARGB32);
		if (!sq_img->isValid()) {
			// Could not allocate the image.
			delete sq_img;
			return nullptr;
		}

		const int addToTop = (width-height)/2;
		const int addToBottom = addToTop + ((width-height)%2);

		// NOTE: Using uint8_t* because stride is measured in bytes.
		uint8_t *dest = static_cast<uint8_t*>(sq_img->bits());
		const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
		const int dest_stride = sq_img->stride();
		const int src_stride = d->backend->stride;

		// Clear the top rows.
		memset(dest, 0, addToTop * dest_stride);
		dest += (addToTop * dest_stride);

		// Copy the image data.
		const int row_bytes = sq_img->row_bytes();
		for (unsigned int y = height; y > 0; y--) {
			memcpy(dest, src, row_bytes);
			dest += dest_stride;
			src += src_stride;
		}

		// Clear the bottom rows.
		// NOTE: Last row may not be the full stride.
		memset(dest, 0, ((addToBottom-1) * dest_stride) + sq_img->row_bytes());
	} else if (width < height) {
		// Image is taller. Add columns to the left and right.
		// TODO: 8bpp support?
		assert(d->backend->format == rp_image::FORMAT_ARGB32);
		if (d->backend->format != rp_image::FORMAT_ARGB32) {
			// Cannot resize this image.
			// Use dup() instead.
			return this->dup();
		}
		sq_img = new rp_image(height, height, rp_image::FORMAT_ARGB32);
		if (!sq_img->isValid()) {
			// Could not allocate the image.
			delete sq_img;
			return nullptr;
		}

		// NOTE: Mega Man Gold amiibo is "shifting" by 1px when
		// refreshing in Win7. (switching from icon to thumbnail)
		// Not sure if this can be fixed easily.
		const int addToLeft = (height-width)/2;
		const int addToRight = addToLeft + ((height-width)%2);

		// NOTE: Using uint8_t* because stride is measured in bytes.
		uint8_t *dest = static_cast<uint8_t*>(sq_img->bits());
		const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
		// "Blanking" area is right border, potential unused space from stride, then left border.
		const int dest_blanking = sq_img->stride() - sq_img->row_bytes();
		const int dest_stride = sq_img->stride();
		const int src_stride = d->backend->stride;

		// Clear the left part of the first row.
		memset(dest, 0, addToLeft * sizeof(uint32_t));
		dest += (addToLeft * sizeof(uint32_t));

		// Copy and clear all but the last line.
		const int row_bytes = sq_img->row_bytes();
		for (unsigned int y = (height-1); y > 0; y--) {
			memcpy(dest, src, row_bytes);
			memset(&dest[row_bytes], 0, dest_blanking);
			dest += dest_stride;
			src += src_stride;
		}

		// Copy the last line.
		// NOTE: Last row may not be the full stride.
		memcpy(dest, src, row_bytes);
		// Clear the end of the line.
		memset(&dest[row_bytes], 0, addToRight * sizeof(uint32_t));
	}

	// Copy sBIT if it's set.
	if (d->has_sBIT) {
		sq_img->set_sBIT(&d->sBIT);
	}

	return sq_img;
}

/**
 * Resize the rp_image.
 *
 * A new rp_image will be created with the specified dimensions,
 * and the current image will be copied into the new image.
 * If the new dimensions are smaller than the old dimensions,
 * the image will be cropped. If the new dimensions are larger,
 * the original image will be in the upper-left corner and the
 * new space will be empty. (ARGB: 0x00000000)
 *
 * @param width New width.
 * @param height New height.
 * @return New rp_image with a resized version of the original, or nullptr on error.
 */
rp_image *rp_image::resized(int width, int height) const
{
	assert(width > 0);
	assert(height > 0);
	if (width <= 0 || height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	RP_D(const rp_image);
	const int orig_width = d->backend->width;
	const int orig_height = d->backend->height;
	assert(orig_width > 0);
	assert(orig_height > 0);
	if (orig_width <= 0 || orig_height <= 0) {
		// Cannot resize the image.
		return nullptr;
	}

	if (width == orig_width && height == orig_height) {
		// No resize is necessary.
		return this->dup();
	}

	const rp_image::Format format = d->backend->format;
	rp_image *img = new rp_image(width, height, format);
	if (!img->isValid()) {
		// Image is invalid.
		delete img;
		return nullptr;
	}

	// Copy the image.
	// NOTE: Using uint8_t* because stride is measured in bytes.
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const uint8_t *src = static_cast<const uint8_t*>(d->backend->data());
	const int dest_stride = img->stride();
	const int src_stride = d->backend->stride;

	// We want to copy the minimum of new vs. old width.
	int row_bytes = std::min(width, orig_width);
	if (format == rp_image::FORMAT_ARGB32) {
		row_bytes *= sizeof(uint32_t);
	}

	for (unsigned int y = (unsigned int)std::min(height, orig_height); y > 0; y--) {
		memcpy(dest, src, row_bytes);
		dest += dest_stride;
		src += src_stride;
	}

	// If CI8, copy the palette.
	if (format == rp_image::FORMAT_CI8) {
		int entries = std::min(img->palette_len(), d->backend->palette_len());
		uint32_t *const dest_pal = img->palette();
		memcpy(dest_pal, d->backend->palette(), entries * sizeof(uint32_t));
		// Palette is zero-initialized, so we don't need to
		// zero remaining entries.
	}

	// Copy sBIT if it's set.
	// TODO: Make sure alpha is at least 1?
	if (d->has_sBIT) {
		img->set_sBIT(&d->sBIT);
	}

	// Image resized.
	return img;
}

/**
 * Convert a chroma-keyed image to standard ARGB32.
 * Standard version using regular C++ code.
 *
 * This operates on the image itself, and does not return
 * a duplicated image with the adjusted image.
 *
 * NOTE: The image *must* be ARGB32.
 *
 * @param key Chroma key color.
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::apply_chroma_key_cpp(uint32_t key)
{
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == FORMAT_ARGB32);
	if (backend->format != FORMAT_ARGB32) {
		// ARGB32 only.
		return -EINVAL;
	}

	const unsigned int diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	uint32_t *img_buf = reinterpret_cast<uint32_t*>(backend->data());

	for (unsigned int y = (unsigned int)backend->height; y > 0; y--) {
		unsigned int x = (unsigned int)backend->width;
		for (; x > 1; x -= 2, img_buf += 2) {
			// Check for chroma key pixels.
			if (img_buf[0] == key) {
				img_buf[0] = 0;
			}
			if (img_buf[1] == key) {
				img_buf[1] = 0;
			}
		}

		if (x == 1) {
			if (*img_buf == key) {
				*img_buf = 0;
			}
			img_buf++;
		}

		// Next row.
		img_buf += diff;
	}

	// Adjust sBIT.
	// TODO: Only if transparent pixels were found.
	if (d->has_sBIT && d->sBIT.alpha == 0) {
		d->sBIT.alpha = 1;
	}

	// Chroma key applied.
	return 0;
}

}

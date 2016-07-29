/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * rp_image.hpp: Image class.                                              *
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

#include "rp_image.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

class rp_image_private
{
	public:
		rp_image_private(int width, int height, rp_image::Format format);
		~rp_image_private();

	private:
		rp_image_private(const rp_image_private &);
		rp_image_private &operator=(const rp_image_private &);

	public:
		/**
		 * Get the pixel size for the image format.
		 * @return Pixel size, in bytes.
		 */
		int pxSize(void) const;

		int width;
		int height;
		rp_image::Format format;

		// Image data.
		void *data;
		size_t data_len;

		// Image palette.
		uint32_t *palette;
		int palette_len;
		int tr_idx;
};

/** rp_image_private **/

rp_image_private::rp_image_private(int width, int height, rp_image::Format format)
	: width(width)
	, height(height)
	, format(format)
	, data(nullptr)
	, data_len(0)
	, palette(nullptr)
	, palette_len(0)
	, tr_idx(0)
{
	if (this->width <= 0 || this->height <= 0 ||
	    this->format == rp_image::FORMAT_NONE)
	{
		// Invalid image specifications.
		this->width = 0;
		this->height = 0;
		this->format = rp_image::FORMAT_NONE;
		return;
	}

	int pxs = pxSize();
	if (pxs <= 0) {
		// Invalid image format.
		this->width = 0;
		this->height = 0;
		this->format = rp_image::FORMAT_NONE;
		return;
	}

	// Allocate memory for the image.
	data_len = width * height * pxs;
	assert(data_len > 0);
	if (data_len == 0) {
		// Somehow we have a 0-length image...
		this->width = 0;
		this->height = 0;
		this->format = rp_image::FORMAT_NONE;
		return;
	}

	data = malloc(data_len);
	assert(data != nullptr);
	if (!data) {
		// Failed to allocate memory.
		this->width = 0;
		this->height = 0;
		this->format = rp_image::FORMAT_NONE;
		return;
	}

	// Do we need to allocate memory for the palette?
	if (this->format == rp_image::FORMAT_CI8) {
		// Palette is initialized to 0 to ensure
		// there's no weird artifacts if the caller
		// is converting a lower-color image.
		palette = (uint32_t*)calloc(256, sizeof(*palette));
		if (!palette) {
			// Failed to allocate memory.
			this->width = 0;
			this->height = 0;
			this->format = rp_image::FORMAT_NONE;
			free(data);
			data = nullptr;
			data_len = 0;
			return;
		}

		// 256 colors allocated in the palette.
		palette_len = 256;
	}
}

rp_image_private::~rp_image_private()
{
	free(data);
}

/**
 * Get the pixel size for the image format.
 * @return Pixel size, in bytes.
 */
int rp_image_private::pxSize(void) const
{
	switch (format) {
		case rp_image::FORMAT_CI8:
			return 1;
		case rp_image::FORMAT_ARGB32:
			return 4;
		case rp_image::FORMAT_NONE:
		default:
			return 0;
	}
}

/** rp_image **/

rp_image::rp_image(int width, int height, rp_image::Format format)
	: d(new rp_image_private(width, height, format))
{ }

rp_image::~rp_image()
{
	delete d;
}

/** Properties. **/

/**
 * Is the image valid?
 * @return True if the image is valid.
 */
bool rp_image::isValid(void) const
{
	return (d->width > 0 && d->height > 0 &&
		d->format != FORMAT_NONE &&
		d->data && d->data_len > 0 &&
		(d->format != FORMAT_CI8 || (d->palette && d->palette_len > 0)));
}

/**
 * Get the image width.
 * @return Image width.
 */
int rp_image::width(void) const
{
	return d->width;
}

/**
 * Get the image height.
 * @return Image height.
 */
int rp_image::height(void) const
{
	return d->height;
}

/**
 * Get the image format.
 * @return Image format.
 */
rp_image::Format rp_image::format(void) const
{
	return d->format;
}

/**
 * Get a pointer to the first line of image data.
 * @return Image data.
 */
const void *rp_image::bits(void) const
{
	return d->data;
}

/**
 * Get a pointer to the first line of image data.
 * TODO: detach()
 * @return Image data.
 */
void *rp_image::bits(void)
{
	return d->data;
}

/**
 * Get a pointer to the specified line of image data.
 * @param i Line number.
 * @return Line of image data, or nullptr if i is out of range.
 */
const void *rp_image::scanLine(int i) const
{
	if (!d->data)
		return nullptr;

	return ((uint8_t*)d->data) + (d->width * i * d->pxSize());
}

/**
 * Get a pointer to the specified line of image data.
 * TODO: Detach.
 * @param i Line number.
 * @return Line of image data, or nullptr if i is out of range.
 */
void *rp_image::scanLine(int i)
{
	if (!d->data)
		return nullptr;

	return ((uint8_t*)d->data) + (d->width * i * d->pxSize());
}

/**
 * Get the image data size, in bytes.
 * This is width * height * pixel size.
 * @return Image data size, in bytes.
 */
size_t rp_image::data_len(void) const
{
	return d->data_len;
}

/**
 * Get the image palette.
 * @return Pointer to image palette, or nullptr if not a paletted image.
 */
const uint32_t *rp_image::palette(void) const
{
	return d->palette;
}

/**
 * Get the image palette.
 * @return Pointer to image palette, or nullptr if not a paletted image.
 */
uint32_t *rp_image::palette(void)
{
	return d->palette;
}

/**
 * Get the number of elements in the image palette.
 * @return Number of elements in the image palette, or 0 if not a paletted image.
 */
int rp_image::palette_len(void) const
{
	return d->palette_len;
}

/**
 * Get the index of the transparency color in the palette.
 * This is useful for images that use a single transparency
 * color instead of alpha transparency.
 * @return Transparent color index, or -1 if ARGB32 is used or the palette has alpha transparent colors.
 */
int rp_image::tr_idx(void) const
{
	if (d->format != FORMAT_CI8)
		return -1;
	return d->tr_idx;
}

/**
* Set the index of the transparency color in the palette.
* This is useful for images that use a single transparency
* color instead of alpha transparency.
* @param tr_idx Transparent color index.
*/
void rp_image::set_tr_idx(int tr_idx)
{
	assert(d->format == FORMAT_CI8);
	assert(tr_idx >= 0 && tr_idx < d->palette_len);

	if (d->format == FORMAT_CI8 &&
	    tr_idx >= 0 && tr_idx < d->palette_len)
	{
		d->tr_idx = tr_idx;
	}
}

}

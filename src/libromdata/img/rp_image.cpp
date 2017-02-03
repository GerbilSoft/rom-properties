/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * rp_image.hpp: Image class.                                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
#include "rp_image_backend.hpp"

#include "common.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

namespace LibRomData {

/** rp_image_backend_default **/

class rp_image_backend_default : public rp_image_backend
{
 	public:
		rp_image_backend_default(int width, int height, rp_image::Format format);
		virtual ~rp_image_backend_default();

	private:
		typedef rp_image_backend super;
		RP_DISABLE_COPY(rp_image_backend_default)

	public:
		virtual void *data(void) override final
		{
			return m_data;
		}

		virtual const void *data(void) const override final
		{
			return m_data;
		}

		virtual size_t data_len(void) const override final
		{
			return m_data_len;
		}

		virtual uint32_t *palette(void) override final
		{
			return m_palette;
		}

		virtual const uint32_t *palette(void) const override final
		{
			return m_palette;
		}

		virtual int palette_len(void) const override final
		{
			return m_palette_len;
		}

	private:
		void *m_data;
		size_t m_data_len;

		uint32_t *m_palette;
		int m_palette_len;
};

rp_image_backend_default::rp_image_backend_default(int width, int height, rp_image::Format format)
	: super(width, height, format)
	, m_data(nullptr)
	, m_data_len(0)
	, m_palette(nullptr)
	, m_palette_len(0)
{
	// Allocate memory for the image.
	m_data_len = height * stride;
	assert(m_data_len > 0);
	if (m_data_len == 0) {
		// Somehow we have a 0-length image...
		clear_properties();
		return;
	}

	m_data = malloc(m_data_len);
	assert(m_data != nullptr);
	if (!m_data) {
		// Failed to allocate memory.
		clear_properties();
		return;
	}

	// Do we need to allocate memory for the palette?
	if (format == rp_image::FORMAT_CI8) {
		// Palette is initialized to 0 to ensure
		// there's no weird artifacts if the caller
		// is converting a lower-color image.
		m_palette = (uint32_t*)calloc(256, sizeof(*m_palette));
		if (!m_palette) {
			// Failed to allocate memory.
			free(m_data);
			m_data = nullptr;
			m_data_len = 0;
			clear_properties();
			return;
		}

		// 256 colors allocated in the palette.
		m_palette_len = 256;
	}
}

rp_image_backend_default::~rp_image_backend_default()
{
	free(m_data);
	free(m_palette);
}

/** rp_image_private **/

class rp_image_private
{
	public:
		/**
		 * Create an rp_image_private.
		 *
		 * If an rp_image_backend has been registered, that backend
		 * will be used; otherwise, the defaul tbackend will be used.
		 *
		 * @param width Image width.
		 * @param height Image height.
		 * @param format Image format.
		 */
		rp_image_private(int width, int height, rp_image::Format format);

		/**
		 * Create an rp_image_private using the specified rp_image_backend.
		 *
		 * NOTE: This rp_image will take ownership of the rp_image_backend.
		 *
		 * @param backend rp_image_backend.
		 */
		explicit rp_image_private(rp_image_backend *backend);

		~rp_image_private();

	private:
		rp_image_private(const rp_image_private &other);
		rp_image_private &operator=(const rp_image_private &other);

	public:
		static rp_image::rp_image_backend_creator_fn backend_fn;

	public:
		// Image backend.
		rp_image_backend *backend;
};

/** rp_image_private **/

rp_image::rp_image_backend_creator_fn rp_image_private::backend_fn = nullptr;

/**
 * Create an rp_image_private.
 *
 * If an rp_image_backend has been registered, that backend
 * will be used; otherwise, the defaul tbackend will be used.
 *
 * @param width Image width.
 * @param height Image height.
 * @param format Image format.
 */
rp_image_private::rp_image_private(int width, int height, rp_image::Format format)
{
	if (width <= 0 || height <= 0 ||
	    (format != rp_image::FORMAT_CI8 && format != rp_image::FORMAT_ARGB32))
	{
		// Invalid image specifications.
		this->backend = new rp_image_backend_default(0, 0, rp_image::FORMAT_NONE);
		return;
	}

	// Allocate a storage object for the image.
	if (backend_fn != nullptr) {
		this->backend = backend_fn(width, height, format);
	} else {
		this->backend = new rp_image_backend_default(width, height, format);
	}
}

/**
 * Create an rp_image_private using the specified rp_image_backend.
 *
 * NOTE: This rp_image will take ownership of the rp_image_backend.
 *
 * @param backend rp_image_backend.
 */
rp_image_private::rp_image_private(rp_image_backend *backend)
	: backend(backend)
{ }

rp_image_private::~rp_image_private()
{
	delete backend;
}

/** rp_image **/

/**
 * Create an rp_image.
 *
 * If an rp_image_backend has been registered, that backend
 * will be used; otherwise, the defaul tbackend will be used.
 *
 * @param width Image width.
 * @param height Image height.
 * @param format Image format.
 */
rp_image::rp_image(int width, int height, rp_image::Format format)
	: d(new rp_image_private(width, height, format))
{ }

/**
 * Create an rp_image using the specified rp_image_backend.
 *
 * NOTE: This rp_image will take ownership of the rp_image_backend.
 *
 * @param backend rp_image_backend.
 */
rp_image::rp_image(rp_image_backend *backend)
	: d(new rp_image_private(backend))
{ }

rp_image::~rp_image()
{
	delete d;
}

/** Creator function. **/

/**
 * Set the image backend creator function.
 * @param backend Image backend creator function.
 */
void rp_image::setBackendCreatorFn(rp_image_backend_creator_fn backend_fn)
{
	rp_image_private::backend_fn = backend_fn;
}

/**
 * Get the image backend creator function.
 * @param backend Image backend creator function, or nullptr if the default is in use.
 */
rp_image::rp_image_backend_creator_fn rp_image::backendCreatorFn(void)
{
	return rp_image_private::backend_fn;
}

/**
 * Get this image's backend object.
 * @return Image backend object.
 */
const rp_image_backend *rp_image::backend(void) const
{
	return d->backend;
}

/** Properties. **/

/**
 * Is the image valid?
 * @return True if the image is valid.
 */
bool rp_image::isValid(void) const
{
	return d->backend->isValid();
}

/**
 * Get the image width.
 * @return Image width.
 */
int rp_image::width(void) const
{
	return d->backend->width;
}

/**
 * Get the image height.
 * @return Image height.
 */
int rp_image::height(void) const
{
	return d->backend->height;
}

/**
 * Get the number of bytes per line.
 * @return Bytes per line.
 */
int rp_image::stride(void) const
{
	return d->backend->stride;
}

/**
 * Get the image format.
 * @return Image format.
 */
rp_image::Format rp_image::format(void) const
{
	return d->backend->format;
}

/**
 * Get a pointer to the first line of image data.
 * @return Image data.
 */
const void *rp_image::bits(void) const
{
	return d->backend->data();
}

/**
 * Get a pointer to the first line of image data.
 * TODO: detach()
 * @return Image data.
 */
void *rp_image::bits(void)
{
	return d->backend->data();
}

/**
 * Get a pointer to the specified line of image data.
 * @param i Line number.
 * @return Line of image data, or nullptr if i is out of range.
 */
const void *rp_image::scanLine(int i) const
{
	const uint8_t *data = static_cast<const uint8_t*>(d->backend->data());
	if (!data)
		return nullptr;

	return data + (d->backend->stride * i);
}

/**
 * Get a pointer to the specified line of image data.
 * TODO: Detach.
 * @param i Line number.
 * @return Line of image data, or nullptr if i is out of range.
 */
void *rp_image::scanLine(int i)
{
	uint8_t *data = static_cast<uint8_t*>(d->backend->data());
	if (!data)
		return nullptr;

	return data + (d->backend->stride * i);
}

/**
 * Get the image data size, in bytes.
 * This is width * height * pixel size.
 * @return Image data size, in bytes.
 */
size_t rp_image::data_len(void) const
{
	return d->backend->data_len();
}

/**
 * Get the image palette.
 * @return Pointer to image palette, or nullptr if not a paletted image.
 */
uint32_t *rp_image::palette(void)
{
	return d->backend->palette();
}

/**
 * Get the image palette.
 * @return Pointer to image palette, or nullptr if not a paletted image.
 */
const uint32_t *rp_image::palette(void) const
{
	return d->backend->palette();
}

/**
 * Get the number of elements in the image palette.
 * @return Number of elements in the image palette, or 0 if not a paletted image.
 */
int rp_image::palette_len(void) const
{
	return d->backend->palette_len();
}

/**
 * Get the index of the transparency color in the palette.
 * This is useful for images that use a single transparency
 * color instead of alpha transparency.
 * @return Transparent color index, or -1 if ARGB32 is used or the palette has alpha transparent colors.
 */
int rp_image::tr_idx(void) const
{
	if (d->backend->format != FORMAT_CI8)
		return -1;

	return d->backend->tr_idx;
}

/**
 * Set the index of the transparency color in the palette.
 * This is useful for images that use a single transparency
 * color instead of alpha transparency.
 * @param tr_idx Transparent color index. (Set to -1 if the palette has alpha transparent colors.)
 */
void rp_image::set_tr_idx(int tr_idx)
{
	assert(d->backend->format == FORMAT_CI8);
	assert(tr_idx >= -1 && tr_idx < d->backend->palette_len());

	if (d->backend->format == FORMAT_CI8 &&
	    tr_idx >= -1 && tr_idx < d->backend->palette_len())
	{
		d->backend->tr_idx = tr_idx;
	}
}

/**
* Get the name of a format
* @param format Format.
* @return String containing the user-friendly name of a format.
*/
const rp_char *rp_image::getFormatName(Format format){
	assert(format >= FORMAT_NONE && format < FORMAT_LAST);
	if (format < FORMAT_NONE || format >= FORMAT_LAST) {
		return nullptr;
	}

	static const rp_char *format_names[] = {
		_RP("None"),
		_RP("CI8"),
		_RP("ARGB32"),
	};
	static_assert(ARRAY_SIZE(format_names) == FORMAT_LAST,
		"format_names[] needs to be updated.");

	return format_names[format];
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * rp_image_backend.hpp: Image backend and storage classes.                *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_RP_IMAGE_BACKEND_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_RP_IMAGE_BACKEND_HPP__

#include "rp_image.hpp"

namespace LibRomData {

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class rp_image_backend
{
	public:
		rp_image_backend(int width, int height, rp_image::Format format);
		virtual ~rp_image_backend();

	private:
		RP_DISABLE_COPY(rp_image_backend)
	public:
		bool isValid(void) const;

	protected:
		/**
		 * Clear the width, height, stride, and format properties.
		 * Used in error paths.
		 * */
		void clear_properties(void);

		/**
		 * Check if the palette contains alpha values other than 0 and 255.
		 * @return True if an alpha value other than 0 and 255 was found; false if not, or if ARGB32.
		 */
		bool has_translucent_palette_entries(void) const;

	public:
		int width;
		int height;
		int stride;
		rp_image::Format format;

		// Image data.
		virtual void *data(void) = 0;
		virtual const void *data(void) const = 0;
		virtual size_t data_len(void) const = 0;

		// Image palette.
		virtual uint32_t *palette(void) = 0;
		virtual const uint32_t *palette(void) const = 0;
		virtual int palette_len(void) const = 0;
		int tr_idx;

	public:
		// Subclasses can have other stuff here.
		// Use dynamic_cast<> to access it.
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RP_IMAGE_BACKEND_HPP__ */

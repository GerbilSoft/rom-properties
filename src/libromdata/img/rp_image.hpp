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

#ifndef __ROMPROPERTIES_LIBROMDATA_RP_IMAGE_HPP__
#define __ROMPROPERTIES_LIBROMDATA_RP_IMAGE_HPP__

#include "config.libromdata.h"
#include "common.h"

// C includes.
#include <stdint.h>

// TODO: Make this implicitly shared.

namespace LibRomData {

class rp_image_backend;

class rp_image_private;
class rp_image
{
	public:
		enum Format {
			FORMAT_NONE,		// No image.
			FORMAT_CI8,		// Color index, 8-bit palette.
			FORMAT_ARGB32,		// 32-bit ARGB.

			FORMAT_LAST		// End of Format.
		};

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
		rp_image(int width, int height, Format format);

		/**
		 * Create an rp_image using the specified rp_image_backend.
		 *
		 * NOTE: This rp_image will take ownership of the rp_image_backend.
		 *
		 * @param backend rp_image_backend.
		 */
		explicit rp_image(rp_image_backend *backend);

		~rp_image();

	private:
		friend class rp_image_private;
		rp_image_private *const d;
	private:
		RP_DISABLE_COPY(rp_image)

	public:
		/**
		 * rp_image_backend creator function.
		 * May be a static member of an rp_image_backend subclass.
		 */
		typedef rp_image_backend* (*rp_image_backend_creator_fn)(int width, int height, rp_image::Format format);

		/**
		 * Set the image backend creator function.
		 * @param backend Image backend creator function.
		 */
		static void setBackendCreatorFn(rp_image_backend_creator_fn backend_fn);

		/**
		 * Get the image backend creator function.
		 * @param backend Image backend creator function, or nullptr if the default is in use.
		 */
		static rp_image_backend_creator_fn backendCreatorFn(void);

	public:
		/**
		 * Get this image's backend object.
		 * @return Image backend object.
		 */
		const rp_image_backend *backend(void) const;

	public:
		/** Properties. **/

		/**
		 * Is the image valid?
		 * @return True if the image is valid.
		 */
		bool isValid(void) const;

		/**
		 * Get the image width.
		 * @return Image width.
		 */
		int width(void) const;

		/**
		 * Get the image height.
		 * @return Image height.
		 */
		int height(void) const;

		/**
		 * Get the number of bytes per line.
		 * @return Bytes per line.
		 */
		int stride(void) const;

		/**
		 * Get the image format.
		 * @return Image format.
		 */
		Format format(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * @return Image data.
		 */
		const void *bits(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * TODO: detach()
		 * @return Image data.
		 */
		void *bits(void);

		/**
		 * Get a pointer to the specified line of image data.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		const void *scanLine(int i) const;

		/**
		 * Get a pointer to the specified line of image data.
		 * TODO: Detach.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		void *scanLine(int i);

		/**
		 * Get the image data size, in bytes.
		 * This is width * height * pixel size.
		 * @return Image data size, in bytes.
		 */
		size_t data_len(void) const;

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		uint32_t *palette(void);

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		const uint32_t *palette(void) const;

		/**
		 * Get the number of elements in the image palette.
		 * @return Number of elements in the image palette, or 0 if not a paletted image.
		 */
		int palette_len(void) const;

		/**
		 * Get the index of the transparency color in the palette.
		 * This is useful for images that use a single transparency
		 * color instead of alpha transparency.
		 * @return Transparent color index, or -1 if ARGB32 is used or the palette has alpha transparent colors.
		 */
		int tr_idx(void) const;

		/**
		 * Set the index of the transparency color in the palette.
		 * This is useful for images that use a single transparency
		 * color instead of alpha transparency.
		 * @param tr_idx Transparent color index. (Set to -1 if the palette has alpha transparent colors.)
		 */
		void set_tr_idx(int tr_idx);

		/**
		 * Get the name of a format
		 * @param format Format.
		 * @return String containing the user-friendly name of a format.
		 */
		static const rp_char *getFormatName(Format format);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_RP_IMAGE_HPP__ */

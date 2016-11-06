/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpPng.hpp: PNG image handler.                                           *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_RPPNG_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_RPPNG_HPP__

namespace LibRomData {

class rp_image;
class IRpFile;

class RpPng
{
	private:
		// RpPng is a static class.
		RpPng();
		~RpPng();
		RpPng(const RpPng &other);
		RpPng &operator=(const RpPng &other);

	public:
		/**
		 * Load a PNG image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load a PNG image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *load(IRpFile *file);

		/**
		 * Save an image in PNG format to an IRpFile.
		 * IRpFile must be open for writing.
		 *
		 * NOTE: If the write fails, the caller will need
		 * to delete the file.
		 *
		 * @param file IRpFile to write to.
		 * @param img rp_image to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(IRpFile *file, const rp_image *img);

		/**
		 * Save an image in PNG format to a file.
		 *
		 * @param filename Destination filename.
		 * @param img rp_image to save.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int save(const rp_char *filename, const rp_image *img);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RPPNG_HPP__ */

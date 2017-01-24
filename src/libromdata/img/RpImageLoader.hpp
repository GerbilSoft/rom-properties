/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpImageLoader.hpp: Image loader class.                                  *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_RPIMAGELOADER_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_RPIMAGELOADER_HPP__

#include "common.h"

namespace LibRomData {

class rp_image;
class IRpFile;

class RpImageLoader
{
	private:
		// RpImageLoader is a static class.
		RpImageLoader();
		~RpImageLoader();
		RP_DISABLE_COPY(RpImageLoader)

	public:
		/**
		 * Load an image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load an image from an IRpFile.
		 *
		 * This image is verified with various tools to ensure
		 * it doesn't have any errors.
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *load(IRpFile *file);
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RP_IMAGE_LOADER_HPP__ */

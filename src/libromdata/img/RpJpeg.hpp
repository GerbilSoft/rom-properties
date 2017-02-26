/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpJpeg.hpp: JPEG image handler.                                         *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_RPJPEG_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_RPJPEG_HPP__

#include "config.libromdata.h"
#include "common.h"

namespace LibRomData {

class IRpFile;
class rp_image;
struct IconAnimData;

class RpJpeg
{
	private:
		// RpJpeg is a static class.
		RpJpeg();
		~RpJpeg();
		RP_DISABLE_COPY(RpJpeg)

	public:
		/**
		 * Load a JPEG image from an IRpFile.
		 *
		 * This image is NOT checked for issues; do not use
		 * with untrusted images!
		 *
		 * @param file IRpFile to load from.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadUnchecked(IRpFile *file);

		/**
		 * Load a JPEG image from an IRpFile.
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

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RPJPEG_HPP__ */

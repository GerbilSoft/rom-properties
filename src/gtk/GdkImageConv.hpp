/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * GdkImageConv.hpp: Helper functions to convert from rp_image to GDK.     *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#ifndef __ROMPROPERTIES_XFCE_GDKIMAGECONV_HPP__
#define __ROMPROPERTIES_XFCE_GDKIMAGECONV_HPP__

// NOTE: GdkPixbuf doesn't natively support 8bpp. Because of this,
// we can't simply make a GdkPixbuf rp_image backend.

#include "libromdata/common.h"
namespace LibRomData {
	class rp_image;
}
#include <gdk-pixbuf/gdk-pixbuf.h>

class GdkImageConv
{
	private:
		// Static class.
		GdkImageConv();
		~GdkImageConv();
		RP_DISABLE_COPY(GdkImageConv)

	public:
		/**
		 * Convert an rp_image to GdkPixbuf.
		 * @param img rp_image.
		 * @return GdkPixbuf, or nullptr on error.
		 */
		static GdkPixbuf *rp_image_to_GdkPixbuf(const LibRomData::rp_image *img);
};

#endif /* __ROMPROPERTIES_XFCE_GDKIMAGECONV_HPP__ */

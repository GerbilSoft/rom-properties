/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * GdkImageConv.cpp: Helper functions to convert from rp_image to GDK.     *
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

#include "GdkImageConv.hpp"

// C include. (C++ namespace)
#include <cassert>

#include "libromdata/img/rp_image.hpp"
using LibRomData::rp_image;

/**
 * Convert an rp_image to GdkPixbuf.
 * @param img rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf(const rp_image *img)
{
	assert(img != nullptr);
	if (!img || !img->isValid())
		return nullptr;

	// NOTE: GdkPixbuf's convenience functions don't do a
	// deep copy, so we can't use them directly.
	const int width = img->width();
	const int height = img->height();
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	assert(pixbuf != nullptr);
	if (!pixbuf)
		return nullptr;

	switch (img->format()) {
		case rp_image::FORMAT_ARGB32: {
			// Copy the image data directly.
			uint32_t *dest = reinterpret_cast<uint32_t*>(gdk_pixbuf_get_pixels(pixbuf));
			const int strideDiff = (gdk_pixbuf_get_rowstride(pixbuf) / sizeof(*dest)) - img->width();
			for (int y = 0; y < height; y++, dest += strideDiff) {
				const uint32_t *src = static_cast<const uint32_t*>(img->scanLine(y));
				for (int x = width; x > 0; x--, dest++, src++) {
					// Swap the R and B channels.
					*dest =  (*src & 0xFF00FF00) |
						((*src & 0x00FF0000) >> 16) |
						((*src & 0x000000FF) << 16);
				}
			}
			break;
		}

		default:
			// TODO
			g_object_unref(pixbuf);
			pixbuf = nullptr;
			break;
	}

	return pixbuf;
}

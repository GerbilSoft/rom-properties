/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * un-premultiply.cpp: Un-premultiply function.                            *
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

#include "un-premultiply.hpp"
#include "rp_image.hpp"
#include "ImageDecoder_p.hpp"
#include "../common.h"

// C includes. (C++ namespace)
#include <cassert>

namespace LibRpBase {

/**
 * Un-premultiply an argb32_t pixel.
 * This is needed in order to convert DXT2/3 to DXT4/5.
 * @param px	[in/out] argb32_t pixel to un-premultiply, in place.
 */
static FORCE_INLINE void un_premultiply_pixel(argb32_t &px)
{
	if (px.a == 255) {
		// Do nothing.
	} else if (px.a == 0) {
		px.u32 = 0;
	} else {
		px.r = px.r * 255 / px.a;
		px.g = px.g * 255 / px.a;
		px.b = px.b * 255 / px.a;
	}
}

/**
 * Un-premultiply an ARGB32 rp_image.
 * @param img	[in] rp_image to un-premultiply.
 * @return 0 on success; non-zero on error.
 */
int un_premultiply_image(rp_image *img)
{
	assert(img->format() != rp_image::FORMAT_ARGB32);
	if (img->format() != rp_image::FORMAT_ARGB32) {
		// Incorrect format...
		return -1;
	}

	const int width = img->width();
	for (int y = img->height()-1; y >= 0; y--) {
		argb32_t *px_dest = static_cast<argb32_t*>(img->scanLine(y));
		int x = width;
		for (; x > 1; x -= 2, px_dest += 2) {
			un_premultiply_pixel(px_dest[0]);
			un_premultiply_pixel(px_dest[1]);
		}
		if (x == 1) {
			un_premultiply_pixel(*px_dest);
		}
	}
	return 0;
}

}

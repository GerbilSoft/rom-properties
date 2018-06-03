/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CairoImageConv.hpp: Helper functions to convert from rp_image to Cairo. *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_CAIROIMAGECONV_HPP__
#define __ROMPROPERTIES_GTK_CAIROIMAGECONV_HPP__

// NOTE: Cairo doesn't natively support 8bpp. Because of this,
// we can't simply make a cairo_surface_t rp_image backend.

#include "librpbase/common.h"
#include "librpbase/cpu_dispatch.h"
namespace LibRpBase {
	class rp_image;
}
#include <cairo.h>

class CairoImageConv
{
	private:
		// Static class.
		CairoImageConv();
		~CairoImageConv();
		RP_DISABLE_COPY(CairoImageConv)

	public:
		/**
		 * Convert an rp_image to cairo_surface_t.
		 * @param img	[in] rp_image.
		 * @return cairo_surface_t, or nullptr on error.
		 */
		static cairo_surface_t *rp_image_to_cairo_surface_t(const LibRpBase::rp_image *img);
};

#endif /* __ROMPROPERTIES_GTK_CAIROIMAGECONV_HPP__ */

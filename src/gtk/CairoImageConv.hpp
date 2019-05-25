/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CairoImageConv.hpp: Helper functions to convert from rp_image to Cairo. *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
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

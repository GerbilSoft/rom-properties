/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_GDKIMAGECONV_HPP__
#define __ROMPROPERTIES_GTK_GDKIMAGECONV_HPP__

// NOTE: GdkPixbuf doesn't natively support 8bpp. Because of this,
// we can't simply make a GdkPixbuf rp_image backend.

#include "librpbase/common.h"
#include "librpbase/cpu_dispatch.h"
namespace LibRpBase {
	class rp_image;
}
#include <gdk-pixbuf/gdk-pixbuf.h>

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpbase/cpuflags_x86.h"
# define GDKIMAGECONV_HAS_SSSE3 1
#endif

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
		 * Standard version using regular C++ code.
		 * @param img	[in] rp_image.
		 * @return GdkPixbuf, or nullptr on error.
		 */
		static GdkPixbuf *rp_image_to_GdkPixbuf_cpp(const LibRpBase::rp_image *img);

#ifdef GDKIMAGECONV_HAS_SSSE3
		/**
		 * Convert an rp_image to GdkPixbuf.
		 * SSSE3-optimized version.
		 * @param img	[in] rp_image.
		 * @return GdkPixbuf, or nullptr on error.
		 */
		static GdkPixbuf *rp_image_to_GdkPixbuf_ssse3(const LibRpBase::rp_image *img);
#endif /* GDKIMAGECONV_HAS_SSSE3 */

		/**
		 * Convert an rp_image to GdkPixbuf.
		 * @param img	[in] rp_image.
		 * @return GdkPixbuf, or nullptr on error.
		 */
		static IFUNC_INLINE GdkPixbuf *rp_image_to_GdkPixbuf(const LibRpBase::rp_image *img);
};

#if !defined(RP_HAS_IFUNC) || (!defined(RP_CPU_I386) && !defined(RP_CPU_AMD64))

// System does not support IFUNC, or we don't have optimizations for these CPUs.
// Use standard inline dispatch.

/**
 * Convert an rp_image to GdkPixbuf.
 * @param img rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
inline GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf(const LibRpBase::rp_image *img)
{
#ifdef GDKIMAGECONV_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return rp_image_to_GdkPixbuf_ssse3(img);
	} else
#endif /* GDKIMAGECONV_HAS_SSSE3 */
	{
		return rp_image_to_GdkPixbuf_cpp(img);
	}
}

#endif /* !defined(RP_HAS_IFUNC) || (!defined(RP_CPU_I386) && !defined(RP_CPU_AMD64)) */

#endif /* __ROMPROPERTIES_GTK_GDKIMAGECONV_HPP__ */

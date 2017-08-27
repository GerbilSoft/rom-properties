/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkImageConv_ifunc.cpp: GdkImageConv IFUNC resolution functions.        *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"
#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "GdkImageConv.hpp"
using LibRpBase::rp_image;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

/**
 * IFUNC resolver function for rp_image_to_GdkPixbuf().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t rp_image_to_GdkPixbuf_resolve(void)
{
#ifdef GDKIMAGECONV_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return (RP_IFUNC_ptr_t)&GdkImageConv::rp_image_to_GdkPixbuf_ssse3;
	} else
#endif /* GDKIMAGECONV_HAS_SSSE3 */
	{
		return (RP_IFUNC_ptr_t)&GdkImageConv::rp_image_to_GdkPixbuf_ssse3;
	}
}

}

GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf(const rp_image *img)
	IFUNC_ATTR(rp_image_to_GdkPixbuf_resolve);

#endif /* RP_HAS_IFUNC */

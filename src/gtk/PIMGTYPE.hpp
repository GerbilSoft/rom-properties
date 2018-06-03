/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_PIMGTYPE_HPP__
#define __ROMPROPERTIES_GTK_PIMGTYPE_HPP__

#include <gtk/gtk.h>

// NOTE: GTK+ 3.x earlier than 3.10 is not supported.
#if GTK_CHECK_VERSION(3,0,0) && !GTK_CHECK_VERSION(3,10,0)
# error GTK+ 3.x earlier than 3.10 is not supported.
#endif

#if GTK_CHECK_VERSION(3,10,0)
# define RP_GTK_USE_CAIRO 1
# include "CairoImageConv.hpp"
typedef cairo_surface_t *PIMGTYPE;
#else
# include "GdkImageConv.hpp"
typedef GdkPixbuf *PIMGTYPE;
#endif

// rp_image_to_PIMGTYPE wrapper function.
static inline PIMGTYPE rp_image_to_PIMGTYPE(const rp_image *img)
{
#ifdef RP_GTK_USE_CAIRO
	return CairoImageConv::rp_image_to_cairo_surface_t(img);
#else /* !RP_GTK_USE_CAIRO */
	return GdkImageConv::rp_image_to_GdkPixbuf(img);
#endif
}

// gtk_image_set_from_PIMGTYPE wrapper function.
static inline void gtk_image_set_from_PIMGTYPE(GtkImage *image, PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	gtk_image_set_from_surface(image, pImgType);
#else /* !RP_GTK_USE_CAIRO */
	gtk_image_set_from_pixbuf(image, pImgType);
#endif
}

// PIMGTYPE free() wrapper function.
static inline void PIMGTYPE_destroy(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	cairo_surface_destroy(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	g_object_unref(pImgType);
#endif
}

#endif /* __ROMPROPERTIES_GTK_PIMGTYPE_HPP__ */

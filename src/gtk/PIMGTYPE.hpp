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
# include <cairo-gobject.h>
# define PIMGTYPE_GOBJECT_TYPE CAIRO_GOBJECT_TYPE_SURFACE
# define GTK_CELL_RENDERER_PIXBUF_PROPERTY "surface"
typedef cairo_surface_t *PIMGTYPE;
#else
# include "GdkImageConv.hpp"
# define PIMGTYPE_GOBJECT_TYPE GDK_TYPE_PIXBUF
# define GTK_CELL_RENDERER_PIXBUF_PROPERTY "pixbuf"
typedef GdkPixbuf *PIMGTYPE;
#endif

// rp_image_to_PIMGTYPE wrapper function.
static inline PIMGTYPE rp_image_to_PIMGTYPE(const rp_image *img)
{
#ifdef RP_GTK_USE_CAIRO
	return CairoImageConv::rp_image_to_cairo_surface_t(img);
#else /* !RP_GTK_USE_CAIRO */
	return GdkImageConv::rp_image_to_GdkPixbuf(img);
#endif /* RP_GTK_USE_CAIRO */
}

// gtk_image_set_from_PIMGTYPE wrapper function.
static inline void gtk_image_set_from_PIMGTYPE(GtkImage *image, PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	gtk_image_set_from_surface(image, pImgType);
#else /* !RP_GTK_USE_CAIRO */
	gtk_image_set_from_pixbuf(image, pImgType);
#endif /* RP_GTK_USE_CAIRO */
}

// PIMGTYPE free() wrapper function.
static inline void PIMGTYPE_destroy(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	cairo_surface_destroy(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	g_object_unref(pImgType);
#endif /* RP_GTK_USE_CAIRO */
}

/**
 * PIMGTYPE size comparison function.
 * @param pImgType PIMGTYPE
 * @param width Expected width
 * @param height Expected height
 * @return True if the size matches; false if not.
 */
static inline bool PIMGTYPE_size_check(PIMGTYPE pImgType, int width, int height)
{
#ifdef RP_GTK_USE_CAIRO
	return (cairo_image_surface_get_width(pImgType)  == width &&
	        cairo_image_surface_get_height(pImgType) == height);
#else /* !RP_GTK_USE_CAIRO */
	return (gdk_pixbuf_get_width(pImgType)  == width &&
	        gdk_pixbuf_get_height(pImgType) == height);
#endif /* RP_GTK_USE_CAIRO */
}

/**
 * PIMGTYPE scaling function.
 * @param pImgType PIMGTYPE
 * @param width New width
 * @param height New height
 * @param bilinear If true, use bilinear interpolation.
 * @return Rescaled image. (If unable to rescale, returns a new reference to pImgType.)
 */
static inline PIMGTYPE PIMGTYPE_scale(PIMGTYPE pImgType, int width, int height, bool bilinear)
{
	// TODO: Maintain aspect ratio, and use nearest-neighbor
	// when scaling up from small sizes.
#ifdef RP_GTK_USE_CAIRO
	int srcWidth = cairo_image_surface_get_width(pImgType);
	int srcHeight = cairo_image_surface_get_width(pImgType);
	assert(srcWidth > 0 && srcHeight > 0);
	if (unlikely(srcWidth <= 0 || srcHeight <= 0)) {
		return cairo_surface_reference(pImgType);
	}

	cairo_surface_t *const surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	assert(surface != nullptr);
	assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!surface)) {
		return cairo_surface_reference(pImgType);
	} else if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	}

	cairo_t *const cr = cairo_create(surface);
	assert(cr != nullptr);
	assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!cr)) {
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	} else if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(surface);
		return cairo_surface_reference(pImgType);
	}

	cairo_scale(cr, (double)width / (double)srcWidth, (double)height / (double)srcHeight);
	cairo_set_source_surface(cr, pImgType, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr),
		(bilinear ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST));
	cairo_paint(cr);
	cairo_destroy(cr);
	return surface;
#else /* !RP_GTK_USE_CAIRO */
	return gdk_pixbuf_scale_simple(pImgType, width, height,
		(bilinear ? GDK_INTERP_BILINEAR : GDK_INTERP_BILINEAR));
#endif /* RP_GTK_USE_CAIRO */
}

#endif /* __ROMPROPERTIES_GTK_PIMGTYPE_HPP__ */

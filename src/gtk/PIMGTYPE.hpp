/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_GTK_PIMGTYPE_HPP__
#define __ROMPROPERTIES_GTK_PIMGTYPE_HPP__

#include <gtk/gtk.h>
#include "stdboolx.h"

// NOTE: GTK+ 3.x earlier than 3.10 is not supported.
#if GTK_CHECK_VERSION(3,0,0) && !GTK_CHECK_VERSION(3,10,0)
# error GTK+ 3.x earlier than 3.10 is not supported.
#endif

#if GTK_CHECK_VERSION(3,10,0)
# define RP_GTK_USE_CAIRO 1
# ifdef __cplusplus
#  include "CairoImageConv.hpp"
# endif /* __cplusplus */
# include <cairo-gobject.h>
# define PIMGTYPE_GOBJECT_TYPE CAIRO_GOBJECT_TYPE_SURFACE
# define GTK_CELL_RENDERER_PIXBUF_PROPERTY "surface"
G_BEGIN_DECLS
typedef cairo_surface_t *PIMGTYPE;
G_END_DECLS
#else
# ifdef __cplusplus
#  include "GdkImageConv.hpp"
# endif /* __cplusplus */
# define PIMGTYPE_GOBJECT_TYPE GDK_TYPE_PIXBUF
# define GTK_CELL_RENDERER_PIXBUF_PROPERTY "pixbuf"
G_BEGIN_DECLS
typedef GdkPixbuf *PIMGTYPE;
G_END_DECLS
#endif

#ifdef __cplusplus
// librptexture
#include "librptexture/img/rp_image.hpp"

// rp_image_to_PIMGTYPE wrapper function.
// NOTE: premultiply is only used for Cairo.
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image *img, bool premultiply = true)
{
#ifdef RP_GTK_USE_CAIRO
	return CairoImageConv::rp_image_to_cairo_surface_t(img, premultiply);
#else /* !RP_GTK_USE_CAIRO */
	((void)premultiply);
	return GdkImageConv::rp_image_to_GdkPixbuf(img);
#endif /* RP_GTK_USE_CAIRO */
}
#endif /* __cplusplus **/

#ifdef __cplusplus
extern "C" {
#endif

// gtk_image_set_from_PIMGTYPE wrapper function.
static inline void gtk_image_set_from_PIMGTYPE(GtkImage *image, PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	gtk_image_set_from_surface(image, pImgType);
#else /* !RP_GTK_USE_CAIRO */
	gtk_image_set_from_pixbuf(image, pImgType);
#endif /* RP_GTK_USE_CAIRO */
}

// gtk_drag_set_icon_PIMGTYPE wrapper function.
static inline void gtk_drag_set_icon_PIMGTYPE(GdkDragContext *context, PIMGTYPE pImgType)
{
	// TODO: Implement hotspot parameters? Requires using
	// cairo_surface_set_device_offset() for Cairo.
#ifdef RP_GTK_USE_CAIRO
	gtk_drag_set_icon_surface(context, pImgType);
#else /* !RP_GTK_USE_CAIRO */
	gtk_drag_set_icon_pixbuf(context, pImgType, 0, 0);
#endif /* RP_GTK_USE_CAIRO */
}

// PIMGTYPE ref() wrapper function.
static inline PIMGTYPE PIMGTYPE_ref(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	return cairo_surface_reference(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	return (PIMGTYPE)g_object_ref(pImgType);
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

#ifdef RP_GTK_USE_CAIRO
/**
 * PIMGTYPE scaling function.
 * @param pImgType PIMGTYPE
 * @param width New width
 * @param height New height
 * @param bilinear If true, use bilinear interpolation.
 * @return Rescaled image. (If unable to rescale, returns a new reference to pImgType.)
 */
PIMGTYPE PIMGTYPE_scale(PIMGTYPE pImgType, int width, int height, bool bilinear);
#else /* !RP_GTK_USE_CAIRO */
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
	return gdk_pixbuf_scale_simple(pImgType, width, height,
		(bilinear ? GDK_INTERP_BILINEAR : GDK_INTERP_NEAREST));
}
#endif /* RP_GTK_USE_CAIRO */

/**
 * Load a PNG image from our glibresources.
 * @param filename Filename within glibresources.
 * @return PIMGTYPE, or nullptr if not found.
 */
PIMGTYPE PIMGTYPE_load_png_from_gresource(const char *filename);

/**
 * Copy a subsurface from another PIMGTYPE.
 * @param pImgType	[in] PIMGTYPE
 * @param x		[in] X position
 * @param y		[in] Y position
 * @param width		[in] Width
 * @param height	[in] Height
 * @return Subsurface, or nullptr on error.
 */
static inline PIMGTYPE PIMGTYPE_get_subsurface(PIMGTYPE pImgType, int x, int y, int width, int height)
{
#ifdef RP_GTK_USE_CAIRO
	return cairo_surface_create_for_rectangle(pImgType, x, y, width, height);
#else /* !RP_GTK_USE_CAIRO */
	PIMGTYPE surface = gdk_pixbuf_new(
		gdk_pixbuf_get_colorspace(pImgType),
		gdk_pixbuf_get_has_alpha(pImgType),
		gdk_pixbuf_get_bits_per_sample(pImgType),
		width, height);
	if (surface) {
		gdk_pixbuf_copy_area(pImgType, x, y, width, height, surface, 0, 0);
	}
	return surface;
#endif /* RP_GTK_USE_CAIRO */
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_GTK_PIMGTYPE_HPP__ */

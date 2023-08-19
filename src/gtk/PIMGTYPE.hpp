/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include <gtk/gtk.h>
#include "stdboolx.h"

// NOTE: GTK+ 3.x earlier than 3.10 is not supported.
#if GTK_CHECK_VERSION(3,0,0) && !GTK_CHECK_VERSION(3,10,0)
#  error GTK+ 3.x earlier than 3.10 is not supported.
#endif

// NOTE: GTK4 doesn't support Cairo anymore, so we'll switch back to GdkPixbuf.
// TODO: Use GdkTexture?
// TODO: Proper GObject cast for Cairo.
#if GTK_CHECK_VERSION(3,10,0) && !GTK_CHECK_VERSION(4,0,0)
#  define RP_GTK_USE_CAIRO 1
#  ifdef __cplusplus
#    include "CairoImageConv.hpp"
#  endif /* __cplusplus */
#  include <cairo-gobject.h>
#  define PIMGTYPE_GOBJECT_TYPE CAIRO_GOBJECT_TYPE_SURFACE
#  define PIMGTYPE_CAST(obj) ((PIMGTYPE)(obj))
#  define GTK_CELL_RENDERER_PIXBUF_PROPERTY "surface"
G_BEGIN_DECLS
typedef cairo_surface_t *PIMGTYPE;
G_END_DECLS
#else
#  ifdef __cplusplus
#    include "GdkImageConv.hpp"
#  endif /* __cplusplus */
#  define PIMGTYPE_GOBJECT_TYPE GDK_TYPE_PIXBUF
#  define PIMGTYPE_CAST(obj) GDK_PIXBUF(obj)
#  define GTK_CELL_RENDERER_PIXBUF_PROPERTY "pixbuf"
G_BEGIN_DECLS
typedef GdkPixbuf *PIMGTYPE;
G_END_DECLS
#endif

#ifdef __cplusplus
// librptexture
#include "librptexture/img/rp_image.hpp"

// C++ includes
#include <memory>

/**
 * rp_image_to_PIMGTYPE wrapper function.
 * @param img rp_image
 * @param premultiply Premultiply alpha (NOTE: Only used for Cairo)
 * @return PIMGTYPE
 */
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image *img, bool premultiply = true)
{
#ifdef RP_GTK_USE_CAIRO
	return CairoImageConv::rp_image_to_cairo_surface_t(img, premultiply);
#else /* !RP_GTK_USE_CAIRO */
	((void)premultiply);
	return GdkImageConv::rp_image_to_GdkPixbuf(img);
#endif /* RP_GTK_USE_CAIRO */
}

/**
 * rp_image_to_PIMGTYPE wrapper function.
 * @param img rp_image_ptr
 * @param premultiply Premultiply alpha (NOTE: Only used for Cairo)
 * @return PIMGTYPE
 */
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image_ptr &img, bool premultiply = true)
{
	return rp_image_to_PIMGTYPE(img.get(), premultiply);
}

/**
 * rp_image_to_PIMGTYPE wrapper function.
 * @param img rp_image_const_ptr
 * @param premultiply Premultiply alpha (NOTE: Only used for Cairo)
 * @return PIMGTYPE
 */
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image_const_ptr &img, bool premultiply = true)
{
	return rp_image_to_PIMGTYPE(img.get(), premultiply);
}

/**
 * rp_image_to_PIMGTYPE wrapper function.
 * @param pImg rp_image_ptr*
 * @param premultiply Premultiply alpha (NOTE: Only used for Cairo)
 * @return PIMGTYPE
 */
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image_ptr *pImg, bool premultiply = true)
{
	return rp_image_to_PIMGTYPE(pImg->get(), premultiply);
}

/**
 * rp_image_to_PIMGTYPE wrapper function.
 * @param pImg rp_image_const_ptr*
 * @param premultiply Premultiply alpha (NOTE: Only used for Cairo)
 * @return PIMGTYPE
 */
static inline PIMGTYPE rp_image_to_PIMGTYPE(const LibRpTexture::rp_image_const_ptr *pImg, bool premultiply = true)
{
	return rp_image_to_PIMGTYPE(pImg->get(), premultiply);
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

#if !GTK_CHECK_VERSION(4,0,0)	// FIXME: GTK4 has a new Drag & Drop API.
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
#endif /* !GTK_CHECK_VERSION(4,0,0) */

// PIMGTYPE ref() wrapper function.
static inline PIMGTYPE PIMGTYPE_ref(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	return cairo_surface_reference(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	return (PIMGTYPE)g_object_ref(pImgType);
#endif /* RP_GTK_USE_CAIRO */
}

// PIMGTYPE unref() wrapper function.
static inline void PIMGTYPE_unref(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	cairo_surface_destroy(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	g_object_unref(pImgType);
#endif /* RP_GTK_USE_CAIRO */
}

/**
 * Get the dimensions of a PIMGTYPE.
 * @param pImgType	[in] PIMGTYPE
 * @param pWidth	[out] Width
 * @param pHeight	[out] Height
 * @return 0 on success; non-zero on error.
 */
static inline int PIMGTYPE_get_size(PIMGTYPE pImgType, int *pWidth, int *pHeight)
{
#ifdef RP_GTK_USE_CAIRO
	// TODO: Verify that this is an image surface.
	*pWidth = cairo_image_surface_get_width(pImgType);
	*pHeight = cairo_image_surface_get_height(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	*pWidth = gdk_pixbuf_get_width(pImgType);
	*pHeight = gdk_pixbuf_get_height(pImgType);
#endif
	return 0;
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

#ifdef __cplusplus
#  define PIMGTYPE_EQ_NULLPTR = nullptr
#else /* !__cplusplus */
#  define PIMGTYPE_EQ_NULLPTR
#endif /* __cplusplus */

/**
 * Get a pointer to the raw image data of a PIMGTYPE.
 * @param pImgType	[in] PIMGTYPE
 * @param pLen		[out,opt] Length of the image data.
 * @return Pointer to the raw image data.
 */
static inline uint8_t *PIMGTYPE_get_image_data(PIMGTYPE pImgType, size_t *pLen PIMGTYPE_EQ_NULLPTR)
{
	// TODO: Verify if the last row is a complete row.

#ifdef RP_GTK_USE_CAIRO
	cairo_surface_flush(pImgType);
	if (pLen) {
		*pLen = (size_t)cairo_image_surface_get_stride(pImgType) *
		        (size_t)cairo_image_surface_get_height(pImgType);
	}
	return cairo_image_surface_get_data(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	if (pLen) {
		*pLen = gdk_pixbuf_get_byte_length(pImgType);
	}
	return gdk_pixbuf_get_pixels(pImgType);
#endif
}

/**
 * Mark a PIMGTYPE as dirty.
 * This must be called after modifying the underlying image data.
 * @param pImgType	[in] PIMGTYPE
 */
static inline void PIMGTYPE_mark_dirty(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	cairo_surface_mark_dirty(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	// Nothing to do here...
	RP_UNUSED(pImgType);
#endif
}

/**
 * Get the row stride of a PIMGTYPE.
 * @param pImgType PIMGTYPE
 * @return Row stride. (bytes per line)
 */
static inline int PIMGTYPE_get_rowstride(PIMGTYPE pImgType)
{
#ifdef RP_GTK_USE_CAIRO
	return cairo_image_surface_get_stride(pImgType);
#else /* !RP_GTK_USE_CAIRO */
	return gdk_pixbuf_get_rowstride(pImgType);
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
PIMGTYPE PIMGTYPE_get_subsurface(PIMGTYPE pImgType, int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

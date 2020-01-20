/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "PIMGTYPE.hpp"

// glib resources
// NOTE: glib-compile-resources doesn't have extern "C".
extern "C" {
#include "glibresources.h"
}

// C includes. (C++ namespace)
#include <cassert>

#ifdef RP_GTK_USE_CAIRO
/**
 * PIMGTYPE scaling function.
 * @param pImgType PIMGTYPE
 * @param width New width
 * @param height New height
 * @param bilinear If true, use bilinear interpolation.
 * @return Rescaled image. (If unable to rescale, returns a new reference to pImgType.)
 */
PIMGTYPE PIMGTYPE_scale(PIMGTYPE pImgType, int width, int height, bool bilinear)
{
	// TODO: Maintain aspect ratio, and use nearest-neighbor
	// when scaling up from small sizes.
	const int srcWidth = cairo_image_surface_get_width(pImgType);
	const int srcHeight = cairo_image_surface_get_height(pImgType);
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
}
#endif /* RP_GTK_USE_CAIRO */

#ifdef RP_GTK_USE_CAIRO
typedef struct _PIMGTYPE_CairoReadFunc_State_t {
	const uint8_t *buf;
	size_t size;
	size_t pos;
} PIMGTYPE_CairoReadFunc_State_t;

/**
 * cairo_read_func_t for cairo_image_surface_create_from_png_stream()
 * @param closure	[in] PIMGTYPE_CairoReadFunc_State_t
 * @param data		[out] Data buffer.
 * @param length	[in] Data to read.
 * @return CAIRO_STATUS_SUCCESS on success; CAIRO_STATUS_READ_ERROR on error.
 */
static cairo_status_t PIMGTYPE_CairoReadFunc(void *closure, unsigned char *data, unsigned int length)
{
	PIMGTYPE_CairoReadFunc_State_t *const d =
		static_cast<PIMGTYPE_CairoReadFunc_State_t*>(closure);
	if (d->pos + length > d->size) {
		// Out of bounds.
		return CAIRO_STATUS_READ_ERROR;
	}
	memcpy(data, &d->buf[d->pos], length);
	d->pos += length;
	return CAIRO_STATUS_SUCCESS;
}
#endif /* RP_GTK_USE_CAIRO */

/**
 * Load a PNG image from our glibresources.
 * @param filename Filename within glibresources.
 * @return PIMGTYPE, or nullptr if not found.
 */
PIMGTYPE PIMGTYPE_load_png_from_gresource(const char *filename)
{
#ifdef RP_GTK_USE_CAIRO
	GBytes *const pData = g_resource_lookup_data(_get_resource(), filename,
		G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
	if (!pData) {
		// Not found.
		return nullptr;
	}

	PIMGTYPE_CairoReadFunc_State_t state;
	state.buf = static_cast<const uint8_t*>(g_bytes_get_data(pData, &state.size));
	state.pos = 0;

	PIMGTYPE surface = cairo_image_surface_create_from_png_stream(
		PIMGTYPE_CairoReadFunc, &state);
	if (surface && cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		surface = nullptr;
	}

	g_bytes_unref(pData);
	return surface;
#else /* !RP_GTK_USE_CAIRO */
	// FIXME: GdkPixbuf version.
	//PIMGTYPE flags_16x16 = gdk_pixbuf_new_from_file(filename, nullptr);
	return nullptr;
#endif /* RP_GTK_USE_CAIRO */
}

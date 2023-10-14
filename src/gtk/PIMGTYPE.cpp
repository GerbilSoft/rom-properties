/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PIMGTYPE.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;
using LibRpFile::MemFile;
using LibRpTexture::rp_image_ptr;

// C++ STL classes
using std::shared_ptr;

// glib resources
// NOTE: glib-compile-resources doesn't have extern "C".
G_BEGIN_DECLS
#include "glibresources.h"
G_END_DECLS

#if defined(RP_GTK_USE_GDKTEXTURE) || defined(RP_GTK_USE_CAIRO)
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
#if defined(RP_GTK_USE_GDKTEXTURE)
#  warning NOT SUPPORTED for GdkTexture - check for better scaling method
	RP_UNUSED(width);
	RP_UNUSED(height);
	RP_UNUSED(bilinear);
	return (PIMGTYPE)g_object_ref(pImgType);
#elif defined(RP_GTK_USE_CAIRO)
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

	cairo_pattern_set_filter(cairo_get_source(cr),
		(bilinear ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST));
	cairo_scale(cr, (double)width / (double)srcWidth, (double)height / (double)srcHeight);
	cairo_set_source_surface(cr, pImgType, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	return surface;
#else
#  error Invalid condition
#endif
}
#endif /* RP_GTK_USE_GDKTEXTURE || RP_GTK_USE_CAIRO */

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
#else /* !RP_GTK_USE_CAIRO */
// Mapping of data pointers to GBytes* objects for unreference.
static std::unordered_map<const void*, GBytes*> map_gbytes_unref;

/**
 * GDestroyNotify for g_memory_input_stream_new_from_data().
 * @param data Data pointer from GBytes.
 */
static void gbytes_destroy_notify(gpointer data)
{
	auto iter = map_gbytes_unref.find(const_cast<const void*>(data));
	if (iter != map_gbytes_unref.end()) {
		GBytes *const pBytes = iter->second;
		map_gbytes_unref.erase(iter);
		g_bytes_unref(pBytes);
	}
}
#endif /* RP_GTK_USE_CAIRO */

/**
 * Load a PNG image from our glibresources.
 * This version returns PIMGTYPE.
 * @param filename Filename within glibresources.
 * @return PIMGTYPE, or nullptr if not found.
 */
PIMGTYPE PIMGTYPE_load_png_from_gresource(const char *filename)
{
	GBytes *const pBytes = g_resource_lookup_data(_get_resource(), filename,
		G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
	if (!pBytes) {
		// Not found.
		return nullptr;
	}

#if defined(RP_GTK_USE_GDKTEXTURE)
	// NOTE: GdkTexture does have return gdk_texture_new_from_resource(),
	// but it isn't working with our internal resources.
	PIMGTYPE texture = gdk_texture_new_from_bytes(pBytes, nullptr);
	g_bytes_unref(pBytes);
	return texture;
#elif defined(RP_GTK_USE_CAIRO)
	PIMGTYPE_CairoReadFunc_State_t state;
	state.buf = static_cast<const uint8_t*>(g_bytes_get_data(pBytes, &state.size));
	state.pos = 0;

	PIMGTYPE surface = cairo_image_surface_create_from_png_stream(
		PIMGTYPE_CairoReadFunc, &state);
	if (surface && cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		surface = nullptr;
	}

	g_bytes_unref(pBytes);
	return surface;
#else /* GdkPixbuf */
	// glib-2.34.0 has g_memory_input_stream_new_from_bytes().
	// We'll use g_memory_input_stream_new_from_data() for compatibility.
	gsize size;
	const void *pData = g_bytes_get_data(pBytes, &size);
	map_gbytes_unref.emplace(pData, pBytes); // FIXME: Memory leak on GTK2/GTK4?
	GInputStream *const stream = g_memory_input_stream_new_from_data(
		pData, size, gbytes_destroy_notify);
	PIMGTYPE pixbuf = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
	g_object_unref(stream);
	return pixbuf;
#endif /* RP_GTK_USE_CAIRO */
}

/**
 * Load a PNG image from our glibresources.
 * This version returns rp_image.
 * @param filename Filename within glibresources.
 * @return rp_imgae_ptr, or nullptr if not found.
 */
rp_image_ptr rp_image_load_png_from_gresource(const char *filename)
{
	GBytes *const pBytes = g_resource_lookup_data(_get_resource(), filename,
		G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr);
	if (!pBytes) {
		// Not found.
		return nullptr;
	}

	gsize size = 0;
	gconstpointer data = g_bytes_get_data(pBytes, &size);
	shared_ptr<IRpFile> memFile = std::make_shared<MemFile>(data, size);

	return RpPng::load(memFile);
}

/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * PIMGTYPE.hpp: PIMGTYPE typedef and wrapper functions.                   *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "PIMGTYPE.hpp"

// librpbase, librpfile, librptexture
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFilePtr;
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
 * Internal Cairo surface scaling function.
 * @param src_surface Source surface
 * @param width New width
 * @param height New height
 * @param bilinear If true, use bilinear interpolation.
 * @return Scaled surface, or nullptr on error.
 */
static cairo_surface_t *rp_cairo_scale_int(cairo_surface_t *src_surface, int width, int height, bool bilinear)
{
	cairo_surface_t *const dest_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	assert(dest_surface != nullptr);
	assert(cairo_surface_status(dest_surface) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!dest_surface)) {
		return nullptr;
	} else if (cairo_surface_status(dest_surface) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(dest_surface);
		return nullptr;
	}

	cairo_t *const cr = cairo_create(dest_surface);
	assert(cr != nullptr);
	assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
	if (unlikely(!cr)) {
		cairo_surface_destroy(dest_surface);
		return nullptr;
	} else if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(cr);
		cairo_surface_destroy(dest_surface);
		return nullptr;
	}

	cairo_pattern_set_filter(cairo_get_source(cr),
		(bilinear ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST));
	cairo_scale(cr, (double)width / (double)cairo_image_surface_get_width(src_surface),
		(double)height / (double)cairo_image_surface_get_height(src_surface));
	cairo_set_source_surface(cr, src_surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	return dest_surface;
}

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
	const int srcWidth = gdk_texture_get_width(pImgType);
	const int srcHeight = gdk_texture_get_height(pImgType);
	assert(srcWidth > 0 && srcHeight > 0);
	if (unlikely(srcWidth <= 0 || srcHeight <= 0)) {
		return PIMGTYPE_ref(pImgType);
	}

	// Use Cairo to scale the GdkTexture.
	// Reference: https://docs.gtk.org/gdk4/method.Texture.download.html
	cairo_surface_t *const src_surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32, srcWidth, srcHeight);
	gdk_texture_download(pImgType, cairo_image_surface_get_data(src_surface),
		cairo_image_surface_get_stride(src_surface));
	cairo_surface_mark_dirty(src_surface);

	cairo_surface_t *const dest_surface = rp_cairo_scale_int(src_surface, width, height, bilinear);
	cairo_surface_destroy(src_surface);
	if (unlikely(!dest_surface)) {
		return PIMGTYPE_ref(pImgType);
	}

	// Create a GdkMemoryTexture using the cairo_surface_t image data directly.
	// NOTE: GdkMemoryTexture only does a g_bytes_ref() if the stride matches
	// what it expects, so we need to do a deep copy here.
	const int dest_stride = cairo_image_surface_get_stride(dest_surface);
	GBytes *const pBytes = g_bytes_new(cairo_image_surface_get_data(dest_surface),
		static_cast<gsize>(height) * static_cast<gsize>(dest_stride));
	// FIXME: GDK_MEMORY_DEFAULT (GDK_MEMORY_B8G8R8A8_PREMULTIPLIED) causes a heap overflow here...
	GdkTexture *const texture = gdk_memory_texture_new(width, height, GDK_MEMORY_B8G8R8A8, pBytes, dest_stride);
	g_bytes_unref(pBytes);
	cairo_surface_destroy(dest_surface);
	return texture;
#elif defined(RP_GTK_USE_CAIRO)
	// TODO: Maintain aspect ratio, and use nearest-neighbor
	// when scaling up from small sizes.
	const int srcWidth = cairo_image_surface_get_width(pImgType);
	const int srcHeight = cairo_image_surface_get_height(pImgType);
	assert(srcWidth > 0 && srcHeight > 0);
	if (unlikely(srcWidth <= 0 || srcHeight <= 0)) {
		return PIMGTYPE_ref(pImgType);
	}

	cairo_surface_t *const dest_surface = rp_cairo_scale_int(pImgType, width, height, bilinear);
	return (dest_surface) ? dest_surface : PIMGTYPE_ref(pImgType);
#else
#  error Invalid condition
#endif
}
#endif /* RP_GTK_USE_GDKTEXTURE || RP_GTK_USE_CAIRO */

#if defined(RP_GTK_USE_GDKTEXTURE)
// nothing special here
#elif defined(RP_GTK_USE_CAIRO)
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
#else /* GdkPixbuf */
// Mapping of data pointers to GBytes* objects for unreference.
static std::map<const void*, GBytes*> map_gbytes_unref;

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
#endif

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
	IRpFilePtr memFile = std::make_shared<MemFile>(data, size);

	return RpPng::load(memFile);
}

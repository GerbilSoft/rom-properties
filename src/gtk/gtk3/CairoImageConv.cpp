/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CairoImageConv.cpp: Helper functions to convert from rp_image to Cairo. *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CairoImageConv.hpp"

// C++ STL classes
using std::array;
using std::shared_ptr;

// librptexture
#include "librptexture/ImageSizeCalc.hpp"
using namespace LibRpTexture;

/**
 * Convert an rp_image to cairo_surface_t.
 * @param img		[in] rp_image.
 * @param premultiply	[in] If true, premultiply. Needed for display; NOT needed for PNG.
 * @return GdkPixbuf, or nullptr on error.
 */
cairo_surface_t *CairoImageConv::rp_image_to_cairo_surface_t(const rp_image *img, bool premultiply)
{
	assert(img != nullptr);
	if (unlikely(!img || !img->isValid()))
		return nullptr;

	// NOTE: cairo_image_surface_create_for_data() doesn't do a
	// deep copy, so we can't use it.
	// NOTE 2: cairo_image_surface_create() always returns a valid
	// pointer, but the status may be CAIRO_STATUS_NULL_POINTER if
	// it failed to create a surface. We'll still check for nullptr.
	const int width = img->width();
	const int height = img->height();
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	// cairo_image_surface_create() always returns a valid pointer.
	assert(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
	if (unlikely(cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)) {
		cairo_surface_destroy(surface);
		return nullptr;
	}

	uint32_t *px_dest = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(surface));
	assert(px_dest != nullptr);

	switch (img->format()) {
		case rp_image::Format::ARGB32: {
			shared_ptr<rp_image> img_prex;
			const rp_image *src_img;
			if (premultiply) {
				// Premultiply the image first.
				// TODO: Combined dup()/premultiply() function?
				img_prex = img->dup();
				img_prex->premultiply();
				src_img = img_prex.get();
			} else {
				// No premultiplication.
				src_img = img;
			}

			// Copy the image data.
			int dest_stride = cairo_image_surface_get_stride(surface);
			int src_stride = src_img->stride();

			if (dest_stride == src_stride) {
				// Stride is identical. Copy the whole image all at once.
				// NOTE: Partial copy for the last line.
				size_t sz = ImageSizeCalc::T_calcImageSize(dest_stride, (height - 1));
				sz += width * sizeof(uint32_t);
				memcpy(px_dest, src_img->bits(), sz);
			} else {
				// Stride is not identical. Copy each scanline.
				const uint32_t *img_buf = static_cast<const uint32_t*>(src_img->bits());
				const int row_bytes = img->row_bytes();
				// We're adding strides to pointers, so the strides
				// must be in uint32_t units here.
				dest_stride /= sizeof(uint32_t);
				src_stride /= sizeof(uint32_t);
				for (unsigned int y = (unsigned int)height; y > 0; y--) {
					memcpy(px_dest, img_buf, row_bytes);
					px_dest += dest_stride;
					img_buf += src_stride;
				}
			}

			// Mark the surface as dirty.
			cairo_surface_mark_dirty(surface);
			break;
		}

		case rp_image::Format::CI8: {
			const uint32_t *const palette = img->palette();
			const unsigned int palette_len = img->palette_len();
			assert(palette != nullptr);
			assert(palette_len > 0);
			assert(palette_len <= 256);
			if (!palette || palette_len == 0 || palette_len > 256)
				break;

			// Premultiply the palette.
			std::array<uint32_t, 256> pal_prex;
			const uint32_t *pal_toUse;
			if (premultiply) {
				for (unsigned int i = 0; i < palette_len; i++) {
					pal_prex[i] = rp_image::premultiply_pixel(palette[i]);
				}
				if (palette_len < pal_prex.size()) {
					// Clear the rest of the palette.
					memset(&pal_prex[palette_len], 0, (pal_prex.size() - palette_len) * sizeof(uint32_t));
				}
				pal_toUse = pal_prex.data();
			} else {
				pal_toUse = palette;
			}

			// Copy the image data.
			const uint8_t *img_buf = static_cast<const uint8_t*>(img->bits());
			const int dest_stride_adj = (cairo_image_surface_get_stride(surface) / sizeof(uint32_t)) - width;
			const int src_stride_adj = img->stride() - width;

			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 3; x -= 4) {
					px_dest[0] = pal_toUse[img_buf[0]];
					px_dest[1] = pal_toUse[img_buf[1]];
					px_dest[2] = pal_toUse[img_buf[2]];
					px_dest[3] = pal_toUse[img_buf[3]];
					px_dest += 4;
					img_buf += 4;
				}
				for (; x > 0; x--, px_dest++, img_buf++) {
					// Last pixels.
					*px_dest = pal_toUse[*img_buf];
					px_dest++;
					img_buf++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			// Mark the surface as dirty.
			cairo_surface_mark_dirty(surface);
			break;
		}

		default:
			// Unsupported image format.
			assert(!"Unsupported rp_image::Format.");
			cairo_surface_destroy(surface);
			surface = nullptr;
			break;
	}

	return surface;
}

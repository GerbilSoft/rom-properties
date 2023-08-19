/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkImageConv.cpp: Helper functions to convert from rp_image to GDK.     *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GdkImageConv.hpp"

// C++ STL classes
using std::array;

// librptexture
using namespace LibRpTexture;

/**
 * Convert an rp_image to GdkPixbuf.
 * Standard version using regular C++ code.
 * @param img	[in] rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf_cpp(const rp_image *img)
{
	assert(img != nullptr);
	if (unlikely(!img || !img->isValid()))
		return nullptr;

	// NOTE: GdkPixbuf's convenience functions don't do a
	// deep copy, so we can't use them directly.
	const int width = img->width();
	const int height = img->height();
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, width, height);
	assert(pixbuf != nullptr);
	if (unlikely(!pixbuf))
		return nullptr;

	argb32_t *px_dest = reinterpret_cast<argb32_t*>(gdk_pixbuf_get_pixels(pixbuf));
	const int dest_stride_adj = (gdk_pixbuf_get_rowstride(pixbuf) / sizeof(*px_dest)) - width;

	switch (img->format()) {
		case rp_image::Format::ARGB32: {
			// Copy the image data.
			const argb32_t *img_buf = static_cast<const argb32_t*>(img->bits());
			const int src_stride_adj = (img->stride() / sizeof(argb32_t)) - width;
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					// Swap the R and B channels
					px_dest[0].u32 = img_buf[0].u32;
					px_dest[1].u32 = img_buf[1].u32;
					std::swap(px_dest[0].r, px_dest[0].b);
					std::swap(px_dest[1].r, px_dest[1].b);

					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Last pixel
					px_dest->u32 = img_buf->u32;
					std::swap(px_dest->r, px_dest->b);

					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;
		}

		case rp_image::Format::CI8: {
			const argb32_t *src_pal = reinterpret_cast<const argb32_t*>(img->palette());
			const unsigned int src_pal_len = img->palette_len();
			assert(src_pal != nullptr);
			assert(src_pal_len > 0);
			if (!src_pal || src_pal_len == 0)
				break;

			// Get the palette.
			array<argb32_t, 256> palette;
			unsigned int i;
			for (i = 0; i+1 < src_pal_len; i += 2, src_pal += 2) {
				// Swap the R and B channels in the palette.
				palette[i+0].u32 = src_pal[0].u32;
				palette[i+1].u32 = src_pal[1].u32;

				std::swap(palette[i+0].r, palette[i+0].b);
				std::swap(palette[i+1].r, palette[i+1].b);
			}
			for (; i < src_pal_len; i++, src_pal++) {
				// Last color.
				palette[i].u32 = src_pal->u32;
				std::swap(palette[i].r, palette[i].b);
			}

			// Zero out the rest of the palette if the new
			// palette is larger than the old palette.
			if (src_pal_len < palette.size()) {
				memset(&palette[src_pal_len], 0, (palette.size() - src_pal_len) * sizeof(uint32_t));
			}

			// Copy the image data.
			const uint8_t *img_buf = static_cast<const uint8_t*>(img->bits());
			const int src_stride_adj = img->stride() - width;
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 3; x -= 4) {
					px_dest[0] = palette[img_buf[0]];
					px_dest[1] = palette[img_buf[1]];
					px_dest[2] = palette[img_buf[2]];
					px_dest[3] = palette[img_buf[3]];
					px_dest += 4;
					img_buf += 4;
				}
				for (; x > 0; x--, px_dest++, img_buf++) {
					// Last pixels.
					*px_dest = palette[*img_buf];
					px_dest++;
					img_buf++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;
		}

		default:
			// Unsupported image format.
			assert(!"Unsupported rp_image::Format.");
			g_clear_object(&pixbuf);
			break;
	}

	return pixbuf;
}

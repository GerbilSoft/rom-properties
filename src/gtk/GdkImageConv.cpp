/***************************************************************************
 * ROM Properties Page shell extension. (XFCE)                             *
 * GdkImageConv.cpp: Helper functions to convert from rp_image to GDK.     *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#include "GdkImageConv.hpp"

// C includes.
#include <stdint.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// librpbase
#include "librpbase/img/rp_image.hpp"
using LibRpBase::rp_image;

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
	GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	assert(pixbuf != nullptr);
	if (unlikely(!pixbuf))
		return nullptr;

	uint32_t *px_dest = reinterpret_cast<uint32_t*>(gdk_pixbuf_get_pixels(pixbuf));
	const int dest_stride_adj = (gdk_pixbuf_get_rowstride(pixbuf) / sizeof(*px_dest)) - img->width();

	switch (img->format()) {
		case rp_image::FORMAT_ARGB32: {
			// Copy the image data.
			const uint32_t *img_buf = static_cast<const uint32_t*>(img->bits());
			const int src_stride_adj = (img->stride() / sizeof(uint32_t)) - width;
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					// Swap the R and B channels.
					px_dest[0] = (img_buf[0] & 0xFF00FF00) |
						    ((img_buf[0] & 0x00FF0000) >> 16) |
						    ((img_buf[0] & 0x000000FF) << 16);
					px_dest[1] = (img_buf[1] & 0xFF00FF00) |
						    ((img_buf[1] & 0x00FF0000) >> 16) |
						    ((img_buf[1] & 0x000000FF) << 16);
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Last pixel.
					*px_dest = (*img_buf & 0xFF00FF00) |
						  ((*img_buf & 0x00FF0000) >> 16) |
						  ((*img_buf & 0x000000FF) << 16);
					img_buf++;
					px_dest++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;
		}

		case rp_image::FORMAT_CI8: {
			const uint32_t *src_pal = img->palette();
			const int src_pal_len = img->palette_len();
			assert(src_pal != nullptr);
			assert(src_pal_len > 0);
			if (!src_pal || src_pal_len <= 0)
				break;

			// Get the palette.
			uint32_t palette[256];
			int i;
			for (i = 0; i < src_pal_len; i += 2, src_pal += 2) {
				// Swap the R and B channels in the palette.
				palette[i+0] = (src_pal[0] & 0xFF00FF00) |
					      ((src_pal[0] & 0x00FF0000) >> 16) |
					      ((src_pal[0] & 0x000000FF) << 16);
				palette[i+1] = (src_pal[1] & 0xFF00FF00) |
					      ((src_pal[1] & 0x00FF0000) >> 16) |
					      ((src_pal[1] & 0x000000FF) << 16);
			}
			for (; i < src_pal_len; i++, src_pal++) {
				// Last color.
				palette[i] = (*src_pal & 0xFF00FF00) |
					    ((*src_pal & 0x00FF0000) >> 16) |
					    ((*src_pal & 0x000000FF) << 16);
			}

			// Zero out the rest of the palette if the new
			// palette is larger than the old palette.
			if (src_pal_len < ARRAY_SIZE(palette)) {
				memset(&palette[src_pal_len], 0, (ARRAY_SIZE(palette) - src_pal_len) * sizeof(uint32_t));
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
			g_object_unref(pixbuf);
			pixbuf = nullptr;
			break;
	}

	return pixbuf;
}

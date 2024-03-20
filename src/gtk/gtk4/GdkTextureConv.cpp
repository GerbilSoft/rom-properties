/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkTextureConv.cpp: Helper functions to convert from rp_image to GDK4.  *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GdkTextureConv.hpp"

// C++ STL classes
using std::array;

// librptexture
using namespace LibRpTexture;

/**
 * Convert an rp_image to GdkPixbuf.
 * @param img	[in] rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
GdkTexture *GdkTextureConv::rp_image_to_GdkTexture(const rp_image *img)
{
	assert(img != nullptr);
	if (unlikely(!img || !img->isValid()))
		return nullptr;

	// NOTE: GdkTexture expects to be given the original data when
	// creating the texture, so we can't create the texture up here.
	const int width = img->width();
	const int height = img->height();
	assert(width > 0);
	assert(height > 0);
	if (width <= 0 || height <= 0)
		return nullptr;

	GdkTexture *texture = nullptr;
	switch (img->format()) {
		case rp_image::Format::ARGB32: {
			// Create a GdkMemoryTexture using the original image data directly.
			// NOTE: The data here technically isn't static, but we don't want to do *two* copies.
			const int stride = img->stride();
			GBytes *const pBytes = g_bytes_new_static(img->bits(), img->data_len());
			assert(pBytes != nullptr);
			if (pBytes) {
				// gdk_memory_texture_new() creates a copy of pBytes.
				// TODO: Verify format on big-endian.
				texture = gdk_memory_texture_new(width, height, GDK_MEMORY_B8G8R8A8, pBytes, stride);
				g_bytes_unref(pBytes);
			}
			break;
		}

		case rp_image::Format::CI8: {
			// Convert from CI8 to ARGB32.
			// TODO: Needs testing.
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
				palette[i+0].u32 = src_pal[0].u32;
				palette[i+1].u32 = src_pal[1].u32;
			}
			for (; i < src_pal_len; i++, src_pal++) {
				// Last color.
				palette[i].u32 = src_pal->u32;
			}

			// Zero out the rest of the palette if the new
			// palette is larger than the old palette.
			if (src_pal_len < palette.size()) {
				memset(&palette[src_pal_len], 0, (palette.size() - src_pal_len) * sizeof(uint32_t));
			}

			// Destination image buffer
			const size_t data_len = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(argb32_t);
			argb32_t *const dest_buf = static_cast<argb32_t*>(malloc(data_len));
			argb32_t *px_dest = dest_buf;

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
			}

			// NOTE: GdkMemoryTexture only does a g_bytes_ref() if the stride matches
			// what it expects, so we need to do a deep copy here.
			GBytes *const pBytes = g_bytes_new(dest_buf, data_len);
			assert(pBytes != nullptr);
			if (pBytes) {
				// TODO: Verify format on big-endian.
				texture = gdk_memory_texture_new(width, height, GDK_MEMORY_B8G8R8A8, pBytes, width * sizeof(argb32_t));
				g_bytes_unref(pBytes);
			}
			free(dest_buf);
			break;
		}

		default:
			// Unsupported image format.
			assert(!"Unsupported rp_image::Format.");
			g_clear_object(&texture);
			break;
	}

	return texture;
}

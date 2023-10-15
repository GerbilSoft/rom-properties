/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * ISpriteSheet.cpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ISpriteSheet.hpp"

// librptexture
using LibRpTexture::argb32_t;
using LibRpTexture::rp_image;
using LibRpTexture::rp_image_ptr;

/**
 * Sprite sheet loader
 * @param cols Number of columns
 * @param rows Number of rows
 * @param width Icon width
 * @param height Icon height
 */
ISpriteSheet::ISpriteSheet(int cols, int rows, int width, int height)
	: m_cols(cols)
	, m_rows(rows)
	, m_width(width)
	, m_height(height)
{ }

/**
 * Get an icon from the sprite sheet.
 * @param col Column
 * @param row Row
 * @param gray If true, load the grayscale version
 * @return Icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE ISpriteSheet::getIcon(int col, int row, bool gray) const
{
	assert(col >= 0);
	assert(col < m_cols);
	assert(row >= 0);
	assert(row <= m_rows);
	if (col < 0 || col >= m_cols || row < 0 || row >= m_rows) {
		// Invalid col/row.
		return nullptr;
	}

	// Do we need to load the sprite sheet?
	rp_image_ptr &imgSpriteSheet = gray
		? const_cast<ISpriteSheet*>(this)->m_imgGray
		: const_cast<ISpriteSheet*>(this)->m_img;
	if (!imgSpriteSheet) {
		// Load the sprite sheet.
		char gres_filename[64];
		int ret = getFilename(gres_filename, sizeof(gres_filename), m_width, m_height, gray);
		if (ret != 0) {
			// Unable to get the filename.
			return nullptr;
		}

		imgSpriteSheet = rp_image_load_png_from_gresource(gres_filename);
		assert(imgSpriteSheet != nullptr);
		if (!imgSpriteSheet) {
			// Unable to load the sprite sheet.
			return nullptr;
		}

		// Needs to be ARGB32.
		switch (imgSpriteSheet->format()) {
			case rp_image::Format::CI8:
				imgSpriteSheet = imgSpriteSheet->dup_ARGB32();
				break;
			case rp_image::Format::ARGB32:
				break;
			default:
				assert(!"Invalid rp_image format");
				imgSpriteSheet.reset();
				return nullptr;
		}

#ifdef RP_GTK_USE_CAIRO
		// Cairo needs premultiplied alpha.
		imgSpriteSheet->premultiply();
#endif /* RP_GTK_USE_CAIRO */

		// Make sure the bitmap has the expected size.
		assert(imgSpriteSheet->width()  == (int)(m_width * m_cols));
		assert(imgSpriteSheet->height() == (int)(m_height * m_rows));
		if (imgSpriteSheet->width()  != (int)(m_width * m_cols) ||
		    imgSpriteSheet->height() != (int)(m_height * m_rows))
		{
			// Incorrect size. We can't use it.
			imgSpriteSheet.reset();
			return nullptr;
		}
	}

	// Extract the sub-icon.
	// NOTE: GTK4's GdkTexture doesn't have any direct access functions, so we'll
	// create a new PIMGTYPE using an offset into the rp_image.
	int src_stride = imgSpriteSheet->stride();
	const int yoffset = (row * m_height * src_stride);
	const int xoffset = (col * m_width * sizeof(uint32_t));

	const uint32_t *pSrcBits = reinterpret_cast<const uint32_t*>(
		static_cast<const uint8_t*>(imgSpriteSheet->bits()) + yoffset + xoffset);
	PIMGTYPE subIcon = nullptr;
#if defined(RP_GTK_USE_GDKTEXTURE)
	const int data_len = ((m_height - 1) * src_stride) + (m_height * sizeof(uint32_t));
	GBytes *const pBytes = g_bytes_new(pSrcBits, data_len);
	if (pBytes) {
		subIcon = gdk_memory_texture_new(m_width, m_height, GDK_MEMORY_B8G8R8A8, pBytes, src_stride);
		g_bytes_unref(pBytes);
	}
#elif defined(RP_GTK_USE_CAIRO)
	subIcon = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_width, m_height);
	// cairo_image_surface_create() always returns a valid pointer.
	assert(cairo_surface_status(subIcon) == CAIRO_STATUS_SUCCESS);
	if (unlikely(cairo_surface_status(subIcon) != CAIRO_STATUS_SUCCESS)) {
		cairo_surface_destroy(subIcon);
		return nullptr;
	}

	uint32_t *px_dest = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(subIcon));
	assert(px_dest != nullptr);

	const uint32_t *img_buf = pSrcBits;
	const int row_bytes = m_width * sizeof(uint32_t);

	// Copy the image data.
	int dest_stride = cairo_image_surface_get_stride(subIcon);
	// We're adding strides to pointers, so the strides
	// must be in uint32_t units here.
	dest_stride /= sizeof(uint32_t);
	src_stride /= sizeof(uint32_t);
	for (unsigned int y = (unsigned int)m_height; y > 0; y--) {
		memcpy(px_dest, img_buf, row_bytes);
		img_buf += src_stride;
		px_dest += dest_stride;
	}

	cairo_surface_mark_dirty(subIcon);
#else
	subIcon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, m_width, m_height);
	// cairo_image_surface_create() always returns a valid pointer.
	assert(subIcon != nullptr);
	if (unlikely(!subIcon)) {
		return nullptr;
	}

	argb32_t *px_dest = reinterpret_cast<argb32_t*>(gdk_pixbuf_get_pixels(subIcon));
	assert(px_dest != nullptr);

	const uint32_t *img_buf = pSrcBits;

	// Copy the image data.
	// NOTE: GdkPixbuf has swapped R and B channels, so we have to swap them here.
	const int dest_stride_adj = (gdk_pixbuf_get_rowstride(subIcon) / sizeof(*px_dest)) - m_width;

	// We're adding strides to pointers, so the strides
	// must be in uint32_t units here.
	const int src_stride_adj = (src_stride / sizeof(argb32_t)) - m_width;
	for (unsigned int y = (unsigned int)m_height; y > 0; y--) {
		unsigned int x;
		for (x = (unsigned int)m_width; x > 1; x -= 2) {
			// Swap the R and B channels
			px_dest[0].u32 = img_buf[0];
			px_dest[1].u32 = img_buf[1];
			std::swap(px_dest[0].r, px_dest[0].b);
			std::swap(px_dest[1].r, px_dest[1].b);

			img_buf += 2;
			px_dest += 2;
		}
		if (x == 1) {
			// Last pixel
			px_dest->u32 = *img_buf;
			std::swap(px_dest->r, px_dest->b);

			img_buf++;
			px_dest++;
		}

		// Next line.
		img_buf += src_stride_adj;
		px_dest += dest_stride_adj;
	}
#endif

	return subIcon;
}

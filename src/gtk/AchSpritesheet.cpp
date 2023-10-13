/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * AchSpritesheet.hpp: Achievement spritesheets loader.                    *
 *                                                                         *
 * Copyright (c) 2020-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "AchSpritesheet.hpp"

// librpbase, librptexture
using LibRpBase::Achievements;
using LibRpTexture::argb32_t;
using LibRpTexture::rp_image;
using LibRpTexture::rp_image_ptr;

/**
 * Achievements spritesheet
 * @param iconSize Icon size
 */
AchSpritesheet::AchSpritesheet(int iconSize)
	: m_iconSize(iconSize)
{
	assert(iconSize == 16 || iconSize == 24 || iconSize == 32 || iconSize == 64);
}

/**
 * Get an Achievements icon.
 * @param id Achievement ID
 * @param gray If true, load the grayscale version
 * @return Achievements icon, or nullptr on error. (caller must free the icon)
 */
PIMGTYPE AchSpritesheet::getIcon(Achievements::ID id, bool gray)
{
	assert((int)id >= 0);
	assert(id < Achievements::ID::Max);
	if ((int)id < 0 || id >= Achievements::ID::Max) {
		// Invalid achievement ID.
		return nullptr;
	}

	// Do we need to load the sprite sheet?
	rp_image_ptr &imgAchSheet = gray ? m_imgGray : m_img;
	if (!imgAchSheet) {
		// Load the sprite sheet.
		char ach_filename[64];
		snprintf(ach_filename, sizeof(ach_filename),
			"/com/gerbilsoft/rom-properties/ach/ach%s-%dx%d.png",
			(gray ? "-gray" : ""), m_iconSize, m_iconSize);
		imgAchSheet = rp_image_load_png_from_gresource(ach_filename);
		assert(imgAchSheet != nullptr);
		if (!imgAchSheet) {
			// Unable to load the achievements sprite sheet.
			return nullptr;
		}

		// Needs to be ARGB32.
		switch (imgAchSheet->format()) {
			case rp_image::Format::CI8:
				imgAchSheet = imgAchSheet->dup_ARGB32();
				break;
			case rp_image::Format::ARGB32:
				break;
			default:
				assert(!"Invalid rp_image format");
				imgAchSheet.reset();
				return nullptr;
		}

#ifdef RP_GTK_USE_CAIRO
		// Cairo needs premultiplied alpha.
		imgAchSheet->premultiply();
#endif /* RP_GTK_USE_CAIRO */

		// Make sure the bitmap has the expected size.
		assert(imgAchSheet->width()  == (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_COLS));
		assert(imgAchSheet->height() == (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_ROWS));
		if (imgAchSheet->width()  != (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_COLS) ||
		    imgAchSheet->height() != (int)(m_iconSize * Achievements::ACH_SPRITE_SHEET_ROWS))
		{
			// Incorrect size. We can't use it.
			imgAchSheet.reset();
			return nullptr;
		}
	}

	// Determine row and column.
	const int col = ((int)id % Achievements::ACH_SPRITE_SHEET_COLS);
	const int row = ((int)id / Achievements::ACH_SPRITE_SHEET_COLS);

	// Extract the sub-icon.
	// NOTE: GTK4's GdkTexture doesn't have any direct access functions, so we'll
	// create a new PIMGTYPE using an offset into the rp_image.
	int src_stride = imgAchSheet->stride();
	const int yoffset = (row * m_iconSize * src_stride);
	const int xoffset = (col * m_iconSize) * sizeof(uint32_t);

	const uint32_t *pSrcBits = reinterpret_cast<const uint32_t*>(
		static_cast<const uint8_t*>(imgAchSheet->bits()) + yoffset + xoffset);
	PIMGTYPE subIcon = nullptr;
#if defined(RP_GTK_USE_GDKTEXTURE)
	const int data_len = ((m_iconSize - 1) * src_stride) + (m_iconSize * sizeof(uint32_t));
	GBytes *const pBytes = g_bytes_new(pSrcBits, data_len);
	if (pBytes) {
		subIcon = gdk_memory_texture_new(m_iconSize, m_iconSize, GDK_MEMORY_B8G8R8A8, pBytes, src_stride);
		g_bytes_unref(pBytes);
	}
#elif defined(RP_GTK_USE_CAIRO)
	subIcon = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_iconSize, m_iconSize);
	// cairo_image_surface_create() always returns a valid pointer.
	assert(cairo_surface_status(subIcon) == CAIRO_STATUS_SUCCESS);
	if (unlikely(cairo_surface_status(subIcon) != CAIRO_STATUS_SUCCESS)) {
		cairo_surface_destroy(subIcon);
		return nullptr;
	}

	uint32_t *px_dest = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(subIcon));
	assert(px_dest != nullptr);

	const uint32_t *img_buf = pSrcBits;
	const int row_bytes = m_iconSize * sizeof(uint32_t);

	// Copy the image data.
	int dest_stride = cairo_image_surface_get_stride(subIcon);
	// We're adding strides to pointers, so the strides
	// must be in uint32_t units here.
	dest_stride /= sizeof(uint32_t);
	src_stride /= sizeof(uint32_t);
	for (unsigned int y = (unsigned int)m_iconSize; y > 0; y--) {
		memcpy(px_dest, img_buf, row_bytes);
		img_buf += src_stride;
		px_dest += dest_stride;
	}

	cairo_surface_mark_dirty(subIcon);
#else
	subIcon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, m_iconSize, m_iconSize);
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
	const int width = m_iconSize;
	const int dest_stride_adj = (gdk_pixbuf_get_rowstride(subIcon) / sizeof(*px_dest)) - width;

	// We're adding strides to pointers, so the strides
	// must be in uint32_t units here.
	const int src_stride_adj = (src_stride / sizeof(argb32_t)) - width;
	for (unsigned int y = (unsigned int)m_iconSize; y > 0; y--) {
		unsigned int x;
		for (x = (unsigned int)width; x > 1; x -= 2) {
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
//		px_dest += dest_stride;
//		img_buf += src_stride;
	}
#endif

	return subIcon;
}

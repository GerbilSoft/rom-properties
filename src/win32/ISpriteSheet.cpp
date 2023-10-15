/***************************************************************************
 * ROM Properties Page shell extension. (GTK+)                             *
 * ISpriteSheet.cpp: Generic sprite sheets loader.                         *
 *                                                                         *
 * Copyright (c) 2020-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "ISpriteSheet.hpp"

#include "RpImageWin32.hpp"
#include "res/resource.h"	// for RT_PNG

// Other rom-properties libraries
#include "librpbase/img/RpPng.hpp"
using namespace LibRpBase;
using LibRpTexture::argb32_t;
using LibRpTexture::rp_image;
using LibRpTexture::rp_image_ptr;

// RpFile_windres
#include "file/RpFile_windres.hpp"

// C++ STL classes
using std::shared_ptr;

/**
 * Sprite sheet loader
 * @param cols Number of columns
 * @param rows Number of rows
 * @param width Icon width
 * @param height Icon height
 * @param flipH If true, flip horizontally for RTL.
 */
ISpriteSheet::ISpriteSheet(int cols, int rows, int width, int height, bool flipH)
	: m_cols(cols)
	, m_rows(rows)
	, m_width(width)
	, m_height(height)
	, m_flipH(flipH)
{ }

/**
 * Get an icon from the sprite sheet.
 * @param col Column
 * @param row Row
 * @param gray If true, load the grayscale version
 * @param dpi DPI value to set in the HBITMAP
 * @return Icon, or nullptr on error. (caller must free the icon)
 */
HBITMAP ISpriteSheet::getIcon(int col, int row, bool gray, UINT dpi) const
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
		LPCTSTR resourceID = getResourceID(m_width, m_height, gray);
		if (!resourceID) {
			// Unable to get the resource ID.
			return nullptr;
		}

		shared_ptr<RpFile_windres> f_res = std::make_shared<RpFile_windres>(
			HINST_THISCOMPONENT, resourceID, MAKEINTRESOURCE(RT_PNG));
		assert(f_res->isOpen());
		if (!f_res->isOpen()) {
			// Unable to open the resource.
			return nullptr;
		}

		imgSpriteSheet = RpPng::load(f_res);
		assert((bool)imgSpriteSheet);
		if (!imgSpriteSheet) {
			// Unable to load the resource as a PNG image.
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

		// If flipH is specified, flip the image horizontally.
		if (m_flipH) {
			const rp_image_ptr flipimg = imgSpriteSheet->flip(rp_image::FLIP_H);
			assert((bool)flipimg);
			if (flipimg) {
				imgSpriteSheet = flipimg;
			}
		}
	}

	if (m_flipH) {
		// Sprite sheet is flipped for RTL.
		col = (m_cols - 1 - col);
	}

	// Extract the sub-icon.
	return RpImageWin32::getSubBitmap(imgSpriteSheet, col*m_width, row*m_height, m_width, m_height, dpi);
}

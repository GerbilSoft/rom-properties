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
{}

/**
 * Extract an HBITMAP sprite from an rp_image sprite sheet.
 * Caller must delete the HBITMAP after use.
 * @param imgSpriteSheet	[in] rp_image sprite sheet
 * @param x			[in] X pos
 * @param y			[in] Y pos
 * @param width			[in] Width
 * @param height		[in] Height
 * @param dpi			[in,opt] DPI value.
 * @return Sub-bitmap, or nullptr on error.
 */
HBITMAP ISpriteSheet::getSubBitmap(const rp_image *imgSpriteSheet, int x, int y, int width, int height, UINT dpi)
{
	// TODO: CI8?
	assert(imgSpriteSheet->format() == rp_image::Format::ARGB32);
	if (imgSpriteSheet->format() != rp_image::Format::ARGB32)
		return nullptr;

	assert(x + width <= imgSpriteSheet->width());
	assert(y + height <= imgSpriteSheet->height());
	if (x + width > imgSpriteSheet->width() || y + height > imgSpriteSheet->height())
		return nullptr;

	const BITMAPINFOHEADER bmihDIBSection = {
		sizeof(BITMAPINFOHEADER),	// biSize
		width,		// biWidth
		-height,	// biHeight (negative for right-side up)
		1,		// biPlanes
		32,		// biBitCount
		BI_RGB,		// biCompression
		0,		// biSizeImage
		(LONG)dpi,	// biXPelsPerMeter
		(LONG)dpi,	// biYPelsPerMeter
		0,		// biClrUsed
		0,		// biClrImportant
	};

	// Create a DIB section for the sub-icon.
	void *pvBits;
	HDC hDC = GetDC(nullptr);
	HBITMAP hbmIcon = CreateDIBSection(
		hDC,		// hdc
		reinterpret_cast<const BITMAPINFO*>(&bmihDIBSection),	// pbmi
		DIB_RGB_COLORS,	// usage
		&pvBits,	// ppvBits
		nullptr,	// hSection
		0);		// offset

	GdiFlush();	// TODO: Not sure if needed here...
	assert(hbmIcon != nullptr);
	if (!hbmIcon) {
		ReleaseDC(nullptr, hDC);
		return nullptr;
	}

	// Copy the icon from the sprite sheet.
	const size_t rowBytes = width * sizeof(uint32_t);
	const int srcStride = imgSpriteSheet->stride() / sizeof(uint32_t);
	const uint32_t *pSrc = static_cast<const uint32_t*>(imgSpriteSheet->scanLine(y));
	pSrc += x;
	uint32_t *pDest = static_cast<uint32_t*>(pvBits);
	for (UINT bmRow = height; bmRow > 0; bmRow--) {
		memcpy(pDest, pSrc, rowBytes);
		pDest += width;
		pSrc += srcStride;
	}

	ReleaseDC(nullptr, hDC);
	return hbmIcon;
}

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
	return getSubBitmap(imgSpriteSheet.get(), col*m_width, row*m_height, m_width, m_height, dpi);
}

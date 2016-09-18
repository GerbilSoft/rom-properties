/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpImageWin32.cpp: rp_image to Win32 conversion functions.               *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "stdafx.h"
#include "RpImageWin32.hpp"

#include "libromdata/img/rp_image.hpp"
using LibRomData::rp_image;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

// Gdiplus for HBITMAP conversion.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>
#include "libromdata/img/GdiplusHelper.hpp"

/**
 * Convert an rp_image to a HBITMAP for use as an icon mask.
 * @param image rp_image.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpImageWin32::toHBITMAP_mask(const LibRomData::rp_image *image)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid())
		return nullptr;

	// References:
	// - http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
	// - http://stackoverflow.com/a/2901465

	/**
	 * Create a monochrome bitmap twice as tall as the image.
	 * Top half is the AND mask; bottom half is the XOR mask.
	 *
	 * Icon truth table:
	 * AND=0, XOR=0: Black
	 * AND=0, XOR=1: White
	 * AND=1, XOR=0: Screen (transparent)
	 * AND=1, XOR=1: Reverse screen (inverted)
	 *
	 * References:
	 * - https://msdn.microsoft.com/en-us/library/windows/desktop/ms648059%28v=vs.85%29.aspx
	 * - https://msdn.microsoft.com/en-us/library/windows/desktop/ms648052%28v=vs.85%29.aspx
	 */
	const int width8 = (image->width() + 8) & ~8;	// Round up to 8px width.
	const int icon_sz = width8 * image->height() / 8;

	BITMAPINFO bmi;
	BITMAPINFOHEADER *bmiHeader = &bmi.bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376%28v=vs.85%29.aspx
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width8;
	bmiHeader->biHeight = image->height();	// FIXME: Top-down isn't working for monochrome...
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 1;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = 0;	// TODO?
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	bmiHeader->biClrUsed = 0;
	bmiHeader->biClrImportant = 0;

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, reinterpret_cast<BITMAPINFO*>(&bmi),
		DIB_RGB_COLORS, reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap)
		return nullptr;

	// NOTE: Windows doesn't support top-down for monochrome icons,
	// so this is vertically flipped.

	// XOR mask: All 0 to disable inversion.
	memset(&pvBits[icon_sz], 0, icon_sz);

	// AND mask: Parse the original image.
	switch (image->format()) {
		case rp_image::FORMAT_CI8: {
			// Get the transparent color index.
			int tr_idx = image->tr_idx();
			if (tr_idx < 0) {
				// tr_idx isn't set.
				// FIXME: This means the image has alpha transparency,
				// so it should be converted to ARGB32.
				DeleteObject(hBitmap);
				return nullptr;
			}

			if (tr_idx >= 0) {
				// Find all pixels matching tr_idx.
				uint8_t *dest = pvBits;
				for (int y = image->height()-1; y >= 0; y--) {
					const uint8_t *src = reinterpret_cast<const uint8_t*>(image->scanLine(y));
					// FIXME: Handle images that aren't a multiple of 8px wide.
					assert(image->width() % 8 == 0);
					for (int x = image->width(); x > 0; x -= 8) {
						uint8_t pxMono = 0;
						for (int px = 8; px > 0; px--, src++) {
							// MSB == left-most pixel.
							pxMono <<= 1;
							pxMono |= (*src != tr_idx);
						}
						*dest++ = pxMono;
					}
				}
			} else {
				// No transparent color.
				memset(pvBits, 0xFF, icon_sz);
			}
			break;
		}

		case rp_image::FORMAT_ARGB32: {
			// Find all pixels with a 0 alpha channel.
			// FIXME: Needs testing.
			uint8_t *dest = pvBits;
			for (int y = image->height()-1; y >= 0; y--) {
				const uint32_t *src = reinterpret_cast<const uint32_t*>(image->scanLine(y));
				// FIXME: Handle images that aren't a multiple of 8px wide.
				assert(image->width() % 8 == 0);
				for (int x = image->width(); x > 0; x -= 8) {
					uint8_t pxMono = 0;
					for (int px = 8; px > 0; px--, src++) {
						// MSB == left-most pixel.
						pxMono <<= 1;
						pxMono |= ((*src & 0xFF000000) == 0);
					}
					*dest++ = pxMono;
				}
			}
		}

		default:
			// Unsupported format.
			assert(false);
			break;
	}

	// Return the bitmap.
	return hBitmap;
}

/**
 * Convert an rp_image to HBITMAP. (CI8)
 * @param image rp_image. (Must be CI8.)
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpImageWin32::toHBITMAP_CI8(const rp_image *image)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid())
		return nullptr;
	assert(image->format() == rp_image::FORMAT_CI8);
	if (image->format() != rp_image::FORMAT_CI8)
		return nullptr;

	// References:
	// - http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
	// - http://stackoverflow.com/a/2901465

	// BITMAPINFO with 256-color palette.
	const size_t szBmi = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD)*256);
	BITMAPINFO *bmi = (BITMAPINFO*)malloc(szBmi);
	BITMAPINFOHEADER *bmiHeader = &bmi->bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376%28v=vs.85%29.aspx
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = image->width();
	bmiHeader->biHeight = -image->height();	// negative for top-down
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 8;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = 0;
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	bmiHeader->biClrUsed = image->palette_len();
	bmiHeader->biClrImportant = bmiHeader->biClrUsed;	// TODO?

	// Copy the palette from the image.
	memcpy(bmi->bmiColors, image->palette(), bmiHeader->biClrUsed * sizeof(RGBQUAD));

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, bmi, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap) {
		free(bmi);
		return nullptr;
	}

	// Copy the data from the rp_image into the bitmap.
	memcpy(pvBits, image->bits(), image->data_len());

	// Return the bitmap.
	free(bmi);
	return hBitmap;
}

/**
 * Convert an rp_image to HBITMAP. (ARGB32)
 * @param image rp_image. (Must be ARGB32.)
 * @param bgColor Background color for images with alpha transparency.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpImageWin32::toHBITMAP_ARGB32(const rp_image *image, COLORREF bgColor)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid())
		return nullptr;
	assert(image->format() == rp_image::FORMAT_ARGB32);
	if (image->format() != rp_image::FORMAT_ARGB32)
		return nullptr;

	// NOTE: Gdiplus requires non-const BYTE*.
	ScopedGdiplus gdip;
	unique_ptr<Gdiplus::Bitmap> gdipBmp(
		new Gdiplus::Bitmap(
			image->width(), image->height(),
			image->stride(),
			PixelFormat32bppARGB,
			(BYTE*)image->bits()));
	if (!gdipBmp)
		return nullptr;

	// Convert the image to HBITMAP.
	// TODO: Allow the caller to specify a background color?
	HBITMAP hbmpRet;
	Gdiplus::Status status = gdipBmp->GetHBITMAP(Gdiplus::Color(bgColor), &hbmpRet);
	if (status != Gdiplus::Status::Ok) {
		// Error converting to HBITMAP.
		return nullptr;
	}

	// Converted to HBITMAP.
	return hbmpRet;
}

/**
 * Convert an rp_image to HBITMAP.
 * @return image rp_image.
 * @param bgColor Background color for images with alpha transparency.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpImageWin32::toHBITMAP(const rp_image *image, COLORREF bgColor)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid())
		return nullptr;

	HBITMAP hbmpRet = nullptr;
	switch (image->format()) {
		case rp_image::FORMAT_CI8:
			// FIXME: If the palette has any alpha-transparent colors,
			// use GDI+ and convert to ARGB32.
			hbmpRet = toHBITMAP_CI8(image);
			break;
		case rp_image::FORMAT_ARGB32:
			hbmpRet = toHBITMAP_ARGB32(image, bgColor);
			break;
		default:
			assert(false);
			break;
	}

	return hbmpRet;
}

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
HICON RpImageWin32::toHICON(const rp_image *image)
{
	// NOTE: Alpha transparency doesn't seem to work in 256-color icons on Windows XP.
	assert(image != nullptr);
	if (!image || !image->isValid())
		return nullptr;

	// Convert to HBITMAP first.
	HBITMAP hBitmap = toHBITMAP(image);
	if (!hBitmap)
		return nullptr;

	// Convert the image to an icon mask.
	HBITMAP hbmMask = toHBITMAP_mask(image);
	if (!hbmMask) {
		DeleteObject(hBitmap);
		return nullptr;
	}

	// Convert to an icon.
	// Reference: http://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP&p=1661856#post1661856
	ICONINFO ii;
	ii.fIcon = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmColor = hBitmap;
	ii.hbmMask = hbmMask;

	// Create the icon.
	HICON hIcon = CreateIconIndirect(&ii);

	// Delete the original bitmaps and we're done.
	DeleteObject(hBitmap);
	DeleteObject(hbmMask);
	return hIcon;
}

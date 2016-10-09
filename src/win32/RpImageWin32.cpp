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

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
#include "libromdata/img/RpGdiplusBackend.hpp"
using namespace LibRomData;

// libcachemgr
#include "libcachemgr/CacheManager.hpp"
using LibCacheMgr::CacheManager;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
#include <vector>
using std::unique_ptr;
using std::vector;

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
 * Get an internal image.
 * NOTE: The image is owned by the RomData object;
 * caller must NOT delete it!
 *
 * @param romData RomData object.
 * @param imageType Image type.
 * @return Internal image, or nullptr on error.
 */
const rp_image *RpImageWin32::getInternalImage(const RomData *romData, RomData::ImageType imageType)
{
	assert(imageType >= RomData::IMG_INT_MIN && imageType <= RomData::IMG_INT_MAX);
	if (imageType < RomData::IMG_INT_MIN || imageType > RomData::IMG_INT_MAX) {
		// Out of range.
		return nullptr;
	}

	return romData->image(imageType);
}

/**
 * Get an external image.
 * NOTE: Caller must delete the image after use.
 *
 * @param romData RomData object.
 * @param imageType Image type.
 * @return External image, or nullptr on error.
 */
rp_image *RpImageWin32::getExternalImage(const RomData *romData, RomData::ImageType imageType)
{
	const vector<RomData::ExtURL> *extURLs = romData->extURLs(imageType);
	if (!extURLs || extURLs->empty()) {
		// No URLs.
		return nullptr;
	}

	// Check each URL.
	CacheManager cache;
	for (vector<RomData::ExtURL>::const_iterator iter = extURLs->begin();
	     iter != extURLs->end(); ++iter)
	{
		const RomData::ExtURL &extURL = *iter;

		// TODO: Have download() return the actual data and/or load the cached file.
		rp_string cache_filename = cache.download(extURL.url, extURL.cache_key);
		if (cache_filename.empty())
			continue;

		// Attempt to load the image.
		unique_ptr<IRpFile> file(new RpFile(cache_filename, RpFile::FM_OPEN_READ));
		if (!file || !file->isOpen())
			continue;

		rp_image *dl_img = RpImageLoader::load(file.get());
		if (dl_img && dl_img->isValid()) {
			// Image loaded.
			return dl_img;
		}
		delete dl_img;

		// Try the next URL.
	}

	// Could not load any external images.
	return nullptr;
}

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
	 * Create a monochrome bitmap to act as the icon mask.
	 * Since the icon being masked is color, this bitmap
	 * only contains an AND mask.
	 *
	 * 1 == opaque pixel
	 * 0 == transparent pixel
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
						pxMono |= ((*src & 0xFF000000) != 0);
					}
					*dest++ = pxMono;
				}
			}
			break;
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
 * Convert an rp_image to HBITMAP.
 * @return image rp_image.
 * @param bgColor Background color for images with alpha transparency. (ARGB32 format)
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpImageWin32::toHBITMAP(const rp_image *image, uint32_t bgColor)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid()) {
		// Invalid image.
		return nullptr;
	}

	// We should be using the RpGdiplusBackend.
	const RpGdiplusBackend *backend =
		dynamic_cast<const RpGdiplusBackend*>(image->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return nullptr;
	}

	// Convert to HBITMAP.
	// TODO: Const-ness stuff.
	return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP(bgColor);
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
	if (!image || !image->isValid()) {
		// Invalid image.
		return nullptr;
	}

	// We should be using the RpGdiplusBackend.
	const RpGdiplusBackend *backend =
		dynamic_cast<const RpGdiplusBackend*>(image->backend());
	assert(backend != nullptr);
	if (!backend) {
		// Incorrect backend set.
		return nullptr;
	}

	// Convert to HBITMAP first.
	// TODO: Const-ness stuff.
	// TODO: Specify a background color?
	// TODO: Proper alpha transparency for ARGB32.
	HBITMAP hBitmap = const_cast<RpGdiplusBackend*>(backend)->toHBITMAP();
	if (!hBitmap) {
		// Error converting to HBITMAP.
		return nullptr;
	}

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

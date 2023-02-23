/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RpImageWin32.cpp: rp_image to Win32 conversion functions.               *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "RpImageWin32.hpp"

// librptexture
#include "librptexture/img/RpGdiplusBackend.hpp"
using LibRpTexture::rp_image;
using LibRpTexture::RpGdiplusBackend;

// C++ STL classes.
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
#include "librptexture/img/GdiplusHelper.hpp"

namespace RpImageWin32 {

/**
 * Convert an rp_image to a HBITMAP for use as an icon mask.
 * @param image rp_image.
 * @return HBITMAP, or nullptr on error.
 */
static HBITMAP toHBITMAP_mask(const rp_image *image)
{
	assert(image != nullptr);
	assert(image->isValid());
	if (!image || !image->isValid())
		return nullptr;

	// References:
	// - http://stackoverflow.com/questions/2886831/win32-c-c-load-image-from-memory-buffer
	// - http://stackoverflow.com/a/2901465

	/**
	 * Create a monochrome bitmap to act as the icon's AND mask.
	 * The XOR mask is the icon data itself.
	 *
	 * 1 == opaque pixel
	 * 0 == transparent pixel
	 *
	 * References:
	 * - https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createicon
	 * - https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-iconinfo
	 */

	// NOTE: Monochrome bitmaps have a stride of 32px. (4 bytes)
	const int width = image->width();
	// Stride adjustment per line, in bytes.
	const int stride = ((width + 31) & 32) / 8;
	// (Difference between used bytes and unused bytes.)
	const int stride_adj = (width % 32 == 0 ? 0 : ((32 - (width % 32)) / 8));
	// Icon size. (stride * height)
	const int icon_sz = stride * image->height();

	BITMAPINFO bmi;
	BITMAPINFOHEADER *bmiHeader = &bmi.bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width;
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
	HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap)
		return nullptr;

	// NOTE: Windows doesn't support top-down for monochrome icons,
	// so this is vertically flipped.

	// AND mask: Parse the original image.
	switch (image->format()) {
		case rp_image::Format::CI8: {
			// Get the transparent color index.
			int tr_idx = image->tr_idx();
			if (tr_idx >= 0) {
				// Find all pixels matching tr_idx.
				uint8_t *dest = pvBits;
				for (int y = image->height()-1; y >= 0; y--) {
					// TODO: Use stride arithmetic instead of image->scanLine().
					const uint8_t *src = static_cast<const uint8_t*>(image->scanLine(y));
					unsigned int x = (unsigned int)width;
					for (x = (unsigned int)width; x > 7; x -= 8) {
						uint8_t pxMono = 0;
						for (unsigned int bit = 8; bit > 0; bit--, src++) {
							// MSB == left-most pixel.
							pxMono <<= 1;
							pxMono |= (*src != tr_idx);
						}
						*dest++ = pxMono;
					}

					// Handle unaligned bits.
					if (x > 0) {
						uint8_t pxMono = 0;
						for (unsigned int bit = x; bit > 0; bit--, src++) {
							// MSB == left-most pixel.
							pxMono <<= 1;
							pxMono |= (*src != tr_idx);
						}
						// Not 8px aligned; shift the bits over.
						pxMono <<= (8 - x);
						*dest++ = pxMono;
					}

					// Clear out unused bytes and go to the next line.
					for (x = (unsigned int)stride_adj; x > 0; x--) {
						*dest++ = 0;
					}
				}
			} else {
				// tr_idx isn't set. This means the image is either
				// fully opaque or it has alpha transparency.
				// TODO: Only set pixels within the used area? (stride_adj)
				memset(pvBits, 0xFF, icon_sz);
			}
			break;
		}

		case rp_image::Format::ARGB32: {
			// Find all pixels with a 0 alpha channel.
			memset(pvBits, 0xFF, icon_sz);
			uint8_t *dest = pvBits;
			for (int y = image->height()-1; y >= 0; y--) {
				// TODO: Use stride arithmetic instead of image->scanLine().
				const uint32_t *src = static_cast<const uint32_t*>(image->scanLine(y));
				unsigned int x = (unsigned int)width;
				for (; x > 7; x -= 8) {
					uint8_t pxMono = 0;
					for (unsigned int bit = 8; bit > 0; bit--, src++) {
						// MSB == left-most pixel.
						pxMono <<= 1;
						pxMono |= ((*src & 0xFF000000) != 0);
					}
					*dest++ = pxMono;
				}

				// Handle unaligned bits.
				if (x > 0) {
					uint8_t pxMono = 0;
					for (unsigned int bit = x; bit > 0; bit--, src++) {
						// MSB == left-most pixel.
						pxMono <<= 1;
						pxMono |= ((*src & 0xFF000000) != 0);
					}
					// Not 8px aligned; shift the bits over.
					pxMono <<= (8 - x);
					*dest++ = pxMono;
				}

				// Clear out unused bytes and go to the next line.
				for (x = (unsigned int)stride_adj; x > 0; x--) {
					*dest++ = 0;
				}
			}
			break;
		}

		default:
			// Unsupported format.
			assert(!"Unsupported rp_image::Format.");
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
HBITMAP toHBITMAP(const rp_image *image, uint32_t bgColor)
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
 * Convert an rp_image to HBITMAP.
 * This version resizes the image.
 * @param image		[in] rp_image
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP(const rp_image *image, uint32_t bgColor, SIZE size, bool nearest)
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
	return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP(bgColor, size, nearest);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel.
 * @param image		[in] rp_image
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP_alpha(const rp_image *image)
{
	static const SIZE size = {0, 0};
	return toHBITMAP_alpha(image, size, false);
}

/**
 * Convert an rp_image to HBITMAP.
 * This version preserves the alpha channel and resizes the image.
 * @param image		[in] rp_image
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP toHBITMAP_alpha(const rp_image *image, SIZE size, bool nearest)
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
	if (size.cx <= 0 || size.cy <= 0) {
		// No resize is required.
		return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha();
	} else {
		// Resize is required.
		return const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha(size, nearest);
	}
}

/**
 * Convert an rp_image to HICON.
 * @param image rp_image.
 * @return HICON, or nullptr on error.
 */
HICON toHICON(const rp_image *image)
{
	HBITMAP hBitmap = nullptr;
	HBITMAP hbmMask = nullptr;
	HICON hIcon = nullptr;
	ICONINFO ii;

	assert(image != nullptr);
	if (!image || !image->isValid()) {
		// Invalid image.
		return nullptr;
	}

	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	rp_image *tmp_img = nullptr;
	if (!image->isSquare()) {
		// Image is non-square.
		tmp_img = image->squared();
		if (tmp_img) {
			image = tmp_img;
		}
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
	hBitmap = const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha();
	if (!hBitmap)
		goto cleanup;

	// Convert the image to an icon mask.
	hbmMask = toHBITMAP_mask(image);
	if (!hbmMask)
		goto cleanup;

	// Convert to an icon.
	// Reference: http://forums.codeguru.com/showthread.php?441251-CBitmap-to-HICON-or-HICON-from-HBITMAP&p=1661856#post1661856
	ii.fIcon = TRUE;
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	ii.hbmColor = hBitmap;
	ii.hbmMask = hbmMask;

	// Create the icon.
	hIcon = CreateIconIndirect(&ii);

cleanup:
	// Delete the original bitmaps and we're done.
	if (hBitmap)
		DeleteBitmap(hBitmap);
	if (hbmMask)
		DeleteBitmap(hbmMask);
	UNREF(tmp_img);
	return hIcon;
}

/**
 * Convert an HBITMAP to rp_image.
 * @param hBitmap HBITMAP.
 * @return rp_image.
 */
rp_image *fromHBITMAP(HBITMAP hBitmap)
{
	BITMAP bm;
	if (!GetObject(hBitmap, sizeof(bm), &bm)) {
		// GetObject() failed.
		return nullptr;
	}

	// Determine the image format.
	rp_image::Format format;
	int copy_len;
	switch (bm.bmBitsPixel) {
		case 8:
			assert(!"fromHBITMAP() doesn't support 8bpp yet.");
			return nullptr;
#if 0
			format = rp_image::Format::CI8;
			copy_len = bm.bmWidth;
			break;
#endif
		case 32:
			format = rp_image::Format::ARGB32;
			copy_len = bm.bmWidth * 4;
			break;
		default:
			assert(!"Unsupported HBITMAP bmBitsPixel value.");
			return nullptr;
	}

	// TODO: Copy the palette for 8-bit.

	// Reference: https://docs.microsoft.com/en-us/windows/win32/gdi/capturing-an-image
	const int height = abs(bm.bmHeight);
	BITMAPINFOHEADER bi;
	bi.biSize = sizeof(bi);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = -height;	// Ensure the image is top-down.
	bi.biPlanes = 1;
	bi.biBitCount = bm.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;	// TODO for 8-bit
	bi.biClrImportant = 0;	// TODO for 8-bit

	// Allocate memory for the bitmap.
	const int src_stride = ((bm.bmWidth * bi.biBitCount + 31) / 32) * 4;
	const DWORD dwBmpSize = src_stride * height;
	unique_ptr<uint8_t[]> pBits(new uint8_t[dwBmpSize]);

	// Get the DIBits.
	HDC hDC = GetDC(nullptr);
	int dib_ret = GetDIBits(hDC, hBitmap, 0, height, pBits.get(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	ReleaseDC(nullptr, hDC);
	if (!dib_ret) {
		// GetDIBits() failed.
		return nullptr;
	}

	// Copy the data into a new rp_image.
	rp_image *const img = new rp_image(bm.bmWidth, height, format);
	if (!img->isValid()) {
		// Could not allocate the image.
		img->unref();
		return nullptr;
	}

	// TODO: Copy the palette for 8-bit.
	// TODO: The image might be upside-down.

	// Copy the image data.
	const uint8_t *src = pBits.get();
	uint8_t *dest = static_cast<uint8_t*>(img->bits());
	const int dest_stride = img->stride();
	for (int y = height; y > 0; y--) {
		memcpy(dest, src, copy_len);
		src += src_stride;
		dest += dest_stride;
	}

	// rp_image created.
	return img;
}

/**
 * Convert an HBITMAP to HICON.
 * @param hBitmap HBITMAP.
 * @return HICON, or nullptr on error.
 */
HICON toHICON(HBITMAP hBitmap)
{
	assert(hBitmap != nullptr);
	if (!hBitmap) {
		// Invalid image.
		return nullptr;
	}

	// Temporarily convert the HBITMAP to rp_image
	// in order to create an icon mask.
	// NOTE: Windows doesn't seem to have any way to get
	// direct access to the HBITMAP's pixels, so this step
	// step is required. (GetDIBits() copies the pixels.)
	rp_image *img = fromHBITMAP(hBitmap);
	if (!img) {
		// Error converting to rp_image.
		return nullptr;
	}

	// Windows doesn't like non-square icons.
	// Add extra transparent columns/rows before
	// converting to HBITMAP.
	HBITMAP hBmpTmp = nullptr;
	if (!img->isSquare()) {
		// Image is non-square.
		rp_image *const tmp_img = img->squared();
		if (tmp_img) {
			UNREF(img);
			img = tmp_img;
		}

		// Create a new temporary bitmap.
		const RpGdiplusBackend* backend =
			dynamic_cast<const RpGdiplusBackend*>(img->backend());
		assert(backend != nullptr);
		if (!backend) {
			// Incorrect backend set.
			UNREF(img);
			return nullptr;
		}
		hBmpTmp = const_cast<RpGdiplusBackend*>(backend)->toHBITMAP_alpha();
		if (hBmpTmp) {
			hBitmap = hBmpTmp;
		}
	}

	// Convert the image to an icon mask.
	HBITMAP hbmMask = toHBITMAP_mask(img);
	if (!hbmMask) {
		// Failed to create the icon mask.
		img->unref();
		if (hBmpTmp) {
			DeleteBitmap(hBmpTmp);
		}
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

	// Delete the bitmaps and we're done.
	DeleteBitmap(hbmMask);
	if (hBmpTmp) {
		DeleteBitmap(hBmpTmp);
	}
	img->unref();
	return hIcon;
}

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
HBITMAP getSubBitmap(const rp_image *imgSpriteSheet, int x, int y, int width, int height, UINT dpi)
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

}

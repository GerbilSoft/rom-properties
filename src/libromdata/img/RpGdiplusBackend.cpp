/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpGdiplusBackend.hpp: rp_image_backend using GDI+.                      *
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

// NOTE: This class is located in libromdata, not Win32,
// since RpPng_gdiplus.cpp uses the backend directly.

#include "RpGdiplusBackend.hpp"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>

#include "img/rp_image.hpp"
#include "img/GdiplusHelper.hpp"

namespace LibRomData {

/**
 * Create an RpGdiplusBackend.
 *
 * This will create an internal Gdiplus::Bitmap
 * with the specified parameters.
 *
 * @param width Image width.
 * @param height Image height.
 * @param format Image format.
 */
RpGdiplusBackend::RpGdiplusBackend(int width, int height, rp_image::Format format)
	: super(width, height, format)
	, m_gdipToken(0)
	, m_pGdipBmp(nullptr)
	, m_gdipFmt(0)
	, m_pGdipPalette(nullptr)
{
	// Initialize GDI+.
	m_gdipToken = GdiplusHelper::InitGDIPlus();
	assert(m_gdipToken != 0);
	if (m_gdipToken == 0)
		return;

	// Initialize the Gdiplus bitmap.
	switch (format) {
		case rp_image::FORMAT_CI8:
			m_gdipFmt = PixelFormat8bppIndexed;
			break;
		case rp_image::FORMAT_ARGB32:
			m_gdipFmt = PixelFormat32bppARGB;
			break;
		default:
			assert(false);
			this->width = 0;
			this->height = 0;
			this->stride = 0;
			this->format = rp_image::FORMAT_NONE;
			return;
	}
	m_pGdipBmp = new Gdiplus::Bitmap(width, height, m_gdipFmt);

	// Do the initial lock.
	if (doInitialLock() != 0)
		return;

	if (this->format == rp_image::FORMAT_CI8) {
		// Initialize the palette.
		// Note that Gdiplus::Image doesn't support directly
		// modifying the palette, so we have to copy our
		// palette data every time the underlying image
		// is requested.
		size_t gdipPalette_sz = sizeof(Gdiplus::ColorPalette) + (sizeof(Gdiplus::ARGB)*255);
		m_pGdipPalette = (Gdiplus::ColorPalette*)calloc(1, gdipPalette_sz);

		// Set this->palette to the first palette entry.
		this->palette = reinterpret_cast<uint32_t*>(&m_pGdipPalette->Entries[0]);
		// 256 colors allocated in the palette.
		this->palette_len = 256;
	}
}

/**
 * Create an RpGdiplusBackend using the specified Gdiplus::Bitmap.
 *
 * NOTE: This RpGdiplusBackend will take ownership of the Gdiplus::Bitmap.
 *
 * @param pGdipBmp Gdiplus::Bitmap.
 */
RpGdiplusBackend::RpGdiplusBackend(Gdiplus::Bitmap *pGdipBmp)
	: super(0, 0, rp_image::FORMAT_NONE)
	, m_gdipToken(0)
	, m_pGdipBmp(pGdipBmp)
	, m_gdipFmt(0)
	, m_pGdipPalette(nullptr)
{
	assert(pGdipBmp != nullptr);
	if (!pGdipBmp)
		return;

	// Initialize GDI+.
	m_gdipToken = GdiplusHelper::InitGDIPlus();
	assert(m_gdipToken != 0);
	if (m_gdipToken == 0) {
		delete m_pGdipBmp;
		m_pGdipBmp = nullptr;
		return;
	}

	// Check the pixel format.
	m_gdipFmt = pGdipBmp->GetPixelFormat();
	switch (m_gdipFmt) {
		case PixelFormat8bppIndexed:
			this->format = rp_image::FORMAT_CI8;
			break;

		case PixelFormat24bppRGB:
		case PixelFormat32bppRGB:
			// TODO: Is conversion needed?
			this->format = rp_image::FORMAT_ARGB32;
			m_gdipFmt = PixelFormat32bppRGB;
			break;

		case PixelFormat32bppARGB:
			this->format = rp_image::FORMAT_ARGB32;
			break;

		default:
			// Unsupported format.
			assert(false);
			delete m_pGdipBmp;
			m_pGdipBmp = nullptr;
			return;
	}

	// Set the width and height.
	this->width = pGdipBmp->GetWidth();
	this->height = pGdipBmp->GetHeight();

	// If the image has a palette, load it.
	if (this->format == rp_image::FORMAT_CI8) {
		// 256-color palette.
		size_t gdipPalette_sz = sizeof(Gdiplus::ColorPalette) + (sizeof(Gdiplus::ARGB)*255);
		m_pGdipPalette = (Gdiplus::ColorPalette*)malloc(gdipPalette_sz);

		// Actual GDI+ palette size.
		int palette_size = pGdipBmp->GetPaletteSize();
		assert(palette_size > 0);

		Gdiplus::Status status = pGdipBmp->GetPalette(m_pGdipPalette, palette_size);
		if (status != Gdiplus::Status::Ok) {
			// Failed to retrieve the palette.
			free(m_pGdipPalette);
			m_pGdipPalette = nullptr;
			delete m_pGdipBmp;
			m_pGdipBmp = nullptr;
			m_gdipFmt = 0;
			this->width = 0;
			this->height = 0;
			this->stride = 0;
			this->format = rp_image::FORMAT_NONE;
			return;
		}

		if (m_pGdipPalette->Count < 256) {
			// Extend the palette to 256 colors.
			// Additional colors will be set to 0.
			int diff = 256 - m_pGdipPalette->Count;
			memset(&m_pGdipPalette->Entries[m_pGdipPalette->Count], 0, diff*sizeof(Gdiplus::ARGB));
			m_pGdipPalette->Count = 256;
		}

		// Set this->palette to the first palette entry.
		this->palette = reinterpret_cast<uint32_t*>(&m_pGdipPalette->Entries[0]);
		// 256 colors allocated in the palette.
		this->palette_len = 256;
	}

	// Do the initial lock.
	doInitialLock();
}

RpGdiplusBackend::~RpGdiplusBackend()
{
	if (m_pGdipBmp) {
		// TODO: Is an Unlock required here?
		m_pGdipBmp->UnlockBits(&m_gdipBmpData);
		delete m_pGdipBmp;
	}

	free(this->m_pGdipPalette);
	GdiplusHelper::ShutdownGDIPlus(m_gdipToken);
}

/**
 * Initial GDI+ bitmap lock and palette initialization.
 * @return 0 on success; non-zero on error.
 */
int RpGdiplusBackend::doInitialLock(void)
{
	// Lock the bitmap.
	// It will only be (temporarily) unlocked when converting to HBITMAP.
	// FIXME: rp_image needs to support "stride", since GDI+ stride is
	// not necessarily the same as width*sizeof(pixel).
	const Gdiplus::Rect bmpRect(0, 0, this->width, this->height);
	Gdiplus::Status status = m_pGdipBmp->LockBits(&bmpRect,
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
		m_gdipFmt, &m_gdipBmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		delete m_pGdipBmp;
		m_pGdipBmp = nullptr;
		m_gdipFmt = 0;
		this->width = 0;
		this->height = 0;
		this->stride = 0;
		this->format = rp_image::FORMAT_NONE;
		return -1;
	}

	// Set the image stride.
	// On Windows, it might not be the same as width*pixelsize.
	// TODO: If Stride is negative, the image is upside-down.
	this->stride = abs(m_gdipBmpData.Stride);
	return 0;
}

/**
 * Creator function for rp_image::setBackendCreatorFn().
 */
rp_image_backend *RpGdiplusBackend::creator_fn(int width, int height, rp_image::Format format)
{
	return new RpGdiplusBackend(width, height, format);
}

void *RpGdiplusBackend::data(void)
{
	// Return the data from the locked GDI+ bitmap.
	return m_gdipBmpData.Scan0;
}

const void *RpGdiplusBackend::data(void) const
{
	// Return the data from the locked GDI+ bitmap.
	return m_gdipBmpData.Scan0;
}

size_t RpGdiplusBackend::data_len(void) const
{
	return this->stride * this->height;
}

/**
 * Convert the GDI+ image to HBITMAP.
 * Caller must delete the HBITMAP.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @return HBITMAP.
 */
HBITMAP RpGdiplusBackend::getHBITMAP(void)
{
	// TODO: Check for errors?

	// Temporarily unlock the GDI+ bitmap.
	m_pGdipBmp->UnlockBits(&m_gdipBmpData);
	
	if (this->format == rp_image::FORMAT_CI8) {
		// Copy the local palette to the GDI+ image.
		m_pGdipBmp->SetPalette(m_pGdipPalette);
	}

	// TODO: Specify a background color?
	HBITMAP hBitmap;
	Gdiplus::Color bgColor(0xFFFFFFFF);
	Gdiplus::Status status = m_pGdipBmp->GetHBITMAP(bgColor, &hBitmap);
	if (status != Gdiplus::Status::Ok) {
		// Error converting to HBITMAP.
		hBitmap = nullptr;
	}

	// Re-lock the bitmap.
	const Gdiplus::Rect bmpRect(0, 0, width, height);
	m_pGdipBmp->LockBits(&bmpRect,
		Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
		m_gdipFmt, &m_gdipBmpData);

	return hBitmap;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * RpGdiplusBackend.hpp: rp_image_backend using GDI+.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This class is located in librptexture, not Win32,
// since gtest_init.cpp uses the backend directly.

#include "stdafx.h"
#include "RpGdiplusBackend.hpp"
#include "aligned_malloc.h"

// C++ STL classes.
using std::unique_ptr;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "img/GdiplusHelper.hpp"

namespace LibRpTexture {

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
	, m_isLocked(false)
	, m_bytesppShift(0)
	, m_gdipFmt(0)
	, m_pImgBuf(nullptr)
	, m_pGdipPalette(nullptr)
{
	memset(&m_gdipBmpData, 0, sizeof(m_gdipBmpData));

	if (this->width <= 0 || this->height <= 0) {
		// Image did not initialize successfully.
		return;
	}

	// Initialize the Gdiplus bitmap.
	switch (format) {
		case rp_image::Format::CI8:
			m_gdipFmt = PixelFormat8bppIndexed;
			m_bytesppShift = 0;
			break;
		case rp_image::Format::ARGB32:
			m_gdipFmt = PixelFormat32bppARGB;
			m_bytesppShift = 2;
			break;
		default:
			assert(!"Unsupported rp_image::Format.");
			clear_properties();
			return;
	}
	m_pGdipBmp.reset(new Gdiplus::Bitmap(width, height, m_gdipFmt));

	// Do the initial lock.
	if (doInitialLock() != 0)
		return;

	if (this->format == rp_image::Format::CI8) {
		// Initialize the palette.
		// Note that Gdiplus::Image doesn't support directly
		// modifying the palette, so we have to copy our
		// palette data every time the underlying image
		// is requested.

		// NOTE: Gdiplus::ColorPalette has two UINT entries,
		// followed by the palette. That's 8 extra bytes.
		static_assert(sizeof(Gdiplus::ColorPalette) == 12, "Need to fix Gdiplus::ColorPalette alignment adjustments!");
		const size_t gdipPalette_sz = sizeof(Gdiplus::ColorPalette) + (sizeof(Gdiplus::ARGB)*255);
		uint8_t *const pPalData = static_cast<uint8_t*>(aligned_malloc(16, gdipPalette_sz + 8));
		if (!pPalData) {
			// ENOMEM
			m_pGdipBmp.reset();
			clear_properties();
			return;
		}

		// Adjust the palette alignment.
		m_pGdipPalette = reinterpret_cast<Gdiplus::ColorPalette*>(pPalData + 8);
		ASSERT_ALIGNMENT(16, &m_pGdipPalette->Entries[0]);

		memset(m_pGdipPalette, 0, gdipPalette_sz);
		m_pGdipPalette->Flags = 0;
		m_pGdipPalette->Count = 256;
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
	: super(0, 0, rp_image::Format::None)
	, m_pGdipBmp(pGdipBmp)
	, m_isLocked(false)
	, m_bytesppShift(0)
	, m_gdipFmt(0)
	, m_pImgBuf(nullptr)
	, m_pGdipPalette(nullptr)
{
	memset(&m_gdipBmpData, 0, sizeof(m_gdipBmpData));

	assert(pGdipBmp != nullptr);
	if (!pGdipBmp)
		return;

	// Check the pixel format.
	m_gdipFmt = pGdipBmp->GetPixelFormat();
	switch (m_gdipFmt) {
		case PixelFormat8bppIndexed:
			this->format = rp_image::Format::CI8;
			m_bytesppShift = 0;
			break;

		case PixelFormat24bppRGB:
		case PixelFormat32bppRGB:
			// TODO: Is conversion needed?
			this->format = rp_image::Format::ARGB32;
			m_gdipFmt = PixelFormat32bppRGB;
			m_bytesppShift = 2;
			break;

		case PixelFormat32bppARGB:
			this->format = rp_image::Format::ARGB32;
			m_bytesppShift = 2;
			break;

		default:
			// Unsupported format.
			assert(!"Unsupported Gdiplus::PixelFormat.");
			m_pGdipBmp.reset();
			return;
	}

	// Set the width and height.
	this->width = pGdipBmp->GetWidth();
	this->height = pGdipBmp->GetHeight();

	// If the image has a palette, load it.
	if (this->format == rp_image::Format::CI8) {
		// 256-color palette.
		size_t gdipPalette_sz = sizeof(Gdiplus::ColorPalette) + (sizeof(Gdiplus::ARGB)*255);
		m_pGdipPalette = (Gdiplus::ColorPalette*)malloc(gdipPalette_sz);
		if (!m_pGdipPalette) {
			// ENOMEM
			m_pGdipBmp.reset();
			m_gdipFmt = 0;
			clear_properties();
			return;
		}

		// Actual GDI+ palette size.
		const int palette_size = pGdipBmp->GetPaletteSize();
		assert(palette_size > 0);

		Gdiplus::Status status = pGdipBmp->GetPalette(m_pGdipPalette, palette_size);
		if (status != Gdiplus::Status::Ok) {
			// Failed to retrieve the palette.
			free(m_pGdipPalette);
			m_pGdipPalette = nullptr;
			m_pGdipBmp.reset();
			m_gdipFmt = 0;
			clear_properties();
			return;
		}

		if (m_pGdipPalette->Count < 256) {
			// Extend the palette to 256 colors.
			// Additional colors will be set to 0.
			const unsigned int diff = 256 - m_pGdipPalette->Count;
			memset(&m_pGdipPalette->Entries[m_pGdipPalette->Count], 0, diff*sizeof(Gdiplus::ARGB));
			m_pGdipPalette->Count = 256;
		}
	}

	// Do the initial lock.
	doInitialLock();
}

RpGdiplusBackend::~RpGdiplusBackend()
{
	if (m_pGdipBmp) {
		// TODO: Is an Unlock required here?
		if (m_isLocked) {
			m_pGdipBmp->UnlockBits(&m_gdipBmpData);
		}
	}

	// Palette is adjusted by 8 bytes in order to ensure
	// that the color table is 16-byte aligned.
	if (m_pGdipPalette) {
		uint8_t *pPalData = reinterpret_cast<uint8_t*>(m_pGdipPalette) - 8;
		aligned_free(pPalData);
	}

	aligned_free(m_pImgBuf);
}

/**
 * Initial GDI+ bitmap lock and palette initialization.
 * @return 0 on success; non-zero on error.
 */
int RpGdiplusBackend::doInitialLock(void)
{
	// Lock the bitmap.
	// It will only be (temporarily) unlocked when converting to HBITMAP.
	Gdiplus::Status status = lock();
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		m_pGdipBmp.reset();
		m_gdipFmt = 0;
		this->width = 0;
		this->height = 0;
		this->stride = 0;
		this->format = rp_image::Format::None;
		return -1;
	}
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
	if (!m_isLocked) {
		// Lock the image.
		const_cast<RpGdiplusBackend*>(this)->lock();
	}

	// Return the data from the locked GDI+ bitmap.
	return m_gdipBmpData.Scan0;
}

const void *RpGdiplusBackend::data(void) const
{
	if (!m_isLocked) {
		// Lock the image.
		const_cast<RpGdiplusBackend*>(this)->lock();
	}

	// Return the data from the locked GDI+ bitmap.
	return m_gdipBmpData.Scan0;
}

size_t RpGdiplusBackend::data_len(void) const
{
	return this->stride * this->height;
}

uint32_t *RpGdiplusBackend::palette(void)
{
	if (!m_pGdipPalette)
		return nullptr;
	return reinterpret_cast<uint32_t*>(m_pGdipPalette->Entries);
}

const uint32_t *RpGdiplusBackend::palette(void) const
{
	if (!m_pGdipPalette)
		return nullptr;
	return reinterpret_cast<const uint32_t*>(m_pGdipPalette->Entries);
}

unsigned int RpGdiplusBackend::palette_len(void) const
{
	if (!m_pGdipPalette)
		return 0;
	return m_pGdipPalette->Count;
}

/**
 * Shrink image dimensions.
 * @param width New width.
 * @param height New height.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpGdiplusBackend::shrink(int width, int height)
{
	assert(width > 0);
	assert(height > 0);
	assert(this->width > 0);
	assert(this->height > 0);
	assert(width <= this->width);
	assert(height <= this->height);
	if (width <= 0 || height <= 0 ||
	    this->width <= 0 || this->height <= 0 ||
	    width > this->width || height > this->height)
	{
		return -EINVAL;
	}

	if (width == this->width && height == this->height) {
		// Attempting to resize to the same size...
		return 0;
	}

	// TODO: Is there a way to resize the Gdiplus::Bitmap in place?
	// NOTE: Lock() locks a region, so maybe we could use that, but
	// Gdiplus::Bitmap to HBITMAP conversion uses the whole image...
	Gdiplus::Status status = this->unlock();
	if (status != Gdiplus::Status::Ok) {
		return -EIO;
	}

	Gdiplus::Bitmap *const pGdipBmp_old = m_pGdipBmp.release();
	m_pGdipBmp.reset(pGdipBmp_old->Clone(0, 0, width, height, m_gdipFmt));
	this->width = width;
	this->height = height;
	delete pGdipBmp_old;

	status = this->lock();
	return (status == Gdiplus::Status::Ok ? 0 : -EIO);
}

/**
 * Lock the GDI+ bitmap.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @return Gdiplus::Status
 */
Gdiplus::Status RpGdiplusBackend::lock(void)
{
	// TODO: Recursive locks?
	// TODO: Atomic locking?
	if (m_isLocked)
		return Gdiplus::Status::Ok;

	// We're using the full stride for the last row
	// to make it easier to manage.

	// We're allocating our own image buffer in order to set a custom stride.
	// Stride should be a multiple of 16 bytes for SSE2 optimization.
	m_gdipBmpData.Stride = this->width << m_bytesppShift;
	m_gdipBmpData.Stride = ALIGN_BYTES(16, m_gdipBmpData.Stride);

	if (!m_pImgBuf) {
		// Allocate the image buffer.
		m_pImgBuf = aligned_malloc(16, ImageSizeCalc::T_calcImageSize(m_gdipBmpData.Stride, this->height));
		if (!m_pImgBuf) {
			// malloc() failed.
			return Gdiplus::Status::OutOfMemory;
		}
	}

	// NOTE: Setting m_gdipBmpData values even if we set it before.
	// Not sure if they get overwritten...
	const Gdiplus::Rect bmpRect(0, 0, this->width, this->height);
	m_gdipBmpData.Width = this->width;
	m_gdipBmpData.Height = this->height;
	m_gdipBmpData.PixelFormat = m_gdipFmt;
	m_gdipBmpData.Scan0 = m_pImgBuf;

	Gdiplus::Status status = m_pGdipBmp->LockBits(&bmpRect,
		Gdiplus::ImageLockModeRead |
		Gdiplus::ImageLockModeWrite |
		Gdiplus::ImageLockModeUserInputBuf,
		m_gdipFmt, &m_gdipBmpData);
	if (status == Gdiplus::Status::Ok) {
		m_isLocked = true;
	}

	// Save the image stride.
	// On Windows, it might not be the same as width*pixelsize.
	// TODO: If Stride is negative, the image is upside-down.
	this->stride = abs(m_gdipBmpData.Stride);
	return status;
}

/**
 * Unlock the GDI+ bitmap.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @return Gdiplus::Status
 */
Gdiplus::Status RpGdiplusBackend::unlock(void)
{
	// TODO: Recursive locks?
	if (!m_isLocked)
		return Gdiplus::Status::Ok;

	Gdiplus::Status status = m_pGdipBmp->UnlockBits(&m_gdipBmpData);
	if (status == Gdiplus::Status::Ok) {
		m_isLocked = false;
	}
	return status;
}

/**
 * Duplicate the GDI+ bitmap.
 *
 * This function is intended to be used when drawing
 * GDI+ bitmaps directly to a window. As such, it will
 * automatically convert images to 32-bit ARGB in order
 * to avoid CI8 alpha transparency artifacting.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @return Duplicated GDI+ bitmap.
 */
Gdiplus::Bitmap *RpGdiplusBackend::dup_ARGB32(void) const
{
	Gdiplus::Bitmap *pBmp = nullptr;
	Gdiplus::Status status;

	switch (this->format) {
		case rp_image::Format::CI8: {
			// FIXME: Since adding custom stride, Bitmap::Clone() seems to
			// automatically replace CI8 color 8 with white.
			// Hence, we have to manually copy the image for CI8.
			pBmp = new Gdiplus::Bitmap(this->width, this->height, PixelFormat32bppARGB);

			const Gdiplus::Rect bmpDestRect(0, 0, this->width, this->height);
			Gdiplus::BitmapData bmpDestData;
			status = pBmp->LockBits(&bmpDestRect, Gdiplus::ImageLockModeWrite,
				PixelFormat32bppARGB, &bmpDestData);
			assert(status == Gdiplus::Status::Ok);
			if (status != Gdiplus::Status::Ok) {
				delete pBmp;
				return nullptr;
			}

			// Convert on a row-by-row basis.
			// FIXME: Handle upside-down images. (stride is negative)
			assert(this->stride > 0);
			if (this->stride <= 0) {
				pBmp->UnlockBits(&bmpDestData);
				delete pBmp;
				return nullptr;
			}

			const int src_stride_adj = this->stride - this->width;
			const int dest_stride_adj = (bmpDestData.Stride / sizeof(uint32_t)) - this->width;
			const uint8_t *src = static_cast<const uint8_t*>(m_pImgBuf);
			uint32_t *dest = static_cast<uint32_t*>(bmpDestData.Scan0);
			for (unsigned int y = this->height; y > 0; y--) {
				// Convert two pixels at a time.
				unsigned int x = this->width;
				for (; x > 1; x -= 2) {
					dest[0] = m_pGdipPalette->Entries[src[0]];
					dest[1] = m_pGdipPalette->Entries[src[1]];
					src += 2;
					dest += 2;
				}
				if (x == 1) {
					*dest = m_pGdipPalette->Entries[*src];
					src++;
					dest++;
				}

				// Next row.
				src += src_stride_adj;
				dest += dest_stride_adj;
			}

			status = pBmp->UnlockBits(&bmpDestData);
			assert(status == Gdiplus::Status::Ok);
			if (status != Gdiplus::Status::Ok) {
				delete pBmp;
				return nullptr;
			}
			break;
		}

		case rp_image::Format::ARGB32:
			status = const_cast<RpGdiplusBackend*>(this)->unlock();
			assert(status == Gdiplus::Status::Ok);
			if (status != Gdiplus::Status::Ok) {
				return nullptr;
			}

			pBmp = m_pGdipBmp->Clone(0, 0, this->width, this->height, PixelFormat32bppARGB);
			status = const_cast<RpGdiplusBackend*>(this)->lock();
			assert(status == Gdiplus::Status::Ok);
			if (status != Gdiplus::Status::Ok) {
				delete pBmp;
				return nullptr;
			}
			break;

		default:
			assert(!"Unsupported rp_image::Format.");
	}

	return pBmp;
}

/**
 * Convert the GDI+ image to HBITMAP.
 * Caller must delete the HBITMAP.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @param bgColor Background color for images with alpha transparency. (ARGB32 format)
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpGdiplusBackend::toHBITMAP(Gdiplus::ARGB bgColor)
{
	// Wrapper for the resizing toHBITMAP() function.
	static constexpr SIZE size = {0, 0};
	return toHBITMAP(bgColor, size, true);
}

/**
 * Convert an rp_image to HBITMAP.
 * Caller must delete the HBITMAP.
 *
 * This version resizes the image.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @param image		[in] rp_image.
 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
 * @param size		[in] If non-zero, resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpGdiplusBackend::toHBITMAP(uint32_t bgColor, SIZE size, bool nearest)
{
	// TODO: Check for errors?
	unique_ptr<Gdiplus::Bitmap> pTmpBmp;
	if (this->format == rp_image::Format::CI8) {
		// Copy the local palette to the GDI+ image.
		m_pGdipBmp->SetPalette(m_pGdipPalette);
		// TODO: Optimize has_translucent_palette_entries().
		if (this->tr_idx < 0 || this->has_translucent_palette_entries()) {
			// Need to convert to ARGB32 first.
			// Otherwise, the translucent entries won't show up correctly.
			// Example: SSBM GCN save icon has color fringing on Windows 7.
			// (...but not Windows XP)
			pTmpBmp.reset(this->dup_ARGB32());
			if (!pTmpBmp) {
				// Error converting to ARGB32.
				return nullptr;
			}
		}
	}

	// TODO: Specify a background color?
	HBITMAP hBitmap;
	Gdiplus::Status status;

	// If the source isn't being resized,
	// we don't need a temporary image.
	if ((size.cx <= 0 || size.cy <= 0) ||
	    (size.cx == this->width && size.cy == this->height))
	{
		// No resize necessary.
		if (pTmpBmp) {
			// Use the temporary ARGB32 bitmap.
			status = pTmpBmp->GetHBITMAP(Gdiplus::Color(bgColor), &hBitmap);
		} else {
			// Use the regular bitmap.
			unlock();
			status = m_pGdipBmp->GetHBITMAP(Gdiplus::Color(bgColor), &hBitmap);
			lock();
		}

		if (status != Gdiplus::Status::Ok) {
			// Error converting to HBITMAP.
			return nullptr;
		}

		return hBitmap;
	}

	// NOTE: We're using ARGB32 because GDI+ doesn't
	// handle resizing on CI8 properly.
	unique_ptr<Gdiplus::Bitmap> pResizeBmp(
		new Gdiplus::Bitmap(size.cx, size.cy, PixelFormat32bppARGB));
	Gdiplus::Graphics g(pResizeBmp.get());

	// Always use PixelOffsetModeHalf.
	// When interpolating, this results in higher-quality
	// anti-aliasing. When using nearest-neighbor, this
	// fixes an issue that causes the scaled image to be
	// shifted to the top-left by 1px.
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

	if (nearest) {
		// Set nearest-neighbor interpolation.
		// TODO: What's the default?
		g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	}

	if (pTmpBmp) {
		// Use the temporary ARGB32 bitmap.
		status = g.DrawImage(pTmpBmp.get(), 0, 0, size.cx, size.cy);
	} else {
		// Use the regular bitmap.
		unlock();
		status = g.DrawImage(m_pGdipBmp.get(), 0, 0, size.cx, size.cy);
		lock();
	}

	if (status != Gdiplus::Status::Ok) {
		// Error drawing to Graphics.
		return nullptr;
	}

	// Convert the resized bitmap to HBITMAP.
	status = pResizeBmp->GetHBITMAP(Gdiplus::Color(bgColor), &hBitmap);
	if (status != Gdiplus::Status::Ok) {
		// Error converting to HBITMAP.
		return nullptr;
	}

	return hBitmap;
}

/**
 * Convert the GDI+ image to HBITMAP.
 * Caller must delete the HBITMAP.
 *
 * This version preserves the alpha channel.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpGdiplusBackend::toHBITMAP_alpha(void)
{
	// Convert to HBITMAP.
	HBITMAP hBitmap = nullptr;
	switch (this->format) {
		case rp_image::Format::ARGB32:
			hBitmap = convBmpData_ARGB32(&m_gdipBmpData);
			break;

		case rp_image::Format::CI8: {
			// Always convert to ARGB32.
			// Windows will end up doing this anyway,
			// and it doesn't really like CI8+alpha.
			static constexpr SIZE size = {0, 0};
			hBitmap = toHBITMAP_alpha(size, false);
			break;
		}

		default:
			assert(!"Unsupported rp_image::Format.");
			break;
	}

	return hBitmap;
}

/**
 * Convert the GDI+ image to HBITMAP.
 * Caller must delete the HBITMAP.
 *
 * This version preserves the alpha channel.
 *
 * WARNING: This *may* invalidate pointers
 * previously returned by data().
 *
 * @param size		[in] Resize the image to this size.
 * @param nearest	[in] If true, use nearest-neighbor scaling.
 * @return HBITMAP, or nullptr on error.
 */
HBITMAP RpGdiplusBackend::toHBITMAP_alpha(SIZE size, bool nearest)
{
	// Convert the image to ARGB32 (if necessary) and resize it.
	Gdiplus::Status status = Gdiplus::Status::GenericError;

	unique_ptr<Gdiplus::Bitmap> pTmpBmp;
	if (this->format == rp_image::Format::CI8) {
		// Convert to ARGB32. Otherwise, translucent and/or
		// transparent entries won't show up correctly.
		// NOTE: dup_ARGB32() copies the palette to the
		// GDI+ image, so we don't have to do that here.
		pTmpBmp.reset(this->dup_ARGB32());
		if (!pTmpBmp) {
			// Error converting to ARGB32.
			return nullptr;
		}
	}

	// If the source is 32-bit ARGB and isn't being resized,
	// we don't need a temporary image.
	if ((size.cx <= 0 || size.cy <= 0) ||
	    (size.cx == this->width && size.cy == this->height))
	{
		if (pTmpBmp) {
			if (pTmpBmp->GetPixelFormat() == PixelFormat32bppARGB) {
				// Use pTmpBmp directly.
				const Gdiplus::Rect bmpTmpRect(0, 0, this->width, this->height);
				Gdiplus::BitmapData bmpTmpData;
				status = pTmpBmp->LockBits(&bmpTmpRect, Gdiplus::ImageLockModeRead,
					PixelFormat32bppARGB, &bmpTmpData);
				if (status != Gdiplus::Status::Ok) {
					// Error re-locking the resized GDI+ bitmap.
					return nullptr;
				}

				HBITMAP hBitmap = convBmpData_ARGB32(&bmpTmpData);
				pTmpBmp->UnlockBits(&bmpTmpData);
				return hBitmap;
			}
		} else {
			if (m_pGdipBmp->GetPixelFormat() == PixelFormat32bppARGB) {
				// Use m_pGdipBmp directly.
				return convBmpData_ARGB32(&m_gdipBmpData);
			}
		}
	}

	/** Create a new bitmap. **/

	if (!pTmpBmp) {
		// Temporarily unlock the GDI+ bitmap.
		status = unlock();
		if (status != Gdiplus::Status::Ok) {
			// Error unlocking the GDI+ bitmap.
			return nullptr;
		}
	}

	// NOTE: We're using ARGB32 because GDI+ doesn't
	// handle resizing on CI8 properly.
	unique_ptr<Gdiplus::Bitmap> pResizeBmp(
		new Gdiplus::Bitmap(size.cx, size.cy, PixelFormat32bppARGB));
	Gdiplus::Graphics g(pResizeBmp.get());

	// Always use PixelOffsetModeHalf.
	// When interpolating, this results in higher-quality
	// anti-aliasing. When using nearest-neighbor, this
	// fixes an issue that causes the scaled image to be
	// shifted to the top-left by 1px.
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

	if (nearest) {
		// Set nearest-neighbor interpolation.
		// TODO: What's the default?
		g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	}

	if (pTmpBmp) {
		// Use the temporary ARGB32 bitmap.
		g.DrawImage(pTmpBmp.get(), 0, 0, size.cx, size.cy);
	} else {
		// Use the regular bitmap.
		g.DrawImage(m_pGdipBmp.get(), 0, 0, size.cx, size.cy);
	}

	if (!pTmpBmp) {
		// Re-lock the bitmap.
		status = lock();
		if (status != Gdiplus::Status::Ok) {
			// Error re-locking the GDI+ bitmap.
			return nullptr;
		}
	}

	// Lock the resized bitmap.
	const Gdiplus::Rect bmpResizeRect(0, 0, size.cx, size.cy);
	Gdiplus::BitmapData bmpResizeData;
	status = pResizeBmp->LockBits(&bmpResizeRect, Gdiplus::ImageLockModeRead,
		PixelFormat32bppARGB, &bmpResizeData);
	if (status != Gdiplus::Status::Ok) {
		// Error re-locking the resized GDI+ bitmap.
		return nullptr;
	}

	// Convert to HBITMAP.
	HBITMAP hBitmap = convBmpData_ARGB32(&bmpResizeData);

	// We're done here.
	pResizeBmp->UnlockBits(&bmpResizeData);
	return hBitmap;
}

/**
 * Convert a locked ARGB32 GDI+ bitmap to an HBITMAP.
 * Alpha transparency is preserved.
 * @param pBmpData Gdiplus::BitmapData.
 * @return HBITMAP.
 */
HBITMAP RpGdiplusBackend::convBmpData_ARGB32(const Gdiplus::BitmapData *pBmpData)
{
	// Create a bitmap.
	BITMAPINFO bmi;
	BITMAPINFOHEADER *const bmiHeader = &bmi.bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = pBmpData->Width;
	bmiHeader->biHeight = -static_cast<int>(pBmpData->Height);	// Top-down
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 32;
	bmiHeader->biCompression = BI_RGB;	// TODO: BI_BITFIELDS?
	bmiHeader->biSizeImage = 0;	// TODO?
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	bmiHeader->biClrUsed = 0;
	bmiHeader->biClrImportant = 0;

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&pvBits), nullptr, 0);
	if (!hBitmap) {
		// Could not create the bitmap.
		return nullptr;
	}

	// Copy the data from the GDI+ bitmap to the HBITMAP directly.
	// HBITMAP stride is a multiple of 4, so we can assume that
	// it's equal to row_bytes.
	const int row_bytes = static_cast<int>(pBmpData->Width) * 4;
	const int gdip_stride = pBmpData->Stride;
	if (row_bytes == gdip_stride) {
		// Copy the entire image all at once.
		memcpy(pvBits, pBmpData->Scan0, static_cast<size_t>(gdip_stride) * pBmpData->Height);
	} else {
		// Copy one line at a time.
		const uint8_t *gdip_px = static_cast<const uint8_t*>(pBmpData->Scan0);
		for (unsigned int y = pBmpData->Height; y > 0; y--) {
			memcpy(pvBits, gdip_px, row_bytes);
			pvBits += row_bytes;
			gdip_px += gdip_stride;
		}
	}

	// Bitmap is ready.
	return hBitmap;
}

/**
 * Convert a locked CI8 GDI+ bitmap to an HBITMAP.
 * Alpha transparency is preserved.
 * @param pBmpData Gdiplus::BitmapData.
 * @return HBITMAP.
 */
HBITMAP RpGdiplusBackend::convBmpData_CI8(const Gdiplus::BitmapData *pBmpData)
{
	// BITMAPINFO with 256-color palette.
	const size_t szBmi = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD)*256);
	BITMAPINFO *bmi = static_cast<BITMAPINFO*>(malloc(szBmi));
	if (!bmi) {
		// ENOMEM
		return nullptr;
	}
	BITMAPINFOHEADER *bmiHeader = &bmi->bmiHeader;

	// Initialize the BITMAPINFOHEADER.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = pBmpData->Width;
	bmiHeader->biHeight = -static_cast<int>(pBmpData->Height);	// Top-down
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 8;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = 0;	// TODO?
	bmiHeader->biXPelsPerMeter = 0;	// TODO
	bmiHeader->biYPelsPerMeter = 0;	// TODO
	// FIXME: Specify palette as a parameter?
	bmiHeader->biClrUsed = m_pGdipPalette->Count;
	bmiHeader->biClrImportant = m_pGdipPalette->Count;	// TODO

	// Copy the palette from the image.
	memcpy(bmi->bmiColors, m_pGdipPalette->Entries, m_pGdipPalette->Count * sizeof(RGBQUAD));

	// Create the bitmap.
	uint8_t *pvBits;
	HBITMAP hBitmap = CreateDIBSection(nullptr, bmi, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&pvBits), nullptr, 0);
	free(bmi);
	if (!hBitmap) {
		// Could not create the bitmap.
		return nullptr;
	}

	// Copy the data from the GDI+ bitmap to the HBITMAP directly.
	// HBITMAP stride is a multiple of 4.
	const size_t row_bytes = pBmpData->Width;
	const int hbmp_stride = ALIGN_BYTES(4, (size_t)row_bytes);
	const int gdip_stride = pBmpData->Stride;
	if (hbmp_stride == gdip_stride) {
		// Copy the entire image all at once.
		memcpy(pvBits, pBmpData->Scan0, gdip_stride * pBmpData->Height);
	} else {
		// Copy one line at a time.
		const uint8_t *gdip_px = static_cast<const uint8_t*>(pBmpData->Scan0);
		for (unsigned int y = pBmpData->Height; y > 0; y--) {
			memcpy(pvBits, gdip_px, row_bytes);
			pvBits += hbmp_stride;
			gdip_px += pBmpData->Stride;
		}
	}

	// Bitmap is ready.
	return hBitmap;
}

}

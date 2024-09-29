/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * RpGdiplusBackend.hpp: rp_image_backend using GDI+.                      *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// NOTE: This class is located in libromdata, not Win32,
// since RpPng_gdiplus.cpp uses the backend directly.

#pragma once

#include "rp_image_backend.hpp"
#include "libwin32common/RpWin32_sdk.h"

// C++ includes
#include <memory>

// Gdiplus for HBITMAP conversion.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <olectl.h>
#include <gdiplus.h>

namespace LibRpTexture {

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class RpGdiplusBackend final : public rp_image_backend
{
	public:
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
		explicit RpGdiplusBackend(int width, int height, rp_image::Format format);

		/**
		 * Create an RpGdiplusBackend using the specified Gdiplus::Bitmap.
		 *
		 * NOTE: This RpGdiplusBackend will take ownership of the Gdiplus::Bitmap.
		 *
		 * @param pGdipBmp Gdiplus::Bitmap.
		 */
		explicit RpGdiplusBackend(Gdiplus::Bitmap *pGdipBmp);

		~RpGdiplusBackend() final;

	private:
		typedef rp_image_backend super;
		RP_DISABLE_COPY(RpGdiplusBackend)

	private:
		/**
		 * Initial GDI+ bitmap lock and palette initialization.
		 * @return 0 on success; non-zero on error.
		 */
		int doInitialLock(void);

	public:
		/**
		 * Creator function for rp_image::setBackendCreatorFn().
		 */
		RP_LIBROMDATA_PUBLIC
		static rp_image_backend *creator_fn(int width, int height, rp_image::Format format);

		// Image data.
		void *data(void) final;
		const void *data(void) const final;
		size_t data_len(void) const final;

		// Image palette.
		uint32_t *palette(void) final;
		const uint32_t *palette(void) const final;
		unsigned int palette_len(void) const final;

	public:
		/**
		 * Shrink image dimensions.
		 * @param width New width.
		 * @param height New height.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int shrink(int width, int height) final;

	protected:
		/**
		 * Lock the GDI+ bitmap.
		 *
		 * WARNING: This *may* invalidate pointers
		 * previously returned by data().
		 *
		 * @return Gdiplus::Status
		 */
		Gdiplus::Status lock(void);

		/**
		 * Unlock the GDI+ bitmap.
		 *
		 * WARNING: This *may* invalidate pointers
		 * previously returned by data().
		 *
		 * @return Gdiplus::Status
		 */
		Gdiplus::Status unlock(void);

	public:
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
		Gdiplus::Bitmap *dup_ARGB32(void) const;

	public:
		/**
		 * Convert the GDI+ image to HBITMAP.
		 * Caller must delete the HBITMAP.
		 *
		 * WARNING: This *may* invalidate pointers
		 * previously returned by data().
		 *
		 * @param bgColor	[in] Background color for images with alpha transparency. (ARGB32 format)
		 * @return HBITMAP, or nullptr on error.
		 */
		RP_LIBROMDATA_PUBLIC
		HBITMAP toHBITMAP(Gdiplus::ARGB bgColor);

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
		RP_LIBROMDATA_PUBLIC
		HBITMAP toHBITMAP(uint32_t bgColor, SIZE size, bool nearest);

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
		RP_LIBROMDATA_PUBLIC
		HBITMAP toHBITMAP_alpha();

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
		RP_LIBROMDATA_PUBLIC
		HBITMAP toHBITMAP_alpha(SIZE size, bool nearest);

	protected:
		/**
		 * Convert a locked ARGB32 GDI+ bitmap to an HBITMAP.
		 * Alpha transparency is preserved.
		 * @param pBmpData Gdiplus::BitmapData.
		 * @return HBITMAP.
		 */
		static HBITMAP convBmpData_ARGB32(const Gdiplus::BitmapData *pBmpData);

		/**
		 * Convert a locked CI8 GDI+ bitmap to an HBITMAP.
		 * Alpha transparency is preserved.
		 * @param pBmpData Gdiplus::BitmapData.
		 * @return HBITMAP.
		 */
		HBITMAP convBmpData_CI8(const Gdiplus::BitmapData *pBmpData);

	protected:
		std::unique_ptr<Gdiplus::Bitmap> m_pGdipBmp;

		// BitmapData for locking.
		bool m_isLocked;
		uint8_t m_bytesppShift;	// bytespp shift value.
		Gdiplus::PixelFormat m_gdipFmt;
		Gdiplus::BitmapData m_gdipBmpData;

		// Allocated image buffer for Bitmap::LockBits().
		void *m_pImgBuf;

		// Color palette.
		// Pointer to Entries[0] is used for rp_image_backend::palette.
		Gdiplus::ColorPalette *m_pGdipPalette;
};

}

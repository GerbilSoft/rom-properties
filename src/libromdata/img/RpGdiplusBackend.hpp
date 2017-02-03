/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpGdiplusBackend.hpp: rp_image_backend using GDI+.                      *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_RPGDIPLUSBACKEND_HPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_RPGDIPLUSBACKEND_HPP__

#include "rp_image_backend.hpp"
#include "RpWin32.hpp"

// C++ includes.
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

namespace LibRomData {

/**
 * rp_image data storage class.
 * This can be overridden for e.g. QImage or GDI+.
 */
class RpGdiplusBackend : public rp_image_backend
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
		RpGdiplusBackend(int width, int height, rp_image::Format format);

		/**
		 * Create an RpGdiplusBackend using the specified Gdiplus::Bitmap.
		 *
		 * NOTE: This RpGdiplusBackend will take ownership of the Gdiplus::Bitmap.
		 *
		 * @param pGdipBmp Gdiplus::Bitmap.
		 */
		explicit RpGdiplusBackend(Gdiplus::Bitmap *pGdipBmp);

		virtual ~RpGdiplusBackend();

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
		static rp_image_backend *creator_fn(int width, int height, rp_image::Format format);

		// Image data.
		virtual void *data(void) override final;
		virtual const void *data(void) const override final;
		virtual size_t data_len(void) const override final;

		// Image palette.
		virtual uint32_t *palette(void) override final;
		virtual const uint32_t *palette(void) const override final;
		virtual int palette_len(void) const override final;

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
		HBITMAP toHBITMAP(uint32_t bgColor, const SIZE &size, bool nearest);

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
		HBITMAP toHBITMAP_alpha(const SIZE &size, bool nearest);

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
		ULONG_PTR m_gdipToken;
		Gdiplus::Bitmap *m_pGdipBmp;

		// BitmapData for locking.
		bool m_isLocked;
		Gdiplus::PixelFormat m_gdipFmt;
		Gdiplus::BitmapData m_gdipBmpData;

		// Color palette.
		// Pointer to Entries[0] is used for rp_image_backend::palette.
		Gdiplus::ColorPalette *m_pGdipPalette;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RPGDIPLUSBACKEND_HPP__ */

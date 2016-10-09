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
		RpGdiplusBackend(Gdiplus::Bitmap *pGdipBmp);

		virtual ~RpGdiplusBackend();

	private:
		typedef rp_image_backend super;
		RpGdiplusBackend(const RpGdiplusBackend &other);
		RpGdiplusBackend &operator=(const RpGdiplusBackend &other);

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
		virtual void *data(void) final;
		virtual const void *data(void) const final;
		virtual size_t data_len(void) const final;

	public:
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
		HBITMAP toHBITMAP(Gdiplus::ARGB bgColor = 0xFFFFFFFF);

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
		HBITMAP toHBITMAP_alpha(void);

	protected:
		ULONG_PTR m_gdipToken;
		Gdiplus::Bitmap *m_pGdipBmp;

		// BitmapData for locking.
		Gdiplus::PixelFormat m_gdipFmt;
		Gdiplus::BitmapData m_gdipBmpData;

		// Color palette.
		// Pointer to Entries[0] is used for rp_image_backend::palette.
		Gdiplus::ColorPalette *m_pGdipPalette;
};

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_RPGDIPLUSBACKEND_HPP__ */

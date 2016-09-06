/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpPng_gdiplus.cpp: PNG handler using gdiplus. (Win32)                   *
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

#include "RpPng.hpp"
#include "rp_image.hpp"
#include "../file/IRpFile.hpp"
#include "../file/RP_IStream_Win32.hpp"

// Win32
#include "../RpWin32.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C includes. (C++ namespace)
#include <memory>
using std::auto_ptr;

// Gdiplus for PNG decoding.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <olectl.h>
#include <gdiplus.h>
#include "GdiplusHelper.hpp"

namespace LibRomData {

class RpPngPrivate
{
	private:
		// RpPngPrivate is a static class.
		RpPngPrivate();
		~RpPngPrivate();
		RpPngPrivate(const RpPngPrivate &other);
		RpPngPrivate &operator=(const RpPngPrivate &other);

	public:
		/**
		 * Load a PNG image from a file.
		 * @param file IStream wrapping an IRpFile.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadPng(IStream *file);
};

/** RpPngPrivate **/

/**
 * Load a PNG image from a file.
 * @param file IStream wrapping an IRpFile.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpPngPrivate::loadPng(IStream *file)
{
	Gdiplus::Status status = Gdiplus::Status::GenericError;

	// Attempt to load the image.
	auto_ptr<Gdiplus::Bitmap> gdipBmp(Gdiplus::Bitmap::FromStream(file, FALSE));
	if (!gdipBmp.get()) {
		// Could not load the image.
		return nullptr;
	}

	// Image loaded.
	// Convert to rp_image.
	rp_image::Format fmt;
	Gdiplus::PixelFormat gdipFmt;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	size_t line_size;	// Number of image bytes per scanline.
	bool argb32_to_grayscale = false;
	switch (gdipBmp->GetPixelFormat()) {
		case PixelFormat1bppIndexed:
		case PixelFormat4bppIndexed:
		case PixelFormat8bppIndexed:
			// Paletted image.
			// 1bpp and 4bpp will be converted to 8bpp by GDI+.
			fmt = rp_image::FORMAT_CI8;
			gdipFmt = PixelFormat8bppIndexed;
			line_size = 1 * bmpRect.Width;
			break;

		case PixelFormat32bppARGB:
			// If the colorspace is gray, this is actually a
			// grayscale image, and should be converted to CI8.
			// Reference: http://stackoverflow.com/questions/30391832/gdi-grayscale-png-loaded-as-pixelformat32bppargb

			// TODO: PARGB or ARGB?
			gdipFmt = PixelFormat32bppARGB;

			if (gdipBmp->GetFlags() & Gdiplus::ImageFlagsColorSpaceGRAY) {
				// Grayscale image.
				// NOTE: Need to manually convert to CI8.
				argb32_to_grayscale = true;
				fmt = rp_image::FORMAT_CI8;
				line_size = bmpRect.Width;
			} else {
				// Some other format. Use ARGB32.
				fmt = rp_image::FORMAT_ARGB32;
				line_size = 4 * bmpRect.Width;
			}
			break;

		default:
			// Convert everything else to ARGB32.
			fmt = rp_image::FORMAT_ARGB32;
			// TODO: PARGB or ARGB?
			gdipFmt = PixelFormat32bppARGB;
			line_size = 4 * bmpRect.Width;
			break;
	}

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead, gdipFmt, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height, fmt);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

	// TODO: Copy the palette if necessary.

	// Copy the image, line by line.
	// NOTE: If Stride is negative, the image is upside-down.
	int rp_line_start, rp_line_inc, gdip_line_inc;
	if (bmpData.Stride < 0) {
		// Bottom-up
		rp_line_start = bmpData.Height - 1;
		rp_line_inc = -1;
		gdip_line_inc = -bmpData.Stride;
	} else {
		// Top-down
		rp_line_start = 0;
		rp_line_inc = 1;
		gdip_line_inc = bmpData.Stride;
	}

	// TODO: Copy CI8 palettes from 256-color images.
	if (argb32_to_grayscale) {
		// Convert from ARGB32 to grayscale.

		// Initialize the rp_image palette.
		uint32_t *palette = reinterpret_cast<uint32_t*>(img->palette());
		uint32_t color = 0xFF000000;
		for (int i = img->palette_len(); i > 0; i--, palette++) {
			*palette = color;
			color += 0x010101;
		}

		// Downconvert the grayscale image.
		// We'll take the least-significant byte. (blue)
		gdip_line_inc -= (bmpData.Width * 4);
		const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
		for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		     rp_y += rp_line_inc, gdip_px += gdip_line_inc)
		{
			uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
			for (int x = bmpData.Width; x > 0; x--) {
				*rp_px = *gdip_px;
				rp_px++;
				gdip_px += 4;
			}
		}
	} else {
		// Copy the image data.
		const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
		for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		     rp_y += rp_line_inc, gdip_px += gdip_line_inc)
		{
			uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
			memcpy(rp_px, gdip_px, line_size);
		}
	}

	// Done reading the PNG image.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Load a PNG image from an IRpFile.
 *
 * This image is NOT checked for issues; do not use
 * with untrusted images!
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpPng::loadUnchecked(IRpFile *file)
{
	// Initialize GDI+.
	// TODO: Don't init/shutdown on every image.
	ScopedGdiplus gdip;
	if (!gdip.isValid()) {
		// Failed to initialize GDI+.
		return nullptr;
	}

	// Create an IStream wrapper for the IRpFile.
	RP_IStream_Win32 *stream = new RP_IStream_Win32(file);

	// Call the actual PNG image reading function.
	rp_image *img = RpPngPrivate::loadPng(stream);

	// Release the IStream wrapper.
	stream->Release();

	// Return the image.
	return img;
}

}

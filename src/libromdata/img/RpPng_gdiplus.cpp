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
using std::unique_ptr;

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

// pngcheck()
#include "pngcheck/pngcheck.hpp"

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
		 * Copy an ARGB32 GDI+ bitmap to an ARGB32 rp_image.
		 * @param gdipBmp GDI+ bitmap.
		 * @return rp_image on success; nullptr on error.
		 */
		static rp_image *gdip_ARGB32_to_rp_image_ARGB32(Gdiplus::Bitmap *gdipBmp);

		/**
		 * Copy an ARGB32 GDI+ bitmap to a grayscale CI8 rp_image.
		 * @param gdipBmp GDI+ bitmap.
		 * @return rp_image on success; nullptr on error.
		 */
		static rp_image *gdip_ARGB32_to_rp_image_CI8_grayscale(Gdiplus::Bitmap *gdipBmp);

		/**
		 * Copy a CI8 GDI+ bitmap to a CI8 rp_image.
		 * @param gdipBmp GDI+ bitmap.
		 * @return rp_image on success; nullptr on error.
		 */
		static rp_image *gdip_CI8_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp);

		/**
		 * Copy a CI4 GDI+ bitmap to a CI8 rp_image.
		 * @param gdipBmp GDI+ bitmap.
		 * @return rp_image on success; nullptr on error.
		 */
		static rp_image *gdip_CI4_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp);

		/**
		 * Copy a monochrome GDI+ bitmap to a CI8 rp_image.
		 * @param gdipBmp GDI+ bitmap.
		 * @return rp_image on success; nullptr on error.
		 */
		static rp_image *gdip_mono_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp);

		/**
		 * Load a PNG image from a file.
		 * @param file IStream wrapping an IRpFile.
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadPng(IStream *file);
};

/** RpPngPrivate **/

/**
 * Copy an ARGB32 GDI+ bitmap to an ARGB32 rp_image.
 * This also works for RGB24 bitmaps.
 * @param gdipBmp GDI+ bitmap.
 * @return rp_image on success; nullptr on error.
 */
rp_image *RpPngPrivate::gdip_ARGB32_to_rp_image_ARGB32(Gdiplus::Bitmap *gdipBmp)
{
	Gdiplus::Status status;
	assert(gdipBmp->GetPixelFormat() == PixelFormat24bppRGB ||
	       gdipBmp->GetPixelFormat() == PixelFormat32bppRGB ||
	       gdipBmp->GetPixelFormat() == PixelFormat32bppARGB);

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead,
				PixelFormat32bppARGB, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height,
				rp_image::FORMAT_ARGB32);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

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

	// Copy the image data.
	const int line_size = (int)bmpData.Width * 4;
	const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
	for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		rp_y += rp_line_inc, gdip_px += gdip_line_inc)
	{
		uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
		memcpy(rp_px, gdip_px, line_size);
	}

	// Unlock the GDI+ bitmap.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Copy an ARGB32 GDI+ bitmap to a grayscale CI8 rp_image.
 * @param gdipBmp GDI+ bitmap.
 * @return rp_image on success; nullptr on error.
 */
rp_image *RpPngPrivate::gdip_ARGB32_to_rp_image_CI8_grayscale(Gdiplus::Bitmap *gdipBmp)
{
	Gdiplus::Status status;
	assert(gdipBmp->GetPixelFormat() == PixelFormat32bppARGB);

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead,
				PixelFormat32bppARGB, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height,
				rp_image::FORMAT_CI8);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

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

	// Unlock the GDI+ bitmap.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Copy a CI8 GDI+ bitmap to a CI8 rp_image.
 * @param gdipBmp GDI+ bitmap.
 * @return rp_image on success; nullptr on error.
 */
rp_image *RpPngPrivate::gdip_CI8_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp)
{
	Gdiplus::Status status;
	assert(gdipBmp->GetPixelFormat() == PixelFormat8bppIndexed);

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead,
				PixelFormat8bppIndexed, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height,
				rp_image::FORMAT_CI8);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

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

	// Copy the palette.
	int palette_size = gdipBmp->GetPaletteSize();
	assert(palette_size > 0);
	Gdiplus::ColorPalette *palette =
		reinterpret_cast<Gdiplus::ColorPalette*>(malloc(palette_size));
	gdipBmp->GetPalette(palette, palette_size);

	// Copy the palette colors.
	// TODO: Check flags for alpha/grayscale?
	assert((int)palette->Count > 0);
	assert((int)palette->Count <= img->palette_len());
	int color_count = std::min((int)palette->Count, img->palette_len());
	memcpy(img->palette(), palette->Entries, color_count*sizeof(uint32_t));

	// Zero out any remaining colors.
	const int diff = img->palette_len() - color_count;
	if (diff > 0) {
		memset(img->palette() + color_count, 0, diff*sizeof(uint32_t));
	}

	// We don't need the GDI+ palette anymore.
	free(palette);

	// Copy the image data.
	const int line_size = (int)bmpData.Width;
	const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
	for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		rp_y += rp_line_inc, gdip_px += gdip_line_inc)
	{
		uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
		memcpy(rp_px, gdip_px, line_size);
	}

	// Unlock the GDI+ bitmap.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Copy a CI4 GDI+ bitmap to a CI8 rp_image.
 * @param gdipBmp GDI+ bitmap.
 * @return rp_image on success; nullptr on error.
 */
rp_image *RpPngPrivate::gdip_CI4_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp)
{
	Gdiplus::Status status;
	assert(gdipBmp->GetPixelFormat() == PixelFormat4bppIndexed);

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead,
				PixelFormat4bppIndexed, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height,
				rp_image::FORMAT_CI8);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

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

	// Copy the palette.
	int palette_size = gdipBmp->GetPaletteSize();
	assert(palette_size > 0);
	Gdiplus::ColorPalette *palette =
		reinterpret_cast<Gdiplus::ColorPalette*>(malloc(palette_size));
	gdipBmp->GetPalette(palette, palette_size);

	// Copy the palette colors.
	// TODO: Check flags for alpha/grayscale?
	assert((int)palette->Count > 0);
	assert((int)palette->Count <= img->palette_len());
	int color_count = std::min((int)palette->Count, img->palette_len());
	memcpy(img->palette(), palette->Entries, color_count*sizeof(uint32_t));

	// Zero out any remaining colors.
	const int diff = img->palette_len() - color_count;
	if (diff > 0) {
		memset(img->palette() + color_count, 0, diff*sizeof(uint32_t));
	}

	// We don't need the GDI+ palette anymore.
	free(palette);

	// Copy the image data.
	// NOTE: Divide width by two, then add one if the image is an odd size,
	// since we can't store a single 4bpp value by itself.
	gdip_line_inc -= (int)((bmpData.Width / 2) + (bmpData.Width % 2));
	const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
	for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		rp_y += rp_line_inc, gdip_px += gdip_line_inc)
	{
		uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
		for (int x = bmpData.Width; x > 0; x -= 2, gdip_px++) {
			// Each source byte has two packed pixels.
			rp_px[0] = *gdip_px >> 4;
			if (x == 1) {
				// Odd number of pixels.
				rp_px++;
			} else {
				// Two or more pixels remaining.
				rp_px[1] = *gdip_px & 0x0F;
				rp_px += 2;
			}
		}
	}

	// Unlock the GDI+ bitmap.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Copy a monochrome GDI+ bitmap to a CI8 rp_image.
 * @param gdipBmp GDI+ bitmap.
 * @return rp_image on success; nullptr on error.
 */
rp_image *RpPngPrivate::gdip_mono_to_rp_image_CI8(Gdiplus::Bitmap *gdipBmp)
{
	Gdiplus::Status status;
	assert(gdipBmp->GetPixelFormat() == PixelFormat1bppIndexed);

	// Lock the GDI+ bitmap for processing.
	Gdiplus::BitmapData bmpData;
	const Gdiplus::Rect bmpRect(0, 0, gdipBmp->GetWidth(), gdipBmp->GetHeight());
	status = gdipBmp->LockBits(&bmpRect, Gdiplus::ImageLockModeRead,
				PixelFormat1bppIndexed, &bmpData);
	if (status != Gdiplus::Status::Ok) {
		// Error locking the GDI+ bitmap.
		return nullptr;
	}

	// Create the rp_image.
	rp_image *img = new rp_image((int)bmpData.Width, (int)bmpData.Height,
				rp_image::FORMAT_CI8);
	if (!img || !img->isValid()) {
		// Error creating an rp_image.
		gdipBmp->UnlockBits(&bmpData);
		return nullptr;
	}

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

	// Copy the palette.
	int palette_size = gdipBmp->GetPaletteSize();
	assert(palette_size > 0);
	Gdiplus::ColorPalette *palette =
		reinterpret_cast<Gdiplus::ColorPalette*>(malloc(palette_size));
	gdipBmp->GetPalette(palette, palette_size);

	// Copy the palette colors.
	// TODO: Check flags for alpha/grayscale?
	assert((int)palette->Count > 0);
	assert((int)palette->Count <= img->palette_len());
	int color_count = std::min((int)palette->Count, img->palette_len());
	memcpy(img->palette(), palette->Entries, color_count*sizeof(uint32_t));

	// Zero out any remaining colors.
	const int diff = img->palette_len() - color_count;
	if (diff > 0) {
		memset(img->palette() + color_count, 0, diff*sizeof(uint32_t));
	}

	// We don't need the GDI+ palette anymore.
	free(palette);

	// Copy the image data.
	// NOTE: Divide width by eight, then add one if the image width
	// isn't a multiple of 8, since 1bpp stores 8 pixels per byte.
	gdip_line_inc -= (int)((bmpData.Width / 8) + (bmpData.Width % 8 != 0));
	const uint8_t *gdip_px = reinterpret_cast<const uint8_t*>(bmpData.Scan0);
	for (int rp_y = rp_line_start; rp_y < (int)bmpData.Height;
		rp_y += rp_line_inc, gdip_px += gdip_line_inc)
	{
		uint8_t *rp_px = reinterpret_cast<uint8_t*>(img->scanLine(rp_y));
		for (int x = bmpData.Width; x > 0; x -= 8, gdip_px++) {
			// Each source byte has eight packed pixels.
			uint8_t mono_pxs = *gdip_px;
			for (int bit = (x >= 8 ? 8 : x); bit > 0; bit--, rp_px++) {
				*rp_px = (mono_pxs >> 7);
				mono_pxs <<= 1;
			}
		}
	}

	// Unlock the GDI+ bitmap.
	gdipBmp->UnlockBits(&bmpData);
	return img;
}

/**
 * Load a PNG image from a file.
 * @param file IStream wrapping an IRpFile.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpPngPrivate::loadPng(IStream *file)
{
	// Attempt to load the image.
	unique_ptr<Gdiplus::Bitmap> gdipBmp(Gdiplus::Bitmap::FromStream(file, FALSE));
	if (!gdipBmp) {
		// Could not load the image.
		return nullptr;
	}

	// Image loaded.
	// Convert to rp_image.
	rp_image *img = nullptr;
	switch (gdipBmp->GetPixelFormat()) {
		case PixelFormat1bppIndexed:
			// 1bpp paletted image. (monochrome)
			// GDI+ on Windows XP doesn't support converting
			// this to 8bpp, so we'll have to do it ourselves.
			img = gdip_mono_to_rp_image_CI8(gdipBmp.get());
			break;

		case PixelFormat4bppIndexed:
			// 4bpp paletted image.
			// GDI+ on Windows XP doesn't support converting
			// this to 8bpp, so we'll have to do it ourselves.
			img = gdip_CI4_to_rp_image_CI8(gdipBmp.get());
			break;

		case PixelFormat8bppIndexed:
			// 8bpp paletted image.
			img = gdip_CI8_to_rp_image_CI8(gdipBmp.get());
			break;

		case PixelFormat32bppARGB:
			// If the colorspace is gray, this is actually a
			// grayscale image, and should be converted to CI8.
			// Reference: http://stackoverflow.com/questions/30391832/gdi-grayscale-png-loaded-as-pixelformat32bppargb

			// NOTE: GDI+ loads 256-color PNG images with tRNS chunks
			// as if they're ARGB32, and there's no way to figure out
			// that this conversion happened through GDI+.

			// Grayscale should be converted to CI8.
			// Grayscale+Alpha and others should be ARGB32.
			if ((gdipBmp->GetFlags() & (Gdiplus::ImageFlagsColorSpaceGRAY | Gdiplus::ImageFlagsHasAlpha)) == Gdiplus::ImageFlagsColorSpaceGRAY) {
				// Grayscale image without alpha transparency.
				// NOTE: Need to manually convert to CI8.
				img = gdip_ARGB32_to_rp_image_CI8_grayscale(gdipBmp.get());
			} else {
				// Some other format. Use ARGB32.
				img = gdip_ARGB32_to_rp_image_ARGB32(gdipBmp.get());
			}
			break;

		default:
			// Convert everything else to ARGB32.
			img = gdip_ARGB32_to_rp_image_ARGB32(gdipBmp.get());
			break;
	}

	// Return the rp_image.
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

/**
 * Load a PNG image from an IRpFile.
 *
 * This image is verified with various tools to ensure
 * it doesn't have any errors.
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpPng::load(IRpFile *file)
{
	// Check the image with pngcheck() first.
	file->rewind();
	int ret = pngcheck(file);
	if (ret != kOK) {
		// PNG image has errors.
		return nullptr;
	}

	// PNG image has been validated.
	file->rewind();
	return loadUnchecked(file);
}

}

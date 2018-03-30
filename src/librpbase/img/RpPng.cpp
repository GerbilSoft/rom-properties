/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPng.cpp: PNG image handler.                                           *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "config.librpbase.h"

#include "RpPng.hpp"
#include "rp_image.hpp"
#include "../file/IRpFile.hpp"

// PNG writer.
#include "RpPngWriter.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
using std::unique_ptr;

// Image format libraries.
#include <png.h>

#if PNG_LIBPNG_VER < 10209 || \
    (PNG_LIBPNG_VER == 10209 && \
        (PNG_LIBPNG_VER_BUILD >= 1 && PNG_LIBPNG_VER_BUILD < 8))
/**
 * libpng-1.2.9beta8 added png_set_expand_gray_1_2_4_to_8()
 * as a replacement for png_set_gray_1_2_4_to_8().
 * The old function was removed in libpng-1.4.0beta1.
 *
 * This macro is for compatibility with older libpng.
 */
#define png_set_expand_gray_1_2_4_to_8(png_ptr) \
	png_set_gray_1_2_4_to_8(png_ptr)
#endif

// PNGCAPI was added in libpng-1.5.0beta14.
// Older versions will need this.
#ifndef PNGCAPI
# ifdef _MSC_VER
#  define PNGCAPI __cdecl
# else
#  define PNGCAPI
# endif
#endif /* !PNGCAPI */

// pngcheck()
#include "pngcheck/pngcheck.hpp"

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
// Need zlib for delay-load checks.
#include <zlib.h>
// MSVC: Exception handling for /DELAYLOAD.
#include "libwin32common/DelayLoadHelper.hpp"
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

namespace LibRpBase {

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
// DelayLoad test implementation.
DELAYLOAD_FILTER_FUNCTION_IMPL(zlib_and_png)
static int DelayLoad_test_zlib_and_png(void)
{
	static bool success = false;
	if (!success) {
		__try {
			zlibVersion();
			png_access_version_number();
		} __except (DelayLoad_filter_zlib_and_png(GetExceptionCode())) {
			return -ENOTSUP;
		}
		success = true;
	}
	return 0;
}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

class RpPngPrivate
{
	private:
		// RpPngPrivate is a static class.
		RpPngPrivate();
		~RpPngPrivate();
		RP_DISABLE_COPY(RpPngPrivate)

	public:
		/** I/O functions. **/

		/**
		 * libpng I/O read handler for IRpFile.
		 * @param png_ptr	[in]  PNG pointer.
		 * @param data		[out] Buffer for the data to read.
		 * @param length	[in]  Size of data.
		 */
		static void PNGCAPI png_io_IRpFile_read(png_structp png_ptr, png_bytep data, png_size_t length);

		/**
		 * libpng I/O write handler for IRpFile.
		 * @param png_ptr	[in] PNG pointer.
		 * @param data		[in] Data to write.
		 * @param length	[in] Size of data.
		 */
		static void PNGCAPI png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length);

		/**
		 * libpng I/O flush handler for IRpFile.
		 * @param png_ptr	[in] PNG pointer.
		 */
		static void PNGCAPI png_io_IRpFile_flush(png_structp png_ptr);

		/** Read functions. **/

		/**
		 * Read the palette for a CI8 image.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @param color_type PNG color type.
		 * @param img rp_image to store the palette in.
		 */
		static void Read_CI8_Palette(png_structp png_ptr, png_infop info_ptr,
					     int color_type, rp_image *img);

		/**
		 * Load a PNG image from an opened PNG handle.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @return rp_image*, or nullptr on error.
		 */
		static rp_image *loadPng(png_structp png_ptr, png_infop info_ptr);
};

/** RpPngPrivate **/

/** I/O functions. **/

/**
 * libpng I/O handler for IRpFile.
 * @param png_ptr PNG pointer.
 * @param data		[out] Buffer for the data to read.
 * @param length	[in]  Size of data.
 */
void PNGCAPI RpPngPrivate::png_io_IRpFile_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	// Assuming io_ptr is an IRpFile*.
	IRpFile *file = static_cast<IRpFile*>(png_get_io_ptr(png_ptr));
	if (!file)
		return;

	// Read data from the IRpFile.
	size_t sz = file->read(data, length);
	if (sz != length) {
		// Short read.
		// TODO: longjmp()?

		if (sz < length) {
			// Zero out the rest of the buffer.
			memset(&data[sz], 0, length-sz);
		}
	}
}

/**
 * libpng I/O write handler for IRpFile.
 * @param png_ptr	[in] PNG pointer.
 * @param data		[in] Data to write.
 * @param length	[in] Size of data.
 */
void PNGCAPI RpPngPrivate::png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length)
{
	// Assuming io_ptr is an IRpFile*.
	IRpFile *file = static_cast<IRpFile*>(png_get_io_ptr(png_ptr));
	if (!file)
		return;

	// Write data to the IRpFile.
	// TODO: Error handling?
	file->write(data, length);
}

/**
 * libpng I/O flush handler for IRpFile.
 * @param png_ptr	[in] PNG pointer.
 */
void PNGCAPI RpPngPrivate::png_io_IRpFile_flush(png_structp png_ptr)
{
	// Assuming io_ptr is an IRpFile*.
	IRpFile *file = static_cast<IRpFile*>(png_get_io_ptr(png_ptr));
	if (!file)
		return;

	// TODO: IRpFile::flush()
}

/** Read functions. **/

/**
 * Read the palette for a CI8 image.
 * @param png_ptr png_structp
 * @param info_ptr png_infop
 * @param color_type PNG color type.
 * @param img rp_image to store the palette in.
 */
void RpPngPrivate::Read_CI8_Palette(png_structp png_ptr, png_infop info_ptr,
				    int color_type, rp_image *img)
{
	png_colorp png_palette;
	png_bytep trans;
	int num_palette, num_trans;

	assert(img->format() == rp_image::FORMAT_CI8);
	if (img->format() != rp_image::FORMAT_CI8)
		return;

	// rp_image's palette data.
	// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
	const int palette_len = img->palette_len();
	uint32_t *img_palette = img->palette();
	assert(palette_len > 0);
	assert(palette_len <= 256);
	assert(img_palette != nullptr);
	if (!img_palette || palette_len <= 0 || palette_len > 256)
		return;

	switch (color_type) {
		case PNG_COLOR_TYPE_PALETTE:
			// Get the palette from the PNG image.
			if (png_get_PLTE(png_ptr, info_ptr, &png_palette, &num_palette) != PNG_INFO_PLTE)
				break;

			// Check if there's a tRNS chunk.
			if (png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, nullptr) != PNG_INFO_tRNS) {
				// No tRNS chunk.
				trans = nullptr;
			}

			// Combine the 24-bit RGB palette with the transparency information.
			for (int i = std::min(num_palette, palette_len);
			     i > 0; i--, img_palette++, png_palette++)
			{
				argb32_t color;
				color.r = png_palette->red;
				color.g = png_palette->green;
				color.b = png_palette->blue;
				if (trans && num_trans > 0) {
					// Copy the transparency information.
					color.a = *trans;
					num_trans--;
				} else {
					// No transparency information.
					// Assume the color is opaque.
					color.a = 0xFF;
				}

				*img_palette = color.u32;
			}

			if (num_palette < palette_len) {
				// Clear the rest of the palette.
				// (NOTE: 0 == fully transparent.)
				memset(img_palette, 0, (palette_len - num_palette) * sizeof(uint32_t));
			}
			break;

		case PNG_COLOR_TYPE_GRAY: {
			// Create a default grayscale palette.
			// NOTE: If the palette isn't 256 entries long,
			// the grayscale values will be incorrect.
			// TODO: Handle the tRNS chunk?
			uint32_t gray = 0xFF000000;
			for (int i = 0; i < std::min(256, palette_len);
			     i++, img_palette++, gray += 0x010101)
			{
				// TODO: tRNS chunk handling.
				*img_palette = gray;
			}
			break;
		}

		default:
			assert(!"Unsupported CI8 palette type.");
			break;
	}
}

/**
 * Load a PNG image from an opened PNG handle.
 * @param png_ptr png_structp
 * @param info_ptr png_infop
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpPngPrivate::loadPng(png_structp png_ptr, png_infop info_ptr)
{
	// Row pointers. (NOTE: Allocated after IHDR is read.)
	const png_byte **row_pointers = nullptr;
	rp_image *img = nullptr;

#ifdef PNG_sBIT_SUPPORTED
	bool has_sBIT = false;
	png_color_8p png_sBIT = nullptr;
#endif /* PNG_sBIT_SUPPORTED */

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		delete img;
		return nullptr;
	}
#endif

	// Read the PNG image information.
	png_read_info(png_ptr, info_ptr);

	// Read the PNG image header.
	// NOTE: libpng-1.2 defines png_uint_32 as long.
	// libpng-1.4.0beta7 appears to redefine it to unsigned int.
	// Since we're using unsigned int in img_data, we can't
	// save the values directly to the struct.
	// TODO: Conditionally use temp variables for libpng <1.4?
	int bit_depth, color_type;
	png_uint_32 width, height;
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
		&bit_depth, &color_type, nullptr, nullptr, nullptr);
	// Sanity check: Don't allow images larger than 32768x32768.
	assert(width > 0);
	assert(height > 0);
	assert(width <= 32768);
	assert(height <= 32768);
	if (width <= 0 || height <= 0 ||
	    width > 32768 || height > 32768)
	{
		// Image size is either invalid or too big.
		return nullptr;
	}

#ifdef PNG_sBIT_SUPPORTED
	// Read the sBIT chunk.
	// TODO: Fake sBIT if the PNG doesn't have one?
	has_sBIT = (png_get_sBIT(png_ptr, info_ptr, &png_sBIT) == PNG_INFO_sBIT);
#endif /* PNG_sBIT_SUPPORTED */

	// Check the color type.
	bool is24bit = false;
	rp_image::Format fmt;
	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:
			// Grayscale is handled as a 256-color image
			// with a grayscale palette.
			fmt = rp_image::FORMAT_CI8;
			if (bit_depth < 8) {
				// Expand to 8-bit grayscale.
				png_set_expand_gray_1_2_4_to_8(png_ptr);
			}
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			// Grayscale+Alpha is handled as ARGB32.
			// QImage, gdk-pixbuf, cairo, and GDI+ don't support IA8.
			// TODO: Does this work with 1, 2, and 4-bit grayscale?
			fmt = rp_image::FORMAT_ARGB32;
			png_set_gray_to_rgb(png_ptr);
			break;
		case PNG_COLOR_TYPE_PALETTE:
			if (bit_depth < 8) {
				// Expand to 8-bit pixels.
				png_set_packing(png_ptr);
			}
			fmt = rp_image::FORMAT_CI8;
			break;
		case PNG_COLOR_TYPE_RGB:
			// 24-bit RGB.
			fmt = rp_image::FORMAT_ARGB32;
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) == PNG_INFO_tRNS) {
				// tRNS chunk is present. Use it as the alpha channel.
				png_set_tRNS_to_alpha(png_ptr);
			} else {
				// 24-bit RGB with no transparency.
				is24bit = true;
			}
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			// 32-bit ARGB.
			fmt = rp_image::FORMAT_ARGB32;
			break;
		default:
			// Unsupported color type.
			return nullptr;
	}

	if (bit_depth > 8) {
		// Strip 16bpc images down to 8.
		png_set_strip_16(png_ptr);
	}

	// Get the new PNG information.
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
		&bit_depth, &color_type, nullptr, nullptr, nullptr);

	if (is24bit) {
		// rp_image doesn't support 24-bit color.
		// Expand it by having libpng fill the alpha channel
		// with 0xFF (opaque).
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	}

	// We're using "BGR" color.
	png_set_bgr(png_ptr);

	// Update the PNG info.
	png_read_update_info(png_ptr, info_ptr);

	// Create the rp_image.

	// Initialize the row pointers array.
	img = new rp_image(width, height, fmt);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		delete img;
		return nullptr;
	}

	// Initialize the row pointers array.
	const png_byte *pb = static_cast<const png_byte*>(img->bits());
	const int stride = img->stride();
	for (png_uint_32 y = 0; y < height; y++, pb += stride) {
		row_pointers[y] = pb;
	}

	// Read the image.
	png_read_image(png_ptr, const_cast<png_byte**>(row_pointers));
	png_free(png_ptr, row_pointers);

	// If CI8, read the palette.
	if (fmt == rp_image::FORMAT_CI8) {
		Read_CI8_Palette(png_ptr, info_ptr, color_type, img);
	}

#ifdef PNG_sBIT_SUPPORTED
	// Set the sBIT metadata.
	if (has_sBIT && png_sBIT) {
		// NOTE: rp_image::sBIT_t has the same format as png_color_8.
		img->set_sBIT(reinterpret_cast<rp_image::sBIT_t*>(png_sBIT));
	}
#endif /* PNG_sBIT_SUPPORTED */

	// Done reading the PNG image.
	return img;
}

/** RpPng **/

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
	if (!file)
		return nullptr;

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		return nullptr;
	}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

	// Rewind the file.
	file->rewind();

	png_structp png_ptr;
	png_infop info_ptr;

	// Initialize libpng.
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		return nullptr;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		return nullptr;
	}

	// Initialize the custom I/O handler for IRpFile.
	png_set_read_fn(png_ptr, file, RpPngPrivate::png_io_IRpFile_read);

	// Call the actual PNG image reading function.
	rp_image *img = RpPngPrivate::loadPng(png_ptr, info_ptr);

	// Free the PNG structs.
	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
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
	if (!file)
		return nullptr;

	// Check the image with pngcheck() first.
	file->rewind();
	int ret = pngcheck(file);
	if (ret != kOK) {
		// PNG image has errors.
		return nullptr;
	}

	// PNG image has been validated.
	return loadUnchecked(file);
}

/**
 * Save an image in PNG format to an IRpFile.
 * IRpFile must be open for writing.
 *
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file IRpFile to write to.
 * @param img rp_image to save.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPng::save(IRpFile *file, const rp_image *img)
{
	assert(file != nullptr);
	assert(img != nullptr);
	if (!file || !file->isOpen() || !img)
		return -EINVAL;

	// Create a PNG writer.
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(file, img));
	if (!pngWriter->isOpen())
		return -pngWriter->lastError();

	// Write the PNG IHDR.
	int ret = pngWriter->write_IHDR();
	if (ret != 0)
		return ret;

	// Write the PNG image data.
	return pngWriter->write_IDAT();
}

/**
 * Save an image in PNG format to a file.
 *
 * @param filename Destination filename.
 * @param img rp_image to save.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPng::save(const char *filename, const rp_image *img)
{
	assert(filename != nullptr);
	assert(filename[0] != 0);
	assert(img != nullptr);
	if (!filename || filename[0] == 0 || !img)
		return -EINVAL;

	// Create a PNG writer.
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(filename, img));
	if (!pngWriter->isOpen())
		return -pngWriter->lastError();

	// Write the PNG IHDR.
	int ret = pngWriter->write_IHDR();
	if (ret != 0)
		return ret;

	// Write the PNG image data.
	return pngWriter->write_IDAT();
}

/**
 * Save an animated image in APNG format to an IRpFile.
 * IRpFile must be open for writing.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * returned. The caller should then save the image
 * as a standard PNG file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file IRpFile to write to.
 * @param iconAnimData Animated image data to save.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPng::save(IRpFile *file, const IconAnimData *iconAnimData)
{
	assert(file != nullptr);
	assert(iconAnimData != nullptr);
	if (!file || !file->isOpen() || !iconAnimData)
		return -EINVAL;

	// Create a PNG writer.
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(file, iconAnimData));
	if (!pngWriter->isOpen())
		return -pngWriter->lastError();

	// Write the PNG IHDR.
	int ret = pngWriter->write_IHDR();
	if (ret != 0)
		return ret;

	// Write the PNG image data.
	return pngWriter->write_IDAT();
}

/**
 * Save an animated image in APNG format to a file.
 * IRpFile must be open for writing.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * returned. The caller should then save the image
 * as a standard PNG file.
 *
 * @param filename Destination filename.
 * @param iconAnimData Animated image data to save.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPng::save(const char *filename, const IconAnimData *iconAnimData)
{
	assert(filename != nullptr);
	assert(filename[0] != 0);
	assert(iconAnimData != nullptr);
	if (!filename || filename[0] == 0 || !iconAnimData)
		return -EINVAL;

	// Create a PNG writer.
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(filename, iconAnimData));
	if (!pngWriter->isOpen())
		return -pngWriter->lastError();

	// Write the PNG IHDR.
	int ret = pngWriter->write_IHDR();
	if (ret != 0)
		return ret;

	// Write the PNG image data.
	return pngWriter->write_IDAT();
}

}

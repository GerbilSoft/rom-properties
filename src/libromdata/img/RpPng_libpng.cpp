/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpPng_libpng.cpp: PNG handler using libpng.                             *
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

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>

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
		 * libpng I/O handler for IRpFile.
		 * @param png_ptr PNG pointer.
		 * @param buf Buffer for the data to read.
		 * @param size Size of buf.
		 */
		static void png_io_IRpFile_read(png_structp png_ptr, png_bytep buf, png_size_t size);

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

/**
 * libpng I/O handler for IRpFile.
 * @param png_ptr PNG pointer.
 * @param buf Buffer for the data to read.
 * @param size Size of buf.
 */
void RpPngPrivate::png_io_IRpFile_read(png_structp png_ptr, png_bytep buf, png_size_t size)
{
	// Assuming io_ptr is an IRpFile*.
	IRpFile *file = reinterpret_cast<IRpFile*>(png_get_io_ptr(png_ptr));
	if (!file)
		return;

	// Read data from the IRpFile.
	size_t sz = file->read(buf, size);
	if (sz != size) {
		// Short read.
		// TODO: longjmp()?

		if (sz < size) {
			// Zero out the rest of the buffer.
			memset(&buf[sz], 0, size-sz);
		}
	}
}

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
	uint32_t *img_palette = img->palette();
	assert(img_palette != nullptr);
	if (!img_palette)
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
			for (int i = std::min(num_palette, img->palette_len());
				i > 0; i--, img_palette++, png_palette++)
			{
				uint32_t color = (png_palette->blue << 0) |
							(png_palette->green << 8) |
							(png_palette->red << 16);
				if (trans && num_trans > 0) {
					// Copy the transparency information.
					color |= (*trans << 24);
					num_trans--;
				} else {
					// No transparency information.
					// Assume the color is opaque.
					color |= 0xFF000000;
				}

				*img_palette = color;
			}

			if (num_palette < img->palette_len()) {
				// Clear the rest of the palette.
				// (NOTE: 0 == fully transparent.)
				for (int i = img->palette_len()-num_palette;
				i > 0; i--, img_palette++)
				{
					*img_palette = 0;
				}
			}
			break;

		case PNG_COLOR_TYPE_GRAY:
			// Create a default grayscale palette.
			// NOTE: If the palette isn't 256 entries long,
			// the grayscale values will be incorrect.
			// TODO: Handle the tRNS chunk?
			for (int i = 0; i < std::min(256, img->palette_len());
				i++, img_palette++)
			{
				uint8_t gray = (uint8_t)i;
				*img_palette = (gray | gray << 8 | gray << 16);
				// TODO: tRNS chunk handling.
				*img_palette |= 0xFF000000;
			}

			if (img->palette_len() > 256) {
				// Clear the rest of the palette.
				// (NOTE: 0 == fully transparent.)
				for (int i = img->palette_len()-256; i > 0;
					i--, img_palette++)
				{
					*img_palette = 0;
				}
			}
			break;

		default:
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
	png_byte **row_pointers = nullptr;
	rp_image *img = nullptr;

	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		delete img;
		return nullptr;
	}

	// Read the PNG image information.
	png_read_info(png_ptr, info_ptr);

	// Read the PNG image header.
	// NOTE: libpng-1.2 defines png_uint_32 as long.
	// libpng-1.4.0beta7 appears to redefine it to unsigned int.
	// Since we're using unsigned int in img_data, we can't
	// save the values directly to the struct.
	// TODO: Conditionally use temp variables for libpng <1.4?
	int bit_depth, color_type;
	png_uint_32 img_w, img_h;
	png_get_IHDR(png_ptr, info_ptr, &img_w, &img_h,
		&bit_depth, &color_type, nullptr, nullptr, nullptr);
	if (img_w <= 0 || img_h <= 0) {
		// Invalid image size.
		return nullptr;
	}

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
	png_get_IHDR(png_ptr, info_ptr, &img_w, &img_h,
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

	// Allocate the row pointers.
	row_pointers = (png_byte**)png_malloc(png_ptr, sizeof(png_byte*) * img_h);
	if (!row_pointers) {
		return nullptr;
	}

	// Initialize the rp_image and the row pointers array.
	img = new rp_image(img_w, img_h, fmt);
	for (int y = img_h-1; y >= 0; y--) {
		row_pointers[y] = reinterpret_cast<png_byte*>(img->scanLine(y));
	}

	// Read the image.
	png_read_image(png_ptr, row_pointers);

	// If CI8, read the palette.
	if (fmt == rp_image::FORMAT_CI8) {
		Read_CI8_Palette(png_ptr, info_ptr, color_type, img);
	}

	// Done reading the PNG image.
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
	// Check the image with pngcheck() first.
	file->rewind();
	int ret = pngcheck(file);
	assert(ret == kOK);
	if (ret != kOK) {
		// PNG image has errors.
		return nullptr;
	}

	// PNG image has been validated.
	file->rewind();
	return loadUnchecked(file);
}

}

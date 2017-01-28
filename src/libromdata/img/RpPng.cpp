/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpPng.cpp: PNG image handler.                                           *
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

#include "config.libromdata.h"

#include "RpPng.hpp"
#include "rp_image.hpp"
#include "IconAnimData.hpp"
#include "../file/RpFile.hpp"
#include "../file/FileSystem.hpp"

// APNG
#include "APNG_dlopen.h"

// C includes. (C++ namespace)
#include <cassert>

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

// pngcheck()
#include "pngcheck/pngcheck.hpp"

namespace LibRomData {

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
		static void png_io_IRpFile_read(png_structp png_ptr, png_bytep data, png_size_t length);

		/**
		 * libpng I/O write handler for IRpFile.
		 * @param png_ptr	[in] PNG pointer.
		 * @param data		[in] Data to write.
		 * @param length	[in] Size of data.
		 */
		static void png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length);

		/**
		 * libpng I/O flush handler for IRpFile.
		 * @param png_ptr	[in] PNG pointer.
		 */
		static void png_io_IRpFile_flush(png_structp png_ptr);

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

		/** Write functions. **/

		/**
		 * Write the palette from a CI8 image.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @param img rp_image containing the palette.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int Write_CI8_Palette(png_structp png_ptr, png_infop info_ptr, const rp_image *img);

		/**
		 * Write a PNG image to an opened PNG handle.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @param img rp_image
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int savePng(png_structp png_ptr, png_infop info_ptr, const rp_image *img);

		/**
		 * Write an APNG image to an opened APNG handle.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @param iconAnimData IconAnimData
		 * @return 0 on success; negative POSIX error code on error.
		 */
		static int saveAPng(png_structp png_ptr, png_infop info_ptr, const IconAnimData *iconAnimData);
};

/** RpPngPrivate **/

/** I/O functions. **/

/**
 * libpng I/O handler for IRpFile.
 * @param png_ptr PNG pointer.
 * @param data		[out] Buffer for the data to read.
 * @param length	[in]  Size of data.
 */
void RpPngPrivate::png_io_IRpFile_read(png_structp png_ptr, png_bytep data, png_size_t length)
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
void RpPngPrivate::png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length)
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
void RpPngPrivate::png_io_IRpFile_flush(png_structp png_ptr)
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
	if (width <= 0 || height <= 0) {
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

	// Allocate the row pointers.
	row_pointers = (png_byte**)png_malloc(png_ptr, sizeof(png_byte*) * height);
	if (!row_pointers) {
		return nullptr;
	}

	// Initialize the row pointers array.
	img = new rp_image(width, height, fmt);
	for (int y = height-1; y >= 0; y--) {
		row_pointers[y] = static_cast<png_byte*>(img->scanLine(y));
	}

	// Read the image.
	png_read_image(png_ptr, row_pointers);
	png_free(png_ptr, row_pointers);

	// If CI8, read the palette.
	if (fmt == rp_image::FORMAT_CI8) {
		Read_CI8_Palette(png_ptr, info_ptr, color_type, img);
	}

	// Done reading the PNG image.
	return img;
}

/** Write functions. **/

/**
 * Write the palette from a CI8 image.
 * @param png_ptr png_structp
 * @param info_ptr png_infop
 * @param img rp_image containing the palette.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngPrivate::Write_CI8_Palette(png_structp png_ptr, png_infop info_ptr, const rp_image *img)
{
	const int num_entries = img->palette_len();
	if (num_entries < 0 || num_entries > 256)
		return -EINVAL;

	// Maximum size.
	png_color png_pal[256];
	uint8_t png_tRNS[256];
	bool has_tRNS = false;

	// Convert the palette.
	const uint32_t *const palette = img->palette();
	for (int i = 0; i < num_entries; i++) {
		png_pal[i].blue  = ( palette[i]        & 0xFF);
		png_pal[i].green = ((palette[i] >> 8)  & 0xFF);
		png_pal[i].red   = ((palette[i] >> 16) & 0xFF);
		png_tRNS[i]      = ((palette[i] >> 24) & 0xFF);
		has_tRNS |= (png_tRNS[i] != 0xFF);
	}

	// Write the PLTE and tRNS chunks.
	png_set_PLTE(png_ptr, info_ptr, png_pal, num_entries);
	if (has_tRNS) {
		// Palette has transparency.
		// Write the tRNS chunk.
		png_set_tRNS(png_ptr, info_ptr, png_tRNS, num_entries, nullptr);
	}

	return 0;
}

/**
 * Write a PNG image to an opened PNG handle.
 * @param png_ptr png_structp
 * @param info_ptr png_infop
 * @param img rp_image
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngPrivate::savePng(png_structp png_ptr, png_infop info_ptr, const rp_image *img)
{
	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const png_byte **row_pointers = nullptr;

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		return -EIO;
	}
#endif

	// Initialize compression parameters.
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
	png_set_compression_level(png_ptr, 5);	// TODO: Customizable?

	const int width = img->width();
	const int height = img->height();

	// Write the PNG header.
	switch (img->format()) {
		case rp_image::FORMAT_ARGB32:
			png_set_IHDR(png_ptr, info_ptr, width, height,
					8, PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
			break;

		case rp_image::FORMAT_CI8:
			png_set_IHDR(png_ptr, info_ptr, width, height,
					8, PNG_COLOR_TYPE_PALETTE,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);

			// Write the palette and tRNS values.
			Write_CI8_Palette(png_ptr, info_ptr, img);
			break;

		default:
			// Unsupported pixel format.
			assert(!"Unsupported rp_image::Format.");
			return -EINVAL;
	}

	// Write the PNG information to the file.
	png_write_info(png_ptr, info_ptr);

	// TODO: Byteswap image data on big-endian systems?
	//ppng_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		return -ENOMEM;
	}

	// Initialize the row pointers array.
	for (int y = height-1; y >= 0; y--) {
		row_pointers[y] = static_cast<const png_byte*>(img->scanLine(y));
	}

	// Write the image data.
	png_write_image(png_ptr, (png_bytepp)row_pointers);
	png_free(png_ptr, row_pointers);

	// Finished writing.
	png_write_end(png_ptr, info_ptr);
	return 0;
}

/**
 * Write an APNG image to an opened APNG handle.
 * @param png_ptr png_structp
 * @param info_ptr png_infop
 * @param iconAnimData IconAnimData
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngPrivate::saveAPng(png_structp png_ptr, png_infop info_ptr, const IconAnimData *iconAnimData)
{
	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const png_byte **row_pointers = nullptr;

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		return -EIO;
	}
#endif

	// Initialize compression parameters.
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
	png_set_compression_level(png_ptr, 5);	// TODO: Customizable?

	// Get the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.
	const rp_image *img0 = iconAnimData->frames[iconAnimData->seq_index[0]];

	const int width = img0->width();
	const int height = img0->height();
	const rp_image::Format format = img0->format();

	// Write the PNG header.
	switch (format) {
		case rp_image::FORMAT_ARGB32:
			png_set_IHDR(png_ptr, info_ptr, width, height,
					8, PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
			break;

		case rp_image::FORMAT_CI8:
			png_set_IHDR(png_ptr, info_ptr, width, height,
					8, PNG_COLOR_TYPE_PALETTE,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);

			// Write the palette and tRNS values.
			// FIXME: Individual palette per frame?
			Write_CI8_Palette(png_ptr, info_ptr, img0);
			break;

		default:
			// Unsupported pixel format.
			assert(!"Unsupported rp_image::Format.");
			return -EINVAL;
	}

	// Write an acTL to indicate that this is an APNG.
	png_set_acTL(png_ptr, info_ptr, iconAnimData->seq_count, 0);

	// Write the PNG information to the file.
	png_write_info(png_ptr, info_ptr);

	// TODO: Byteswap image data on big-endian systems?
	//ppng_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		return -ENOMEM;
	}

	for (int i = 0; i < iconAnimData->seq_count; i++) {
		const rp_image *img = iconAnimData->frames[iconAnimData->seq_index[i]];
		if (!img)
			break;

		// Initialize the row pointers array.
		for (int y = height-1; y >= 0; y--) {
			row_pointers[y] = static_cast<const png_byte*>(img->scanLine(y));
		}

		// Frame header.
		png_write_frame_head(png_ptr, info_ptr, (png_bytepp)row_pointers,
				width, height, 0, 0,		// width, height, x offset, y offset
				iconAnimData->delays[i].numer,
				iconAnimData->delays[i].denom,
				PNG_DISPOSE_OP_NONE,
				PNG_BLEND_OP_SOURCE);

		// Write the image data.
		// TODO: Individual palette for CI8?
		png_write_image(png_ptr, (png_bytepp)row_pointers);

		// Frame tail.
		png_write_frame_tail(png_ptr, info_ptr);
	}

	png_free(png_ptr, row_pointers);

	// Finished writing.
	png_write_end(png_ptr, info_ptr);
	return 0;
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
	file->rewind();
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
	if (!file || !img)
		return -EINVAL;

	// Truncate the file initially.
	int ret = file->truncate(0);
	if (ret != 0) {
		// Cannot truncate the file for some reason.
		return ret;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	file->rewind();

	png_structp png_ptr;
	png_infop info_ptr;

	// Initialize libpng.
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		return -ENOMEM;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, nullptr);
		return -ENOMEM;
	}

	// Initialize the custom I/O handler for IRpFile.
	png_set_write_fn(png_ptr, file,
		RpPngPrivate::png_io_IRpFile_write,
		RpPngPrivate::png_io_IRpFile_flush);

	// Call the actual PNG image writing function.
	ret = RpPngPrivate::savePng(png_ptr, info_ptr, img);

	// Free the PNG structs.
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return ret;
}

/**
 * Save an image in PNG format to a file.
 *
 * @param filename Destination filename.
 * @param img rp_image to save.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPng::save(const rp_char *filename, const rp_image *img)
{
	if (!filename || filename[0] == 0 || !img)
		return -EINVAL;

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_CREATE_WRITE));
	if (!file->isOpen()) {
		// Error opening the file.
		int err = file->lastError();
		if (err == 0)
			err = EIO;
		return -err;
	}

	int ret = save(file.get(), img);
	if (ret != 0) {
		// PNG write failed. Remove the file.
		file.reset();
		FileSystem::delete_file(filename);
	}
	return ret;
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
	if (!file || !iconAnimData)
		return -EINVAL;

	if (iconAnimData->seq_count <= 0) {
		// Nothing to save...
		return -EINVAL;
	}

	// If we have a single image, save it as a regular PNG.
	if (iconAnimData->seq_count == 1) {
		// Single image.
		return save(file, iconAnimData->frames[iconAnimData->seq_index[0]]);
	}

	// Truncate the file initially.
	int ret = file->truncate(0);
	if (ret != 0) {
		// Cannot truncate the file for some reason.
		return ret;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	file->rewind();

	png_structp png_ptr;
	png_infop info_ptr;

	// Initialize libpng.
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		return -ENOMEM;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, nullptr);
		return -ENOMEM;
	}

	// Initialize the custom I/O handler for IRpFile.
	png_set_write_fn(png_ptr, file,
		RpPngPrivate::png_io_IRpFile_write,
		RpPngPrivate::png_io_IRpFile_flush);

	// Call the actual PNG image writing function.
	ret = RpPngPrivate::saveAPng(png_ptr, info_ptr, iconAnimData);

	// Free the PNG structs.
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return ret;
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
int RpPng::save(const rp_char *filename, const IconAnimData *iconAnimData)
{
	if (!filename || filename[0] == 0 || !iconAnimData)
		return -EINVAL;

	if (iconAnimData->seq_count <= 0) {
		// Nothing to save...
		return -EINVAL;
	}

	// If we have a single image, save it as a regular PNG.
	if (iconAnimData->seq_count == 1) {
		// Single image.
		return save(filename, iconAnimData->frames[iconAnimData->seq_index[0]]);
	}

	// Load APNG.
	int apng_ret = APNG_ref();
	if (apng_ret != 0) {
		// Error loading APNG.
		return -ENOTSUP;
	}

	unique_ptr<RpFile> file(new RpFile(filename, RpFile::FM_CREATE_WRITE));
	if (!file->isOpen()) {
		// Error opening the file.
		int err = file->lastError();
		if (err == 0)
			err = EIO;
		return -err;
	}

	int ret = save(file.get(), iconAnimData);
	if (ret != 0) {
		// PNG write failed. Remove the file.
		file.reset();
		FileSystem::delete_file(filename);
	}

	APNG_unref();
	return ret;
}

}

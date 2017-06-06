/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPngWriter.cpp: PNG image writer.                                      *
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

#include "librpbase/config.librpbase.h"
#include "RpPngWriter.hpp"

#include "../common.h"
#include "../file/RpFile.hpp"
#include "../img/rp_image.hpp"

// C includes. (C++ namespace)
#include <cassert>

// libpng
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

class RpPngWriterPrivate
{
	public:
		RpPngWriterPrivate(const rp_char *filename, const rp_image *img);
		RpPngWriterPrivate(IRpFile *file, const rp_image *img);
		~RpPngWriterPrivate();

	private:
		RP_DISABLE_COPY(RpPngWriterPrivate)

	public:
		// Last error value.
		int lastError;

		// Filename. This is used to unlink() the file on error.
		// If the IRpFile constructor was used, this is empty.
		rp_string filename;

		// Open instance of the file.
		// If the filename constructor was used, we own this file.
		// Otherwise, it's a dup()'d instance of the IRpFile.
		IRpFile *file;

		// rp_image
		const rp_image *img;

		// PNG pointers.
		png_structp png_ptr;
		png_infop info_ptr;

		// Current state.
		bool IHDR_written;

	public:
		/**
		 * Initialize the PNG write structs.
		 * This is called by the constructor.
		 * @return 0 on success; non-zero on error.
		 */
		int init_png_write_structs(void);

	public:
		/** I/O functions. **/

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

	public:
		/** Internal functions. **/

		/**
		 * Write the palette from a CI8 image.
		 * @param png_ptr png_structp
		 * @param info_ptr png_infop
		 * @param img rp_image containing the palette.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_CI8_palette(void);
};

/** RpPngWriterPrivate **/

RpPngWriterPrivate::RpPngWriterPrivate(const rp_char *filename, const rp_image *img)
	: lastError(0)
	, file(nullptr)
	, img(img)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	if (!filename || filename[0] == 0 || !img || !img->isValid()) {
		// Invalid parameters.
		lastError = EINVAL;
		return;
	}
	this->filename = filename;

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		lastError = ENOTSUP;
		return;
	}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

	file = new RpFile(filename, RpFile::FM_CREATE_WRITE);
	if (!file->isOpen()) {
		// Unable to open the file.
		lastError = file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		delete file;
		file = nullptr;
		return;
	}

	// Initialize the PNG write structs.
	int ret = init_png_write_structs();
	if (ret != 0) {
		// FIXME: Unlink the file if necessary.
		lastError = -ret;
		delete this->file;
		this->file = nullptr;
	}
}

RpPngWriterPrivate::RpPngWriterPrivate(IRpFile *file, const rp_image *img)
	: lastError(0)
	, file(nullptr)
	, img(img)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	if (!file || !img || !img->isValid()) {
		// Invalid parameters.
		lastError = EINVAL;
		return;
	}

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		lastError = ENOTSUP;
		return;
	}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

	if (!file->isOpen()) {
		// File isn't open.
		lastError = file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		return;
	}

	this->file = file->dup();

	// Truncate the file.
	int ret = this->file->truncate(0);
	if (ret != 0) {
		// Unable to truncate the file.
		lastError = this->file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	this->file->rewind();

	// Initialize the PNG write structs.
	ret = init_png_write_structs();
	if (ret != 0) {
		// FIXME: Unlink the file if necessary.
		lastError = -ret;
		delete this->file;
		this->file = nullptr;
	}
}

RpPngWriterPrivate::~RpPngWriterPrivate()
{
	if (png_ptr || info_ptr) {
		// PNG structs are still present...
		// TODO: Unlink the PNG file, since it
		// wasn't written correctly.
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}

	// Close the IRpFile.
	delete this->file;
}

/**
 * Initialize the PNG write structs.
 * This is called by the constructor.
 * @return 0 on success; non-zero on error.
 */
int RpPngWriterPrivate::init_png_write_structs(void)
{
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
		png_io_IRpFile_write,
		png_io_IRpFile_flush);
	return 0;
}

/**
 * libpng I/O write handler for IRpFile.
 * @param png_ptr	[in] PNG pointer.
 * @param data		[in] Data to write.
 * @param length	[in] Size of data.
 */
void RpPngWriterPrivate::png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length)
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
void RpPngWriterPrivate::png_io_IRpFile_flush(png_structp png_ptr)
{
	// Assuming io_ptr is an IRpFile*.
	IRpFile *file = static_cast<IRpFile*>(png_get_io_ptr(png_ptr));
	if (!file)
		return;

	// TODO: IRpFile::flush()
}

/**
 * Write the palette from a CI8 image.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_CI8_palette(void)
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

/** RpPngWriter **/

/**
 * Write an image to a PNG file.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * @param filename	[in] Filename.
 * @param img		[in] rp_image.
 */
RpPngWriter::RpPngWriter(const rp_char *filename, const rp_image *img)
	: d_ptr(new RpPngWriterPrivate(filename, img))
{ }

/**
 * Write an image to a PNG file.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * @param file	[in] IRpFile open for writing.
 * @param img	[in] rp_image.
 */
RpPngWriter::RpPngWriter(IRpFile *file, const rp_image *img)
	: d_ptr(new RpPngWriterPrivate(file, img))
{ }

RpPngWriter::~RpPngWriter()
{
	delete d_ptr;
}

/**
 * Is the PNG file open?
 * @return True if the PNG file is open; false if not.
 */
bool RpPngWriter::isOpen(void) const
{
	RP_D(const RpPngWriter);
	return (d->file != nullptr);
}

/**
 * Get the last error.
 * @return Last POSIX error, or 0 if no error.
 */
int RpPngWriter::lastError(void) const
{
	RP_D(const RpPngWriter);
	return d->lastError;
}

/**
 * Write the PNG IHDR.
 * This must be called before writing any other image data.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_IHDR(void)
{
	RP_D(RpPngWriter);
	assert(d->file != nullptr);
	assert(d->img != nullptr);
	assert(!d->IHDR_written);
	if (!d->file || !d->img) {
		// Invalid state.
		d->lastError = EIO;
		return -d->lastError;
	}
	if (d->IHDR_written) {
		// IHDR has already been written.
		d->lastError = EEXIST;
		return -d->lastError;
	}

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(d->png_ptr))) {
		// PNG write failed.
		d->lastError = EIO;
		return -d->lastError;
	}
#endif

	// Initialize compression parameters.
	png_set_filter(d->png_ptr, 0, PNG_FILTER_NONE);
	png_set_compression_level(d->png_ptr, 5);	// TODO: Customizable?

	// Write the PNG header.
	switch (d->img->format()) {
		case rp_image::FORMAT_ARGB32:
			png_set_IHDR(d->png_ptr, d->info_ptr,
					d->img->width(), d->img->height(),
					8, PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
			break;

		case rp_image::FORMAT_CI8:
			png_set_IHDR(d->png_ptr, d->info_ptr,
					d->img->width(), d->img->height(),
					8, PNG_COLOR_TYPE_PALETTE,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);

			// Write the palette and tRNS values.
			d->write_CI8_palette();
			break;

		default:
			// Unsupported pixel format.
			assert(!"Unsupported rp_image::Format.");
			d->lastError = EINVAL;
			return -d->lastError;
	}

	// Write the PNG information to the file.
	png_write_info(d->png_ptr, d->info_ptr);
	d->IHDR_written = true;
	return 0;
}

/**
 * Write the rp_image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * If constructed using a filename instead of IRpFile,
 * this will automatically close the file.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_IDAT(void)
{
	RP_D(RpPngWriter);
	assert(d->file != nullptr);
	assert(d->img != nullptr);
	assert(d->IHDR_written);
	if (!d->file || !d->img) {
		// Invalid state.
		d->lastError = EIO;
		return -d->lastError;
	}
	if (!d->IHDR_written) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		d->lastError = EIO;
		return -d->lastError;
	}

	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const int height = d->img->height();
	const png_byte **row_pointers = nullptr;

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(d->png_ptr))) {
		// PNG read failed.
		png_free(d->png_ptr, row_pointers);
		return -EIO;
	}
#endif

	// TODO: Byteswap image data on big-endian systems?
	//ppng_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(d->png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(d->png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		d->lastError = ENOMEM;
		return -d->lastError;
	}

	// Initialize the row pointers array.
	for (int y = height-1; y >= 0; y--) {
		row_pointers[y] = static_cast<const png_byte*>(d->img->scanLine(y));
	}

	// Write the image data.
	png_write_image(d->png_ptr, (png_bytepp)row_pointers);
	png_free(d->png_ptr, row_pointers);
	row_pointers = nullptr;

	// Finished writing.
	png_write_end(d->png_ptr, d->info_ptr);

	// Free the PNG structs and close the file.
	png_destroy_write_struct(&d->png_ptr, &d->info_ptr);
	delete d->file;
	d->file = nullptr;
	return 0;
}

}

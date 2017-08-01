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

// APNG
#include "../img/IconAnimData.hpp"
#include "APNG_dlopen.h"

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

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

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
		RpPngWriterPrivate(const rp_char *filename, const IconAnimData *iconAnimData);
		RpPngWriterPrivate(IRpFile *file, const IconAnimData *iconAnimData);
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

		// Image and/or animated image data to save.
		bool isAnimated;
		union {
			const rp_image *img;
			const IconAnimData *iconAnimData;
		};

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
		int write_IDAT(void);

		/**
		 * Write the animated image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * If constructed using a filename instead of IRpFile,
		 * this will automatically close the file.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT_APNG(void);
};

/** RpPngWriterPrivate **/

RpPngWriterPrivate::RpPngWriterPrivate(const rp_char *filename, const rp_image *img)
	: lastError(0)
	, file(nullptr)
	, isAnimated(false)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->img = img;
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
	, isAnimated(false)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->img = img;
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

RpPngWriterPrivate::RpPngWriterPrivate(const rp_char *filename, const IconAnimData *iconAnimData)
	: lastError(0)
	, file(nullptr)
	, isAnimated(false)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->iconAnimData = nullptr;
	if (!filename || filename[0] == 0 || !iconAnimData || iconAnimData->seq_count <= 0) {
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

	if (iconAnimData->seq_count > 1) {
		// Load APNG.
		int ret = APNG_ref();
		if (ret != 0) {
			// Error loading APNG.
			lastError = ENOTSUP;
			return;
		}
		this->isAnimated = true;
	}

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

	// Set img or iconAnimData.
	if (this->isAnimated) {
		this->iconAnimData = iconAnimData;
	} else {
		this->img = iconAnimData->frames[iconAnimData->seq_index[0]];
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

RpPngWriterPrivate::RpPngWriterPrivate(IRpFile *file, const IconAnimData *iconAnimData)
	: lastError(0)
	, file(nullptr)
	, isAnimated(false)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->iconAnimData = nullptr;
	if (!file || !iconAnimData || iconAnimData->seq_count <= 0) {
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

	if (iconAnimData->seq_count > 1) {
		// Load APNG.
		int ret = APNG_ref();
		if (ret != 0) {
			// Error loading APNG.
			lastError = ENOTSUP;
			return;
		}
		this->isAnimated = true;
	}

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

	// Set img or iconAnimData.
	if (this->isAnimated) {
		this->iconAnimData = iconAnimData;
	} else {
		this->img = iconAnimData->frames[iconAnimData->seq_index[0]];
	}

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

	if (this->isAnimated) {
		// Unreference APNG.
		APNG_unref();
	}
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
	// Get the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.
	// Also, does PNG support separate palettes per frame?
	// If not, the frames may need to be converted to ARGB32.
	const rp_image *const img0 = (this->isAnimated
		? this->iconAnimData->frames[iconAnimData->seq_index[0]]
		: this->img);

	const int num_entries = img0->palette_len();
	if (num_entries < 0 || num_entries > 256)
		return -EINVAL;

	// Maximum size.
	png_color png_pal[256];
	uint8_t png_tRNS[256];
	bool has_tRNS = false;

	// Convert the palette.
	const uint32_t *const palette = img0->palette();
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
 * Write the rp_image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * If constructed using a filename instead of IRpFile,
 * this will automatically close the file.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_IDAT(void)
{
	assert(file != nullptr);
	assert(img != nullptr);
	assert(!isAnimated);
	assert(IHDR_written);
	if (!file || !img || isAnimated) {
		// Invalid state.
		lastError = EIO;
		return -lastError;
	}
	if (!IHDR_written) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		lastError = EIO;
		return -lastError;
	}

	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const int height = img->height();
	const png_byte **row_pointers = nullptr;

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		return -EIO;
	}
#endif

	// TODO: Byteswap image data on big-endian systems?
	//png_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		lastError = ENOMEM;
		return -lastError;
	}

	// Initialize the row pointers array.
	for (int y = height-1; y >= 0; y--) {
		row_pointers[y] = static_cast<const png_byte*>(img->scanLine(y));
	}

	// Write the image data.
	png_write_image(png_ptr, (png_bytepp)row_pointers);
	png_free(png_ptr, row_pointers);
	row_pointers = nullptr;

	// Finished writing.
	png_write_end(png_ptr, info_ptr);

	// Free the PNG structs and close the file.
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete file;
	file = nullptr;
	return 0;
}

/**
 * Write the animated image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * If constructed using a filename instead of IRpFile,
 * this will automatically close the file.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_IDAT_APNG(void)
{
	assert(file != nullptr);
	assert(img != nullptr);
	assert(isAnimated);
	assert(IHDR_written);
	if (!file || !img || !isAnimated) {
		// Invalid state.
		lastError = EIO;
		return -lastError;
	}
	if (!IHDR_written) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		lastError = EIO;
		return -lastError;
	}

	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const png_byte **row_pointers = nullptr;

	// Get the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.
	const rp_image *const img0 = (this->isAnimated
		? this->iconAnimData->frames[iconAnimData->seq_index[0]]
		: this->img);
	const int width = img0->width();
	const int height = img0->height();

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		return -EIO;
	}
#endif

	// TODO: Byteswap image data on big-endian systems?
	//ppng_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * height);
	if (!row_pointers) {
		lastError = ENOMEM;
		return -lastError;
	}

	// Write the images.
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
	row_pointers = nullptr;

	// Finished writing.
	png_write_end(png_ptr, info_ptr);

	// Free the PNG structs and close the file.
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete file;
	file = nullptr;
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
 * IRpFile must be open for writing.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file	[in] IRpFile open for writing.
 * @param img	[in] rp_image.
 */
RpPngWriter::RpPngWriter(IRpFile *file, const rp_image *img)
	: d_ptr(new RpPngWriterPrivate(file, img))
{ }

/**
 * Write an animated image to an APNG file.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * set as the last error. The caller should then save
 * the image as a standard PNG file.
 *
 * @param file		[in] IRpFile open for writing.
 * @param iconAnimData	[in] Animated image data.
 */
RpPngWriter::RpPngWriter(const rp_char *filename, const IconAnimData *iconAnimData)
	: d_ptr(new RpPngWriterPrivate(filename, iconAnimData))
{ }

/**
 * Write an animated image to an APNG file.
 * IRpFile must be open for writing.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * If the animated image contains a single frame,
 * a standard PNG image will be written.
 *
 * NOTE: If the image has multiple frames and APNG
 * write support is unavailable, -ENOTSUP will be
 * set as the last error. The caller should then save
 * the image as a standard PNG file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file		[in] IRpFile open for writing.
 * @param iconAnimData	[in] Animated image data.
 */
RpPngWriter::RpPngWriter(IRpFile *file, const IconAnimData *iconAnimData)
	: d_ptr(new RpPngWriterPrivate(file, iconAnimData))
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

	// Get the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.
	const rp_image *const img0 = (d->isAnimated
		? d->iconAnimData->frames[d->iconAnimData->seq_index[0]]
		: d->img);

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
	switch (img0->format()) {
		case rp_image::FORMAT_ARGB32: {
			png_set_IHDR(d->png_ptr, d->info_ptr,
					img0->width(), img0->height(), 8,
					PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
			break;
		}

		case rp_image::FORMAT_CI8:
			png_set_IHDR(d->png_ptr, d->info_ptr,
					img0->width(), img0->height(), 8,
					PNG_COLOR_TYPE_PALETTE,
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

	if (d->isAnimated) {
		// Write an acTL chunk to indicate that this is an APNG image.
		png_set_acTL(d->png_ptr, d->info_ptr, d->iconAnimData->seq_count, 0);
	}

#ifdef PNG_sBIT_SUPPORTED
	// Get the rp_image's sBIT data.
	// If alpha == 0, we can write RGB and/or skip tRNS.
	rp_image::sBIT_t sBIT;
	bool has_sBIT = (img0->get_sBIT(&sBIT) == 0);
	if (has_sBIT) {
		// Write the sBIT chunk.
		// NOTE: rp_image::sBIT_t has the same format as png_color_8.
		png_set_sBIT(d->png_ptr, d->info_ptr, reinterpret_cast<const png_color_8*>(&sBIT));
	}
#endif /* PNG_sBIT_SUPPORTED */

	// Write the PNG information to the file.
	png_write_info(d->png_ptr, d->info_ptr);
	d->IHDR_written = true;
	return 0;
}

#ifdef RP_ENABLE_WRITE_TEXT
/**
 * Write an array of text chunks.
 * This is needed for e.g. the XDG thumbnailing specification.
 *
 * NOTE: This currently only supports Latin-1 strings.
 * FIXME: Untested!
 *
 * @param kv_vector Vector of key/value pairs.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_tEXt(const kv_vector &kv)
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

	if (kv.empty()) {
		// Empty vector...
		return 0;
	}

	// Allocate string pointer arrays for kv_vector.
	// Since kv_vector is Latin-1, we don't have to
	// strdup() the strings.
	// TODO: unique_ptr<>?
	png_text *text = new png_text[kv.size()];
	png_text *pTxt = text;
	for (unsigned int i = 0; i < kv.size(); i++, pTxt++) {
		pTxt->compression = PNG_TEXT_COMPRESSION_NONE;
		pTxt->key = (png_charp)kv[i].first.c_str();
		pTxt->text = (png_charp)kv[i].second.c_str();
	}

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(d->png_ptr))) {
		// PNG write failed.
		delete[] text;
		d->lastError = EIO;
		return -d->lastError;
	}
#endif

	png_set_text(d->png_ptr, d->info_ptr, text, kv.size());
	delete[] text;
	return 0;
}
#endif /* RP_ENABLE_WRITE_TEXT */

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
	if (d->isAnimated) {
		// Write an animated PNG image.
		// NOTE: d->isAnimated is only set if APNG is loaded,
		// so we don't have to check it again here.
		return d->write_IDAT_APNG();
	} else {
		// Write a regular PNG image.
		return d->write_IDAT();
	}
}

}

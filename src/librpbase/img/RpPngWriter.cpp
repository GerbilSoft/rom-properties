/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpPngWriter.cpp: PNG image writer.                                      *
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

#include "librpbase/config.librpbase.h"
#include "RpPngWriter.hpp"

#include "common.h"
#include "byteorder.h"
#include "TextFuncs.hpp"
#include "file/RpFile.hpp"

// APNG
#include "img/IconAnimData.hpp"
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

// PNGCAPI was added in libpng-1.5.0beta14.
// Older versions will need this.
#ifndef PNGCAPI
# ifdef _MSC_VER
#  define PNGCAPI __cdecl
# else
#  define PNGCAPI
# endif
#endif /* !PNGCAPI */

// libpng-1.5.0beta42 marked several function parameters as const.
// Older versions don't have them marked as const, but they're
// effectively const anyway.
#if PNG_LIBPNG_VER < 10500 || \
    (PNG_LIBPNG_VER == 10500 && \
        ((PNG_LIBPNG_BUILD_BASE_TYPE & PNG_LIBPNG_BUILD_RELEASE_STATUS_MASK) < PNG_LIBPNG_BUILD_BETA || \
            ((PNG_LIBPNG_BUILD_BASE_TYPE & PNG_LIBPNG_BUILD_RELEASE_STATUS_MASK) == PNG_LIBPNG_BUILD_BETA && PNG_LIBPNG_VER_BUILD < 42)))
// libpng-1.5.0beta41 or earlier: Functions do *not* have const pointer arguments.
# define PNG_CONST_CAST(type) const_cast<type>
#else
// libpng-1.5.0beta42: Functions have const pointer arguments.
# define PNG_CONST_CAST(type)
#endif

// libpng-1.5 doesn't define PNG_Z_DEFAULT_COMPRESSION.
#ifndef PNG_Z_DEFAULT_COMPRESSION
# define PNG_Z_DEFAULT_COMPRESSION (-1)
#endif

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <csetjmp>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

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
		// NOTE: The public class constructor must dup() file.
		RpPngWriterPrivate(IRpFile *file, int width, int height, rp_image::Format format);
		RpPngWriterPrivate(IRpFile *file, const rp_image *img);
		RpPngWriterPrivate(IRpFile *file, const IconAnimData *iconAnimData);
		~RpPngWriterPrivate();

	private:
		RP_DISABLE_COPY(RpPngWriterPrivate)

	public:
		// Last error value.
		int lastError;

		// Open instance of the file.
		// This is a dup()'d instance from the public class constructor.
		IRpFile *file;

		// Image and/or animated image data to save.
		enum {
			IMGT_INVALID = 0,	// Invalid image.
			IMGT_RAW,		// Raw image.
			IMGT_RP_IMAGE,		// rp_image
			IMGT_ICONANIMDATA,	// iconAnimData
		} imageTag;
		union {
			const rp_image *img;
			const IconAnimData *iconAnimData;
		};

		// Cached width, height, and image format.
		struct cache_t {
			int width;
			int height;
			rp_image::Format format;

			// Palette for CI8 images.
			const uint32_t *palette;
			int palette_len;

#ifdef PNG_sBIT_SUPPORTED
			// sBIT data. If we have sBIT, and alpha == 0,
			// we'll skip saving the alpha channel.
			bool has_sBIT;
			bool skip_alpha;
			rp_image::sBIT_t sBIT;
#endif /* PNG_sBIT_SUPPORTED */

			cache_t()
				: width(0)
				, height(0)
				, format(rp_image::FORMAT_NONE)
				, palette(nullptr)
				, palette_len(0)
			{
#ifdef PNG_sBIT_SUPPORTED
				has_sBIT = false;
				skip_alpha = false;
				memset(&sBIT, 0, sizeof(sBIT));
#endif /* PNG_sBIT_SUPPORTED */
			}

			void setFrom(const rp_image *img) {
				this->width = img->width();
				this->height = img->height();
				this->format = img->format();
				if (this->format == rp_image::FORMAT_CI8) {
					// Get the palette.
					this->palette = img->palette();
					this->palette_len = img->palette_len();
				}
#ifdef PNG_sBIT_SUPPORTED
				// Get the rp_image's sBIT data.
				// If alpha == 0, we can write RGB and/or skip tRNS.
				has_sBIT = (img->get_sBIT(&sBIT) == 0);
				skip_alpha = (has_sBIT && sBIT.alpha == 0);
#endif /* PNG_sBIT_SUPPORTED */
			}

			void set_sBIT(const rp_image::sBIT_t* sBIT)
			{
				static const rp_image::sBIT_t sBIT_invalid = {0,0,0,0,0};
				if (sBIT && memcmp(sBIT, &sBIT_invalid, sizeof(*sBIT)) != 0) {
					// sBIT is valid.
					this->sBIT = *sBIT;
					has_sBIT = true;
					skip_alpha = (has_sBIT && this->sBIT.alpha == 0);
				} else {
					// No sBIT, or sBIT is invalid.
					has_sBIT = false;
					skip_alpha = false;
				}
			}
		};
		cache_t cache;

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

		// Close the PNG image.
		void close(void);

	public:
		/** I/O functions. **/

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

	public:
		/** Internal functions. **/

		/**
		 * Write the palette from a CI8 image.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_CI8_palette(void);

		/**
		 * Write raw image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * NOTE: This will automatically close the file.
		 * TODO: Keep it open so we can write text after IDAT?
		 *
		 * @param row_pointers PNG row pointers. Array must have cache.height elements.
		 * @param is_abgr If true, image data is ABGR instead of ARGB.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT(const png_byte *const *row_pointers, bool is_abgr = false);

		/**
		 * Write the rp_image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * NOTE: This will automatically close the file.
		 * TODO: Keep it open so we can write text after IDAT?
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT(void);

		/**
		 * Write the animated image data to the PNG image.
		 *
		 * This must be called after any other modifier functions.
		 *
		 * NOTE: This will automatically close the file.
		 * TODO: Keep it open so we can write text after IDAT?
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int write_IDAT_APNG(void);
};

/** RpPngWriterPrivate **/

RpPngWriterPrivate::RpPngWriterPrivate(IRpFile *file, int width, int height, rp_image::Format format)
	: lastError(0)
	, file(file)
	, imageTag(IMGT_INVALID)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->img = nullptr;
	if (!file || width <= 0 || height <= 0 ||
	    (format != rp_image::FORMAT_CI8 && format != rp_image::FORMAT_ARGB32))
	{
		// Invalid parameters.
		delete this->file;
		this->file = nullptr;
		lastError = EINVAL;
		return;
	}

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		delete this->file;
		this->file = nullptr;
		lastError = ENOTSUP;
		return;
	}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

	if (!file || !file->isOpen()) {
		// File isn't open.
		lastError = (file ? file->lastError() : 0);
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncate the file.
	int ret = file->truncate(0);
	if (ret != 0) {
		// Unable to truncate the file.
		lastError = file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	file->rewind();

	// Initialize the PNG write structs.
	ret = init_png_write_structs();
	if (ret != 0) {
		// FIXME: Unlink the file if necessary.
		lastError = -ret;
		delete this->file;
		this->file = nullptr;
	}

	// Cache the image parameters.
	// NOTE: sBIT is specified in write_IHDR().
	imageTag = IMGT_RAW;
	cache.width = width;
	cache.height = height;
	cache.format = format;
}

RpPngWriterPrivate::RpPngWriterPrivate(IRpFile *file, const rp_image *img)
	: lastError(0)
	, file(file)
	, imageTag(IMGT_INVALID)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->img = img;
	if (!file || !img || !img->isValid()) {
		// Invalid parameters.
		delete this->file;
		this->file = nullptr;
		lastError = EINVAL;
		return;
	}

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		delete this->file;
		this->file = nullptr;
		lastError = ENOTSUP;
		return;
	}
#endif /* defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL)) */

	if (!file || !file->isOpen()) {
		// File isn't open.
		lastError = (file ? file->lastError() : 0);
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncate the file.
	int ret = file->truncate(0);
	if (ret != 0) {
		// Unable to truncate the file.
		lastError = file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	file->rewind();

	// Initialize the PNG write structs.
	ret = init_png_write_structs();
	if (ret != 0) {
		// FIXME: Unlink the file if necessary.
		lastError = -ret;
		delete this->file;
		this->file = nullptr;
	}

	// Cache the image parameters.
	imageTag = IMGT_RP_IMAGE;
	cache.setFrom(img);
}

RpPngWriterPrivate::RpPngWriterPrivate(IRpFile *file, const IconAnimData *iconAnimData)
	: lastError(0)
	, file(file)
	, imageTag(IMGT_INVALID)
	, png_ptr(nullptr)
	, info_ptr(nullptr)
	, IHDR_written(false)
{
	this->iconAnimData = nullptr;
	if (!file || !iconAnimData || iconAnimData->seq_count <= 0) {
		// Invalid parameters.
		delete this->file;
		this->file = nullptr;
		lastError = EINVAL;
		return;
	}

#if defined(_MSC_VER) && (defined(ZLIB_IS_DLL) || defined(PNG_IS_DLL))
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_zlib_and_png() != 0) {
		// Delay load failed.
		delete this->file;
		this->file = nullptr;
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
		imageTag = IMGT_ICONANIMDATA;
	} else {
		imageTag = IMGT_RP_IMAGE;
	}

	if (!file || !file->isOpen()) {
		// File isn't open.
		lastError = (file ? file->lastError() : 0);
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncate the file.
	int ret = file->truncate(0);
	if (ret != 0) {
		// Unable to truncate the file.
		lastError = file->lastError();
		if (lastError == 0) {
			lastError = EIO;
		}
		delete this->file;
		this->file = nullptr;
		return;
	}

	// Truncation should automatically rewind,
	// but let's do it anyway.
	file->rewind();

	// Set img or iconAnimData.
	if (imageTag == IMGT_ICONANIMDATA) {
		this->iconAnimData = iconAnimData;
		// Cache the image parameters.
		const rp_image *img0 = iconAnimData->frames[iconAnimData->seq_index[0]];
		assert(img0 != nullptr);
		if (unlikely(!img0)) {
			// Invalid animated image.
			delete this->file;
			this->file = nullptr;
			lastError = EINVAL;
			imageTag = IMGT_INVALID;
		}
		cache.setFrom(img0);
	} else {
		this->img = iconAnimData->frames[iconAnimData->seq_index[0]];
		cache.setFrom(img);
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
	this->close();

	if (imageTag == IMGT_ICONANIMDATA) {
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
 * Close the PNG image.
 */
void RpPngWriterPrivate::close(void)
{
	// Close libpng.
	if (png_ptr || info_ptr) {
		// If PNG write failed, png_write_end()
		// may call longjmp().
#ifdef PNG_SETJMP_SUPPORTED
		if (setjmp(png_jmpbuf(png_ptr))) {
			// PNG write failed.
			// TODO: unlink()?
		} else
#endif /* PNG_SETJMP_SUPPORTED */
		{
			// Attempt to finish writing the PNG file.
			png_write_end(png_ptr, info_ptr);
		}

		png_destroy_write_struct(&png_ptr, &info_ptr);
		png_ptr = nullptr;
		info_ptr = nullptr;
	}

	// Close the file.
	delete this->file;
	this->file = nullptr;
}

/**
 * libpng I/O write handler for IRpFile.
 * @param png_ptr	[in] PNG pointer.
 * @param data		[in] Data to write.
 * @param length	[in] Size of data.
 */
void PNGCAPI RpPngWriterPrivate::png_io_IRpFile_write(png_structp png_ptr, png_bytep data, png_size_t length)
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
void PNGCAPI RpPngWriterPrivate::png_io_IRpFile_flush(png_structp png_ptr)
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
	assert(cache.format == rp_image::FORMAT_CI8);
	if (unlikely(cache.format != rp_image::FORMAT_CI8)) {
		// Not a CI8 image.
		return -EINVAL;
	}

	// Using the cached palette from the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.
	// Also, does PNG support separate palettes per frame?
	// If not, the frames may need to be converted to ARGB32.
	if (cache.palette_len <= 0 || cache.palette_len > 256)
		return -EINVAL;

	// Maximum size.
	png_color png_pal[256];
	uint8_t png_tRNS[256];
	bool has_tRNS = false;

	// Convert the palette.
	const argb32_t *p_img_pal = reinterpret_cast<const argb32_t*>(cache.palette);
	png_color *p_png_pal = png_pal;
	uint8_t *p_png_tRNS = png_tRNS;
	for (int i = cache.palette_len; i > 0; i--, p_img_pal++, p_png_pal++, p_png_tRNS++) {
		// NOTE: Shifting method is actually more
		// efficient on gcc, but MSVC handles both
		// the same as gcc with argb32_t. (movzx)
		p_png_pal->blue  = p_img_pal->b;
		p_png_pal->green = p_img_pal->g;
		p_png_pal->red   = p_img_pal->r;
		*p_png_tRNS      = p_img_pal->a;
		has_tRNS |= (*p_png_tRNS != 0xFF);
	}

	// Write the PLTE and tRNS chunks.
	png_set_PLTE(png_ptr, info_ptr, png_pal, cache.palette_len);
	if (has_tRNS) {
		// Palette has transparency.
		// Write the tRNS chunk.
		// NOTE: Ignoring skip_alpha here, since it doesn't make
		// sense to skip for paletted images.
		png_set_tRNS(png_ptr, info_ptr, png_tRNS, cache.palette_len, nullptr);
	}
	return 0;
}

/**
 * Write raw image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * NOTE: This will automatically close the file.
 * TODO: Keep it open so we can write text after IDAT?
 *
 * @param row_pointers PNG row pointers. Array must have cache.height elements.
 * @param is_abgr If true, image data is ABGR instead of ARGB.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_IDAT(const png_byte *const *row_pointers, bool is_abgr)
{
	assert(file != nullptr);
	assert(imageTag == IMGT_RAW || imageTag == IMGT_RP_IMAGE);
	assert(IHDR_written);
	if (unlikely(!file || (imageTag != IMGT_RAW && imageTag != IMGT_RP_IMAGE))) {
		// Invalid state.
		lastError = EIO;
		return -lastError;
	}
	if (unlikely(!IHDR_written)) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		lastError = EIO;
		return -lastError;
	}

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		return -EIO;
	}
#endif /* PNG_SETJMP_SUPPORTED */

	// TODO: Byteswap image data on big-endian systems?
	//png_set_swap(png_ptr);

	// TODO: What format on big-endian?
	if (!is_abgr) {
		png_set_bgr(png_ptr);
	}

	if (cache.skip_alpha && cache.format == rp_image::FORMAT_ARGB32) {
		// Need to skip the alpha bytes.
		// Assuming 'after' on LE, 'before' on BE.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		static const int flags = PNG_FILLER_AFTER;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		static const int flags = PNG_FILLER_BEFORE;
#endif
		png_set_filler(png_ptr, 0xFF, flags);
	}

	// Write the image data.
	png_write_image(png_ptr, const_cast<png_bytepp>(row_pointers));
	return 0;
}

/**
 * Write the rp_image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * NOTE: This will automatically close the file.
 * TODO: Keep it open so we can write text after IDAT?
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_IDAT(void)
{
	assert(file != nullptr);
	assert(img != nullptr);
	assert(imageTag == IMGT_RP_IMAGE);
	assert(IHDR_written);
	if (unlikely(!file || !img || imageTag != IMGT_RP_IMAGE)) {
		// Invalid state.
		lastError = EIO;
		return -lastError;
	}
	if (unlikely(!IHDR_written)) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		lastError = EIO;
		return -lastError;
	}

	// Allocate the row pointers.
	const png_byte **row_pointers = static_cast<const png_byte**>(
		png_malloc(png_ptr, sizeof(const png_byte*) * cache.height));
	if (!row_pointers) {
		lastError = ENOMEM;
		return -lastError;
	}

	// Initialize the row pointers array.
	for (int y = cache.height-1; y >= 0; y--) {
		row_pointers[y] = static_cast<const png_byte*>(img->scanLine(y));
	}

	// Write the image data.
	int ret = write_IDAT(row_pointers);
	// Free the row pointers.
	png_free(png_ptr, row_pointers);
	return ret;
}

/**
 * Write the animated image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * NOTE: This will automatically close the file.
 * TODO: Keep it open so we can write text after IDAT?
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriterPrivate::write_IDAT_APNG(void)
{
	assert(file != nullptr);
	assert(img != nullptr);
	assert(imageTag == IMGT_ICONANIMDATA);
	assert(IHDR_written);
	if (unlikely(!file || !img || imageTag != IMGT_ICONANIMDATA)) {
		// Invalid state.
		lastError = EIO;
		return -lastError;
	}
	if (unlikely(!IHDR_written)) {
		// IHDR has not been written yet.
		// TODO: Better error code?
		lastError = EIO;
		return -lastError;
	}

	// Row pointers. (NOTE: Allocated after IHDR is written.)
	const png_byte **row_pointers = nullptr;

	// Using the cached width/height from the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG read failed.
		png_free(png_ptr, row_pointers);
		return -EIO;
	}
#endif /* PNG_SETJMP_SUPPORTED */

	// TODO: Byteswap image data on big-endian systems?
	//ppng_set_swap(png_ptr);
	// TODO: What format on big-endian?
	png_set_bgr(png_ptr);

	// Allocate the row pointers.
	row_pointers = (const png_byte**)png_malloc(png_ptr, sizeof(const png_byte*) * cache.height);
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
		for (int y = cache.height-1; y >= 0; y--) {
			row_pointers[y] = static_cast<const png_byte*>(img->scanLine(y));
		}

		// Frame header.
		png_write_frame_head(png_ptr, info_ptr, (png_bytepp)row_pointers,
				cache.width, cache.height, 0, 0,	// width, height, x offset, y offset
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
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param filename	[in] Filename.
 * @param img		[in] rp_image.
 */
RpPngWriter::RpPngWriter(const char *filename, const rp_image *img)
	: d_ptr(new RpPngWriterPrivate(
		(filename ? new RpFile(filename, RpFile::FM_CREATE_WRITE) : nullptr), img))
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
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file	[in] IRpFile open for writing.
 * @param img	[in] rp_image.
 */
RpPngWriter::RpPngWriter(IRpFile *file, const rp_image *img)
	: d_ptr(new RpPngWriterPrivate(
		(file ? file->dup() : nullptr), img))
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
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file		[in] IRpFile open for writing.
 * @param iconAnimData	[in] Animated image data.
 */
RpPngWriter::RpPngWriter(const char *filename, const IconAnimData *iconAnimData)
	: d_ptr(new RpPngWriterPrivate(
		(filename ? new RpFile(filename, RpFile::FM_CREATE_WRITE) : nullptr), iconAnimData))
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
	: d_ptr(new RpPngWriterPrivate(
		(file ? file->dup() : nullptr), iconAnimData))
{ }

/**
 * Write a raw image to a PNG file.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param filename	[in] Filename.
 * @param width 	[in] Image width.
 * @param height 	[in] Image height.
 * @param format 	[in] Image format.
 */
RpPngWriter::RpPngWriter(const char *filename, int width, int height, rp_image::Format format)
	: d_ptr(new RpPngWriterPrivate(
		(filename ? new RpFile(filename, RpFile::FM_CREATE_WRITE) : nullptr), width, height, format))
{ }

/**
 * Write a raw image to a PNG file.
 * IRpFile must be open for writing.
 *
 * Check isOpen() after constructing to verify that
 * the file was opened.
 *
 * NOTE: If the write fails, the caller will need
 * to delete the file.
 *
 * NOTE 2: If the write fails, the caller will need
 * to delete the file.
 *
 * @param file	[in] IRpFile open for writing.
 * @param width 	[in] Image width.
 * @param height 	[in] Image height.
 * @param format 	[in] Image format.
 */
RpPngWriter::RpPngWriter(IRpFile *file, int width, int height, rp_image::Format format)
	: d_ptr(new RpPngWriterPrivate(
		(file ? file->dup() : nullptr), width, height, format))
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
 * Close the PNG file.
 */
void RpPngWriter::close(void)
{
	RP_D(RpPngWriter);
	d->close();
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
	assert(!d->IHDR_written);
	if (unlikely(!d->file)) {
		// Invalid state.
		d->lastError = EIO;
		return -d->lastError;
	}
	if (unlikely(d->IHDR_written)) {
		// IHDR has already been written.
		d->lastError = EEXIST;
		return -d->lastError;
	}

	// Using the cached width/height from the first image.
	// TODO: Handle animated images where the different frames
	// have different widths, heights, and/or formats.

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(d->png_ptr))) {
		// PNG write failed.
		d->lastError = EIO;
		return -d->lastError;
	}
#endif /* PNG_SETJMP_SUPPORTED */

	// Initialize compression parameters.
	// TODO: Customizable?
	png_set_filter(d->png_ptr, 0, PNG_FILTER_NONE);
	png_set_compression_level(d->png_ptr, PNG_Z_DEFAULT_COMPRESSION);

	// Write the PNG header.
	switch (d->cache.format) {
		case rp_image::FORMAT_ARGB32: {
			// TODO: Use PNG_COLOR_TYPE_GRAY and/or PNG_COLOR_TYPE_GRAY_ALPHA
			// if sBIT.gray > 0?
#ifdef PNG_sBIT_SUPPORTED
			const int color_type = (d->cache.skip_alpha ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA);
#else /* !PNG_sBIT_SUPPORTED */
			static const int color_type = PNG_COLOR_TYPE_RGB_ALPHA;
#endif /* PNG_sBIT_SUPPORTED */
			png_set_IHDR(d->png_ptr, d->info_ptr,
					d->cache.width, d->cache.height, 8,
					color_type,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
			break;
		}

		case rp_image::FORMAT_CI8:
			png_set_IHDR(d->png_ptr, d->info_ptr,
					d->cache.width, d->cache.height, 8,
					PNG_COLOR_TYPE_PALETTE,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);

			// Write the palette and tRNS values.
			d->write_CI8_palette();

#ifdef PNG_sBIT_SUPPORTED
			// Make sure sBIT.alpha = 0.
			// libpng will complain if it's not zero, since alpha
			// is handled differently in paletted images.
			// NOTE: Or maybe not? It only checks if the color type
			// has PNG_COLOR_MASK_ALPHA set.
			d->cache.sBIT.alpha = 0;
#endif /* PNG_sBIT_SUPPORTED */
			break;

		default:
			// Unsupported pixel format.
			assert(!"Unsupported rp_image::Format.");
			d->lastError = EINVAL;
			return -d->lastError;
	}

	if (d->imageTag == RpPngWriterPrivate::IMGT_ICONANIMDATA) {
		// Write an acTL chunk to indicate that this is an APNG image.
		png_set_acTL(d->png_ptr, d->info_ptr, d->iconAnimData->seq_count, 0);
	}

#ifdef PNG_sBIT_SUPPORTED
	if (d->cache.has_sBIT) {
		// Write the sBIT chunk.
		// NOTE: rp_image::sBIT_t has the same format as png_color_8.
		const png_color_8 *const sBIT_pc8 = reinterpret_cast<const png_color_8*>(&d->cache.sBIT);
		png_set_sBIT(d->png_ptr, d->info_ptr, PNG_CONST_CAST(png_color_8p)(sBIT_pc8));
	}
#endif /* PNG_sBIT_SUPPORTED */

	// Write the PNG information to the file.
	png_write_info(d->png_ptr, d->info_ptr);
	d->IHDR_written = true;
	return 0;
}

/**
 * Write the PNG IHDR.
 * This must be called before writing any other image data.
 *
 * This function sets the cached sBIT before writing IHDR.
 * It should only be used for raw images. Use write_IHDR()
 * for rp_image and IconAnimData.
 *
 * @param sBIT		[in] sBIT metadata.
 * @param palette	[in,opt] Palette for CI8 images.
 * @param palette_len	[in,opt] Number of entries in `palette`.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_IHDR(const rp_image::sBIT_t *sBIT, const uint32_t *palette, int palette_len)
{
	RP_D(RpPngWriter);
	assert(d->imageTag == RpPngWriterPrivate::IMGT_RAW);
	if (d->imageTag != RpPngWriterPrivate::IMGT_RAW) {
		// Can't be used for this type.
		return -EINVAL;
	}

	d->cache.set_sBIT(sBIT);
	d->cache.palette = palette;
	d->cache.palette_len = palette_len;
	return write_IHDR();
}

/**
 * Check if a UTF-8 string contains any characters not allowed
 * by libpng's Latin-1 tEXt chunk.
 *
 * libpng doesn't allow the following characters:
 * - C0 control characters (0x00-0x1F) other than '\n'
 * - 0x7F (DEL)
 * - C1 control characters (0x80-0x9F)
 *
 * If any forbidden Latin-1 characters, or characters outside
 * of Latin-1, are detected, the function will return 2, which
 * indicates that the string must be stored as iTXt.
 *
 * If the string is ASCII only, the function will return 0, which
 * indicates that the string can be stored as tEXt with no changes.
 *
 * If the string contains allowed Latin-1 code points between
 * 0xA0-0xFF, the function will return 1, which indicates that the
 * string must be converted from UTF-8 to Latin-1.
 *
 * @param str UTF-8 string.
 * @return 0 if it's ASCII; 1 if it's Latin-1; 2 if it's not Latin-1.
 */
static inline int u8strIsPngLatin1(const char *str)
{
	int ret = 0;
	for (; *str != 0; str++) {
		if (!(*str & 0x80))
			continue;

		// Check for a 2-byte UTF-8 sequence.
		if ((*str & 0xE0) == 0xC0) {
			// 2-byte sequence.
			if ((str[1] & 0xC0) != 0x80) {
				// Invalid second character.
				return 2;
			}
			unsigned int chr = (((unsigned int)str[0] & 0x1F) << 5) |
					    ((unsigned int)str[1] & 0x3F);
			if ((chr > 0xFF) ||
			    (chr < 0x20 && chr != '\n') ||
			    (chr > 0x7E && chr < 0xA0))
			{
				// Not a valid Latin-1 character,
				// or not an allowed control character.
				return 2;
			}

			// This is an allowed Latin-1 character.
			ret = 1;
			str++;
		} else {
			// More than 2 bytes.
			// This is definitely not Latin-1.
			return 2;
		}
	}

	return ret;
}

/**
 * Write an array of text chunks.
 * This is needed for e.g. the XDG thumbnailing specification.
 *
 * If called before write_IHDR(), tEXt chunks will be written
 * before the IDAT chunk.
 *
 * If called after write_IHDR(), tEXt chunks will be written
 * after the IDAT chunk.
 *
 * NOTE: Key must be Latin-1. Value must be UTF-8.
 * If value is ASCII or exclusively uses code points compatible
 * with Latin-1, it will be saved as Latin-1.
 *
 * Strings that are 40 bytes or longer will be saved as zTXt.
 *
 * @param kv Vector of key/value pairs.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_tEXt(const kv_vector &kv)
{
	RP_D(RpPngWriter);
	assert(d->file != nullptr);
	if (unlikely(!d->file)) {
		// Invalid state.
		d->lastError = EIO;
		return -d->lastError;
	}

	if (unlikely(kv.empty())) {
		// Empty vector...
		return 0;
	}

	// Vector of string pointers to be freed.
	// Needed in case we have to convert from
	// UTF-8 to Latin-1.
	vector<char*> vU8toL1;
	vU8toL1.reserve(kv.size());

	// Allocate string pointer arrays for kv_vector.
	// Since kv_vector is Latin-1, we don't have to
	// strdup() the strings.
	// TODO: unique_ptr<>?
	png_text *text = new png_text[kv.size()];
	png_text *pTxt = text;
	for (unsigned int i = 0; i < kv.size(); i++, pTxt++) {
		// Check if the string is ASCII, Latin-1, or other.
		const string &value = kv[i].second;
		const bool compress = (value.size() >= 40);	// same as Qt

		const int status = u8strIsPngLatin1(value.c_str());
		switch (status) {
			case 0:
				// ASCII. Use it as-is.
				pTxt->compression = (compress ? PNG_TEXT_COMPRESSION_zTXt : PNG_TEXT_COMPRESSION_NONE);
				pTxt->key = (png_charp)kv[i].first;
				pTxt->text = (png_charp)value.c_str();
				break;
			case 1: {
				// Latin-1. Convert it.
				// TODO: utf8_to_latin1() wrapper that takes std::string?
				char *const latin1_str = strdup(utf8_to_latin1(value.c_str(), (int)value.size()).c_str());
				vU8toL1.push_back(latin1_str);
				pTxt->compression = (compress ? PNG_TEXT_COMPRESSION_zTXt : PNG_TEXT_COMPRESSION_NONE);
				pTxt->key = (png_charp)kv[i].first;
				pTxt->text = (png_charp)latin1_str;
				break;
			}
			case 2:
				// UTF-8. Use iTXt.
				pTxt->key = (png_charp)kv[i].first;
				pTxt->text = (png_charp)value.c_str();
#ifdef PNG_iTXt_SUPPORTED
				// iTXt is supported at compile time.
				// libpng-1.4 enabled this by default.
				// Previous versions did not, since it changed the struct size.
				pTxt->compression = (compress ? PNG_ITXT_COMPRESSION_zTXt : PNG_ITXT_COMPRESSION_NONE);
				pTxt->lang = nullptr;
				pTxt->lang_key = nullptr;
#else /* !PNG_iTXt_SUPPORTED */
				// iTXt is not supported at compile time.
				// Fall back to storing UTF-8 as tEXt.
				// TODO: Convert to Latin-1 instead?
				pTxt->compression = (compress ? PNG_TEXT_COMPRESSION_zTXt : PNG_TEXT_COMPRESSION_NONE);
#endif /* PNG_iTXt_SUPPORTED */
				break;
			default:
				assert(!"Should not get here...");
				pTxt->compression = PNG_TEXT_COMPRESSION_NONE;
				pTxt->key = (png_charp)"";
				pTxt->text = (png_charp)"";
				break;
		}
	}

#ifdef PNG_SETJMP_SUPPORTED
	// WARNING: Do NOT initialize any C++ objects past this point!
	if (setjmp(png_jmpbuf(d->png_ptr))) {
		// PNG write failed.
		delete[] text;
		for (auto iter = vU8toL1.begin(); iter != vU8toL1.end(); ++iter) {
			free(*iter);
		}
		d->lastError = EIO;
		return -d->lastError;
	}
#endif /* PNG_SETJMP_SUPPORTED */

	png_set_text(d->png_ptr, d->info_ptr, text, (int)kv.size());
	delete[] text;
	for (auto iter = vU8toL1.begin(); iter != vU8toL1.end(); ++iter) {
		free(*iter);
	}
	return 0;
}

/**
 * Write raw image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * If constructed using a filename instead of IRpFile,
 * this will automatically close the file.
 *
 * NOTE: This version is *only* for raw images!
 *
 * @param row_pointers PNG row pointers. Array must have cache.height elements.
 * @param is_abgr If true, image data is ABGR instead of ARGB.
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_IDAT(const uint8_t *const *row_pointers, bool is_abgr)
{
	assert(row_pointers != nullptr);
	if (unlikely(!row_pointers)) {
		return -EINVAL;
	}

	RP_D(RpPngWriter);
	assert(d->imageTag == RpPngWriterPrivate::IMGT_RAW);
	if (unlikely(d->imageTag != RpPngWriterPrivate::IMGT_RAW)) {
		// Can't be used for this type.
		return -EINVAL;
	}

	return d->write_IDAT(row_pointers, is_abgr);
}

/**
 * Write the rp_image data to the PNG image.
 *
 * This must be called after any other modifier functions.
 *
 * If constructed using a filename instead of IRpFile,
 * this will automatically close the file.
 *
 * NOTE: Do NOT use this function for raw images!
 * Use the version that takes an array of row pointers.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int RpPngWriter::write_IDAT(void)
{
	RP_D(RpPngWriter);
	int ret = -1;
	switch (d->imageTag) {
		case RpPngWriterPrivate::IMGT_RP_IMAGE:
			// Write a regular PNG image.
			ret = d->write_IDAT();
			break;

		case RpPngWriterPrivate::IMGT_ICONANIMDATA:
			// Write an animated PNG image.
			// NOTE: d->isAnimated is only set if APNG is loaded,
			// so we don't have to check it again here.
			ret = d->write_IDAT_APNG();
			break;

		default:
			// Can't be used for this type.
			assert(!"Function does not support this image tag.");
			ret = -EINVAL;
			break;
	}
	return ret;
}

}

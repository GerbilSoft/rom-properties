/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * RpJpeg.cpp: JPEG image handler.                                         *
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

#include "config.libromdata.h"

#include "RpJpeg.hpp"
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
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>

namespace LibRomData {

class RpJpegPrivate
{
	private:
		// RpJpegPrivate is a static class.
		RpJpegPrivate();
		~RpJpegPrivate();
		RP_DISABLE_COPY(RpJpegPrivate)

	public:
		/** Error handling functions. **/

		// Error manager.
		// Based on libjpeg-turbo 1.5.1's read_JPEG_file(). (example.c)
		struct my_error_mgr {
			jpeg_error_mgr pub;	// "public" fields
			jmp_buf setjmp_buffer;	// for return to caller
		};

		/**
		 * error_exit replacement for JPEG.
		 * @param cinfo j_common_ptr
		 */
		static void my_error_exit(j_common_ptr cinfo);

		/** I/O functions. **/

		// JPEG source manager struct.
		// Based on libjpeg-turbo 1.5.1's jpeg_stdio_src(). (jdatasrc.c)
		static const unsigned int INPUT_BUF_SIZE = 4096;
		struct MySourceMgr {
			jpeg_source_mgr pub;

			IRpFile *infile;	// Source stream.
			JOCTET *buffer;		// Start of buffer.
			bool start_of_file;	// Have we gotten any data yet?
		};

		/**
		 * Initialize MySourceMgr.
		 * @param cinfo j_decompress_ptr
		 */
		static void init_source(j_decompress_ptr cinfo);

		/**
		 * Fill the input buffer.
		 * @param cinfo j_decompress_ptr
		 * @return TRUE on success. (NOTE: 'boolean' is a JPEG typedef.)
		 */
		static boolean fill_input_buffer(j_decompress_ptr cinfo);

		/**
		 * Terminate the source.
		 * This is called once all JPEG data has been read.
		 * Usually a no-op.
		 *
		 * @param cinfo j_decompress_ptr
		 */
		static void term_source(j_decompress_ptr cinfo);

		/**
		 * Skip data in the source file.
		 * @param cinfo j_decompress_ptr
		 * @param num_bytes Number of bytes to skip.
		 */
		static void skip_input_data(j_decompress_ptr cinfo, long num_bytes);

		/**
		 * Initialize a JPEG source manager for an IRpFile.
		 * @param cinfo j_decompress_ptr
		 * @param file IRpFile
		 */
		static void jpeg_IRpFile_src(j_decompress_ptr cinfo, IRpFile *infile);
};

/** RpJpegPrivate **/

/** Error handling functions. **/

void RpJpegPrivate::my_error_exit(j_common_ptr cinfo)
{
	// Based on libjpeg-turbo 1.5.1's read_JPEG_file(). (example.c)
	my_error_mgr *myerr = reinterpret_cast<my_error_mgr*>(cinfo->err);

	// Always display the message.
	// We could postpone this until after returning, if we chose.
	(*cinfo->err->output_message)(cinfo);

	// Return control to the setjmp point.
	longjmp(myerr->setjmp_buffer, 1);
}

/** I/O functions. **/

/**
 * Initialize MySourceMgr.
 * @param cinfo j_decompress_ptr
 */
void RpJpegPrivate::init_source(j_decompress_ptr cinfo)
{
	MySourceMgr *const src = reinterpret_cast<MySourceMgr*>(cinfo->src);

	// Reset the empty-input-file flag for each image,
	// but don't clear the input buffer.
	// This is correct behavior for reading a series of images from one source.
	src->start_of_file = true;
}

/**
 * Fill the input buffer.
 * @param cinfo j_decompress_ptr
 * @return TRUE on success. (NOTE: 'boolean' is a JPEG typedef.)
 */
boolean RpJpegPrivate::fill_input_buffer(j_decompress_ptr cinfo)
{
	MySourceMgr *const src = reinterpret_cast<MySourceMgr*>(cinfo->src);
	size_t nbytes;

	nbytes = src->infile->read(src->buffer, INPUT_BUF_SIZE);

	if (nbytes <= 0) {
		if (src->start_of_file) {
			/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		}
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET)0xFF;
		src->buffer[1] = (JOCTET)JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = false;

	return TRUE;
}

/**
 * Skip data in the source file.
 * @param cinfo j_decompress_ptr
 * @param num_bytes Number of bytes to skip.
 */
void RpJpegPrivate::skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr *const src = cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		while (num_bytes > (long) src->bytes_in_buffer) {
			num_bytes -= (long) src->bytes_in_buffer;
			(void) (*src->fill_input_buffer) (cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			 * so suspension need not be handled.
			 */
		}
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}

/**
 * Terminate the source.
 * This is called once all JPEG data has been read.
 * Usually a no-op.
 *
 * @param cinfo j_decompress_ptr
 */
void RpJpegPrivate::term_source(j_decompress_ptr cinfo)
{
	// Nothing to do here...
	((void)cinfo);
}

/**
 * Set the JPEG source manager for an IRpFile.
 * @param cinfo j_decompress_ptr
 * @param file IRpFile
 */
void RpJpegPrivate::jpeg_IRpFile_src(j_decompress_ptr cinfo, IRpFile *infile)
{
	// Based on libjpeg-turbo 1.5.1's jpeg_stdio_src(). (jdatasrc.c)
	if (!cinfo->src) {
		// No JPEG source manager set.
		cinfo->src = static_cast<jpeg_source_mgr*>(
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
						   sizeof(MySourceMgr)));
		MySourceMgr *src = reinterpret_cast<MySourceMgr*>(cinfo->src);
		src->buffer = static_cast<JOCTET*>(
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
						   INPUT_BUF_SIZE * sizeof(JOCTET)));
	} else if (cinfo->src->init_source != init_source) {
		// Cannot reuse the existing source manager if it wasn't
		// created by our init_source function.
		ERREXIT(cinfo, JERR_BUFFER_SIZE);
	}

	// Initialize the function pointers.
	MySourceMgr *src = reinterpret_cast<MySourceMgr*>(cinfo->src);
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

/** RpJpeg **/

/**
 * Load a JPEG image from an IRpFile.
 *
 * This image is NOT checked for issues; do not use
 * with untrusted images!
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpJpeg::loadUnchecked(IRpFile *file)
{
	if (!file)
		return nullptr;

	file->rewind();

	RpJpegPrivate::my_error_mgr jerr;
	jpeg_decompress_struct cinfo;
	int row_stride;			// Physical row width in output buffer
	rp_image *img = nullptr;	// Image.

	/** Step 1: Allocate and initialize JPEG decompression object. **/

	// Set up error handling.
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = RpJpegPrivate::my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		// An error occurred while decoding the JPEG.
		// NOTE: buffer is allocated using JPEG allocation functions,
		// so it's automatically freed when we destroy cinfo.
		jpeg_destroy_decompress(&cinfo);
		if (img) {
			delete img;
		}
		return nullptr;
	}

	// Set up the decompression struct.
	jpeg_create_decompress(&cinfo);

	/** Step 2: Specify data source. **/
	RpJpegPrivate::jpeg_IRpFile_src(&cinfo, file);

	/** Step 3: Read file parameters with jpeg_read_header(). */
	// Return value is not useful here since:
	// a. Suspension is not possible with stdio data source (and IRpFile).
	// b. We passed TRUE to reject a tables-only JPEG file as an error.
	jpeg_read_header(&cinfo, TRUE);

	// Sanity check: Don't allow images larger than 65536x65536.
	assert(cinfo.output_width <= 65536);
	assert(cinfo.output_height <= 65536);
	if (cinfo.output_width > 65536 ||
	    cinfo.output_height > 65536)
	{
		// Image is too big.
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}

	/** Step 4: Set parameters for decompression. **/
	// Make sure we use libjpeg's built-in colorspace conversion
	// where possible.
	switch (cinfo.jpeg_color_space) {
		case JCS_YCbCr:
			// libjpeg (standard) supports RGB->YCbCr conversion.
			// TODO: libjpeg-turbo supports RGBX output as well.
			cinfo.out_color_space = JCS_RGB;
			break;
		case JCS_YCCK:
			// libjpeg (standard) supports YCCK->CMYK.
			cinfo.out_color_space = JCS_CMYK;
			break;
		default:
			break;
	}

	/** Step 5: Start decompressor. **/
	// We can ignore the return value since suspension is not possible
	// with the stdio data source (and IRpFile).
	jpeg_start_decompress(&cinfo);

	// Create the rp_image.
	bool direct_copy = false;
	switch (cinfo.out_color_space) {
		case JCS_GRAYSCALE: {
			// Grayscale JPEG.
			assert(cinfo.output_components == 1);
			if (cinfo.output_components != 1) {
				// Only 8-bit grayscale is supported.
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}

			// Create the image.
			img = new rp_image(cinfo.output_width, cinfo.output_height, rp_image::FORMAT_CI8);

			// Create a grayscale palette.
			uint32_t *img_palette = img->palette();
			assert(img_palette != nullptr);
			if (!img_palette) {
				// No palette...
				jpeg_destroy_decompress(&cinfo);
				delete img;
				return nullptr;
			}

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

			// 8-bit grayscale can be directly read into rp_image.
			direct_copy = true;
			break;
		}

		case JCS_RGB: {
			// RGB colorspace.
			assert(cinfo.output_components == 3);
			if (cinfo.output_components != 3) {
				// Only 24-bit RGB/YCbCr is supported.
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}
			cinfo.out_color_space = JCS_RGB;
			img = new rp_image(cinfo.image_width, cinfo.image_height, rp_image::FORMAT_ARGB32);
			break;
		}

		case JCS_CMYK:
			// CMYK/YCCK colorspace.
			// libjpeg can convert from YCCK to CMYK, but we'll
			// have to convert CMYK to RGB ourselves.
			assert(cinfo.output_components == 4);
			if (cinfo.output_components != 4) {
				// Only 4-component CMYK/YCCK is supported.
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}
			cinfo.out_color_space = JCS_CMYK;
			img = new rp_image(cinfo.image_width, cinfo.image_height, rp_image::FORMAT_ARGB32);
			break;

		default:
			// Unsupported colorspace.
			assert(!"Colorspace is not supported.");
			jpeg_destroy_decompress(&cinfo);
			delete img;
			return nullptr;
	}

	/** Step 6: while (scan lines remain to be read) jpeg_read_scanlines(...); */
	row_stride = cinfo.output_width * cinfo.output_components;
	if (!direct_copy) {
		// Not a grayscale image. Manual image expansion is needed.
		// TODO: Needs testing.
		JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)
			((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
		while (cinfo.output_scanline < cinfo.output_height) {
			// NOTE: jpeg_read_scanlines() increments the scanline value,
			// so we have to save the rp_image scanline pointer first.
			void *const vdest = img->scanLine(cinfo.output_scanline);
			jpeg_read_scanlines(&cinfo, buffer, 1);
			switch (cinfo.out_color_space) {
				case JCS_RGB: {
					// Convert from 24-bit RGB to 32-bit ARGB.
					// TODO: Optimized version?
					// TODO: Unroll the loop?
					uint32_t *dest = static_cast<uint32_t*>(vdest);
					const uint8_t *src = buffer[0];
					for (unsigned int i = cinfo.output_width; i > 0; i--, dest++, src += 3) {
						*dest = 0xFF000000 | (src[0] << 16) | (src[1] << 8) | src[2];
					}
					break;
				}

				case JCS_CMYK: {
					// Convert from CMYK to 32-bit ARGB.
					// Reference: https://github.com/qt/qtbase/blob/ffa578faf02226eb53793854ad53107afea4ab91/src/plugins/imageformats/jpeg/qjpeghandler.cpp#L395
					// TODO: Optimized version?
					// TODO: Unroll the loop?
					uint8_t *dest = static_cast<uint8_t*>(vdest);
					const uint8_t *src = buffer[0];
					for (unsigned int i = cinfo.output_width; i > 0; i--, dest += 4, src += 4) {
						unsigned int k = src[3];
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
						dest[3] = 255;			// Alpha
						dest[2] = k * src[0] / 255;	// Red
						dest[1] = k * src[1] / 255;	// Green
						dest[0] = k * src[2] / 255;	// Blue
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
						dest[0] = 255;			// Alpha
						dest[1] = k * src[0] / 255;	// Red
						dest[2] = k * src[1] / 255;	// Green
						dest[3] = k * src[2] / 255;	// Blue
#endif
					}
					break;
				}

				default: {
					assert(!"Unsupported JPEG colorspace.");
					jpeg_finish_decompress(&cinfo);
					jpeg_destroy_decompress(&cinfo);
					delete img;
					return nullptr;
				}
			}
		}
	} else {
		// Grayscale image. Decompress directly to the rp_image.
		// NOTE: jpeg_read_scanlines() has an option to specify how many
		// scanlines to read, but it doesn't work. We'll have to read
		// one scanline at a time.
		for (unsigned int i = 0; i < cinfo.output_height; i++) {
			uint8_t *dest = static_cast<uint8_t*>(img->scanLine(i));
			jpeg_read_scanlines(&cinfo, &dest, 1);
		}
	}

	/** Step 7: Finish decompression. **/
	// We can ignore the return value since suspension is not possible
	// with the stdio data source (and IRpFile).
	jpeg_finish_decompress(&cinfo);

	/** Step 8: Release JPEG decompression object. **/
	// This will automatically free any memory allocated using
	// libjpeg's allocation functions.
	jpeg_destroy_decompress(&cinfo);

	// Return the image.
	return img;
}

/**
 * Load a JPEG image from an IRpFile.
 *
 * This image is verified with various tools to ensure
 * it doesn't have any errors.
 *
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image *RpJpeg::load(IRpFile *file)
{
	if (!file)
		return nullptr;

	// FIXME: Add a JPEG equivalent of pngcheck().
	file->rewind();
	return loadUnchecked(file);
}

}

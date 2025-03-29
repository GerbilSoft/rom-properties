/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg.cpp: JPEG image handler.                                         *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librpbase.h"

#include "RpJpeg.hpp"
#include "librpfile/IRpFile.hpp"

// libi18n
#include "libi18n/i18n.h"

// Other rom-properties libraries
using namespace LibRpFile;
using namespace LibRpTexture;

// C includes (C++ namespace)
#include <csetjmp>
#include <cstdio>	// jpeglib.h needs stdio included first

#ifdef _WIN32
// For OutputDebugStringA()
#  include <windows.h>
#endif /* _WIN32 */

// libjpeg
#include <jpeglib.h>
#include <jerror.h>

// JPEGCALL might not be defined if we're using the system libjpeg.
#ifndef JPEGCALL
# ifdef _MSC_VER
#  define JPEGCALL __cdecl
# else
#  define JPEGCALL
# endif
#endif /* !JPEGCALL */

#if defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
# define RPJPEG_HAS_SSSE3 1
#endif

#ifdef RPJPEG_HAS_SSSE3
#  include "librpcpuid/cpuflags_x86.h"
#  include "RpJpeg_ssse3.hpp"
#endif /* RPJPEG_HAS_SSSE3 */

namespace LibRpBase { namespace RpJpeg {

namespace Private {

/** Error handling functions **/

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
static void JPEGCALL my_error_exit(j_common_ptr cinfo)
{
	// Based on libjpeg-turbo 1.5.1's read_JPEG_file(). (example.c)
	my_error_mgr *myerr = reinterpret_cast<my_error_mgr*>(cinfo->err);

	// TODO: Don't show errors if using standard libjpeg
	// and JCS_EXT_BGRA failed.

	// Print the message.
	(*cinfo->err->output_message)(cinfo);

	// Return control to the setjmp point.
	longjmp(myerr->setjmp_buffer, 1);
}

/**
 * output_message replacement for JPEG.
 * @param cinfo j_common_ptr
 */
static void JPEGCALL my_output_message(j_common_ptr cinfo)
{
	// Format the string.
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);

#ifdef _WIN32
	// The default libjpeg error handler uses MessageBox() on Windows.
	// This is bad design, so we'll use OutputDebugStringA() instead.
	char txtbuf[JMSG_LENGTH_MAX+64];
	snprintf(txtbuf, sizeof(txtbuf), "libjpeg error: %s\n", buffer);
	OutputDebugStringA(txtbuf);
#else /* !_WIN32 */
	// Print to stderr.
	fmt::print(stderr, "libjpeg error: {:s}\n", buffer);
#endif /* _WIN32 */
}

/** I/O functions **/

// JPEG source manager struct
// Based on libjpeg-turbo 1.5.1's jpeg_stdio_src(). (jdatasrc.c)
static constexpr size_t INPUT_BUF_SIZE = 4096U;
struct MySourceMgr {
	jpeg_source_mgr pub;

	LibRpFile::IRpFile *infile;	// Source stream.
	JOCTET *buffer;		// Start of buffer.
	bool start_of_file;	// Have we gotten any data yet?
};

/**
 * Initialize MySourceMgr.
 * @param cinfo j_decompress_ptr
 */
static void JPEGCALL init_source(j_decompress_ptr cinfo)
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
static boolean JPEGCALL fill_input_buffer(j_decompress_ptr cinfo)
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
		src->buffer[0] = static_cast<JOCTET>(0xFF);
		src->buffer[1] = static_cast<JOCTET>(JPEG_EOI);
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
static void JPEGCALL skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr *const src = cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		while (num_bytes > static_cast<long>(src->bytes_in_buffer)) {
			num_bytes -= static_cast<long>(src->bytes_in_buffer);
			(void) (*src->fill_input_buffer) (cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			 * so suspension need not be handled.
			 */
		}
		src->next_input_byte += static_cast<size_t>(num_bytes);
		src->bytes_in_buffer -= static_cast<size_t>(num_bytes);
	}
}

/**
 * Terminate the source.
 * This is called once all JPEG data has been read.
 * Usually a no-op.
 *
 * @param cinfo j_decompress_ptr
 */
static void JPEGCALL term_source(j_decompress_ptr cinfo)
{
	// Nothing to do here...
	RP_UNUSED(cinfo);
}

/**
 * Set the JPEG source manager for an IRpFile.
 * @param cinfo j_decompress_ptr
 * @param file IRpFile
 */
static void jpeg_IRpFile_src(j_decompress_ptr cinfo, IRpFile *infile)
{
	// Based on libjpeg-turbo 1.5.1's jpeg_stdio_src(). (jdatasrc.c)
	if (!cinfo->src) {
		// No JPEG source manager set.
		cinfo->src = static_cast<jpeg_source_mgr*>(
			(*cinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(cinfo),
				JPOOL_PERMANENT, sizeof(MySourceMgr)));
		MySourceMgr *src = reinterpret_cast<MySourceMgr*>(cinfo->src);
		src->buffer = static_cast<JOCTET*>(
			(*cinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(cinfo),
				JPOOL_PERMANENT, INPUT_BUF_SIZE * sizeof(JOCTET)));
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
	src->pub.next_input_byte = nullptr; /* until buffer loaded */
}

} // namespace Private

/** RpJpeg **/

/**
 * Load a JPEG image from an IRpFile.
 * @param file IRpFile to load from.
 * @return rp_image*, or nullptr on error.
 */
rp_image_ptr load(IRpFile *file)
{
	if (!file) {
		return nullptr;
	}

	// Rewind the file.
	file->rewind();

	Private::my_error_mgr jerr;
	jpeg_decompress_struct cinfo;
	rp_image *img = nullptr;	// Image
	int row_stride;			// Physical row width in output buffer
	bool direct_copy = false;	// True if a direct copy can be made

	// libjpeg-turbo BGRA extension.
	// Defining MY_JCS_EXT_BGRA so it can be compiled with libjpeg and
	// later used with libjpeg-turbo. Prefixed so it doesn't conflict
	// with libjpeg-turbo's headers.
	static constexpr int MY_JCS_EXT_BGRA = 13;
	bool try_ext_bgra = true;	// True if we should try JCS_EXT_BGRA first.
	bool tried_ext_bgra = false;	// True if we tried JCS_EXT_BGRA.

	/** Step 1: Allocate and initialize JPEG decompression object. **/

	// Set up error handling.
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = Private::my_error_exit;
	jerr.pub.output_message = Private::my_output_message;

	// Multi-level setjmp() so we can test for JCS_EXT_BGRA.
	int jmperr = setjmp(jerr.setjmp_buffer);
	if (jmperr) {
		delete img;
		img = nullptr;

		if (try_ext_bgra && tried_ext_bgra) {
			// Tried using JCS_EXT_BGRA and it didn't work.
			// Try again with JCS_RGB.
			fmt::print(stderr, "JCS_EXT_BGRA FAILED, trying JCS_RGB\n");
			try_ext_bgra = false;
			direct_copy = false;
			file->rewind();
			jmperr = setjmp(jerr.setjmp_buffer);
		}
		if (jmperr) {
			fmt::print(stderr, "JPEG decoding FAILED\n");
			// An error occurred while decoding the JPEG.
			// NOTE: buffer is allocated using JPEG allocation functions,
			// so it's automatically freed when we destroy cinfo.
			delete img;
			jpeg_destroy_decompress(&cinfo);
			return nullptr;
		}
	}

	// Set up the decompression struct.
	jpeg_create_decompress(&cinfo);

	/** Step 2: Specify data source. **/
	Private::jpeg_IRpFile_src(&cinfo, file);

	/** Step 3: Read file parameters with jpeg_read_header(). */
	// Return value is not useful here since:
	// a. Suspension is not possible with stdio data source (and IRpFile).
	// b. We passed TRUE to reject a tables-only JPEG file as an error.
	jpeg_read_header(&cinfo, TRUE);

	// Sanity check: Don't allow images larger than 32768x32768.
	assert(cinfo.image_width > 0);
	assert(cinfo.image_height > 0);
	assert(cinfo.image_width <= 32768);
	assert(cinfo.image_height <= 32768);
	if (cinfo.image_width <= 0 || cinfo.image_height <= 0 ||
	    cinfo.image_width > 32768 || cinfo.image_height > 32768)
	{
		// Image size is either invalid or too big.
		jpeg_destroy_decompress(&cinfo);
		return nullptr;
	}

	/** Step 4: Set parameters for decompression. **/
	// Make sure we use libjpeg's built-in colorspace conversion
	// where possible.
	switch (cinfo.jpeg_color_space) {
		case JCS_RGB:
			// libjpeg-turbo supports RGB->BGRA conversion.
			if (try_ext_bgra) {
				cinfo.out_color_space = static_cast<J_COLOR_SPACE>(MY_JCS_EXT_BGRA);
				cinfo.output_components = 4;
				direct_copy = true;
				tried_ext_bgra = true;
			}
			break;
		case JCS_YCbCr:
			// libjpeg (standard) supports YCbCr->RGB conversion.
			// libjpeg-turbo supports YCbCr->BGRA conversion.
			if (try_ext_bgra) {
				cinfo.out_color_space = static_cast<J_COLOR_SPACE>(MY_JCS_EXT_BGRA);
				cinfo.output_components = 4;
				direct_copy = true;
				tried_ext_bgra = true;
			} else {
				cinfo.out_color_space = JCS_RGB;
			}
			break;
		case JCS_YCCK:
			// libjpeg (standard) supports YCCK->CMYK conversion.
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
			img = new rp_image(cinfo.output_width, cinfo.output_height, rp_image::Format::CI8);
			if (!img->isValid()) {
				// Could not allocate the image.
				delete img;
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}

			// Create a grayscale palette.
			uint32_t *img_palette = img->palette();
			assert(img_palette != nullptr);
			if (!img_palette) {
				// No palette...
				delete img;
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}

			const unsigned int img_palette_len = img->palette_len();
			for (unsigned int i = 0; i < std::min(256U, img_palette_len);
				i++, img_palette++)
			{
				const uint8_t gray = static_cast<uint8_t>(i);
				*img_palette = (gray | gray << 8 | gray << 16);
				*img_palette |= 0xFF000000;
			}

			if (img_palette_len > 256) {
				// Clear the rest of the palette.
				// (NOTE: 0 == fully transparent.)
				for (unsigned i = img_palette_len-256; i > 0; i--, img_palette++) {
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

			img = new rp_image(cinfo.image_width, cinfo.image_height, rp_image::Format::ARGB32);
			if (!img->isValid()) {
				// Could not allocate the image.
				delete img;
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}
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

			img = new rp_image(cinfo.image_width, cinfo.image_height, rp_image::Format::ARGB32);
			if (!img->isValid()) {
				// Could not allocate the image.
				delete img;
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}
			break;

		case MY_JCS_EXT_BGRA: {
			// BGRA colorspace.
			// Matches ARGB32 images.
			assert(cinfo.output_components == 4);
			if (cinfo.output_components != 4) {
				// Only 32-bit BGRA is supported.
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}

			img = new rp_image(cinfo.image_width, cinfo.image_height, rp_image::Format::ARGB32);
			if (!img->isValid()) {
				// Could not allocate the image.
				delete img;
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
			}
			break;
		}

		default:
			// Unsupported colorspace.
			assert(!"Unsupported JPEG colorspace. (Step 5)");
			jpeg_destroy_decompress(&cinfo);
			return nullptr;
	}

	/** Step 6: while (scan lines remain to be read) jpeg_read_scanlines(...); */
	row_stride = cinfo.output_width * cinfo.output_components;
	if (!direct_copy) {
		// NOTE: jmemmgr's memory alignment is sizeof(double), so we'll
		// need to allocate a bit more to get 16-byte alignment.
		JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)
			(reinterpret_cast<j_common_ptr>(&cinfo), JPOOL_IMAGE, row_stride + 16, 1);
		buffer[0] = reinterpret_cast<JSAMPROW>(
			(ALIGN_BYTES(16, reinterpret_cast<uintptr_t>(buffer[0]))));

		switch (cinfo.out_color_space) {
			case JCS_RGB: {
				// Convert from 24-bit BGR to 32-bit ARGB.
				// NOTE: libjpeg-turbo has SSE2-optimized * to ARGB conversion,
				// which is preferred because it usually skips an intermediate
				// conversion step.
#ifdef RPJPEG_HAS_SSSE3
				if (RP_CPU_x86_HasSSSE3()) {
					ssse3::decodeBGRtoARGB(img, &cinfo, buffer);
					break;
				}
#endif /* RPJPEG_HAS_SSSE3 */

				argb32_t *dest = static_cast<argb32_t*>(img->bits());
				const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
				while (cinfo.output_scanline < cinfo.output_height) {
					jpeg_read_scanlines(&cinfo, buffer, 1);
					const uint8_t *src = buffer[0];

					// Process 2 pixels per iteration.
					unsigned int x = cinfo.output_width;
					for (; x > 1; x -= 2, dest += 2, src += 2*3) {
						dest[0].b = src[2];
						dest[0].g = src[1];
						dest[0].r = src[0];
						dest[0].a = 0xFF;

						dest[1].b = src[5];
						dest[1].g = src[4];
						dest[1].r = src[3];
						dest[1].a = 0xFF;
					}
					// Remaining pixels.
					for (; x > 0; x--, dest++, src += 3) {
						dest->b = src[2];
						dest->g = src[1];
						dest->r = src[0];
						dest->a = 0xFF;
					}

					// Next line.
					dest += dest_stride_adj;
				}
				break;
			}

			case JCS_CMYK: {
				// Convert from CMYK to 32-bit ARGB.
				// Reference: https://github.com/qt/qtbase/blob/ffa578faf02226eb53793854ad53107afea4ab91/src/plugins/imageformats/jpeg/qjpeghandler.cpp#L395
				argb32_t *dest = static_cast<argb32_t*>(img->bits());
				const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
				while (cinfo.output_scanline < cinfo.output_height) {
					jpeg_read_scanlines(&cinfo, buffer, 1);
					const uint8_t *src = buffer[0];

					// TODO: Optimized version?
					unsigned int x;
					for (x = cinfo.output_width; x > 1; x -= 2, dest += 2, src += 8) {
						unsigned int k = src[3];

						dest[0].b = k * src[2] / 255;	// Blue
						dest[0].g = k * src[1] / 255;	// Green
						dest[0].r = k * src[0] / 255;	// Red
						dest[0].a = 255;		// Alpha

						k = src[7];
						dest[1].b = k * src[6] / 255;	// Blue
						dest[1].g = k * src[5] / 255;	// Green
						dest[1].r = k * src[4] / 255;	// Red
						dest[1].a = 255;		// Alpha
					}
					// Remaining pixels.
					for (; x > 0; x--, dest++, src += 4) {
						const unsigned int k = src[3];
						dest->b = k * src[2] / 255;	// Blue
						dest->g = k * src[1] / 255;	// Green
						dest->r = k * src[0] / 255;	// Red
						dest->a = 255;			// Alpha
					}

					// Next line.
					dest += dest_stride_adj;
				}
				break;
			}

			default:
				assert(!"Unsupported JPEG colorspace. (Step 6)");
				delete img;
				jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);
				return nullptr;
		}

		// Set the sBIT metadata.
		// TODO: 10-bit/12-bit JPEGs?
		static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
		img->set_sBIT(&sBIT);
	} else {
		// Grayscale image, or RGB image with libjpeg-turbo's JCS_EXT_BGRA.
		// Decompress directly to the rp_image.
		// NOTE: jpeg_read_scanlines() has an option to specify how many
		// scanlines to read, but it doesn't work. We'll have to read
		// one scanline at a time.
		// FIXME: Might be because it's expecting an array of row pointers?
		uint8_t *dest = static_cast<uint8_t*>(img->bits());
		const int stride = img->stride();
		for (unsigned int i = 0; i < cinfo.output_height; i++, dest += stride) {
			// TODO: Verify return value.
			jpeg_read_scanlines(&cinfo, &dest, 1);
		}

		// Set the sBIT metadata.
		if (cinfo.out_color_space == JCS_GRAYSCALE) {
			// NOTE: Setting the grayscale value, though we're
			// not saving grayscale PNGs at the moment.
			static const rp_image::sBIT_t sBIT = {8,8,8,8,0};
			img->set_sBIT(&sBIT);
		} else {
			static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
			img->set_sBIT(&sBIT);
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
	return rp_image_ptr(img);
}

} }

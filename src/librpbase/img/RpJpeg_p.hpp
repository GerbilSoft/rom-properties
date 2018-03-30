/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * RpJpeg_p.hpp: JPEG image handler. (Private class)                       *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_P_HPP__
#define __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_P_HPP__

#include "../common.h"
#include "rp_image.hpp"

// JPEG header.
#include <jpeglib.h>
#include <jerror.h>
#include <jmorecfg.h>
// JPEG error handler.
#include <jerror.h>
#include <setjmp.h>

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

namespace LibRpBase {

class IRpFile;

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
		static void JPEGCALL my_error_exit(j_common_ptr cinfo);

		/**
		 * output_message replacement for JPEG.
		 * @param cinfo j_common_ptr
		 */
		static void JPEGCALL my_output_message(j_common_ptr cinfo);

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
		static void JPEGCALL init_source(j_decompress_ptr cinfo);

		/**
		 * Fill the input buffer.
		 * @param cinfo j_decompress_ptr
		 * @return TRUE on success. (NOTE: 'boolean' is a JPEG typedef.)
		 */
		static boolean JPEGCALL fill_input_buffer(j_decompress_ptr cinfo);

		/**
		 * Terminate the source.
		 * This is called once all JPEG data has been read.
		 * Usually a no-op.
		 *
		 * @param cinfo j_decompress_ptr
		 */
		static void JPEGCALL term_source(j_decompress_ptr cinfo);

		/**
		 * Skip data in the source file.
		 * @param cinfo j_decompress_ptr
		 * @param num_bytes Number of bytes to skip.
		 */
		static void JPEGCALL skip_input_data(j_decompress_ptr cinfo, long num_bytes);

		/**
		 * Initialize a JPEG source manager for an IRpFile.
		 * @param cinfo j_decompress_ptr
		 * @param file IRpFile
		 */
		static void jpeg_IRpFile_src(j_decompress_ptr cinfo, IRpFile *infile);

	public:
#ifdef RPJPEG_HAS_SSSE3
		/**
		 * Decode a 24-bit BGR JPEG to 32-bit ARGB.
		 * SSSE3-optimized version.
		 * NOTE: This function should ONLY be called from RpJpeg::loadUnchecked().
		 * @param img		[in/out] rp_image.
		 * @param cinfo		[in/out] JPEG decompression struct.
		 * @param buffer 	[in/out] Line buffer. (Must be 16-byte aligned!)
		 */
		static void decodeBGRtoARGB(LibRpBase::rp_image *RESTRICT img, jpeg_decompress_struct *RESTRICT cinfo, JSAMPARRAY buffer);
#endif /* RPJPEG_HAS_SSSE3 */
};

}

#endif /* __ROMPROPERTIES_LIBRPBASE_IMG_RPJPEG_P_HPP__ */

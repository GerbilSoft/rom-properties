/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image.hpp: Image class.                                              *
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

#ifndef __ROMPROPERTIES_LIBRPBASE_RP_IMAGE_HPP__
#define __ROMPROPERTIES_LIBRPBASE_RP_IMAGE_HPP__

#include "librpbase/config.librpbase.h"
#include "../common.h"
#include "../byteorder.h"
#include "cpu_dispatch.h"

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpbase/cpuflags_x86.h"
# define RP_IMAGE_HAS_SSE2 1
#endif
#ifdef RP_CPU_AMD64
# define RP_IMAGE_ALWAYS_HAS_SSE2 1
#endif

// TODO: Make this implicitly shared.

namespace LibRpBase {

// ARGB32 value with byte accessors.
union argb32_t {
	struct {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		uint8_t a;
		uint8_t r;
		uint8_t g;
		uint8_t b;
#endif
	};
	uint32_t u32;
};
ASSERT_STRUCT(argb32_t, 4);

class rp_image_backend;

class rp_image_private;
class rp_image
{
	public:
		enum Format {
			FORMAT_NONE,		// No image.
			FORMAT_CI8,		// Color index, 8-bit palette.
			FORMAT_ARGB32,		// 32-bit ARGB.

			FORMAT_LAST		// End of Format.
		};

		/**
		 * Create an rp_image.
		 *
		 * If an rp_image_backend has been registered, that backend
		 * will be used; otherwise, the defaul tbackend will be used.
		 *
		 * @param width Image width.
		 * @param height Image height.
		 * @param format Image format.
		 */
		rp_image(int width, int height, Format format);

		/**
		 * Create an rp_image using the specified rp_image_backend.
		 *
		 * NOTE: This rp_image will take ownership of the rp_image_backend.
		 *
		 * @param backend rp_image_backend.
		 */
		explicit rp_image(rp_image_backend *backend);

		~rp_image();

	private:
		RP_DISABLE_COPY(rp_image)
	private:
		friend class rp_image_private;
		rp_image_private *const d_ptr;

	public:
		/**
		 * rp_image_backend creator function.
		 * May be a static member of an rp_image_backend subclass.
		 */
		typedef rp_image_backend* (*rp_image_backend_creator_fn)(int width, int height, rp_image::Format format);

		/**
		 * Set the image backend creator function.
		 * @param backend Image backend creator function.
		 */
		static void setBackendCreatorFn(rp_image_backend_creator_fn backend_fn);

		/**
		 * Get the image backend creator function.
		 * @param backend Image backend creator function, or nullptr if the default is in use.
		 */
		static rp_image_backend_creator_fn backendCreatorFn(void);

	public:
		/**
		 * Get this image's backend object.
		 * @return Image backend object.
		 */
		const rp_image_backend *backend(void) const;

	public:
		/** Properties. **/

		/**
		 * Is the image valid?
		 * @return True if the image is valid.
		 */
		bool isValid(void) const;

		/**
		 * Get the image width.
		 * @return Image width.
		 */
		int width(void) const;

		/**
		 * Get the image height.
		 * @return Image height.
		 */
		int height(void) const;

		/**
		 * Is this rp_image square?
		 * @return True if width == height; false if not.
		 */
		bool isSquare(void) const;

		/**
		 * Get the total number of bytes per line.
		 * This includes memory alignment padding.
		 * @return Bytes per line.
		 */
		int stride(void) const;

		/**
		 * Get the number of active bytes per line.
		 * This does not include memory alignment padding.
		 * @return Number of active bytes per line.
		 */
		int row_bytes(void) const;

		/**
		 * Get the image format.
		 * @return Image format.
		 */
		Format format(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * @return Image data.
		 */
		const void *bits(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * TODO: detach()
		 * @return Image data.
		 */
		void *bits(void);

		/**
		 * Get a pointer to the specified line of image data.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		const void *scanLine(int i) const;

		/**
		 * Get a pointer to the specified line of image data.
		 * TODO: Detach.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		void *scanLine(int i);

		/**
		 * Get the image data size, in bytes.
		 * This is height * stride.
		 * @return Image data size, in bytes.
		 */
		size_t data_len(void) const;

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		const uint32_t *palette(void) const;

		/**
		 * Get the image palette.
		 * @return Pointer to image palette, or nullptr if not a paletted image.
		 */
		uint32_t *palette(void);

		/**
		 * Get the number of elements in the image palette.
		 * @return Number of elements in the image palette, or 0 if not a paletted image.
		 */
		int palette_len(void) const;

		/**
		 * Get the index of the transparency color in the palette.
		 * This is useful for images that use a single transparency
		 * color instead of alpha transparency.
		 * @return Transparent color index, or -1 if ARGB32 is used or the palette has alpha transparent colors.
		 */
		int tr_idx(void) const;

		/**
		 * Set the index of the transparency color in the palette.
		 * This is useful for images that use a single transparency
		 * color instead of alpha transparency.
		 * @param tr_idx Transparent color index. (Set to -1 if the palette has alpha transparent colors.)
		 */
		void set_tr_idx(int tr_idx);

		/**
		 * Get the name of a format
		 * @param format Format.
		 * @return String containing the user-friendly name of a format.
		 */
		static const char *getFormatName(Format format);

	public:
		/** Metadata. **/

		// sBIT struct.
		// This matches libpng's.
		struct sBIT_t {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
			uint8_t gray;
			uint8_t alpha;	// Set to 0 to write an RGB image in RpPngWriter.
		};

		/**
		 * Set the number of significant bits per channel.
		 * @param sBIT	[in] sBIT_t struct.
		 */
		void set_sBIT(const sBIT_t *sBIT);

		/**
		 * Get the number of significant bits per channel.
		 * @param sBIT	[out] sBIT_t struct.
		 * @return 0 on success; non-zero if not set or error.
		 */
		int get_sBIT(sBIT_t *sBIT) const;

		/**
		 * Clear the sBIT data.
		 */
		void clear_sBIT(void);

	public:
		/** Image operations. **/

		/**
		 * Duplicate the rp_image.
		 * @return New rp_image with a copy of the image data.
		 */
		rp_image *dup(void) const;

		/**
		 * Duplicate the rp_image, converting to ARGB32 if necessary.
		 * @return New ARGB32 rp_image with a copy of the image data.
		 */
		rp_image *dup_ARGB32(void) const;

		/**
		 * Square the rp_image.
		 *
		 * If the width and height don't match, transparent rows
		 * and/or columns will be added to "square" the image.
		 * Otherwise, this is the same as dup().
		 *
		 * @return New rp_image with a squared version of the original, or nullptr on error.
		 */
		rp_image *squared(void) const;

		/**
		 * Resize the rp_image.
		 *
		 * A new rp_image will be created with the specified dimensions,
		 * and the current image will be copied into the new image.
		 * If the new dimensions are smaller than the old dimensions,
		 * the image will be cropped. If the new dimensions are larger,
		 * the original image will be in the upper-left corner and the
		 * new space will be empty. (ARGB: 0x00000000)
		 *
		 * @param width New width.
		 * @param height New height.
		 * @return New rp_image with a resized version of the original, or nullptr on error.
		 */
		rp_image *resized(int width, int height) const;

		/**
		 * Un-premultiply this image.
		 * Image must be ARGB32.
		 * @return 0 on success; non-zero on error.
		 */
		int un_premultiply(void);

		/**
		 * Convert a chroma-keyed image to standard ARGB32.
		 * Standard version using regular C++ code.
		 *
		 * This operates on the image itself, and does not return
		 * a duplicated image with the adjusted image.
		 *
		 * NOTE: The image *must* be ARGB32.
		 *
		 * @param key Chroma key color.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int apply_chroma_key_cpp(uint32_t key);

#ifdef RP_IMAGE_HAS_SSE2
		/**
		 * Convert a chroma-keyed image to standard ARGB32.
		 * SSE2-optimized version.
		 *
		 * This operates on the image itself, and does not return
		 * a duplicated image with the adjusted image.
		 *
		 * NOTE: The image *must* be ARGB32.
		 *
		 * @param key Chroma key color.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int apply_chroma_key_sse2(uint32_t key);
#endif /* RP_IMAGE_HAS_SSE2 */

		/**
		 * Convert a chroma-keyed image to standard ARGB32.
		 *
		 * This operates on the image itself, and does not return
		 * a duplicated image with the adjusted image.
		 *
		 * NOTE: The image *must* be ARGB32.
		 *
		 * @param key Chroma key color.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int apply_chroma_key(uint32_t key);
};

/**
 * Convert a chroma-keyed image to standard ARGB32.
 *
 * This operates on the image itself, and does not return
 * a duplicated image with the adjusted image.
 *
 * NOTE: The image *must* be ARGB32.
 *
 * @param key Chroma key color.
 * @return 0 on success; negative POSIX error code on error.
 */
inline int rp_image::apply_chroma_key(uint32_t key)
{
#ifdef RP_IMAGE_ALWAYS_HAS_SSE2
	// amd64 always has SSE2.
	return apply_chroma_key_sse2(key);
#else /* !RP_IMAGE_ALWAYS_HAS_SSE2 */
# ifdef RP_IMAGE_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return apply_chroma_key_sse2(key);
	} else
# endif /* RP_IMAGE_HAS_SSE2 */
	{
		return apply_chroma_key_cpp(key);
	}
#endif /* RP_IMAGE_ALWAYS_HAS_SSE2 */
}

}

#endif /* __ROMPROPERTIES_LIBRPBASE_RP_IMAGE_HPP__ */

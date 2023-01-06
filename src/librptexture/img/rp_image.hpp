/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image.hpp: Image class.                                              *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "common.h"
#include "dll-macros.h"
#include "RefBase.hpp"

// C includes.
#include <stddef.h>	/* size_t */
#include <stdint.h>

#include "librpcpu/cpu_dispatch.h"
#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
#  include "librpcpu/cpuflags_x86.h"
#  define RP_IMAGE_HAS_SSE2 1
#  define RP_IMAGE_HAS_SSSE3 1
#  define RP_IMAGE_HAS_SSE41 1
#endif
#ifdef RP_CPU_AMD64
#  define RP_IMAGE_ALWAYS_HAS_SSE2 1
#endif

#include "../argb32_t.hpp"

// TODO: Make this implicitly shared.

namespace LibRpTexture {

class rp_image_backend;

class rp_image_private;
class rp_image : public RefBase
{
	public:
		enum class Format {
			None,	// No image.
			CI8,	// Color index, 8-bit palette.
			ARGB32,	// 32-bit ARGB.

			Max
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
		RP_LIBROMDATA_PUBLIC
		rp_image(int width, int height, Format format);

		/**
		 * Create an rp_image using the specified rp_image_backend.
		 *
		 * NOTE: This rp_image will take ownership of the rp_image_backend.
		 *
		 * @param backend rp_image_backend.
		 */
		explicit rp_image(rp_image_backend *backend);

	protected:
		~rp_image();	// call unref() instead

	private:
		RP_DISABLE_COPY(rp_image)
	private:
		friend class rp_image_private;
		rp_image_private *const d_ptr;

	public:
		inline rp_image *ref(void)
		{
			return RefBase::ref<rp_image>();
		}

		/**
		 * Special case ref() function to allow
		 * const rp_image* to be ref'd.
		 */
		inline const rp_image *ref(void) const
		{
			return const_cast<rp_image*>(this)->RefBase::ref<rp_image>();
		}

		/**
		 * Special case unref() function to allow
		 * const rp_image* to be unref'd.
		 */
		inline void unref(void) const
		{
			const_cast<rp_image*>(this)->RefBase::unref();
		}

	public:
		/**
		* Inverted pre-multiplication factors.
		* From Qt 5.9.1's qcolor.cpp.
		* These values are: 0x00FF00FF / alpha
		*/
		static const unsigned int qt_inv_premul_factor[256];

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
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
		const rp_image_backend *backend(void) const;

	public:
		/** Properties. **/

		/**
		 * Is the image valid?
		 * @return True if the image is valid.
		 */
		RP_LIBROMDATA_PUBLIC
		bool isValid(void) const;

		/**
		 * Get the image width.
		 * @return Image width.
		 */
		RP_LIBROMDATA_PUBLIC
		int width(void) const;

		/**
		 * Get the image height.
		 * @return Image height.
		 */
		RP_LIBROMDATA_PUBLIC
		int height(void) const;

		/**
		 * Is this rp_image square?
		 * @return True if width == height; false if not.
		 */
		RP_LIBROMDATA_PUBLIC
		bool isSquare(void) const;

		/**
		 * Get the total number of bytes per line.
		 * This includes memory alignment padding.
		 * @return Bytes per line.
		 */
		RP_LIBROMDATA_PUBLIC
		int stride(void) const;

		/**
		 * Get the number of active bytes per line.
		 * This does not include memory alignment padding.
		 * @return Number of active bytes per line.
		 */
		RP_LIBROMDATA_PUBLIC
		int row_bytes(void) const;

		/**
		 * Get the image format.
		 * @return Image format.
		 */
		RP_LIBROMDATA_PUBLIC
		Format format(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * @return Image data.
		 */
		RP_LIBROMDATA_PUBLIC
		const void *bits(void) const;

		/**
		 * Get a pointer to the first line of image data.
		 * TODO: detach()
		 * @return Image data.
		 */
		RP_LIBROMDATA_PUBLIC
		void *bits(void);

		/**
		 * Get a pointer to the specified line of image data.
		 * @param i Line number.
		 * @return Line of image data, or nullptr if i is out of range.
		 */
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
		unsigned int palette_len(void) const;

		/**
		 * Get the index of the transparency color in the palette.
		 * This is useful for images that use a single transparency
		 * color instead of alpha transparency.
		 * @return Transparent color index, or -1 if ARGB32 is used or the palette has alpha transparent colors.
		 */
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
		rp_image *dup(void) const;

		/**
		 * Duplicate the rp_image, converting to ARGB32 if necessary.
		 * @return New ARGB32 rp_image with a copy of the image data.
		 */
		RP_LIBROMDATA_PUBLIC
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
		RP_LIBROMDATA_PUBLIC
		rp_image *squared(void) const;

		/**
		 * Alignment constants for resized().
		 *
		 * NOTE: These constants match Qt::Alignment.
		 */
		enum Alignment {
			AlignDefault	= 0x00,

			/* NOT IMPLEMENTED YET */
#if 0
			AlignLeft	= 0x01,
			AlignRight	= 0x02,
			AlignHCenter	= 0x04,
			//AlignJustify	= 0x08,
			//AlignAbsolute	= 0x10,
			AlignLeading	= AlignLeft,
			AlignTrailing	= AlignRight,
#endif

			AlignTop	= 0x20,
			AlignBottom	= 0x40,
			AlignVCenter	= 0x80,

			/* NOT IMPLEMENTED due to no AlignHCenter */
			//AlignCenter	= AlignVCenter | AlignHCenter,

			//AlignHorizontal_Mask	= AlignLeft | AlignRight | AlignHCenter | AlignJustify | AlignAbsolute,
			AlignVertical_Mask	= AlignTop | AlignBottom | AlignVCenter,
		};

		/**
		 * Resize the rp_image.
		 *
		 * A new rp_image will be created with the specified dimensions,
		 * and the current image will be copied into the new image.
		 *
		 * If the new dimensions are smaller than the old dimensions,
		 * the image will be cropped according to the specified alignment.
		 *
		 * If the new dimensions are larger, the original image will be
		 * aligned to the top, center, or bottom, depending on alignment,
		 * and if the image is ARGB32, the new space will be set to bgColor.
		 *
		 * @param width New width
		 * @param height New height
		 * @param alignment Alignment (Vertical only!)
		 * @param bgColor Background color for empty space. (default is ARGB 0x00000000)
		 * @return New rp_image with a resized version of the original, or nullptr on error.
		 */
		RP_LIBROMDATA_PUBLIC
		rp_image *resized(int width, int height,
			Alignment alignment = AlignDefault,
			uint32_t bgColor = 0x00000000) const;

		/**
		 * Un-premultiply this image.
		 * Standard version using regular C++ code.
		 *
		 * Image must be ARGB32.
		 *
		 * @return 0 on success; non-zero on error.
		 */
		RP_LIBROMDATA_PUBLIC
		int un_premultiply_cpp(void);

#ifdef RP_IMAGE_HAS_SSE41
		/**
		 * Un-premultiply this image.
		 * SSE4.1-optimized version.
		 *
		 * Image must be ARGB32.
		 *
		 * @return 0 on success; non-zero on error.
		 */
		RP_LIBROMDATA_PUBLIC
		int un_premultiply_sse41(void);
#endif /* RP_IMAGE_HAS_SSE41 */

		/**
		 * Un-premultiply this image.
		 *
		 * Image must be ARGB32.
		 *
		 * @return 0 on success; non-zero on error.
		 */
		inline int un_premultiply(void);

		/**
		 * rp_image wrapper function for premultiply_pixel().
		 * @param px	[in] ARGB32 pixel to premultiply.
		 * @return Premultiplied pixel.
		 */
		RP_LIBROMDATA_PUBLIC
		static uint32_t premultiply_pixel(uint32_t px);

		/**
		 * Premultiply this image.
		 *
		 * Image must be ARGB32.
		 *
		 * @return 0 on success; non-zero on error.
		 */
		RP_LIBROMDATA_PUBLIC
		int premultiply(void);

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
		inline int apply_chroma_key(uint32_t key);

		enum FlipOp : uint8_t {
			FLIP_NONE	= 0,
			FLIP_V		= (1U << 0),
			FLIP_H		= (1U << 1),
			FLIP_VH		= (FLIP_V | FLIP_H),
		};

		/**
		 * Flip the image.
		 *
		 * This function returns a *new* image and leaves the
		 * original image unmodified.
		 *
		 * @param op Flip operation.
		 * @return Flipped image, or nullptr on error.
		 */
		RP_LIBROMDATA_PUBLIC
		rp_image *flip(FlipOp op) const;

		/**
		 * Shrink image dimensions.
		 * @param width New width.
		 * @param height New height.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int shrink(int width, int height);

		/** swapRB() **/

		/**
		 * Swap Red and Blue channels in an ARGB32 image.
		 * Standard version using regular C++ code.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int swapRB_cpp(void);

#ifdef RP_IMAGE_HAS_SSSE3
		/**
		 * Swap Red and Blue channels in an ARGB32 image.
		 * SSSE3-optimized version.
		 *
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int swapRB_ssse3(void);
#endif /* RP_IMAGE_HAS_SSSE3 */

		/**
		 * Swap Red and Blue channels in an ARGB32 image.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline int swapRB(void);

		/** swizzle() **/

		/**
		 * Swizzle the image channels.
		 * Standard version using regular C++ code.
		 *
		 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int swizzle_cpp(const char *swz_spec);

#ifdef RP_IMAGE_HAS_SSSE3
		/**
		 * Swizzle the image channels.
		 * SSSE3-optimized version.
		 *
		 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int swizzle_ssse3(const char *swz_spec);
#endif /* RP_IMAGE_HAS_SSSE3 */

		/**
		 * Swizzle the image channels.
		 *
		 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
		 * @return 0 on success; negative POSIX error code on error.
		 */
		inline int swizzle(const char *swz_spec);

		/**
		 * Unswizzle GIMP-DDS YCoCg.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int unswizzle_YCoCg(void);
};

/**
 * Un-premultiply this image.
 *
 * Image must be ARGB32.
 *
 * @return 0 on success; non-zero on error.
 */
inline int rp_image::un_premultiply(void)
{
	// FIXME: Figure out how to get IFUNC working with C++ member functions.
#ifdef RP_IMAGE_HAS_SSE41
	if (RP_CPU_HasSSE41()) {
		return un_premultiply_sse41();
	} else
#endif /* RP_IMAGE_HAS_SSE2 */
	{
		return un_premultiply_cpp();
	}
}

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
	// FIXME: Figure out how to get IFUNC working with C++ member functions.
#if defined(RP_IMAGE_ALWAYS_HAS_SSE2)
	// amd64 always has SSE2.
	return apply_chroma_key_sse2(key);
#else
#  if defined(RP_IMAGE_HAS_SSE2)
	if (RP_CPU_HasSSE2()) {
		return apply_chroma_key_sse2(key);
	} else
#  endif /* RP_IMAGE_HAS_SSE2 */
	{
		return apply_chroma_key_cpp(key);
	}
#endif /* RP_IMAGE_ALWAYS_HAS_SSE2 */
}

/**
 * Swap Red and Blue channels in an ARGB32 image.
 * @return 0 on success; negative POSIX error code on error.
 */
inline int rp_image::swapRB(void)
{
	// FIXME: Figure out how to get IFUNC working with C++ member functions.
#if defined(RP_IMAGE_HAS_SSSE3)
	if (RP_CPU_HasSSSE3()) {
		return swapRB_ssse3();
	} else
#endif /* RP_IMAGE_HAS_SSSE3 */
	{
		return swapRB_cpp();
	}
}

/**
 * Swizzle the image channels.
 *
 * @param swz_spec Swizzle specification: [rgba01]{4} [matches KTX2]
 * @return 0 on success; negative POSIX error code on error.
 */
inline int rp_image::swizzle(const char *swz_spec)
{
	// FIXME: Figure out how to get IFUNC working with C++ member functions.
#if defined(RP_IMAGE_HAS_SSSE3)
	if (RP_CPU_HasSSSE3()) {
		return swizzle_ssse3(swz_spec);
	} else
#endif /* RP_IMAGE_HAS_SSSE3 */
	{
		return swizzle_cpp(swz_spec);
	}
}

}

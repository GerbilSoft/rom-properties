/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_Linear.hpp: Image decoding functions: Linear               *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ImageDecoder_common.hpp"

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a linear CI4 image to rp_image with a little-endian 16-bit palette.
 * @param px_format Palette pixel format.
 * @param msn_left If true, most-significant nybble is the left pixel.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2 for 16-bit, >= 16*4 for 32-bit]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 5, 6)
ATTR_ACCESS_SIZE(read_only, 7, 8)
rp_image_ptr fromLinearCI4(PixelFormat px_format, bool msn_left,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const void *RESTRICT pal_buf, size_t pal_siz);

/**
 * Convert a linear CI8 image to rp_image with a little-endian 16-bit palette.
 * @param px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2 for 16-bit, >= 256*4 for 32-bit]
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
ATTR_ACCESS_SIZE(read_only, 6, 7)
rp_image_ptr fromLinearCI8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	const void *RESTRICT pal_buf, size_t pal_siz);

/**
 * Convert a linear monochrome image to rp_image.
 * @param width		[in] Image width
 * @param height	[in] Image height
 * @param img_buf	[in] Monochrome image buffer
 * @param img_siz	[in] Size of image data [must be >= (w*h)/8]
 * @param stride	[in,opt] Stride, in bytes (if 0, assumes width*bytespp)
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 3, 4)
rp_image_ptr fromLinearMono(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

/**
 * Convert a linear 8-bit RGB image to rp_image.
 * Usually used for luminance and alpha images.
 * @param px_format	[in] 8-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 8-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
rp_image_ptr fromLinear8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

/** 16-bit **/

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear16_cpp(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

#ifdef IMAGEDECODER_HAS_SSE2
/**
 * Convert a linear 16-bit RGB image to rp_image.
 * SSE2-optimized version.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear16_sse2(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSE2 */

#if defined(HAVE_IFUNC) && (defined(RP_CPU_I386) || defined(RP_CPU_AMD64))

#  ifdef IMAGEDECODER_ALWAYS_HAS_SSE2
// System does support IFUNC, but it's always guaranteed to have SSE2.
// Eliminate the IFUNC dispatch on this system.

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
static inline rp_image_ptr fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *img_buf, size_t img_siz, int stride = 0)
{
	// amd64 always has SSE2.
	return fromLinear16_sse2(px_format, width, height, img_buf, img_siz, stride);
}
#  else /* !IMAGEDECODER_ALWAYS_HAS_SSE2 */
// System supports IFUNC and is not guaranteed to always have SSE2.

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
IFUNC_SSE2_STATIC_INLINE rp_image_ptr fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#  endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */

#else /* !HAVE_IFUNC or not i386/amd64 */
// System does not support IFUNC, or we aren't guaranteed to have
// optimizations for these CPUs. Use standard inline dispatch.

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
static inline rp_image_ptr fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, size_t img_siz, int stride = 0)
{
#  ifdef IMAGEDECODER_ALWAYS_HAS_SSE2
	// amd64 always has SSE2.
	return fromLinear16_sse2(px_format, width, height, img_buf, img_siz, stride);
#  else /* !IMAGEDECODER_ALWAYS_HAS_SSE2 */
#    ifdef IMAGEDECODER_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return fromLinear16_sse2(px_format, width, height, img_buf, img_siz, stride);
	} else
#    endif /* IMAGEDECODER_HAS_SSE2 */
	{
		return fromLinear16_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
#  endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */
}

#endif /* HAVE_IFUNC */

/** 24-bit **/

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear24_cpp(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

#ifdef IMAGEDECODER_HAS_SSSE3
/**
 * Convert a linear 24-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear24_ssse3(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSSE3 */

#if defined(HAVE_IFUNC) && (defined(RP_CPU_I386) || defined(RP_CPU_AMD64))
/**
 * Convert a linear 24-bit RGB image to rp_image.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
IFUNC_STATIC_INLINE rp_image_ptr fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#else
// System does not support IFUNC, or we aren't guaranteed to have
// optimizations for these CPUs. Use standard inline dispatch.

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
static inline rp_image_ptr fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz, int stride = 0)
{
#  ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return fromLinear24_ssse3(px_format, width, height, img_buf, img_siz, stride);
	} else
#  endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return fromLinear24_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
}
#endif /* HAVE_IFUNC && (RP_CPU_I386 || RP_CPU_AMD64) */

/** 32-bit **/

/**
 * Convert a linear 32-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear32_cpp(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride = 0);

#ifdef IMAGEDECODER_HAS_SSSE3
/**
 * Convert a linear 32-bit RGB image to rp_image.
 * SSSE3-optimized version.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
ATTR_ACCESS_SIZE(read_only, 4, 5)
RP_LIBROMDATA_PUBLIC
rp_image_ptr fromLinear32_ssse3(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSSE3 */

#if defined(HAVE_IFUNC) && (defined(RP_CPU_I386) || defined(RP_CPU_AMD64))
/**
 * Convert a linear 32-bit RGB image to rp_image.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
RP_LIBROMDATA_PUBLIC
IFUNC_STATIC_INLINE rp_image_ptr fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride = 0);
#else /* !(HAVE_IFUNC && (RP_CPU_I386 || RP_CPU_AMD64)) */
// System does not support IFUNC, or we aren't guaranteed to have
// optimizations for these CPUs. Use standard inline dispatch.

/**
 * Convert a linear 32-bit RGB image to rp_image.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
static inline rp_image_ptr fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, size_t img_siz, int stride = 0)
{
#  ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return fromLinear32_ssse3(px_format, width, height, img_buf, img_siz, stride);
	} else
#  endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
}
#endif /* HAVE_IFUNC && (RP_CPU_I386 || RP_CPU_AMD64) */

} }

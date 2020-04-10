/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder.cpp: Image decoding functions.                             *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_HPP__
#define __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_HPP__

#include "config.librptexture.h"

#include "common.h"
#include "librpcpu/cpu_dispatch.h"

// C includes.
#include <stdint.h>

#if defined(RP_CPU_I386) || defined(RP_CPU_AMD64)
# include "librpcpu/cpuflags_x86.h"
# ifdef RP_HAS_IFUNC
#  define IMAGEDECODER_HAS_IFUNC 1
# endif
# define IMAGEDECODER_HAS_SSE2 1
# define IMAGEDECODER_HAS_SSSE3 1
#endif
#ifdef RP_CPU_AMD64
# define IMAGEDECODER_ALWAYS_HAS_SSE2 1
#endif

namespace LibRpTexture {
	class rp_image;
}

namespace LibRpTexture { namespace ImageDecoder {

// Pixel formats.
enum PixelFormat {
	PXF_UNKNOWN,

	// 16-bit
	PXF_RGB565,	// xRRRRRGG GGGBBBBB
	PXF_BGR565,	// xBBBBBGG GGGRRRRR
	PXF_ARGB1555,	// ARRRRRGG GGGBBBBB
	PXF_ABGR1555,	// ABBBBBGG GGGRRRRR
	PXF_RGBA5551,	// RRRRRGGG GGBBBBBA
	PXF_BGRA5551,	// BBBBBGGG GGRRRRRA
	PXF_ARGB4444,	// AAAARRRR GGGGBBBB
	PXF_ABGR4444,	// AAAABBBB GGGGRRRR
	PXF_RGBA4444,	// RRRRGGGG BBBBAAAA
	PXF_BGRA4444,	// BBBBGGGG RRRRAAAA
	PXF_xRGB4444,	// xxxxRRRR GGGGBBBB
	PXF_xBGR4444,	// xxxxBBBB GGGGRRRR
	PXF_RGBx4444,	// RRRRGGGG BBBBxxxx
	PXF_BGRx4444,	// BBBBGGGG RRRRxxxx

	// Uncommon 16-bit formats.
	PXF_ARGB8332,	// AAAAAAAA RRRGGGBB

	// GameCube-specific 16-bit
	PXF_RGB5A3,	// High bit determines RGB555 or ARGB4444.
	PXF_IA8,	// Intensity/Alpha.

	// PlayStation 2-specific 16-bit
	PXF_BGR5A3,	// Like PXF_RGB5A3, but with
			// swapped R and B channels.

	// 15-bit
	PXF_RGB555,
	PXF_BGR555,
	PXF_BGR555_PS1,	// Special transparency handling.

	// 24-bit
	PXF_RGB888,
	PXF_BGR888,

	// 32-bit with alpha channel.
	PXF_ARGB8888,
	PXF_ABGR8888,
	PXF_RGBA8888,
	PXF_BGRA8888,
	// 32-bit with unused alpha channel.
	PXF_xRGB8888,
	PXF_xBGR8888,
	PXF_RGBx8888,
	PXF_BGRx8888,

	// PlayStation 2-specific 32-bit
	PXF_BGR888_ABGR7888,	// Why is this a thing.
				// If the high bit is set, it's BGR888.
				// Otherwise, it's ABGR7888.

	// Uncommon 32-bit formats.
	PXF_G16R16,
	PXF_A2R10G10B10,
	PXF_A2B10G10R10,
	PXF_RGB9_E5,

	// Uncommon 16-bit formats.
	PXF_RG88,
	PXF_GR88,

	// VTFEdit uses this as "ARGB8888".
	// TODO: Might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
	PXF_RABG8888,

	// Luminance formats.
	PXF_L8,		// LLLLLLLL
	PXF_A4L4,	// AAAAllll
	PXF_L16,	// LLLLLLLL llllllll
	PXF_A8L8,	// AAAAAAAA LLLLLLLL
	PXF_L8A8,	// LLLLLLLL AAAAAAAA

	// Alpha formats.
	PXF_A8,		// AAAAAAAA

	// Endian-specific ARGB32 definitions.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
	PXF_HOST_ARGB32 = PXF_ARGB8888,
	PXF_HOST_RGBA32 = PXF_RGBA8888,
	PXF_HOST_xRGB32 = PXF_xRGB8888,
	PXF_HOST_RGBx32 = PXF_RGBx8888,
	PXF_SWAP_ARGB32 = PXF_BGRA8888,
	PXF_SWAP_RGBA32 = PXF_ABGR8888,
	PXF_SWAP_xRGB32 = PXF_BGRx8888,
	PXF_SWAP_RGBx32 = PXF_xBGR8888,
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
	PXF_HOST_ARGB32 = PXF_BGRA8888,
	PXF_HOST_RGBA32 = PXF_ABGR8888,
	PXF_HOST_xRGB32 = PXF_BGRx8888,
	PXF_HOST_RGBx32 = PXF_xBGR8888,
	PXF_SWAP_ARGB32 = PXF_ARGB8888,
	PXF_SWAP_RGBA32 = PXF_RGBA8888,
	PXF_SWAP_xRGB32 = PXF_xRGB8888,
	PXF_SWAP_RGBx32 = PXF_RGBx8888,
#endif
};

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
rp_image *fromLinearCI4(PixelFormat px_format, bool msn_left,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const void *RESTRICT pal_buf, int pal_siz);

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
rp_image *fromLinearCI8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const void *RESTRICT pal_buf, int pal_siz);

/**
 * Convert a linear monochrome image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf Monochrome image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/8]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromLinearMono(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

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
rp_image *fromLinear8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride = 0);

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
rp_image *fromLinear16_cpp(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz, int stride = 0);

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
rp_image *fromLinear16_sse2(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSE2 */

#if defined(IMAGEDECODER_HAS_IFUNC) && !defined(IMAGEDECODER_ALWAYS_HAS_SSE2)
/* System has IFUNC. Use it for dispatching. */

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
rp_image *fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz, int stride = 0);

#else

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
static inline rp_image *fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz, int stride = 0)
{
#ifdef IMAGEDECODER_ALWAYS_HAS_SSE2
	// amd64 always has SSE2.
	return fromLinear16_sse2(px_format, width, height, img_buf, img_siz, stride);
#else /* !IMAGEDECODER_ALWAYS_HAS_SSE2 */
# ifdef IMAGEDECODER_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return fromLinear16_sse2(px_format, width, height, img_buf, img_siz, stride);
	} else
# endif /* IMAGEDECODER_HAS_SSE2 */
	{
		return fromLinear16_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
#endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */
}

#endif

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
rp_image *fromLinear24_cpp(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride = 0);

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
rp_image *fromLinear24_ssse3(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSSE3 */

#ifdef IMAGEDECODER_HAS_IFUNC
/* System has IFUNC. Use it for dispatching. */

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
rp_image *fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride = 0);

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
static inline rp_image *fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride = 0)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return fromLinear24_ssse3(px_format, width, height, img_buf, img_siz, stride);
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return fromLinear24_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
}

#endif

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
rp_image *fromLinear32_cpp(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, int img_siz, int stride = 0);

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
rp_image *fromLinear32_ssse3(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, int img_siz, int stride = 0);
#endif /* IMAGEDECODER_HAS_SSSE3 */

#ifdef IMAGEDECODER_HAS_IFUNC
/* System has IFUNC. Use it for dispatching. */

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
rp_image *fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, int img_siz, int stride = 0);

#else
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
static inline rp_image *fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, int img_siz, int stride = 0)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return fromLinear32_ssse3(px_format, width, height, img_buf, img_siz, stride);
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return fromLinear32_cpp(px_format, width, height, img_buf, img_siz, stride);
	}
}

#endif

/** GameCube **/

/**
 * Convert a GameCube 16-bit image to rp_image.
 * @param px_format 16-bit pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB5A3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromGcn16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a GameCube CI8 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromGcnCI8(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);

/**
 * Convert a GameCube I8 image to rp_image.
 * NOTE: Uses a grayscale palette.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf I8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromGcnI8(int width, int height,
	const uint8_t *img_buf, int img_siz);

/** Nintendo DS **/

/**
 * Convert a Nintendo DS CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromNDS_CI4(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);

/** Nintendo 3DS **/

/**
 * Convert a Nintendo 3DS RGB565 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromN3DSTiledRGB565(int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a Nintendo 3DS RGB565+A4 tiled icon to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf RGB565 tiled image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param alpha_buf A4 tiled alpha buffer.
 * @param alpha_siz Size of alpha data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromN3DSTiledRGB565_A4(int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz,
	const uint8_t *RESTRICT alpha_buf, int alpha_siz);

/* S3TC */

/**
 * Convert a GameCube DXT1 image to rp_image.
 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT1_GCN(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as black.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT1(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT1_A1(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT2 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT2 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT2(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT3(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT4(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a DXT5 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDXT5(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a BC4 (ATI1) image to rp_image.
 * Color component is Red.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromBC4(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a BC5 (ATI2) image to rp_image.
 * Color components are Red and Green.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromBC5(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a Red image to Luminance.
 * Use with fromBC4() to decode an LATC1 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
int fromRed8ToL8(rp_image *img);

/**
 * Convert a Red+Green image to Luminance+Alpha.
 * Use with fromBC5() to decode an LATC2 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
 int fromRG8ToLA8(rp_image *img);

/* Dreamcast */

/**
 * Convert a Dreamcast square twiddled 16-bit image to rp_image.
 * @param px_format 16-bit pixel format.
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf 16-bit image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDreamcastSquareTwiddled16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz);

/**
 * Convert a Dreamcast vector-quantized image to rp_image.
 * @param px_format Palette pixel format.
 * @param smallVQ If true, handle this image as SmallVQ.
 * @param hasMipmaps If true, the image has mipmaps. (Needed for SmallVQ.)
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf VQ image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 1024*2; for SmallVQ, 64*2, 256*2, or 512*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromDreamcastVQ16(PixelFormat px_format,
	bool smallVQ, bool hasMipmaps,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);

/**
 * Get the number of palette entries for Dreamcast SmallVQ textures.
 * This version is for textures without mipmaps.
 * TODO: constexpr?
 * @param width Texture width.
 * @return Number of palette entries.
 */
static inline int calcDreamcastSmallVQPaletteEntries_NoMipmaps(int width)
{
	if (width <= 16) {
		return 8*4;
	} else if (width <= 32) {
		return 32*4;
	} else if (width <= 64) {
		return 128*4;
	} else {
		return 256*4;
	}
}

/**
 * Get the number of palette entries for Dreamcast SmallVQ textures.
 * This version is for textures with mipmaps.
 * TODO: constexpr?
 * @param width Texture width.
 * @return Number of palette entries.
 */
static inline int calcDreamcastSmallVQPaletteEntries_WithMipmaps(int width)
{
	if (width <= 16) {
		return 16*4;
	} else if (width <= 32) {
		return 64*4;
	} else if (width <= 64) {
		return 128*4;
	} else {
		return 256*4;
	}
}

/* ETC1 */

/**
 * Convert an ETC1 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromETC1(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert an ETC2 RGB image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromETC2_RGB(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert an ETC2 RGBA image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGBA image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromETC2_RGBA(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

/**
 * Convert an ETC2 RGB+A1 (punchthrough alpha) image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC2 RGB+A1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromETC2_RGB_A1(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz);

#ifdef ENABLE_PVRTC
/* PVRTC */

enum PVRTC_Mode_e {
	// bpp
	PVRTC_4BPP		= (0U << 0),
	PVRTC_2BPP		= (1U << 0),
	PVRTC_BPP_MASK		= (1U << 0),

	// Alpha channel (PVRTC-I only)
	PVRTC_ALPHA_NONE	= (0U << 1),
	PVRTC_ALPHA_YES		= (1U << 1),
	PVRTC_ALPHA_MASK	= (1U << 1),
};

/**
 * Convert a PVRTC 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTC(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	uint8_t mode);

/**
 * Convert a PVRTC-II 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTCII(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	uint8_t mode);
#endif /* ENABLE_PVRTC */

/* BC7 */

/**
 * Convert a BC7 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC7 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromBC7(int width, int height,
	const uint8_t *img_buf, int img_siz);

} }

#endif /* __ROMPROPERTIES_LIBRPTEXTURE_DECODER_IMAGEDECODER_HPP__ */

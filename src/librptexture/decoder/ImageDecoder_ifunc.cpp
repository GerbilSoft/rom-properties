/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_ifunc.cpp: ImageDecoder IFUNC resolution functions.        *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "librpcpu/cpu_dispatch.h"

#ifdef HAVE_IFUNC

#include "ImageDecoder.hpp"
using namespace LibRpTexture;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

#ifndef IMAGEDECODER_ALWAYS_HAS_SSE2
/**
 * IFUNC resolver function for fromLinear16().
 * @return Function pointer.
 */
static __typeof__(&ImageDecoder::fromLinear16_cpp) fromLinear16_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return &ImageDecoder::fromLinear16_sse2;
	} else
#endif /* IMAGEDECODER_HAS_SSE2 */
	{
		return &ImageDecoder::fromLinear16_cpp;
	}
}
#endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */

/**
 * IFUNC resolver function for fromLinear24().
 * @return Function pointer.
 */
static __typeof__(&ImageDecoder::fromLinear24_cpp) fromLinear24_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &ImageDecoder::fromLinear24_ssse3;
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return &ImageDecoder::fromLinear24_cpp;
	}
}

/**
 * IFUNC resolver function for fromLinear32().
 * @return Function pointer.
 */
static __typeof__(&ImageDecoder::fromLinear32_cpp) fromLinear32_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return &ImageDecoder::fromLinear32_ssse3;
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return &ImageDecoder::fromLinear32_cpp;
	}
}

}

#ifndef IMAGEDECODER_ALWAYS_HAS_SSE2
rp_image *ImageDecoder::fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *img_buf, size_t img_siz, int stride)
	IFUNC_ATTR(fromLinear16_resolve);
#endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */

rp_image *ImageDecoder::fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *img_buf, size_t img_siz, int stride)
	IFUNC_ATTR(fromLinear24_resolve);

rp_image *ImageDecoder::fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *img_buf, size_t img_siz, int stride)
	IFUNC_ATTR(fromLinear32_resolve);

#endif /* HAVE_IFUNC */

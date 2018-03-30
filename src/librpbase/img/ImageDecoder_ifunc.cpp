/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_ifunc.cpp: ImageDecoder IFUNC resolution functions.        *
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

#include "cpu_dispatch.h"

#ifdef RP_HAS_IFUNC

#include "ImageDecoder.hpp"
using LibRpBase::rp_image;
using LibRpBase::ImageDecoder;

// IFUNC attribute doesn't support C++ name mangling.
extern "C" {

#ifndef IMAGEDECODER_ALWAYS_HAS_SSE2
/**
 * IFUNC resolver function for fromLinear16().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t fromLinear16_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSE2
	if (RP_CPU_HasSSE2()) {
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear16_sse2;
	} else
#endif /* IMAGEDECODER_HAS_SSE2 */
	{
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear16_cpp;
	}
}
#endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */

/**
 * IFUNC resolver function for fromLinear24().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t fromLinear24_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear24_ssse3;
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear24_cpp;
	}
}

/**
 * IFUNC resolver function for fromLinear32().
 * @return Function pointer.
 */
static RP_IFUNC_ptr_t fromLinear32_resolve(void)
{
#ifdef IMAGEDECODER_HAS_SSSE3
	if (RP_CPU_HasSSSE3()) {
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear32_ssse3;
	} else
#endif /* IMAGEDECODER_HAS_SSSE3 */
	{
		return (RP_IFUNC_ptr_t)&ImageDecoder::fromLinear32_cpp;
	}
}

}

#ifndef IMAGEDECODER_ALWAYS_HAS_SSE2
rp_image *ImageDecoder::fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *img_buf, int img_siz, int stride)
	IFUNC_ATTR(fromLinear16_resolve);
#endif /* IMAGEDECODER_ALWAYS_HAS_SSE2 */

rp_image *ImageDecoder::fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *img_buf, int img_siz, int stride)
	IFUNC_ATTR(fromLinear24_resolve);

rp_image *ImageDecoder::fromLinear32(PixelFormat px_format,
	int width, int height,
	const uint32_t *img_buf, int img_siz, int stride)
	IFUNC_ATTR(fromLinear32_resolve);

#endif /* RP_HAS_IFUNC */

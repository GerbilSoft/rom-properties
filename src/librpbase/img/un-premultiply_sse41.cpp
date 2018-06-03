/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * un-premultiply_sse41.cpp: Un-premultiply function.                      *
 * SSE4.1-optimized version.                                               *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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

#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

#include "../common.h"

// C includes. (C++ namespace)
#include <cassert>

// SSE4.1 headers.
#include <emmintrin.h>
#include <smmintrin.h>

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpBase {

/**
 * Un-premultiply an argb32_t pixel. (SSE4.1 version)
 * From qt-5.11.0's qdrawingprimitive_sse2_p.h.
 * qUnpremultiply_sse4()
 *
 * This is needed in order to convert DXT2/3 to DXT4/5.
 *
 * @param px	[in] ARGB32 pixel to un-premultiply.
 * @return Un-premultiplied pixel.
 */
static FORCEINLINE uint32_t un_premultiply_pixel_sse41(uint32_t px)
{
	const unsigned int alpha = (px >> 24);
	if (alpha == 255 || alpha == 0)
		return px;

	const unsigned int invAlpha = rp_image::qt_inv_premul_factor[alpha];
	const __m128i via = _mm_set1_epi32(invAlpha);
	const __m128i vr = _mm_set1_epi32(0x8000);
	__m128i vl = _mm_cvtepu8_epi32(_mm_cvtsi32_si128(px));
	vl = _mm_mullo_epi32(vl, via);
	vl = _mm_add_epi32(vl, vr);
	vl = _mm_srai_epi32(vl, 16);
	vl = _mm_insert_epi32(vl, alpha, 3);
	vl = _mm_packus_epi32(vl, vl);
	vl = _mm_packus_epi16(vl, vl);
	return _mm_cvtsi128_si32(vl);
}

/**
 * Un-premultiply an ARGB32 rp_image.
 * Image must be ARGB32.
 * @return 0 on success; non-zero on error.
 */
int rp_image::un_premultiply_sse41(void)
{
	RP_D(const rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == rp_image::FORMAT_ARGB32);
	if (backend->format != rp_image::FORMAT_ARGB32) {
		// Incorrect format...
		return -1;
	}

	const int width = backend->width;
	argb32_t *px_dest = static_cast<argb32_t*>(backend->data());
	int dest_stride_adj = (backend->stride / sizeof(*px_dest)) - width;
	for (int y = backend->height; y > 0; y--, px_dest += dest_stride_adj) {
		int x = width;
		for (; x > 1; x -= 2, px_dest += 2) {
			px_dest[0].u32 = un_premultiply_pixel_sse41(px_dest[0].u32);
			px_dest[1].u32 = un_premultiply_pixel_sse41(px_dest[1].u32);
		}
		if (x == 1) {
			px_dest->u32 = un_premultiply_pixel_sse41(px_dest->u32);
			px_dest++;
		}
	}
	return 0;
}

}

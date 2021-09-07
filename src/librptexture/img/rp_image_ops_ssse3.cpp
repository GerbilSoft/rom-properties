/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * rp_image_ops.cpp: Image class. (operations)                             *
 * SSSE3-optimized version.                                                 *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// SSSE3 intrinsics
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpTexture {

/** Image operations. **/

/**
 * Swap Red and Blue channels in an ARGB32 image.
 * SSSE3-optimized version.
 *
 * NOTE: The image *must* be ARGB32.
 *
 * @return 0 on success; negative POSIX error code on error.
 */
int rp_image::swapRB_ssse3(void)
{
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;

	// ABGR shuffle mask
	const __m128i shuf_mask = _mm_setr_epi8(2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15);

	switch (backend->format) {
		default:
			// Unsupported image format.
			assert(!"Unsupported rp_image::Format.");
			return -EINVAL;

		case rp_image::Format::ARGB32: {
			argb32_t *img_buf = static_cast<argb32_t*>(backend->data());
			const int row_width = backend->stride / sizeof(uint32_t);

			for (unsigned int y = static_cast<unsigned int>(backend->height); y > 0; y--) {
				// Process 16 pixels per iteration using SSSE3.
				unsigned int x = static_cast<unsigned int>(backend->width);
				__m128i *xmm_buf = reinterpret_cast<__m128i*>(img_buf);
				for (; x > 15; x -= 16, xmm_buf += 4) {
					__m128i sa = _mm_load_si128(&xmm_buf[0]);
					__m128i sb = _mm_load_si128(&xmm_buf[1]);
					__m128i sc = _mm_load_si128(&xmm_buf[2]);
					__m128i sd = _mm_load_si128(&xmm_buf[3]);

					_mm_store_si128(&xmm_buf[0], _mm_shuffle_epi8(sa, shuf_mask));
					_mm_store_si128(&xmm_buf[1], _mm_shuffle_epi8(sb, shuf_mask));
					_mm_store_si128(&xmm_buf[2], _mm_shuffle_epi8(sc, shuf_mask));
					_mm_store_si128(&xmm_buf[3], _mm_shuffle_epi8(sd, shuf_mask));
				}

				// Remaining pixels.
				argb32_t *px32 = reinterpret_cast<argb32_t*>(xmm_buf);
				for (; x > 0; x--, px32++) {
					std::swap(px32->r, px32->b);
				}

				// Next line.
				img_buf += row_width;
			}
			break;
		}

		case rp_image::Format::CI8: {
			argb32_t *pal = reinterpret_cast<argb32_t*>(backend->palette());
			const int pal_len = backend->palette_len();
			assert(pal != nullptr);
			assert(pal_len > 0);
			if (!pal || pal_len <= 0) {
				return -EINVAL;
			}

			// Process 16 colors per iteration using SSSE3.
			unsigned int i;
			__m128i *xmm_pal = reinterpret_cast<__m128i*>(pal);
			for (i = (unsigned int)pal_len; i > 15; i -= 16, pal += 4) {
				__m128i sa = _mm_load_si128(&xmm_pal[0]);
				__m128i sb = _mm_load_si128(&xmm_pal[1]);
				__m128i sc = _mm_load_si128(&xmm_pal[2]);
				__m128i sd = _mm_load_si128(&xmm_pal[3]);

				_mm_store_si128(&xmm_pal[0], _mm_shuffle_epi8(sa, shuf_mask));
				_mm_store_si128(&xmm_pal[1], _mm_shuffle_epi8(sb, shuf_mask));
				_mm_store_si128(&xmm_pal[2], _mm_shuffle_epi8(sc, shuf_mask));
				_mm_store_si128(&xmm_pal[3], _mm_shuffle_epi8(sd, shuf_mask));
			}

			// Remaining colors
			argb32_t *pal32 = reinterpret_cast<argb32_t*>(xmm_pal);
			for (; i > 0; i--, pal32++) {
				std::swap(pal32->r, pal32->b);
			}
			break;
		}
	}

	// R and B channels swapped.
	return 0;
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * rp_image_ops.cpp: Image class. (operations)                             *
 * SSE2-optimized version.                                                 *
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

#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// SSE2 intrinsics.
#include <emmintrin.h>

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpBase {

/** Image operations. **/

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
int rp_image::apply_chroma_key_sse2(uint32_t key)
{
	RP_D(rp_image);
	rp_image_backend *const backend = d->backend;
	assert(backend->format == FORMAT_ARGB32);
	if (backend->format != FORMAT_ARGB32) {
		// ARGB32 only.
		return -EINVAL;
	}

	const unsigned int diff = (backend->stride - this->row_bytes()) / sizeof(uint32_t);
	uint32_t *img_buf = reinterpret_cast<uint32_t*>(backend->data());

	// SSE2 constants.
	const __m128i xmm_key = _mm_setr_epi32(key, key, key, key);
	static const __m128i xmm_ones = _mm_setr_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);

	for (unsigned int y = (unsigned int)backend->height; y > 0; y--) {
		// Process 4 pixels per iteration with SSE2.
		unsigned int x = (unsigned int)backend->width;
		for (; x > 3; x -= 4, img_buf += 4) {
			__m128i *xmm_data = reinterpret_cast<__m128i*>(img_buf);

			// Compare the pixels to the chroma key.
			// Equal values will be 0xFFFFFFFF.
			// Non-equal values will be 0x00000000.
			__m128i res = _mm_cmpeq_epi32(*xmm_data, xmm_key);

			// Invert the results and mask the original data.
			// Original data will now have 00s for chroma-keyed pixels.
			*xmm_data = _mm_and_si128(_mm_xor_si128(res, xmm_ones), *xmm_data);
		}

		// Remaining pixels.
		for (; x > 0; x--, img_buf++) {
			if (*img_buf == key) {
				*img_buf = 0;
			}
		}

		// Next row.
		img_buf += diff;
	}

	// Adjust sBIT.
	// TODO: Only if transparent pixels were found.
	if (d->has_sBIT && d->sBIT.alpha == 0) {
		d->sBIT.alpha = 1;
	}

	// Chroma key applied.
	return 0;
}

}

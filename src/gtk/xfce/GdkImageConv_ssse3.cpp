/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * GdkImageConv.cpp: Helper functions to convert from rp_image to GDK.     *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GdkImageConv.hpp"

// librptexture
using namespace LibRpTexture;

// SSSE3 intrinsics
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

/**
 * GdkPixbufDestroyNotify() callback.
 * @param pixels Pixel data.
 * @param data Other data. (unused)
 */
static void rp_gdkPixbufDestroyNotify(guchar *pixels, gpointer data)
{
	RP_UNUSED(data);
	aligned_free(pixels);
}

/**
 * Convert an rp_image to GdkPixbuf.
 * SSSE3-optimized version.
 * @param img	[in] rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf_ssse3(const rp_image *img)
{
	assert(img != nullptr);
	if (unlikely(!img || !img->isValid()))
		return nullptr;

	// We need to allocate our own image buffer, since GdkPixbuf
	// only guarantees 4-byte alignment.
	const int width = img->width();
	const int height = img->height();
	const int rowstride = ALIGN_BYTES(16, width * sizeof(uint32_t));
	uint32_t *px_dest = static_cast<uint32_t*>(aligned_malloc(16, height * rowstride));
	assert(px_dest != nullptr);
	if (unlikely(!px_dest)) {
		// Unable to allocate memory.
		return nullptr;
	}

	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(
		reinterpret_cast<const guchar*>(px_dest),
		GDK_COLORSPACE_RGB, true, 8, width, height,
		rowstride, rp_gdkPixbufDestroyNotify, nullptr);
	assert(pixbuf != nullptr);
	if (unlikely(!pixbuf)) {
		// Unable to create a GdkPixbuf.
		aligned_free(px_dest);
		return nullptr;
	}

	// Sanity check: Make sure rowstride is correct.
	assert(gdk_pixbuf_get_rowstride(pixbuf) == rowstride);
	const int dest_row_width = rowstride / sizeof(*px_dest);

	// ABGR shuffle mask
	const __m128i shuf_mask = _mm_setr_epi8(2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15);

	switch (img->format()) {
		case rp_image::Format::ARGB32: {
			// Copy the image data.
			const argb32_t *img_buf = static_cast<const argb32_t*>(img->bits());
			const int src_row_width = img->stride() / sizeof(argb32_t);

			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 16 pixels per iteration using SSSE3.
				unsigned int x = static_cast<unsigned int>(width);
				const __m128i *xmm_src = reinterpret_cast<const __m128i*>(img_buf);
				__m128i *xmm_dest = reinterpret_cast<__m128i*>(px_dest);
				for (; x > 15; x -= 16, xmm_src += 4, xmm_dest += 4) {
					__m128i sa = _mm_load_si128(&xmm_src[0]);
					__m128i sb = _mm_load_si128(&xmm_src[1]);
					__m128i sc = _mm_load_si128(&xmm_src[2]);
					__m128i sd = _mm_load_si128(&xmm_src[3]);

					_mm_store_si128(&xmm_dest[0], _mm_shuffle_epi8(sa, shuf_mask));
					_mm_store_si128(&xmm_dest[1], _mm_shuffle_epi8(sb, shuf_mask));
					_mm_store_si128(&xmm_dest[2], _mm_shuffle_epi8(sc, shuf_mask));
					_mm_store_si128(&xmm_dest[3], _mm_shuffle_epi8(sd, shuf_mask));
				}

				// Remaining pixels.
				const argb32_t *rpx_src = reinterpret_cast<const argb32_t*>(xmm_src);
				argb32_t *rpx_dest = reinterpret_cast<argb32_t*>(xmm_dest);
				for (; x > 0; x--, rpx_src++, rpx_dest++) {
					rpx_dest->u32 = rpx_src->u32;
					std::swap(rpx_dest->r, rpx_dest->b);
				}

				// Next line.
				img_buf += src_row_width;
				px_dest += dest_row_width;
			}
			break;
		}

		case rp_image::Format::CI8: {
			const argb32_t *src_pal = reinterpret_cast<const argb32_t*>(img->palette());
			const unsigned int src_pal_len = img->palette_len();
			assert(src_pal != nullptr);
			assert(src_pal_len > 0);
			if (!src_pal || src_pal_len == 0)
				break;

			// Get the palette.
			static const unsigned int dest_pal_len = 256;
			uint32_t *const palette = static_cast<uint32_t*>(aligned_malloc(16, dest_pal_len*sizeof(uint32_t)));
			assert(palette != nullptr);
			if (unlikely(!palette)) {
				// Unable to allocate memory for the palette.
				g_object_unref(pixbuf);
				return nullptr;
			}

			// Process 16 colors per iteration using SSSE3.
			const __m128i *xmm_src = reinterpret_cast<const __m128i*>(src_pal);
			__m128i *xmm_dest = reinterpret_cast<__m128i*>(palette);
			unsigned int i = src_pal_len;
			for (; i > 15; i -= 16, xmm_src += 4, xmm_dest += 4) {
				__m128i sa = _mm_load_si128(&xmm_src[0]);
				__m128i sb = _mm_load_si128(&xmm_src[1]);
				__m128i sc = _mm_load_si128(&xmm_src[2]);
				__m128i sd = _mm_load_si128(&xmm_src[3]);

				_mm_store_si128(&xmm_dest[0], _mm_shuffle_epi8(sa, shuf_mask));
				_mm_store_si128(&xmm_dest[1], _mm_shuffle_epi8(sb, shuf_mask));
				_mm_store_si128(&xmm_dest[2], _mm_shuffle_epi8(sc, shuf_mask));
				_mm_store_si128(&xmm_dest[3], _mm_shuffle_epi8(sd, shuf_mask));
			}

			// Remaining colors.
			src_pal = reinterpret_cast<const argb32_t*>(xmm_src);
			argb32_t *dest_pal = reinterpret_cast<argb32_t*>(xmm_dest);
			for (; i > 0; i--, dest_pal++, src_pal++) {
				dest_pal->u32 = src_pal->u32;
				std::swap(dest_pal->r, dest_pal->b);
			}

			// Zero out the rest of the palette if the new
			// palette is larger than the old palette.
			if (src_pal_len < dest_pal_len) {
				memset(dest_pal, 0, (dest_pal_len - src_pal_len) * sizeof(argb32_t));
			}

			// Convert the image data from CI8 to ARGB32.
			// (GdkPixbuf doesn't support CI8.)
			const int dest_stride_adj = (rowstride / sizeof(*px_dest)) - img->width();
			const int src_stride_adj = img->stride() - width;

			const uint8_t *img_buf = static_cast<const uint8_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 3; x -= 4) {
					px_dest[0] = palette[img_buf[0]];
					px_dest[1] = palette[img_buf[1]];
					px_dest[2] = palette[img_buf[2]];
					px_dest[3] = palette[img_buf[3]];
					px_dest += 4;
					img_buf += 4;
				}
				for (; x > 0; x--, px_dest++, img_buf++) {
					// Last pixels.
					*px_dest = palette[*img_buf];
					px_dest++;
					img_buf++;
				}

				// Next line.
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}

			aligned_free(palette);
			break;
		}

		default:
			// Unsupported image format.
			assert(!"Unsupported rp_image::Format.");
			g_clear_object(&pixbuf);
			break;
	}

	return pixbuf;
}

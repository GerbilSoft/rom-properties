/****************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                       *
 * GdkImageConv_neon.cpp: Helper functions to convert from rp_image to GDK. *
 * NEON-optimized version.                                                 *
 *                                                                          *
 * Copyright (c) 2017-2026 by David Korth.                                  *
 * SPDX-License-Identifier: GPL-2.0-or-later                                *
 ****************************************************************************/

#include "GdkImageConv.hpp"

// Other rom-properties libraries
#include "aligned_malloc.h"
#include "argb32_t.hpp"
#include "librptexture/img/rp_image.hpp"
using namespace LibRpTexture;

// C++ STL classes
#include <algorithm>
using std::array;

// ARM NEON intrinsics
#include <arm_neon.h>

// TODO: Combine this with rp_image_ops_neon.cpp?
#if defined(RP_CPU_ARM64)
static constexpr size_t VEC_LEN_U8 = 16;
typedef uint8x16_t uint8xVTBL_t;
#define vld1VTBL_u8  vld1q_u8
#define vqtbl1q_u8_u32(a, b) vreinterpretq_u32_u8(vqtbl1q_u8(vreinterpretq_u8_u32(a), (b)))
#elif defined(RP_CPU_ARM)
static constexpr size_t VEC_LEN_U8 = 8;
typedef uint8x8_t uint8xVTBL_t;
#define vld1VTBL_u8  vld1_u8
#define vtbl1_u8_u32(a, b) vreinterpret_u32_u8(vtbl1_u8(vreinterpret_u8_u32(a), (b)))
#else
#  error Unsupported CPU?
#endif

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
 * NEON-optimized version.
 * @param img	[in] rp_image.
 * @return GdkPixbuf, or nullptr on error.
 */
GdkPixbuf *GdkImageConv::rp_image_to_GdkPixbuf_neon(const rp_image *img)
{
	assert(img != nullptr);
	if (unlikely(!img || !img->isValid()))
		return nullptr;

	const int width = img->width();
	const int height = img->height();
	assert(width > 0);
	assert(height > 0);
	if (unlikely(width <= 0 || height <= 0)) {
		return nullptr;
	}

	// We need to allocate our own image buffer, since GdkPixbuf
	// only guarantees 4-byte alignment.
	const int rowstride = ALIGN_BYTES(16, static_cast<size_t>(width) * sizeof(uint32_t));
	uint32_t *px_dest = static_cast<uint32_t*>(aligned_malloc(16, static_cast<size_t>(height) * static_cast<size_t>(rowstride)));
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
	static const array<uint8_t, VEC_LEN_U8> shuf_mask_u8 = {{
		2,1,0,3, 6,5,4,7,
	#ifdef RP_CPU_ARM64
		10,9,8,11, 14,13,12,15
	#endif /* RP_CPU_ARM64 */
	}};
	uint8xVTBL_t shuf_mask = vld1VTBL_u8(shuf_mask_u8.data());

	switch (img->format()) {
		case rp_image::Format::ARGB32: {
			// Copy the image data.
			const argb32_t *img_buf = static_cast<const argb32_t*>(img->bits());
			const int src_row_width = img->stride() / sizeof(argb32_t);

			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				// Process 16 pixels per iteration using NEON (64-bit).
				unsigned int x = static_cast<unsigned int>(width);
				const uint32_t *neon_src = reinterpret_cast<const uint32_t*>(img_buf);
				uint32_t *neon_dest = px_dest;
				for (; x > 15; x -= 16, neon_src += 16, neon_dest += 16) {
#if defined(RP_CPU_ARM64)
					uint32x4x4_t sa = vld1q_u32_x4(neon_src);

					sa.val[0] = vqtbl1q_u8_u32(sa.val[0], shuf_mask);
					sa.val[1] = vqtbl1q_u8_u32(sa.val[1], shuf_mask);
					sa.val[2] = vqtbl1q_u8_u32(sa.val[2], shuf_mask);
					sa.val[3] = vqtbl1q_u8_u32(sa.val[3], shuf_mask);

					vst1q_u32_x4(neon_dest, sa);
#elif defined(RP_CPU_ARM)
					uint32x2x4_t sa = vld1_u32_x4(&neon_src[0]);
					uint32x2x4_t sb = vld1_u32_x4(&neon_src[8]);

					sa.val[0] = vtbl1_u8_u32(sa.val[0], shuf_mask);
					sa.val[1] = vtbl1_u8_u32(sa.val[1], shuf_mask);
					sa.val[2] = vtbl1_u8_u32(sa.val[2], shuf_mask);
					sa.val[3] = vtbl1_u8_u32(sa.val[3], shuf_mask);
					sb.val[0] = vtbl1_u8_u32(sb.val[0], shuf_mask);
					sb.val[1] = vtbl1_u8_u32(sb.val[1], shuf_mask);
					sb.val[2] = vtbl1_u8_u32(sb.val[2], shuf_mask);
					sb.val[3] = vtbl1_u8_u32(sb.val[3], shuf_mask);

					vst1_u32_x4(&neon_dest[0], sa);
					vst1_u32_x4(&neon_dest[8], sb);
#endif
				}

				// Remaining pixels.
				const argb32_t *rpx_src = reinterpret_cast<const argb32_t*>(neon_src);
				argb32_t *rpx_dest = reinterpret_cast<argb32_t*>(neon_dest);
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
			if (!src_pal || src_pal_len == 0) {
				g_clear_object(&pixbuf);
				break;
			}

			// Get the palette.
			static constexpr unsigned int dest_pal_len = 256;
			auto palette_uptr = aligned_uptr<uint32_t>(16, dest_pal_len);
			uint32_t *const palette = palette_uptr.get();

			// Process 16 colors per iteration using SSSE3.
			const uint32_t *neon_src = reinterpret_cast<const uint32_t*>(src_pal);
			uint32_t *neon_dest = reinterpret_cast<uint32_t*>(palette);
			unsigned int i = src_pal_len;
			for (; i > 15; i -= 16, neon_src += 16, neon_dest += 16) {
#if defined(RP_CPU_ARM64)
				uint32x4x4_t sa = vld1q_u32_x4(neon_src);

				sa.val[0] = vqtbl1q_u8_u32(sa.val[0], shuf_mask);
				sa.val[1] = vqtbl1q_u8_u32(sa.val[1], shuf_mask);
				sa.val[2] = vqtbl1q_u8_u32(sa.val[2], shuf_mask);
				sa.val[3] = vqtbl1q_u8_u32(sa.val[3], shuf_mask);

				vst1q_u32_x4(neon_dest, sa);
#elif defined(RP_CPU_ARM)
				uint32x2x4_t sa = vld1_u32_x4(&neon_src[0]);
				uint32x2x4_t sb = vld1_u32_x4(&neon_src[8]);

				sa.val[0] = vtbl1_u8_u32(sa.val[0], shuf_mask);
				sa.val[1] = vtbl1_u8_u32(sa.val[1], shuf_mask);
				sa.val[2] = vtbl1_u8_u32(sa.val[2], shuf_mask);
				sa.val[3] = vtbl1_u8_u32(sa.val[3], shuf_mask);
				sb.val[0] = vtbl1_u8_u32(sb.val[0], shuf_mask);
				sb.val[1] = vtbl1_u8_u32(sb.val[1], shuf_mask);
				sb.val[2] = vtbl1_u8_u32(sb.val[2], shuf_mask);
				sb.val[3] = vtbl1_u8_u32(sb.val[3], shuf_mask);

				vst1_u32_x4(&neon_dest[0], sa);
				vst1_u32_x4(&neon_dest[8], sb);
#endif
			}

			// Remaining colors.
			src_pal = reinterpret_cast<const argb32_t*>(neon_src);
			argb32_t *dest_pal = reinterpret_cast<argb32_t*>(neon_dest);
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

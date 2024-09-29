/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * un-premultiply.cpp: Un-premultiply function.                            *
 * Standard version. (C++ code only)                                       *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "rp_image.hpp"
#include "rp_image_p.hpp"
#include "rp_image_backend.hpp"

// C++ STL classes
using std::array;

// Workaround for RP_D() expecting the no-underscore, UpperCamelCase naming convention.
#define rp_imagePrivate rp_image_private

namespace LibRpTexture {

/**
 * Inverted pre-multiplication factors.
 * From Qt 5.9.1's qcolor.cpp.
 * These values are: 0x00FF00FF / alpha
 */
const array<unsigned int, 256> rp_image::qt_inv_premul_factor = {{
	0, 16711935, 8355967, 5570645, 4177983, 3342387, 2785322, 2387419,
	2088991, 1856881, 1671193, 1519266, 1392661, 1285533, 1193709, 1114129,
	1044495, 983055, 928440, 879575, 835596, 795806, 759633, 726605,
	696330, 668477, 642766, 618960, 596854, 576273, 557064, 539094,
	522247, 506422, 491527, 477483, 464220, 451673, 439787, 428511,
	417798, 407608, 397903, 388649, 379816, 371376, 363302, 355573,
	348165, 341059, 334238, 327685, 321383, 315319, 309480, 303853,
	298427, 293191, 288136, 283253, 278532, 273966, 269547, 265268,
	261123, 257106, 253211, 249431, 245763, 242201, 238741, 235379,
	232110, 228930, 225836, 222825, 219893, 217038, 214255, 211543,
	208899, 206320, 203804, 201348, 198951, 196611, 194324, 192091,
	189908, 187774, 185688, 183647, 181651, 179698, 177786, 175915,
	174082, 172287, 170529, 168807, 167119, 165464, 163842, 162251,
	160691, 159161, 157659, 156186, 154740, 153320, 151926, 150557,
	149213, 147893, 146595, 145321, 144068, 142837, 141626, 140436,
	139266, 138115, 136983, 135869, 134773, 133695, 132634, 131590,
	130561, 129549, 128553, 127572, 126605, 125653, 124715, 123792,
	122881, 121984, 121100, 120229, 119370, 118524, 117689, 116866,
	116055, 115254, 114465, 113686, 112918, 112160, 111412, 110675,
	109946, 109228, 108519, 107818, 107127, 106445, 105771, 105106,
	104449, 103800, 103160, 102527, 101902, 101284, 100674, 100071,
	99475, 98887, 98305, 97730, 97162, 96600, 96045, 95496,
	94954, 94417, 93887, 93362, 92844, 92331, 91823, 91322,
	90825, 90334, 89849, 89368, 88893, 88422, 87957, 87497,
	87041, 86590, 86143, 85702, 85264, 84832, 84403, 83979,
	83559, 83143, 82732, 82324, 81921, 81521, 81125, 80733,
	80345, 79961, 79580, 79203, 78829, 78459, 78093, 77729,
	77370, 77013, 76660, 76310, 75963, 75619, 75278, 74941,
	74606, 74275, 73946, 73620, 73297, 72977, 72660, 72346,
	72034, 71725, 71418, 71114, 70813, 70514, 70218, 69924,
	69633, 69344, 69057, 68773, 68491, 68211, 67934, 67659,
	67386, 67116, 66847, 66581, 66317, 66055, 65795, 65537
}};

/**
 * Un-premultiply an argb32_t pixel. (Standard version)
 * From qt-5.11.0's qrgb.h.
 * qUnpremultiply()
 *
 * This is needed in order to convert DXT2/4 to DXT3/5.
 *
 * @param px	[in] ARGB32 pixel to un-premultiply.
 * @return Un-premultiplied pixel.
 */
static FORCEINLINE uint32_t un_premultiply_pixel(uint32_t px)
{
	argb32_t rpx;
	rpx.u32 = px;
	if (likely(rpx.a == 255 || rpx.a == 0))
		return px;

	// Based on Qt 5.9.1's qUnpremultiply().
	// (p*(0x00ff00ff/alpha)) >> 16 == (p*255)/alpha for all p and alpha <= 256.
	const unsigned int invAlpha = rp_image::qt_inv_premul_factor[rpx.a];
	// We add 0x8000 to get even rounding.
	// The rounding also ensures that qPremultiply(qUnpremultiply(p)) == p for all p.
	rpx.r = (rpx.r * invAlpha + 0x8000) >> 16;
	rpx.g = (rpx.g * invAlpha + 0x8000) >> 16;
	rpx.b = (rpx.b * invAlpha + 0x8000) >> 16;
	return rpx.u32;
}

/**
 * Un-premultiply an ARGB32 rp_image.
 * Standard version using regular C++ code.
 *
 * Image must be ARGB32.
 *
 * @return 0 on success; non-zero on error.
 */
int rp_image::un_premultiply_cpp(void)
{
	RP_D(const rp_image);
	rp_image_backend *const backend = d->backend.get();
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// Incorrect format...
		return -1;
	}

	const int width = backend->width;
	argb32_t *px_dest = static_cast<argb32_t*>(backend->data());
	const int dest_stride_adj = (backend->stride / sizeof(*px_dest)) - width;
	for (int y = backend->height; y > 0; y--, px_dest += dest_stride_adj) {
		int x = width;
		for (; x > 1; x -= 2, px_dest += 2) {
			px_dest[0].u32 = un_premultiply_pixel(px_dest[0].u32);
			px_dest[1].u32 = un_premultiply_pixel(px_dest[1].u32);
		}
		if (x == 1) {
			px_dest->u32 = un_premultiply_pixel(px_dest->u32);
			px_dest++;
		}
	}
	return 0;
}

/**
 * Premultiply an argb32_t pixel. (Standard version)
 * From qt-5.11.0's qrgb.h.
 * qPremultiply()
 *
 * This is needed in order to use the Cairo graphics library.
 *
 * @param px	[in] ARGB32 pixel to premultiply.
 * @return Premultiplied pixel.
 */
static FORCEINLINE uint32_t premultiply_pixel_inl(uint32_t px)
{
	const unsigned int a = (px >> 24);
	if (likely(a == 255 || a == 0))
		return px;

	// Based on Qt 5.9.1's qPremultiply().
	unsigned int t = (px & 0xff00ff) * a;
	t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
	t &= 0xff00ff;

	px = ((px >> 8) & 0xff) * a;
	px = (px + ((px >> 8) & 0xff) + 0x80);
	px &= 0xff00;
	return (px | t | (a << 24));
}

/**
 * rp_image wrapper function for premultiply_pixel().
 * @param px	[in] ARGB32 pixel to premultiply.
 * @return Premultiplied pixel.
 */
uint32_t rp_image::premultiply_pixel(uint32_t px)
{
	return premultiply_pixel_inl(px);
}

/**
 * Premultiply an ARGB32 rp_image.
 *
 * Image must be ARGB32.
 *
 * @return 0 on success; non-zero on error.
 */
int rp_image::premultiply(void)
{
	// TODO: Qt doesn't have SSE-optimized builds.

	RP_D(const rp_image);
	rp_image_backend *const backend = d->backend.get();
	assert(backend->format == rp_image::Format::ARGB32);
	if (backend->format != rp_image::Format::ARGB32) {
		// Incorrect format...
		return -1;
	}

	const int width = backend->width;
	argb32_t *px_dest = static_cast<argb32_t*>(backend->data());
	const int dest_stride_adj = (backend->stride / sizeof(*px_dest)) - width;
	for (int y = backend->height; y > 0; y--, px_dest += dest_stride_adj) {
		int x = width;
		for (; x > 1; x -= 2, px_dest += 2) {
			px_dest[0].u32 = premultiply_pixel_inl(px_dest[0].u32);
			px_dest[1].u32 = premultiply_pixel_inl(px_dest[1].u32);
		}
		if (x == 1) {
			px_dest->u32 = premultiply_pixel_inl(px_dest->u32);
			px_dest++;
		}
	}
	return 0;
}

}

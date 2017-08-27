/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_DC.cpp: Image decoding functions. (Dreamcast)              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

// Reference: https://github.com/nickworonekin/puyotools/blob/548a52684fd48d936526fd91e8ead8e52aa33eb3/Libraries/VrSharp/PvrTexture/PvrDataCodec.cs

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

// C++ includes.
#include <memory>
using std::unique_ptr;

// One-time initialization.
#include "threads/pthread_once.h"

namespace LibRpBase {

/**
 * Dreamcast twiddle map.
 * Must be initialized using initDreamcastTwiddleMap().
 *
 * Supports textures up to 4096x4096.
 */
static unsigned int dc_tmap[4096];

// pthread_once() control variable.
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

/**
 * Initialize the Dreamcast twiddle map.
 * This initializes dc_tmap[].
 *
 * This function MUST be called using pthread_once().
 */
static void initDreamcastTwiddleMap_int(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(dc_tmap); i++) {
		dc_tmap[i] = 0;
		for (unsigned int j = 0, k = 1; k <= i; j++, k <<= 1) {
			dc_tmap[i] |= ((i & k) << j);
		}
	}
}

/**
 * Initialize the Dreamcast twiddle map.
 * This initializes dc_tmap[].
 */
static FORCEINLINE void initDreamcastTwiddleMap(void)
{
	pthread_once(&once_control, initDreamcastTwiddleMap_int);
}

/**
 * Convert a Dreamcast square twiddled 16-bit image to rp_image.
 * @param px_format 16-bit pixel format.
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf 16-bit image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDreamcastSquareTwiddled16(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(width <= 4096);
	assert(img_siz >= ((width * height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    width != height || width > 4096 ||
	    img_siz < ((width * height) * 2))
	{
		return nullptr;
	}

	// Initialize the twiddle map.
	initDreamcastTwiddleMap();

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Convert one line at a time. (16-bit -> ARGB32)
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
	const int dest_stride_adj = (img->stride() / sizeof(uint32_t)) - img->width();
	switch (px_format) {
		case PXF_ARGB1555: {
			for (unsigned int y = 0; y < (unsigned int)height; y++) {
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((dc_tmap[x] << 1) | dc_tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,1};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_RGB565: {
			for (unsigned int y = 0; y < (unsigned int)height; y++) {
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((dc_tmap[x] << 1) | dc_tmap[y]);
					*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,6,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_ARGB4444: {
			for (unsigned int y = 0; y < (unsigned int)height; y++) {
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((dc_tmap[x] << 1) | dc_tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {4,4,4,0,4};
			img->set_sBIT(&sBIT);
			break;
		}

		default:
			assert(!"Invalid pixel format for this function.");
			delete img;
			return nullptr;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a Dreamcast vector-quantized image to rp_image.
 * @tparam smallVQ If true, handle this image as SmallVQ.
 * @param px_format Palette pixel format.
 * @param width Image width. (Maximum is 4096.)
 * @param height Image height. (Must be equal to width.)
 * @param img_buf VQ image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 1024*2; for SmallVQ, 64*2, 256*2, or 512*2]
 * @return rp_image, or nullptr on error.
 */
template<bool smallVQ>
rp_image *ImageDecoder::fromDreamcastVQ16(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(width <= 4096);
	assert(img_siz > 0);
	assert(pal_siz > 0);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    width != height || width > 4096 ||
	    img_siz == 0 || pal_siz == 0)
	{
		return nullptr;
	}

	// Determine the number of palette entries.
	const int pal_entry_count = (smallVQ ? calcDreamcastSmallVQPaletteEntries(width) : 1024);
	assert(pal_entry_count % 2 == 0);
	assert(pal_entry_count * 2 >= pal_siz);
	if ((pal_entry_count % 2 != 0) ||
	    (pal_entry_count * 2 < pal_siz))
	{
		// Palette isn't large enough,
		// or palette isn't an even multiple.
		return nullptr;
	}

	// Initialize the twiddle map.
	initDreamcastTwiddleMap();

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Convert the palette.
	unique_ptr<uint32_t[]> palette(new uint32_t[pal_entry_count]);
	switch (px_format) {
		case PXF_ARGB1555: {
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i += 2) {
				palette[i+0] = ImageDecoderPrivate::ARGB1555_to_ARGB32(pal_buf[i+0]);
				palette[i+1] = ImageDecoderPrivate::ARGB1555_to_ARGB32(pal_buf[i+1]);
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,1};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_RGB565: {
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i += 2) {
				palette[i+0] = ImageDecoderPrivate::RGB565_to_ARGB32(pal_buf[i+0]);
				palette[i+1] = ImageDecoderPrivate::RGB565_to_ARGB32(pal_buf[i+1]);
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,6,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_ARGB4444: {
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i += 2) {
				palette[i+0] = ImageDecoderPrivate::ARGB4444_to_ARGB32(pal_buf[i+0]);
				palette[i+1] = ImageDecoderPrivate::ARGB4444_to_ARGB32(pal_buf[i+1]);
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {4,4,4,0,4};
			img->set_sBIT(&sBIT);
			break;
		}

		default:
			assert(!"Invalid pixel format for this function.");
			delete img;
			return nullptr;
	}

	// Convert one line at a time. (16-bit -> ARGB32)
	// Reference: https://github.com/nickworonekin/puyotools/blob/548a52684fd48d936526fd91e8ead8e52aa33eb3/Libraries/VrSharp/PvrTexture/PvrDataCodec.cs#L149
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
	const int dest_stride = (img->stride() / sizeof(uint32_t));
	const int dest_stride_adj = dest_stride + dest_stride - img->width();
	for (unsigned int y = 0; y < (unsigned int)height; y += 2, px_dest += dest_stride_adj) {
	for (unsigned int x = 0; x < (unsigned int)width; x += 2, px_dest += 2) {
		const unsigned int srcIdx = ((dc_tmap[x >> 1] << 1) | dc_tmap[y >> 1]);
		assert(srcIdx < (unsigned int)img_siz);
		if (srcIdx >= (unsigned int)img_siz) {
			// Out of bounds.
			delete img;
			return nullptr;
		}

		// Palette index.
		// Each block of 2x2 pixels uses a 4-element block of
		// the palette, so the palette index needs to be
		// multiplied by 4.
		const unsigned int palIdx = img_buf[srcIdx] * 4;
		if (smallVQ) {
			assert(palIdx < (unsigned int)pal_entry_count);
			if (palIdx >= (unsigned int)pal_entry_count) {
				// Palette index is out of bounds.
				// NOTE: This can only happen with SmallVQ,
				// since VQ always has 1024 palette entries.
				delete img;
				return nullptr;
			}
		}

		px_dest[0]		= palette[palIdx];
		px_dest[1]		= palette[palIdx+2];
		px_dest[dest_stride]	= palette[palIdx+1];
		px_dest[dest_stride+1]	= palette[palIdx+3];
	} }

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromDreamcastVQ16<false>(
	PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromDreamcastVQ16<true>(
	PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);

}

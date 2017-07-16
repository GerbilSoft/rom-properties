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

namespace LibRpBase {

/**
 * Create a Dreamcast twiddle map.
 * NOTE: Implementation is in ImageDecoder_DC.cpp.
 * @param size Twiddle map size. (usually texture width)
 * @return Twiddle map: unsigned int[size] (caller must delete[] this)
 */
unsigned int *ImageDecoderPrivate::createDreamcastTwiddleMap(int size)
{
	// TODO: Verify that size is a power of two.
	assert(size > 0);
	if (size <= 0)
		return nullptr;

	unsigned int *tmap = new unsigned int[size];
	for (int i = 0; i < size; i++) {
		tmap[i] = 0;
		for (int j = 0, k = 1; k <= i; j++, k <<= 1) {
			tmap[i] |= ((i & k) << j);
		}
	}

	return tmap;
}

/**
 * Convert a Dreamcast square twiddled 16-bit image to rp_image.
 * @tparam px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf 16-bit image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromDreamcastSquareTwiddled16(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(img_siz >= ((width * height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    width != height ||
	    img_siz < ((width * height) * 2))
	{
		return nullptr;
	}

	// Create a twiddle map.
	unsigned int *tmap = ImageDecoderPrivate::createDreamcastTwiddleMap(width);
	if (!tmap)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Convert one line at a time. (16-bit -> ARGB32)
	switch (px_format) {
		case PXF_ARGB1555:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = 0; x < width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		case PXF_RGB565:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = 0; x < width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		case PXF_ARGB4444:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = 0; x < width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		default:
			assert(!"Invalid pixel format for this function.");
			return nullptr;
	}

	// Image has been converted.
	delete[] tmap;
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_ARGB1555>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_RGB565>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromDreamcastSquareTwiddled16<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);

}

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

/**
 * Convert a Dreamcast vector-quantized image to rp_image.
 * @tparam px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf VQ image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromDreamcastVQ16(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(img_siz > 1024*2);
	if (!img_buf || width <= 0 || height <= 0 ||
	    width != height || img_siz <= 1024*2)
	{
		return nullptr;
	}

	// Create a twiddle map.
	unsigned int *tmap = ImageDecoderPrivate::createDreamcastTwiddleMap(width);
	if (!tmap)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Palette is 1024 entries at the start of the image buffer.
	const uint16_t *const palette = reinterpret_cast<const uint16_t*>(img_buf);
	img_buf += 1024*2;
	img_siz -= 1024*2;

	// Convert one line at a time. (16-bit -> ARGB32)
	// Reference: https://github.com/nickworonekin/puyotools/blob/548a52684fd48d936526fd91e8ead8e52aa33eb3/Libraries/VrSharp/PvrTexture/PvrDataCodec.cs#L149
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	const unsigned int destMax = width * height;
	switch (px_format) {
		case PXF_ARGB1555:
			for (int y = 0; y < height; y += 2) {
			for (int x = 0; x < width; x += 2) {
				const int srcIdx = ((tmap[x >> 1] << 1) | tmap[y >> 1]);
				if (srcIdx >= img_siz) {
					// Out of bounds.
					delete[] tmap;
					delete img;
					return nullptr;
				}

				// Palette index.
				// Each block of 2x2 pixels uses a 4-element block of
				// the palette, so the palette index needs to be
				// multiplied by 4.
				unsigned int palIdx = img_buf[srcIdx] * 4;

				for (unsigned int x2 = 0; x2 < 2; x2++) {
				for (unsigned int y2 = 0; y2 < 2; y2++) {
					const unsigned int destIdx = (((y + y2) * width) + (x + x2));
					if (destIdx > destMax) {
						// Out of bounds.
						delete[] tmap;
						delete img;
						return nullptr;
					}
					dest[destIdx] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(palette[palIdx]));
					palIdx++;
				} }
			} }
			break;

		case PXF_RGB565:
			for (int y = 0; y < height; y += 2) {
			for (int x = 0; x < width; x += 2) {
				const int srcIdx = ((tmap[x >> 1] << 1) | tmap[y >> 1]);
				if (srcIdx >= img_siz) {
					// Out of bounds.
					delete[] tmap;
					delete img;
					return nullptr;
				}

				// Palette index.
				// Each block of 2x2 pixels uses a 4-element block of
				// the palette, so the palette index needs to be
				// multiplied by 4.
				unsigned int palIdx = img_buf[srcIdx] * 4;

				for (unsigned int x2 = 0; x2 < 2; x2++) {
				for (unsigned int y2 = 0; y2 < 2; y2++) {
					const unsigned int destIdx = (((y + y2) * width) + (x + x2));
					if (destIdx > destMax) {
						// Out of bounds.
						delete[] tmap;
						delete img;
						return nullptr;
					}
					dest[destIdx] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(palette[palIdx]));
					palIdx++;
				} }
			} }
			break;

		case PXF_ARGB4444:
			for (int y = 0; y < height; y += 2) {
			for (int x = 0; x < width; x += 2) {
				const int srcIdx = ((tmap[x >> 1] << 1) | tmap[y >> 1]);
				if (srcIdx >= img_siz) {
					// Out of bounds.
					delete[] tmap;
					delete img;
					return nullptr;
				}

				// Palette index.
				// Each block of 2x2 pixels uses a 4-element block of
				// the palette, so the palette index needs to be
				// multiplied by 4.
				unsigned int palIdx = img_buf[srcIdx] * 4;

				for (unsigned int x2 = 0; x2 < 2; x2++) {
				for (unsigned int y2 = 0; y2 < 2; y2++) {
					const unsigned int destIdx = (((y + y2) * width) + (x + x2));
					if (destIdx > destMax) {
						// Out of bounds.
						delete[] tmap;
						delete img;
						return nullptr;
					}
					dest[destIdx] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(palette[palIdx]));
					palIdx++;
				} }
			} }
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
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_ARGB1555>(
	int width, int height,
	const uint8_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_RGB565>(
	int width, int height,
	const uint8_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint8_t *img_buf, int img_siz);

}

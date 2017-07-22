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
 * @tparam px_format 16-bit pixel format.
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
	// Verify px_format.
	static_assert(px_format == PXF_ARGB1555 ||
		      px_format == PXF_RGB565 ||
		      px_format == PXF_ARGB4444,
		      "Invalid pixel format for this function.");

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
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		case PXF_RGB565:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		case PXF_ARGB4444:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = 0; x < (unsigned int)width; x++) {
					const unsigned int srcIdx = ((tmap[x] << 1) | tmap[y]);
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(img_buf[srcIdx]));
					px_dest++;
				}
			}
			break;

		default:
			assert(!"Invalid pixel format for this function.");
			delete[] tmap;
			delete img;
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
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 1024*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromDreamcastVQ16(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify px_format.
	static_assert(px_format == PXF_ARGB1555 ||
		      px_format == PXF_RGB565 ||
		      px_format == PXF_ARGB4444,
		      "Invalid pixel format for this function.");

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(img_siz > 0);
	assert(pal_siz >= 1024*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    width != height || img_siz == 0 || pal_siz < (1024*2))
	{
		return nullptr;
	}

	// Create a twiddle map.
	unsigned int *tmap = ImageDecoderPrivate::createDreamcastTwiddleMap(width);
	if (!tmap)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Convert the palette.
	static const unsigned int pal_entry_count = 1024;
	unique_ptr<uint32_t[]> palette(new uint32_t[pal_entry_count]);
	switch (px_format) {
		case PXF_ARGB1555:
			for (unsigned int i = 0; i < pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(pal_buf[i]);
			}
			break;
		case PXF_RGB565:
			for (unsigned int i = 0; i < pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::RGB565_to_ARGB32(pal_buf[i]);
			}
			break;
		case PXF_ARGB4444:
			for (unsigned int i = 0; i < pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(pal_buf[i]);
			}
			break;
		default:
			assert(!"Invalid pixel format for this function.");
			delete[] tmap;
			delete img;
			return nullptr;
	}

	// Convert one line at a time. (16-bit -> ARGB32)
	// Reference: https://github.com/nickworonekin/puyotools/blob/548a52684fd48d936526fd91e8ead8e52aa33eb3/Libraries/VrSharp/PvrTexture/PvrDataCodec.cs#L149
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	for (unsigned int y = 0; y < (unsigned int)height; y += 2) {
	for (unsigned int x = 0; x < (unsigned int)width; x += 2) {
		const unsigned int srcIdx = ((tmap[x >> 1] << 1) | tmap[y >> 1]);
		if (srcIdx >= (unsigned int)img_siz) {
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

		// NOTE: Can't use BlitTile() due to the inverted x/y order here.
		for (unsigned int x2 = 0; x2 < 2; x2++) {
			const unsigned int destIdx = ((y * width) + (x + x2));
			dest[destIdx] = palette[palIdx];
			dest[destIdx+width] = palette[palIdx+1];
			palIdx += 2;
		}
	} }

	// Image has been converted.
	delete[] tmap;
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_ARGB1555>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_RGB565>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromDreamcastVQ16<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);

/**
 * Convert a Dreamcast small vector-quantized image to rp_image.
 * @tparam px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf VQ image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 64*2, 256*2, 512*2, or 1024*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromDreamcastSmallVQ16(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify px_format.
	static_assert(px_format == PXF_ARGB1555 ||
		      px_format == PXF_RGB565 ||
		      px_format == PXF_ARGB4444,
		      "Invalid pixel format for this function.");

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(width == height);
	assert(img_siz > 0);
	assert(pal_siz > 0);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    width != height || img_siz == 0 || pal_siz == 0)
	{
		return nullptr;
	}

	// Determine the number of palette entries.
	const int pal_entry_count = calcDreamcastSmallVQPaletteEntries(width);
	assert((pal_entry_count * 2) >= pal_siz);
	if ((pal_entry_count * 2) < pal_siz) {
		// Palette isn't large enough.
		return nullptr;
	}

	// Create a twiddle map.
	unsigned int *tmap = ImageDecoderPrivate::createDreamcastTwiddleMap(width);
	if (!tmap)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Convert the palette.
	unique_ptr<uint32_t[]> palette(new uint32_t[pal_entry_count]);
	switch (px_format) {
		case PXF_ARGB1555:
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(pal_buf[i]);
			}
			break;
		case PXF_RGB565:
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::RGB565_to_ARGB32(pal_buf[i]);
			}
			break;
		case PXF_ARGB4444:
			for (unsigned int i = 0; i < (unsigned int)pal_entry_count; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(pal_buf[i]);
			}
			break;
		default:
			assert(!"Invalid pixel format for this function.");
			delete[] tmap;
			delete img;
			return nullptr;
	}

	// Convert one line at a time. (16-bit -> ARGB32)
	// Reference: https://github.com/nickworonekin/puyotools/blob/548a52684fd48d936526fd91e8ead8e52aa33eb3/Libraries/VrSharp/PvrTexture/PvrDataCodec.cs#L149
	uint32_t *dest = static_cast<uint32_t*>(img->bits());
	for (unsigned int y = 0; y < (unsigned int)height; y += 2) {
	for (unsigned int x = 0; x < (unsigned int)width; x += 2) {
		const unsigned int srcIdx = ((tmap[x >> 1] << 1) | tmap[y >> 1]);
		if (srcIdx >= (unsigned int)img_siz) {
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
		if (palIdx >= (unsigned int)pal_entry_count) {
			// Out of bounds.
			delete[] tmap;
			delete img;
			return nullptr;
		}

		// NOTE: Can't use BlitTile() due to the inverted x/y order here.
		for (unsigned int x2 = 0; x2 < 2; x2++) {
			const unsigned int destIdx = ((y * width) + (x + x2));
			dest[destIdx] = palette[palIdx];
			dest[destIdx+width] = palette[palIdx+1];
			palIdx += 2;
		}
	} }

	// Image has been converted.
	delete[] tmap;
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromDreamcastSmallVQ16<ImageDecoder::PXF_ARGB1555>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromDreamcastSmallVQ16<ImageDecoder::PXF_RGB565>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromDreamcastSmallVQ16<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);

}

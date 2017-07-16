/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
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

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

namespace LibRpBase {

/**
 * Convert a linear CI4 image to rp_image with a little-endian 16-bit palette.
 * @param px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromLinearCI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	assert(pal_siz >= 16*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2) || pal_siz < 16*2)
	{
		return nullptr;
	}

	// CI4 width must be a multiple of two.
	assert(width % 2 == 0);
	if (width % 2 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 16);
	if (img->palette_len() < 16) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	switch (px_format) {
		case PXF_ARGB1555:
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = i;
				}
			}
			break;
		case PXF_RGB565:
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(pal_buf[i]));
			}
			break;
		case PXF_ARGB4444:
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = i;
				}
			}
			break;
		default:
			assert(!"Invalid pixel format for this function.");
			return nullptr;
	}
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (CI4 -> CI8)
	for (int y = 0; y < height; y++) {
		uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 2) {
			px_dest[0] = (*img_buf >> 4);
			px_dest[1] = (*img_buf & 0x0F);
			img_buf++;
			px_dest += 2;
		}
	}

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromLinearCI4<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);

/**
 * Convert a linear CI8 image to rp_image with a little-endian 16-bit palette.
 * @param px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 256*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromLinearCI8(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(pal_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (width * height));
	assert(pal_siz >= 256*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height) || pal_siz < 256*2)
	{
		return nullptr;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Convert the palette.
	// TODO: Optimize using pointers instead of indexes?
	uint32_t *palette = img->palette();
	assert(img->palette_len() >= 256);
	if (img->palette_len() < 256) {
		// Not enough colors...
		delete img;
		return nullptr;
	}

	int tr_idx = -1;
	switch (px_format) {
		case PXF_ARGB1555:
			for (unsigned int i = 0; i < 256; i++) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = i;
				}
			}
			break;
		case PXF_RGB565:
			for (unsigned int i = 0; i < 256; i++) {
				palette[i] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(pal_buf[i]));
			}
			break;
		case PXF_ARGB4444:
			for (unsigned int i = 0; i < 256; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = i;
				}
			}
			break;
		default:
			assert(!"Invalid pixel format for this function.");
			return nullptr;
	}
	img->set_tr_idx(tr_idx);

	// Copy one line at a time. (CI8 -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	const int stride = img->stride();
	for (int y = height; y > 0; y--) {
		memcpy(px_dest, img_buf, width);
		px_dest += stride;
		img_buf += width;
	}

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromLinearCI8<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);

/**
 * Convert a linear 16-bit image to rp_image.
 * @param px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf 16-bit image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format>
rp_image *ImageDecoder::fromLinear16(int width, int height,
	const uint16_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * 2))
	{
		return nullptr;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// Convert one line at a time. (16-bit -> ARGB32)
	switch (px_format) {
		case PXF_ARGB1555:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
			}
			break;
		case PXF_RGB565:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
			}
			break;
		case PXF_ARGB4444:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (int x = width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
			}
			break;
		default:
			assert(!"Invalid pixel format for this function.");
			return nullptr;
	}

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromLinear16<ImageDecoder::PXF_ARGB1555>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromLinear16<ImageDecoder::PXF_RGB565>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);
template rp_image *ImageDecoder::fromLinear16<ImageDecoder::PXF_ARGB4444>(
	int width, int height,
	const uint16_t *img_buf, int img_siz);

/**
 * Convert a linear monochrome image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf Monochrome image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/8]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinearMono(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 8));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 8))
	{
		return nullptr;
	}

	// Monochrome width must be a multiple of eight.
	assert(width % 8 == 0);
	if (width % 8 != 0)
		return nullptr;

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);

	// Set a default monochrome palette.
	uint32_t *palette = img->palette();
	palette[0] = 0xFFFFFFFF;	// white
	palette[1] = 0xFF000000;	// black
	img->set_tr_idx(-1);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (monochrome -> CI8)
	for (int y = 0; y < height; y++) {
		uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 8) {
			uint8_t pxMono = *img_buf++;
			// TODO: Unroll this loop?
			for (int bit = 8; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel.
				*px_dest = (pxMono >> 7);
				pxMono <<= 1;
			}
		}
	}

	// Image has been converted.
	return img;
}

}

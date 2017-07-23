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
 * @tparam px_format Palette pixel format.
 * @tparam msn_left If true, most-significant nybble is the left pixel.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
template<ImageDecoder::PixelFormat px_format, bool msn_left>
rp_image *ImageDecoder::fromLinearCI4(int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz)
{
	// Verify px_format.
	static_assert(px_format == PXF_ARGB1555 ||
		      px_format == PXF_RGB565 ||
		      px_format == PXF_ARGB4444 ||
		      px_format == PXF_BGR555 ||
		      px_format == PXF_BGR555_PS1,
		      "Invalid pixel format for this function.");

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
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

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
					tr_idx = (int)i;
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
					tr_idx = (int)i;
				}
			}
			break;

		case PXF_BGR555:
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::BGR555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
			}
			break;

		case PXF_BGR555_PS1:
			for (unsigned int i = 0; i < 16; i++) {
				// For PS1 BGR555, if the color value is $0000, it's transparent.
				const uint16_t px16 = le16_to_cpu(pal_buf[i]);
				if (px16 == 0) {
					// Transparent color.
					palette[i] = 0;
					if (tr_idx < 0) {
						tr_idx = i;
					}
				} else {
					// Non-transparent color.
					palette[i] = ImageDecoderPrivate::BGR555_to_ARGB32(px16);
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
	if (msn_left) {
		// Left pixel is the Most Significant Nybble.
		for (int y = 0; y < height; y++) {
			uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
			for (int x = width; x > 0; x -= 2) {
				px_dest[0] = (*img_buf >> 4);
				px_dest[1] = (*img_buf & 0x0F);
				img_buf++;
				px_dest += 2;
			}
		}
	} else {
		// Left pixel is the Least Significant Nybble.
		for (int y = 0; y < height; y++) {
			uint8_t *px_dest = static_cast<uint8_t*>(img->scanLine(y));
			for (int x = width; x > 0; x -= 2) {
				px_dest[0] = (*img_buf & 0x0F);
				px_dest[1] = (*img_buf >> 4);
				img_buf++;
				px_dest += 2;
			}
		}
	}

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromLinearCI4<ImageDecoder::PXF_ARGB4444, true>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);
// PS1: LSN is left.
template rp_image *ImageDecoder::fromLinearCI4<ImageDecoder::PXF_BGR555_PS1, false>(
	int width, int height,
	const uint8_t *img_buf, int img_siz,
	const uint16_t *pal_buf, int pal_siz);

/**
 * Convert a linear CI8 image to rp_image with a little-endian 16-bit palette.
 * @tparam px_format Palette pixel format.
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
	assert(img_siz >= (width * height));
	assert(pal_siz >= 256*2);
	if (!img_buf || !pal_buf || width <= 0 || height <= 0 ||
	    img_siz < (width * height) || pal_siz < 256*2)
	{
		return nullptr;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_CI8);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

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
	for (unsigned int y = (unsigned int)height; y > 0; y--) {
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
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

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
		for (unsigned int x = (unsigned int)width; x > 0; x -= 8) {
			uint8_t pxMono = *img_buf++;
			// TODO: Unroll this loop?
			for (unsigned int bit = 8; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel.
				*px_dest = (pxMono >> 7);
				pxMono <<= 1;
			}
		}
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param pitch		[in,opt] Pitch, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear16(PixelFormat px_format,
	int width, int height,
	const uint16_t *img_buf, int img_siz, int pitch)
{
	static const int bytespp = 2;

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * bytespp))
	{
		return nullptr;
	}

	// Line offset adjustment.
	int line_offset_adj = 0;
	assert(pitch >= 0);
	if (pitch > 0) {
		// Set line_offset_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(pitch % bytespp == 0);
		if (pitch % bytespp != 0) {
			// Invalid pitch.
			return nullptr;
		}
		line_offset_adj = width - (pitch / bytespp);
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Convert one line at a time. (16-bit -> ARGB32)
	switch (px_format) {
		case PXF_ARGB1555:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
				img_buf += line_offset_adj;
			}
			break;

		case PXF_RGB565:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
				img_buf += line_offset_adj;
			}
			break;

		case PXF_ARGB4444:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					*px_dest = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(*img_buf));
					img_buf++;
					px_dest++;
				}
				img_buf += line_offset_adj;
			}
			break;

		default:
			assert(!"Unsupported 16-bit pixel format.");
			return nullptr;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param pitch		[in,opt] Pitch, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear24(PixelFormat px_format,
	int width, int height,
	const uint8_t *img_buf, int img_siz, int pitch)
{
	static const int bytespp = 3;

	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) * bytespp));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) * bytespp))
	{
		return nullptr;
	}

	// Line offset adjustment.
	int line_offset_adj = 0;
	assert(pitch >= 0);
	if (pitch > 0) {
		// Set line_offset_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(pitch % bytespp == 0);
		if (pitch % bytespp != 0) {
			// Invalid pitch.
			return nullptr;
		}
		// Byte addressing, so keep it in units of bytespp.
		line_offset_adj = (width * bytespp) - pitch;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Convert one line at a time. (24-bit -> ARGB32)
	switch (px_format) {
		case PXF_RGB888:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					*px_dest = 0xFF000000 | (img_buf[2] << 16) |
						(img_buf[1] << 8) | img_buf[0];
					img_buf += 3;
					px_dest++;
				}
				img_buf += line_offset_adj;
			}
			break;

		case PXF_BGR888:
			for (int y = 0; y < height; y++) {
				uint32_t *px_dest = static_cast<uint32_t*>(img->scanLine(y));
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					*px_dest = 0xFF000000 | (img_buf[0] << 16) |
						(img_buf[1] << 8) | img_buf[2];
					img_buf += 3;
					px_dest++;
				}
				img_buf += line_offset_adj;
			}
			break;

		default:
			assert(!"Unsupported 24-bit pixel format.");
			return nullptr;
	}

	// Image has been converted.
	return img;
}

}

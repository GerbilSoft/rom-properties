/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_Linear.cpp: Image decoding functions. (Linear)             *
 * Standard version. (C++ code only)                                       *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

namespace LibRpBase {

// 2-bit alpha lookup table.
const uint32_t ImageDecoderPrivate::a2_lookup[4] = {
	0x00000000, 0x55000000,
	0xAA000000, 0xFF000000
};

// 2-bit color lookup table.
const uint8_t ImageDecoderPrivate::c2_lookup[4] = {
	0x00, 0x55, 0xAA, 0xFF
};

// 3-bit color lookup table.
const uint8_t ImageDecoderPrivate::c3_lookup[8] = {
	0x00, 0x24, 0x49, 0x6D,
	0x92, 0xB6, 0xDB, 0xFF
};

/**
 * Convert a linear CI4 image to rp_image with a little-endian 16-bit palette.
 * @tparam msn_left If true, most-significant nybble is the left pixel.
 * @param px_format Palette pixel format.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
template<bool msn_left>
rp_image *ImageDecoder::fromLinearCI4(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz)
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
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = img->stride() - img->width();

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
		case PXF_ARGB1555: {
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,1};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_RGB565: {
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(pal_buf[i]));
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,6,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_ARGB4444: {
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {4,4,4,0,4};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_BGR555: {
			for (unsigned int i = 0; i < 16; i++) {
				palette[i] = ImageDecoderPrivate::BGR555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_BGR555_PS1: {
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
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		default:
			assert(!"Invalid pixel format for this function.");
			delete img;
			return nullptr;
	}
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (CI4 -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	if (msn_left) {
		// Left pixel is the Most Significant Nybble.
		for (unsigned int y = (unsigned int)height; y > 0; y--) {
			for (unsigned int x = (unsigned int)width; x > 0; x -= 2) {
				px_dest[0] = (*img_buf >> 4);
				px_dest[1] = (*img_buf & 0x0F);
				img_buf++;
				px_dest += 2;
			}
			px_dest += dest_stride_adj;
		}
	} else {
		// Left pixel is the Least Significant Nybble.
		for (unsigned int y = (unsigned int)height; y > 0; y--) {
			for (unsigned int x = (unsigned int)width; x > 0; x -= 2) {
				px_dest[0] = (*img_buf & 0x0F);
				px_dest[1] = (*img_buf >> 4);
				img_buf++;
				px_dest += 2;
			}
			px_dest += dest_stride_adj;
		}
	}

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromLinearCI4<true>(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);
template rp_image *ImageDecoder::fromLinearCI4<false>(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz);

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
rp_image *ImageDecoder::fromLinearCI8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	const uint16_t *RESTRICT pal_buf, int pal_siz)
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
		case PXF_ARGB1555: {
			for (unsigned int i = 0; i < 256; i += 2) {
				palette[i] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
				palette[i+1] = ImageDecoderPrivate::ARGB1555_to_ARGB32(le16_to_cpu(pal_buf[i+1]));
				if (tr_idx < 0 && ((palette[i+1] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i+1;
				}
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,5,5,0,1};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_RGB565: {
			for (unsigned int i = 0; i < 256; i += 2) {
				palette[i+0] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(pal_buf[i+0]));
				palette[i+1] = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(pal_buf[i+1]));
			}
			// Set the sBIT metadata.
			static const rp_image::sBIT_t sBIT = {5,6,5,0,0};
			img->set_sBIT(&sBIT);
			break;
		}

		case PXF_ARGB4444: {
			for (unsigned int i = 0; i < 256; i++) {
				palette[i] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i]));
				if (tr_idx < 0 && ((palette[i] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i;
				}
				palette[i+1] = ImageDecoderPrivate::ARGB4444_to_ARGB32(le16_to_cpu(pal_buf[i+1]));
				if (tr_idx < 0 && ((palette[i+1] >> 24) == 0)) {
					// Found the transparent color.
					tr_idx = (int)i+1;
				}
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

/**
 * Convert a linear monochrome image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf Monochrome image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/8]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinearMono(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz)
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
	const int dest_stride_adj = img->stride() - img->width();

	// Set a default monochrome palette.
	uint32_t *palette = img->palette();
	palette[0] = 0xFFFFFFFF;	// white
	palette[1] = 0xFF000000;	// black
	img->set_tr_idx(-1);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert one line at a time. (monochrome -> CI8)
	uint8_t *px_dest = static_cast<uint8_t*>(img->bits());
	for (unsigned int y = (unsigned int)height; y > 0; y--) {
		for (unsigned int x = (unsigned int)width; x > 0; x -= 8) {
			uint8_t pxMono = *img_buf++;
			// TODO: Unroll this loop?
			for (unsigned int bit = 8; bit > 0; bit--, px_dest++) {
				// MSB == left-most pixel.
				*px_dest = (pxMono >> 7);
				pxMono <<= 1;
			}
		}
		px_dest += dest_stride_adj;
	}

	// Set the sBIT metadata.
	// NOTE: Setting the grayscale value, though we're
	// not saving grayscale PNGs at the moment.
	static const rp_image::sBIT_t sBIT = {1,1,1,1,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 8-bit RGB image to rp_image.
 * Usually used for luminance and alpha images.
 * @param px_format	[in] 8-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 8-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear8(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride)
{
	static const int bytespp = 1;

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

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		assert(stride >= (width * bytespp));
		if (unlikely(stride % bytespp != 0 || stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		src_stride_adj = (stride / bytespp) - width;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

#define fromLinear8_convert(fmt, r,g,b,gr,a) \
		case PXF_##fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				unsigned int x; \
				for (x = (unsigned int)width; x > 1; x -= 2) { \
					px_dest[0] = ImageDecoderPrivate::fmt##_to_ARGB32(img_buf[0]); \
					px_dest[1] = ImageDecoderPrivate::fmt##_to_ARGB32(img_buf[1]); \
					img_buf += 2; \
					px_dest += 2; \
				} \
				if (x == 1) { \
					/* Extra pixel. */ \
					*px_dest = ImageDecoderPrivate::fmt##_to_ARGB32(*img_buf); \
					img_buf++; \
					px_dest++; \
				} \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT data. */ \
			static const rp_image::sBIT_t sBIT = {r,g,b,gr,a}; \
			img->set_sBIT(&sBIT); \
		} break

	// Convert one line at a time. (8-bit -> ARGB32)
	switch (px_format) {
		// Luminance.
		fromLinear8_convert(L8, 8,8,8,8,0);
		fromLinear8_convert(A4L4, 4,4,4,4,4);

		// Alpha.
		// NOTE: Have to specify RGB bits...
		fromLinear8_convert(A8, 1,1,1,1,8);

		default:
			assert(!"Unsupported 8-bit pixel format.");
			delete img;
			return nullptr;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 16-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 16-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 16-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear16_cpp(PixelFormat px_format,
	int width, int height,
	const uint16_t *RESTRICT img_buf, int img_siz, int stride)
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

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		assert(stride >= (width * bytespp));
		if (unlikely(stride % bytespp != 0 || stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		src_stride_adj = (stride / bytespp) - width;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	uint32_t *px_dest = static_cast<uint32_t*>(img->bits());

#define fromLinear16_convert(fmt, r,g,b,gr,a) \
		case PXF_##fmt: { \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				for (unsigned int x = (unsigned int)width; x > 0; x--) { \
					*px_dest = ImageDecoderPrivate::fmt##_to_ARGB32(le16_to_cpu(*img_buf)); \
					img_buf++; \
					px_dest++; \
				} \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT data. */ \
			static const rp_image::sBIT_t sBIT = {r,g,b,gr,a}; \
			img->set_sBIT(&sBIT); \
		} break
			
	// Convert one line at a time. (16-bit -> ARGB32)
	switch (px_format) {
		// 16-bit RGB.
		fromLinear16_convert(RGB565, 5,6,5,0,0);
		fromLinear16_convert(BGR565, 5,6,5,0,0);
		fromLinear16_convert(ARGB1555, 5,5,5,0,1);
		fromLinear16_convert(ABGR1555, 5,5,5,0,1);
		fromLinear16_convert(RGBA5551, 5,5,5,0,1);
		fromLinear16_convert(BGRA5551, 5,5,5,0,1);
		fromLinear16_convert(ARGB4444, 4,4,4,0,4);
		fromLinear16_convert(ABGR4444, 4,4,4,0,4);
		fromLinear16_convert(RGBA4444, 4,4,4,0,4);
		fromLinear16_convert(BGRA4444, 4,4,4,0,4);
		fromLinear16_convert(xRGB4444, 4,4,4,0,4);
		fromLinear16_convert(xBGR4444, 4,4,4,0,4);
		fromLinear16_convert(RGBx4444, 4,4,4,0,4);
		fromLinear16_convert(BGRx4444, 4,4,4,0,4);
		fromLinear16_convert(ARGB8332, 3,3,2,0,8);

		// 15-bit RGB.
		fromLinear16_convert(RGB555, 5,5,5,0,0);
		fromLinear16_convert(BGR555, 5,5,5,0,0);

		// Luminance.
		// TODO: 16-bit support. Downconverted to 8 for now.
		fromLinear16_convert(L16, 8,8,8,8,0);
		fromLinear16_convert(A8L8, 8,8,8,8,8);

		// RG formats.
		fromLinear16_convert(RG88, 8,8,1,0,0);
		fromLinear16_convert(GR88, 8,8,1,0,0);

		default:
			assert(!"Unsupported 16-bit pixel format.");
			delete img;
			return nullptr;
	}

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 24-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 24-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] Image buffer. (must be byte-addressable)
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*3]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear24_cpp(PixelFormat px_format,
	int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz, int stride)
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

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of bytes we need to
		// add to the end of each line to get to the next row.
		assert(stride >= (width * bytespp));
		if (unlikely(stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		// NOTE: Byte addressing, so keep it in units of bytespp.
		src_stride_adj = stride - (width * bytespp);
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();
	argb32_t *px_dest = static_cast<argb32_t*>(img->bits());

	// TODO: Is it faster or slower to use argb32_t vs. shifting?

	// Convert one line at a time. (24-bit -> ARGB32)
	switch (px_format) {
		case PXF_RGB888:
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					px_dest->b = img_buf[0];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[2];
					px_dest->a = 0xFF;
					img_buf += 3;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		case PXF_BGR888:
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				for (unsigned int x = (unsigned int)width; x > 0; x--) {
					px_dest->b = img_buf[2];
					px_dest->g = img_buf[1];
					px_dest->r = img_buf[0];
					px_dest->a = 0xFF;
					img_buf += 3;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			break;

		default:
			assert(!"Unsupported 24-bit pixel format.");
			delete img;
			return nullptr;
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a linear 32-bit RGB image to rp_image.
 * Standard version using regular C++ code.
 * @param px_format	[in] 32-bit pixel format.
 * @param width		[in] Image width.
 * @param height	[in] Image height.
 * @param img_buf	[in] 32-bit image buffer.
 * @param img_siz	[in] Size of image data. [must be >= (w*h)*2]
 * @param stride	[in,opt] Stride, in bytes. If 0, assumes width*bytespp.
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromLinear32_cpp(PixelFormat px_format,
	int width, int height,
	const uint32_t *RESTRICT img_buf, int img_siz, int stride)
{
	static const int bytespp = 4;

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

	// Stride adjustment.
	int src_stride_adj = 0;
	assert(stride >= 0);
	if (stride > 0) {
		// Set src_stride_adj to the number of pixels we need to
		// add to the end of each line to get to the next row.
		assert(stride % bytespp == 0);
		assert(stride >= (width * bytespp));
		if (unlikely(stride % bytespp != 0 || stride < (width * bytespp))) {
			// Invalid stride.
			return nullptr;
		}
		src_stride_adj = (stride / bytespp) - width;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}
	const int dest_stride_adj = (img->stride() / sizeof(argb32_t)) - img->width();

	// sBIT for standard ARGB32.
	static const rp_image::sBIT_t sBIT_x32 = {8,8,8,0,0};
	static const rp_image::sBIT_t sBIT_A32 = {8,8,8,0,8};

	// Convert one line at a time. (32-bit -> ARGB32)
	// NOTE: All functions except PXF_HOST_ARGB32 are partially unrolled.
	switch (px_format) {
		case PXF_HOST_ARGB32:
			// Host-endian ARGB32.
			// We can directly copy the image data without conversions.
			if (stride == 0) {
				// Calculate the stride based on image width.
				stride = width * bytespp;
			}

			if (stride == img->stride()) {
				// Stride is identical. Copy the whole image all at once.
				memcpy(img->bits(), img_buf, stride * height);
			} else {
				// Stride is not identical. Copy each scanline.
				const int dest_stride = img->stride() / sizeof(argb32_t);
				uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
				const unsigned int copy_len = (unsigned int)width * bytespp;
				for (unsigned int y = (unsigned int)height; y > 0; y--) {
					memcpy(px_dest, img_buf, copy_len);
					img_buf += (stride / bytespp);
					px_dest += dest_stride;
				}
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_A32);
			break;

		case PXF_HOST_RGBA32: {
			// Host-endian RGBA32.
			// Pixel copy is needed, with shifting.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = (img_buf[0] >> 8) | (img_buf[0] << 24);
					px_dest[1] = (img_buf[1] >> 8) | (img_buf[1] << 24);
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = (*img_buf >> 8) | (*img_buf << 24);
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_A32);
			break;
		}

		case PXF_HOST_xRGB32: {
			// Host-endian XRGB32.
			// Pixel copy is needed, with alpha channel masking.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = img_buf[0] | 0xFF000000;
					px_dest[1] = img_buf[1] | 0xFF000000;
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = *img_buf | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_x32);
			break;
		}

		case PXF_HOST_RGBx32: {
			// Host-endian RGBx32.
			// Pixel copy is needed, with a right shift.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = (img_buf[0] >> 8) | 0xFF000000;
					px_dest[1] = (img_buf[1] >> 8) | 0xFF000000;
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = (*img_buf >> 8) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_x32);
			break;
		}

		case PXF_SWAP_ARGB32: {
			// Byteswapped ARGB32.
			// Pixel copy is needed, with byteswapping.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = __swab32(img_buf[0]);
					px_dest[1] = __swab32(img_buf[1]);
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = __swab32(*img_buf);
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_A32);
			break;
		}

		case PXF_SWAP_RGBA32: {
			// Byteswapped ABGR32.
			// Pixel copy is needed, with shifting.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					const uint32_t px0 = __swab32(img_buf[0]);
					const uint32_t px1 = __swab32(img_buf[1]);
					px_dest[0] = (px0 >> 8) | (px0 << 24);
					px_dest[1] = (px1 >> 8) | (px1 << 24);
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					const uint32_t px = __swab32(*img_buf);
					*px_dest = (px >> 8) | (px << 24);
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_A32);
			break;
		}

		case PXF_SWAP_xRGB32: {
			// Byteswapped XRGB32.
			// Pixel copy is needed, with byteswapping and alpha channel masking.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = __swab32(img_buf[0]) | 0xFF000000;
					px_dest[1] = __swab32(img_buf[1]) | 0xFF000000;
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = __swab32(*img_buf) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_x32);
			break;
		}

		case PXF_SWAP_RGBx32: {
			// Byteswapped RGBx32.
			// Pixel copy is needed, with byteswapping and a right shift.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0] = (__swab32(img_buf[0]) >> 8) | 0xFF000000;
					px_dest[1] = (__swab32(img_buf[1]) >> 8) | 0xFF000000;
					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest = (__swab32(*img_buf) >> 8) | 0xFF000000;
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_x32);
			break;
		}

		case PXF_RABG8888: {
			// VTF "ARGB8888", which is actually RABG.
			// TODO: This might be a VTFEdit bug. (Tested versions: 1.2.5, 1.3.3)
			// TODO: Verify on big-endian.
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits());
			for (unsigned int y = (unsigned int)height; y > 0; y--) {
				unsigned int x;
				for (x = (unsigned int)width; x > 1; x -= 2) {
					px_dest[0]  = (img_buf[0] >> 8) & 0xFF;
					px_dest[0] |= (img_buf[0] & 0xFF) << 8;
					px_dest[0] |= (img_buf[0] << 8) & 0xFF000000;
					px_dest[0] |= (img_buf[0] >> 8) & 0x00FF0000;

					px_dest[1]  = (img_buf[1] >> 8) & 0xFF;
					px_dest[1] |= (img_buf[1] & 0xFF) << 8;
					px_dest[1] |= (img_buf[1] << 8) & 0xFF000000;
					px_dest[1] |= (img_buf[1] >> 8) & 0x00FF0000;

					img_buf += 2;
					px_dest += 2;
				}
				if (x == 1) {
					// Extra pixel.
					*px_dest  = (*img_buf >> 8) & 0xFF;
					*px_dest |= (*img_buf & 0xFF) << 8;
					*px_dest |= (*img_buf << 8) & 0xFF000000;
					*px_dest |= (*img_buf >> 8) & 0x00FF0000;
					img_buf++;
					px_dest++;
				}
				img_buf += src_stride_adj;
				px_dest += dest_stride_adj;
			}
			// Set the sBIT metadata.
			img->set_sBIT(&sBIT_x32);
			break;
		}

		/** Uncommon 32-bit formats. **/

#define fromLinear32_convert(fmt, r,g,b,gr,a) \
		case PXF_##fmt: { \
			uint32_t *px_dest = static_cast<uint32_t*>(img->bits()); \
			for (unsigned int y = (unsigned int)height; y > 0; y--) { \
				for (unsigned int x = (unsigned int)width; x > 0; x--) { \
					*px_dest = ImageDecoderPrivate::fmt##_to_ARGB32(le32_to_cpu(*img_buf)); \
					img_buf++; \
					px_dest++; \
				} \
				img_buf += src_stride_adj; \
				px_dest += dest_stride_adj; \
			} \
			/* Set the sBIT data. */ \
			static const rp_image::sBIT_t sBIT = {r,g,b,gr,a}; \
			img->set_sBIT(&sBIT); \
		} break

		// TODO: Add an ARGB64 format to rp_image.
		// For now, truncating it to G8R8.
		// TODO: This might be a candidate for SSE2 optimization.
		// NOTE: We have to set '1' for the empty Blue channel,
		// since libpng complains if it's set to '0'.
		fromLinear32_convert(G16R16, 8,8,1,0,0);

		// TODO: Add an ARGB64 format to rp_image.
		// For now, truncating it to ARGB32.
		fromLinear32_convert(A2R10G10B10, 8,8,8,0,2);
		fromLinear32_convert(A2B10G10R10, 8,8,8,0,2);

		default:
			assert(!"Unsupported 16-bit pixel format.");
			delete img;
			return nullptr;
	}

	// Image has been converted.
	return img;
}

}

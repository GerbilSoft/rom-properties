/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_PS1.cpp: Image decoding functions. (PlayStation)           *
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
 * Convert a PlayStation CI4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @param pal_buf Palette buffer.
 * @param pal_siz Size of palette data. [must be >= 16*2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromPS1_CI4(int width, int height,
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

	// PS1 CI4 is linear. No tiles.

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
	for (int i = 0; i < 16; i++) {
		// PS1 color format is BGR555.
		// NOTE: If the color value is $0000, it's transparent.
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
	img->set_tr_idx(tr_idx);

	// NOTE: rp_image initializes the palette to 0,
	// so we don't need to clear the remaining colors.

	// Convert from CI4 to CI8.
	for (int y = 0; y < height; y++) {
		uint8_t *dest = static_cast<uint8_t*>(img->scanLine(y));
		for (int x = width; x > 0; x -= 2, dest += 2, img_buf++) {
			dest[0] = (*img_buf & 0x0F);
			dest[1] = (*img_buf >> 4);
		}
	}

	// Image has been converted.
	return img;
}

}

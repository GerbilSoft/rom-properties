/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * ImageDecoder_S3TC.cpp: Image decoding functions. (S3TC)                 *
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

// References:
// - http://www.matejtomcik.com/Public/KnowHow/DXTDecompression/
// - http://www.fsdeveloper.com/wiki/index.php?title=DXT_compression_explained

namespace LibRpBase {

/**
 * Convert a DXT1 image to rp_image.
 * @tparam big_endian If true, the DXT1 is encoded using big-endian values.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf CI8 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
template<bool big_endian>
rp_image *ImageDecoder::fromDXT1(int width, int height,
	const uint8_t *img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 2))
	{
		return nullptr;
	}

	// DXT1 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);

	// DXT1 block format.
	struct dxt1_block {
		uint16_t color[2];	// Colors 0 and 1, in RGB565 format.
		uint32_t indexes;	// Two-bit color indexes.
	};
	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// ARGB32 value with byte accessors.
	union argb32_t {
		struct {
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			uint8_t a;
			uint8_t r;
			uint8_t g;
			uint8_t b;
#endif
		};
		uint32_t u32;
	};
	argb32_t pal[4];		// 4-color palette per block.

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	// Tiles are arranged in 2x2 blocks.
	// Reference: https://github.com/nickworonekin/puyotools/blob/80f11884f6cae34c4a56c5b1968600fe7c34628b/Libraries/VrSharp/GvrTexture/GvrDataCodec.cs#L712
	for (unsigned int y = 0; y < tilesY; y += 2) {
	for (unsigned int x = 0; x < tilesX; x += 2) {
		for (unsigned int y2 = 0; y2 < 2; y2++) {
		for (unsigned int x2 = 0; x2 < 2; x2++, dxt1_src++) {
			// Convert the first two colors from RGB565.
			if (big_endian) {
				pal[0].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(be16_to_cpu(dxt1_src->color[0]));
				pal[1].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(be16_to_cpu(dxt1_src->color[1]));
			} else {
				pal[0].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(dxt1_src->color[0]));
				pal[1].u32 = ImageDecoderPrivate::RGB565_to_ARGB32(le16_to_cpu(dxt1_src->color[1]));
			}

			// Calculate the second two colors.
			if (pal[0].u32 > pal[1].u32) {
				pal[2].r = ((2 * pal[0].r) + pal[1].r) / 3;
				pal[2].g = ((2 * pal[0].g) + pal[1].g) / 3;
				pal[2].b = ((2 * pal[0].b) + pal[1].b) / 3;
				pal[2].a = 0xFF;

				pal[3].r = ((2 * pal[1].r) + pal[0].r) / 3;
				pal[3].g = ((2 * pal[1].g) + pal[0].g) / 3;
				pal[3].b = ((2 * pal[1].b) + pal[0].b) / 3;
				pal[3].a = 0xFF;
			} else {
				pal[2].r = (pal[0].r + pal[1].r) / 2;
				pal[2].g = (pal[0].g + pal[1].g) / 2;
				pal[2].b = (pal[0].b + pal[1].b) / 2;
				pal[2].a = 0xFF;

				// TODO: This may be either black or transparent.
				// Provide a way to specify that.
				pal[3].u32 = 0;
			}

			// FIXME: This is correct for BE DXT1 (GameCube),
			// but might not be right for LE DXT1 (PC).

			// Process the 16 color indexes.
			// NOTE: MSB has the left-most pixel of the *bottom* row.
			// LSB has the right-most pixel of the *top* row.
			static const uint8_t pxmap[16] = {3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12};
			// TODO: Pointer optimization.
			uint32_t indexes = dxt1_src->indexes;
			for (unsigned int i = 0; i < 16; i++, indexes >>= 2) {
				tileBuf[pxmap[i]] = pal[indexes & 3].u32;
			}

			// Blit the tile to the main image buffer.
			ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x+x2, y+y2);
		} }
	} }

	// Image has been converted.
	return img;
}

// Explicit instantiation.
template rp_image *ImageDecoder::fromDXT1<true>(
	int width, int height,
	const uint8_t *img_buf, int img_siz);

}

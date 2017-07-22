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

// DXT1 block format.
struct dxt1_block {
	uint16_t color[2];	// Colors 0 and 1, in RGB565 format.
	uint32_t indexes;	// Two-bit color indexes.
};

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

/**
 * Decode DXT1 tile palettes.
 * @tparam big_endian If true, use big-endian value decoding.
 * @tparam col3_black If true, color 3 is black. Otherwise, color 3 is transparent.
 * @param pal		[out] Array of four argb32_t values.
 * @param dxt1_src	[in] DXT1 block.
 */
template<bool big_endian, bool col3_black>
static inline void decode_DXT1_tile_palette(argb32_t pal[4], const dxt1_block *dxt1_src)
{
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

		// Black and/or transparent.
		pal[3].u32 = (col3_black ? 0xFF000000 : 0x00000000);
	}
}

/**
 * Convert a GameCube DXT1 image to rp_image.
 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *ImageDecoder::fromDXT1_GCN(int width, int height,
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
	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	// Tiles are arranged in 2x2 blocks.
	// Reference: https://github.com/nickworonekin/puyotools/blob/80f11884f6cae34c4a56c5b1968600fe7c34628b/Libraries/VrSharp/GvrTexture/GvrDataCodec.cs#L712
	for (unsigned int y = 0; y < tilesY; y += 2) {
	for (unsigned int x = 0; x < tilesX; x += 2) {
		for (unsigned int y2 = 0; y2 < 2; y2++) {
		for (unsigned int x2 = 0; x2 < 2; x2++, dxt1_src++) {
			// Decode the DXT1 tile palette.
			// TODO: Color 3 may be either black or transparent.
			// Figure out if there's a way to specify that in GVR.
			argb32_t pal[4];
			decode_DXT1_tile_palette<true, false>(pal, dxt1_src);

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

/**
 * Convert a DXT1 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
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
	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt1_src++) {
		// Decode the DXT1 tile palette.
		// TODO: Color 3 may be either black or transparent.
		// Figure out if there's a way to specify that in DDS.
		argb32_t pal[4];
		decode_DXT1_tile_palette<false, false>(pal, dxt1_src);

		// Process the 16 color indexes.
		uint32_t indexes = dxt1_src->indexes;
		for (unsigned int i = 0; i < 16; i++, indexes >>= 2) {
			tileBuf[i] = pal[indexes & 3].u32;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Image has been converted.
	return img;
}

}

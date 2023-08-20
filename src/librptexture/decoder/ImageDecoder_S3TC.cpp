/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_S3TC.cpp: Image decoding functions: S3TC                   *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "ImageDecoder_S3TC.hpp"
#include "ImageDecoder_p.hpp"

#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// C++ STL classes
using std::array;

// References:
// - http://www.matejtomcik.com/Public/KnowHow/DXTDecompression/
// - http://www.fsdeveloper.com/wiki/index.php?title=DXT_compression_explained
// - https://en.wikipedia.org/wiki/S3_Texture_Compression
// - https://www.khronos.org/opengl/wiki/S3_Texture_Compression
// - https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression

// S2TC: https://github.com/divVerent/s2tc/blob/master/s2tc_libtxc_dxtn.cpp

// TODO: Precalculate an alpha "palette" similar to the 4-color tile palettes?
// Also applies to BC4/BC5 color palettes.

namespace LibRpTexture { namespace ImageDecoder {

// DXT1 block format.
struct dxt1_block {
	uint16_t color[2];	// Colors 0 and 1, in RGB565 format.
	uint32_t indexes;	// Two-bit color indexes.
};
ASSERT_STRUCT(dxt1_block, 8);

// DXT5 alpha+codes struct.
// Also used by BC4/BC5 for color channels.
union dxt5_alpha {
	struct {
		uint8_t values[2];	// Alpha values.
		uint8_t codes[6];	// Alpha operation codes. (48-bit unsigned; 3-bit per pixel)
	};
	uint64_t u64;	// Access the 48-bit code value directly. (Requires shifting.)
};
ASSERT_STRUCT(dxt5_alpha, 8);

/**
 * Extract the 48-bit code value from dxt5_alpha.
 * @param data dxt5_alpha.
 * @return 48-bit code value.
 */
static FORCEINLINE uint64_t extract48(const dxt5_alpha *RESTRICT data)
{
	// codes[6] starts at 0x02 within dxt5_alpha.
	// Hence, we need to lshift it after byteswapping.
	// TODO: constexpr?
	return le64_to_cpu(data->u64) >> 16;
}

// decode_DXTn_tile_color_palette flags.
enum DXTn_Palette_Flags {
	DXTn_PALETTE_BIG_ENDIAN		= (1U << 0),
	DXTn_PALETTE_COLOR3_ALPHA	= (1U << 1),	// GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
	DXTn_PALETTE_COLOR0_GT_COLOR1	= (1U << 2),	// Assume color0 > color1. (DXT2/DXT3)
};

/**
 * Decode a DXTn tile color palette. (S3TC version)
 * @tparam flags Flags. (See DXTn_Palette_Flags)
 * @param pal		[out] Array of four argb32_t values.
 * @param dxt1_src	[in] DXT1 block.
 */
template<unsigned int flags>
static inline void decode_DXTn_tile_color_palette_S3TC(argb32_t *RESTRICT pal, const dxt1_block *RESTRICT dxt1_src)
{
	// Convert the first two colors from RGB565.
	uint16_t c0, c1;
	if (flags & DXTn_PALETTE_BIG_ENDIAN) {
		c0 = be16_to_cpu(dxt1_src->color[0]);
		c1 = be16_to_cpu(dxt1_src->color[1]);
	} else {
		c0 = le16_to_cpu(dxt1_src->color[0]);
		c1 = le16_to_cpu(dxt1_src->color[1]);
	}
	pal[0].u32 = RGB565_to_ARGB32(c0);
	pal[1].u32 = RGB565_to_ARGB32(c1);

	// Calculate the second two colors.
	if ((flags & DXTn_PALETTE_COLOR0_GT_COLOR1) || (c0 > c1)) {
		// color0 > color1
		pal[2].r = ((2 * pal[0].r) + pal[1].r) / 3;
		pal[2].g = ((2 * pal[0].g) + pal[1].g) / 3;
		pal[2].b = ((2 * pal[0].b) + pal[1].b) / 3;
		pal[2].a = 0xFF;

		pal[3].r = ((2 * pal[1].r) + pal[0].r) / 3;
		pal[3].g = ((2 * pal[1].g) + pal[0].g) / 3;
		pal[3].b = ((2 * pal[1].b) + pal[0].b) / 3;
		pal[3].a = 0xFF;
	} else {
		// color0 <= color1
		pal[2].r = (pal[0].r + pal[1].r) / 2;
		pal[2].g = (pal[0].g + pal[1].g) / 2;
		pal[2].b = (pal[0].b + pal[1].b) / 2;
		pal[2].a = 0xFF;

		// Black and/or transparent.
		pal[3].u32 = ((flags & DXTn_PALETTE_COLOR3_ALPHA) ? 0x00000000 : 0xFF000000);
	}
}

/**
 * Decode the DXT5 alpha channel value. (S3TC version)
 * @param a3	3-bit alpha selector code.
 * @param alpha	2-element alpha array from dxt5_block.
 * @return Alpha channel value.
 */
static inline uint8_t decode_DXT5_alpha_S3TC(unsigned int a3, const uint8_t *RESTRICT alpha)
{
	unsigned int a_ret = 255;

	if (alpha[0] > alpha[1]) {
		switch (a3 & 7) {
			case 0:
				a_ret = alpha[0];
				break;
			case 1:
				a_ret = alpha[1];
				break;
			case 2:
				a_ret = ((6 * alpha[0]) + (1 * alpha[1])) / 7;
				break;
			case 3:
				a_ret = ((5 * alpha[0]) + (2 * alpha[1])) / 7;
				break;
			case 4:
				a_ret = ((4 * alpha[0]) + (3 * alpha[1])) / 7;
				break;
			case 5:
				a_ret = ((3 * alpha[0]) + (4 * alpha[1])) / 7;
				break;
			case 6:
				a_ret = ((2 * alpha[0]) + (5 * alpha[1])) / 7;
				break;
			case 7:
				a_ret = ((1 * alpha[0]) + (6 * alpha[1])) / 7;
				break;
		}
	} else {
		switch (a3 & 7) {
			case 0:
				a_ret = alpha[0];
				break;
			case 1:
				a_ret = alpha[1];
				break;
			case 2:
				a_ret = ((4 * alpha[0]) + (1 * alpha[1])) / 5;
				break;
			case 3:
				a_ret = ((3 * alpha[0]) + (2 * alpha[1])) / 5;
				break;
			case 4:
				a_ret = ((2 * alpha[0]) + (3 * alpha[1])) / 5;
				break;
			case 5:
				a_ret = ((1 * alpha[0]) + (4 * alpha[1])) / 5;
				break;
			case 6:
				a_ret = 0;
				break;
			case 7:
				a_ret = 255;
				break;
		}
	}

	// Prevent overflow.
	return static_cast<uint8_t>(a_ret > 255 ? 255 : a_ret);
}

/**
 * Convert a GameCube DXT1 image to rp_image.
 * The GameCube variant has 2x2 block tiling in addition to 4x4 pixel tiling.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT1_GCN(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= (((size_t)width * (size_t)height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)width * (size_t)height) / 2))
	{
		return nullptr;
	}

	// GameCube DXT1 uses 2x2 blocks of 4x4 tiles.
	assert(width % 8 == 0);
	assert(height % 8 == 0);
	if (width % 8 != 0 || height % 8 != 0)
		return nullptr;

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(width, height, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 4);
	const unsigned int tilesY = static_cast<unsigned int>(height / 4);

	// Temporary 4-tile buffer.
	array<array<uint32_t, 4*4>, 4> tileBuf;

	// Tiles are arranged in 2x2 blocks.
	// Reference: https://github.com/nickworonekin/puyotools/blob/80f11884f6cae34c4a56c5b1968600fe7c34628b/Libraries/VrSharp/GvrTexture/GvrDataCodec.cs#L712
	for (unsigned int y = 0; y < tilesY; y += 2) {
	for (unsigned int x = 0; x < tilesX; x += 2) {
		// Decode 4 tiles at once.
		for (unsigned int tile = 0; tile < 4; tile++, dxt1_src++) {
			// Decode the DXT1 tile palette.
			// TODO: Color 3 may be either black or transparent.
			// Figure out if there's a way to specify that in GVR.
			// Assuming transparent for now, since most GVR DXT1
			// textures use transparency.
			argb32_t pal[4];
			decode_DXTn_tile_color_palette_S3TC<DXTn_PALETTE_BIG_ENDIAN | DXTn_PALETTE_COLOR3_ALPHA>(pal, dxt1_src);

			// Process the 16 color indexes.
			// NOTE: The tile indexes are stored "backwards" due to
			// big-endian shenanigans.
			uint32_t indexes = be32_to_cpu(dxt1_src->indexes);
			const auto tileBuf_rend = tileBuf[tile].rend();
			for (auto iter = tileBuf[tile].rbegin();
			     iter != tileBuf_rend; ++iter, indexes >>= 2)
			{
				*iter = pal[indexes & 3].u32;
			}
		}

		// Blit the tiles to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf[0], x+0, y+0);
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf[1], x+1, y+0);
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf[2], x+0, y+1);
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf[3], x+1, y+1);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,1};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT1 image to rp_image.
 * @param palflags decode_DXTn_tile_color_palette_S3TC<>() flags.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
template<unsigned int palflags>
static rp_image_ptr T_fromDXT1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// DXT1 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= (((size_t)physWidth * (size_t)physHeight) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)physWidth * (size_t)physHeight) / 2))
	{
		return nullptr;
	}

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	const dxt1_block *dxt1_src = reinterpret_cast<const dxt1_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(physWidth / 4);
	const unsigned int tilesY = static_cast<unsigned int>(physHeight / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt1_src++) {
		// Decode the DXT1 tile palette.
		argb32_t pal[4];
		decode_DXTn_tile_color_palette_S3TC<palflags>(pal, dxt1_src);

		// Process the 16 color indexes.
		uint32_t indexes = le32_to_cpu(dxt1_src->indexes);
		for (uint32_t &p : tileBuf) {
			p = pal[indexes & 3].u32;
			indexes >>= 2;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
	} }

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,1};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as black.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	return T_fromDXT1<0>(width, height, img_buf, img_siz);
}

/**
 * Convert a DXT1 image to rp_image.
 * S3TC palette index 3 will be interpreted as fully transparent.
 *
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT1_A1(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	return T_fromDXT1<DXTn_PALETTE_COLOR3_ALPHA>(width, height, img_buf, img_siz);
}

/**
 * Convert a DXT2 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT2 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT2(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// TODO: Completely untested. Needs testing!

	// Use fromDXT3(), then convert from premultiplied alpha
	// to standard alpha.
	const rp_image_ptr img = fromDXT3(width, height, img_buf, img_siz);
	if (!img) {
		return nullptr;
	}

	// Un-premultiply the image.
	int ret = img->un_premultiply();
	if (ret != 0) {
		return nullptr;
	}
	return img;
}

/**
 * Convert a DXT3 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT3 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT3(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// DXT3 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= ((size_t)physWidth * (size_t)physHeight));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((size_t)physWidth * (size_t)physHeight))
	{
		return nullptr;
	}

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// DXT3 block format.
	struct dxt3_block {
		uint64_t alpha;		// Alpha values. (4-bit per pixel)
		dxt1_block colors;	// DXT1-style color block.
	};
	ASSERT_STRUCT(dxt3_block, 16);
	const dxt3_block *dxt3_src = reinterpret_cast<const dxt3_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(physWidth / 4);
	const unsigned int tilesY = static_cast<unsigned int>(physHeight / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt3_src++) {
		// Decode the DXT3 tile palette.
		argb32_t pal[4];
		decode_DXTn_tile_color_palette_S3TC<DXTn_PALETTE_COLOR0_GT_COLOR1>(pal, &dxt3_src->colors);

		// Process the 16 color indexes and apply alpha.
		uint32_t indexes = le32_to_cpu(dxt3_src->colors.indexes);
		uint64_t alpha = le64_to_cpu(dxt3_src->alpha);
		for (uint32_t &p : tileBuf) {
			argb32_t color = pal[indexes & 3];
			// TODO: Verify alpha value handling for DXT3.
			color.a = (alpha & 0xF) | ((alpha & 0xF) << 4);
			p = color.u32;

			// Next indexes.
			indexes >>= 2;
			alpha >>= 4;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
	} }

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,4};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a DXT4 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// TODO: Completely untested. Needs testing!

	// Use fromDXT5(), then convert from premultiplied alpha
	// to standard alpha.
	const rp_image_ptr img = fromDXT5(width, height, img_buf, img_siz);
	if (!img) {
		return nullptr;
	}

	// Un-premultiply the image.
	int ret = img->un_premultiply();
	if (ret != 0) {
		return nullptr;
	}
	return img;
}

/**
 * Convert a DXT5 image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf DXT5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromDXT5(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// DXT5 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= ((size_t)physWidth * (size_t)physHeight));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((size_t)physWidth * (size_t)physHeight))
	{
		return nullptr;
	}

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// DXT5 block format.
	struct dxt5_block {
		dxt5_alpha alpha;
		dxt1_block colors;	// DXT1-style color block.
	};
	ASSERT_STRUCT(dxt5_block, 16);
	const dxt5_block *dxt5_src = reinterpret_cast<const dxt5_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(physWidth / 4);
	const unsigned int tilesY = static_cast<unsigned int>(physHeight / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, dxt5_src++) {
		// Decode the DXT5 tile palette.
		argb32_t pal[4];
		decode_DXTn_tile_color_palette_S3TC<0>(pal, &dxt5_src->colors);

		// Get the DXT5 alpha codes.
		uint64_t alpha48 = extract48(&dxt5_src->alpha);

		// Process the 16 color and alpha indexes.
		uint32_t indexes = le32_to_cpu(dxt5_src->colors.indexes);
		for (uint32_t &p : tileBuf) {
			argb32_t color = pal[indexes & 3];
			// Decode the alpha channel value.
			color.a = decode_DXT5_alpha_S3TC(alpha48 & 7, dxt5_src->alpha.values);
			p = color.u32;

			// Next indexes.
			indexes >>= 2;
			alpha48 >>= 3;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
	} }

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a BC4 (ATI1) image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC4 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromBC4(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// BC4 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= (((size_t)width * (size_t)height) / 2));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < (((size_t)physWidth * (size_t)physHeight) / 2))
	{
		return nullptr;
	}

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// BC4 block format.
	struct bc4_block {
		dxt5_alpha red;
	};
	ASSERT_STRUCT(bc4_block, 8);
	const bc4_block *bc4_src = reinterpret_cast<const bc4_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(physWidth / 4);
	const unsigned int tilesY = static_cast<unsigned int>(physHeight / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

	// S3TC version.
	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, bc4_src++) {
		// BC4 colors are determined using DXT5-style alpha interpolation.

		// Get the BC4 color codes.
		uint64_t red48 = extract48(&bc4_src->red);

		// Process the 16 color indexes.
		// NOTE: Using red instead of grayscale here.
		argb32_t color;
		color.u32 = 0xFF000000U;	// opaque black
		for (uint32_t &p : tileBuf) {
			// Decode the red channel value.
			color.r = decode_DXT5_alpha_S3TC(red48 & 7, bc4_src->red.values);
			p = color.u32;

			// Next index.
			red48 >>= 3;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
	} }

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	// NOTE: We have to set '1' for the empty Green and Blue channels,
	// since libpng complains if it's set to '0'.
	static const rp_image::sBIT_t sBIT = {8,1,1,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a BC5 (ATI2) image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf BC5 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)]
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromBC5(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// BC5 uses 4x4 tiles, but some container formats allow
	// the last tile to be cut off, so round up for the
	// physical tile size.
	const int physWidth = ALIGN_BYTES(4, width);
	const int physHeight = ALIGN_BYTES(4, height);

	assert(img_siz >= ((size_t)width * (size_t)height));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((size_t)physWidth * (size_t)physHeight))
	{
		return nullptr;
	}

	// Create an rp_image.
	const rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// BC5 block format.
	struct bc5_block {
		dxt5_alpha red;
		dxt5_alpha green;
	};
	ASSERT_STRUCT(bc5_block, 16);
	const bc5_block *bc5_src = reinterpret_cast<const bc5_block*>(img_buf);

	// Calculate the total number of tiles.
	const unsigned int tilesX = static_cast<unsigned int>(width / 4);
	const unsigned int tilesY = static_cast<unsigned int>(height / 4);

	// Temporary tile buffer.
	array<uint32_t, 4*4> tileBuf;

	// S3TC version.
	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++, bc5_src++) {
		// BC5 colors are determined using DXT5-style alpha interpolation.

		// Get the BC5 color codes.
		uint64_t red48   = extract48(&bc5_src->red);
		uint64_t green48 = extract48(&bc5_src->green);

		// Process the 16 color indexes.
		argb32_t color;
		color.u32 = 0xFF000000U;	// opaque black
		for (uint32_t &p : tileBuf) {
			// Decode the red and green channel values.
			color.r = decode_DXT5_alpha_S3TC(red48   & 7, bc5_src->red.values);
			color.g = decode_DXT5_alpha_S3TC(green48 & 7, bc5_src->green.values);
			p = color.u32;

			// Next indexes.
			red48 >>= 3;
			green48 >>= 3;
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img.get(), tileBuf, x, y);
	} }

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	// NOTE: We have to set '1' for the empty Blue channel,
	// since libpng complains if it's set to '0'.
	static const rp_image::sBIT_t sBIT = {8,8,1,0,0};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a Red image to Luminance.
 * Use with fromBC4() to decode an LATC1 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
int fromRed8ToL8(const rp_image_ptr &img)
{
	assert(img != nullptr);
	assert(img->format() == rp_image::Format::ARGB32);
	if (!img || img->format() != rp_image::Format::ARGB32)
		return -EINVAL;

	// TODO: Optimize with SSE2/SSSE3?
	const unsigned int width = static_cast<unsigned int>(img->width());
	const unsigned int diff = (img->stride() - img->row_bytes()) / sizeof(argb32_t);
	argb32_t *line = static_cast<argb32_t*>(img->bits());
	for (unsigned int y = static_cast<unsigned int>(img->height()); y > 0; y--, line += diff) {
		unsigned int x = width;
		for (; x > 1; x -= 2, line += 2) {
			line[0].a = 0xFF;
			line[0].b = line[0].r;
			line[0].g = line[0].r;

			line[1].a = 0xFF;
			line[1].b = line[1].r;
			line[1].g = line[1].r;
		}
		if (x == 1) {
			line[0].a = 0xFF;
			line[0].b = line[0].r;
			line[0].g = line[0].r;
			line++;
		}
	}

	return 0;
}

/**
 * Convert a Red+Green image to Luminance+Alpha.
 * Use with fromBC5() to decode an LATC2 texture.
 * @param img rp_image to convert in-place.
 * @return 0 on success; negative POSIX error code on error.
 */
int fromRG8ToLA8(const rp_image_ptr &img)
{
	assert(img != nullptr);
	assert(img->format() == rp_image::Format::ARGB32);
	if (!img || img->format() != rp_image::Format::ARGB32)
		return -EINVAL;

	// TODO: Optimize with SSE2/SSSE3?
	const unsigned int width = static_cast<unsigned int>(img->width());
	const unsigned int diff = (img->stride() - img->row_bytes()) / sizeof(argb32_t);
	argb32_t *line = static_cast<argb32_t*>(img->bits());
	for (unsigned int y = static_cast<unsigned int>(img->height()); y > 0; y--, line += diff) {
		unsigned int x = width;
		for (; x > 1; x -= 2, line += 2) {
			line[0].a = line[0].g;
			line[0].b = line[0].r;
			line[0].g = line[0].r;

			line[1].a = line[1].g;
			line[1].b = line[1].r;
			line[1].g = line[1].r;
		}
		if (x == 1) {
			line[0].a = line[0].g;
			line[0].b = line[0].r;
			line[0].g = line[0].r;
			line++;
		}
	}

	return 0;
}

} }

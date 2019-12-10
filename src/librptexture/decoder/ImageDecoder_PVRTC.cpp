/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_PVRTC.cpp: Image decoding functions. (PVRTC)               *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

#include "PixelConversion.hpp"
using namespace LibRpTexture::PixelConversion;

// References:
// - https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
// - https://gist.github.com/andreysm/bf835e634de37c2ee48d
// - http://downloads.isee.biz/pub/files/igep-dsp-gst-framework-3_40_00/Graphics_SDK_4_05_00_03/GFX_Linux_SDK/OGLES/SDKPackage/Utilities/PVRTC/Documentation/PVRTC%20Texture%20Compression.Usage%20Guide.1.4f.External.pdf
// - https://s3.amazonaws.com/pvr-sdk-live/sdk-documentation/PVRTC-and-Texture-Compression-User-Guide.pdf
// - http://cdn2.imgtec.com/documentation/PVRTextureCompression.pdf

namespace LibRpTexture { namespace ImageDecoder {

// PVRTC data block.
union pvrtc_block {
	struct {
		// Modulation data.
		uint32_t mod_data;

		// Color B:
		// - Bit 15: Opaque bit 'Q'
		// - Bits 1-14:
		//   - If Q == 1:  RGB554
		//   - If Q == 0: ARGB3443
		// - Bit 0: Mode bit
		uint16_t colorB;

		// Color A:
		// - Bit 15: Opaque bit 'Q'
		// - Bits 0-14:
		//   - If Q == 1:  RGB555
		//   - If Q == 0: ARGB3444
		// NOTE: This format is the same as GCN RGB5A3.
		uint16_t colorA;
	};

	uint64_t u64;
};

/**
 * Convert color A to ARGB32.
 * @param px16 Color A.
 * @return ARGB32.
 */
static inline uint32_t colorAtoARGB32(uint16_t px16)
{
	// Color A uses the same format as GCN RGB5A3.
	return RGB5A3_to_ARGB32(px16);
}

/**
 * Convert color B to ARGB32.
 * @param px16 Color B.
 * @return ARGB32.
 */
static inline uint32_t colorBtoARGB32(uint16_t px16)
{
	// Color A is almost the same as GCN RGB5A3,
	// except the blue channel is smaller.
	uint32_t px32;

	if (px16 & 0x8000) {
		// BGR555: xRRRRRGG GGGBBBBx
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  = 0xFF000000U;	// no alpha channel
		px32 |= (((px16 << 3) & 0x0000F0) | ((px16 >> 1) & 0x00000F));	// B
		px32 |= (((px16 << 6) & 0x00F800) | ((px16 << 1) & 0x000700));	// G
		px32 |= (((px16 << 9) & 0xF80000) | ((px16 << 4) & 0x070000));	// R
	} else {
		// RGB4A3: xAAARRRR GGGGBBBx
		// ARGB32: AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
		px32  = ((px16 & 0x00F0) << 4);	// G
		px32 |= ((px16 & 0x0F00) << 8);	// R
		px32 |= (px32 << 4);		// Copy to the top nybble.

		// Calculate the blue channel.
		uint8_t b = ((px16 << 4) & 0xE0);
		b |= (b >> 3);
		b |= (b >> 3);

		// Calculate the alpha channel.
		uint8_t a = ((px16 >> 7) & 0xE0);
		a |= (a >> 3);
		a |= (a >> 3);

		// Apply the alpha and blue channels.
		px32 |= (a << 24);
		px32 |=  b;
	}

	return px32;
}

// Temporary RGBA structure that allows us to clamp it later.
// TODO: Use SSE2?
struct ColorRGBA {
	int B;
	int G;
	int R;
	int A;
};

/**
 * Clamp a ColorRGBA struct and convert it to ARGB32.
 * @param color ColorRGBA struct.
 * @return ARGB32 value.
 */
static inline uint32_t clamp_ColorRGBA(const ColorRGBA &color)
{
	uint32_t argb32 = 0;
	if (color.B > 255) {
		argb32 = 255;
	} else if (color.B > 0) {
		argb32 = color.B;
	}
	if (color.G > 255) {
		argb32 |= (255 << 8);
	} else if (color.G > 0) {
		argb32 |= (color.G << 8);
	}
	if (color.R > 255) {
		argb32 |= (255 << 16);
	} else if (color.R > 0) {
		argb32 |= (color.R << 16);
	}
	if (color.A > 255) {
		argb32 |= (255 << 24);
	} else if (color.A > 0) {
		argb32 |= (color.A << 24);
	}
	return argb32;
}

/**
 * Mode 0 color interpolation.
 * @param colors Array containing two ARGB32 colors.
 * @param mod_data 2-bit modulation data.
 * @return Interpolated color.
 */
static inline uint32_t interp_colors_mode0(const uint32_t color[2], unsigned int mod_data)
{
	if (mod_data == 0) {
		// No modulation.
		return color[0];
	}

	// TODO: Optimize using SSE.
	argb32_t argb[2];
	argb[0].u32 = color[0];
	argb[1].u32 = color[1];

	// Interpolation formula: Output = A + Mod*(B - A)
	ColorRGBA rgba;
	rgba.B = argb[1].b - argb[0].b;
	rgba.G = argb[1].g - argb[0].g;
	rgba.R = argb[1].r - argb[0].r;
	switch (mod_data) {
		default:
			assert(!"Unhandled modulation data.");
			return color[0];

		case 1:
			// Weight: 4/8
			rgba.B = rgba.B / 2;
			rgba.G = rgba.G / 2;
			rgba.R = rgba.R / 2;

			rgba.A = argb[1].a - argb[0].a;
			rgba.A = rgba.A / 2;
			break;

		case 2:
			// Weight: 4/8, punch-through alpha
			// NOTE: Color values are kept as-is,
			// even though A=0.
			rgba.B = rgba.B / 2;
			rgba.G = rgba.G / 2;
			rgba.R = rgba.R / 2;
			rgba.A = 0;
			break;

		case 3:
			// Weight: 1
			rgba.A = argb[1].a - argb[0].a;
			break;
	}

	rgba.B += argb[0].b;
	rgba.G += argb[0].g;
	rgba.R += argb[0].r;
	if (mod_data != 2) {
		// TODO: Move into the switch/case?
		rgba.A += argb[0].a;
	}

	// Clamp the color components.
	return clamp_ColorRGBA(rgba);
}

/**
 * Mode 1 color interpolation.
 * @param colors Array containing two ARGB32 colors.
 * @param mod_data 2-bit modulation data.
 * @return Interpolated color.
 */
static inline uint32_t interp_colors_mode1(const uint32_t color[2], unsigned int mod_data)
{
	if (mod_data == 0) {
		// No modulation.
		return color[0];
	}

	// TODO: Optimize using SSE.
	argb32_t argb[2];
	argb[0].u32 = color[0];
	argb[1].u32 = color[1];

	// Interpolation formula: Output = A + Mod*(B - A)
	ColorRGBA rgba;
	rgba.B = argb[1].b - argb[0].b;
	rgba.G = argb[1].g - argb[0].g;
	rgba.R = argb[1].r - argb[0].r;
	rgba.A = argb[1].a - argb[0].a;
	switch (mod_data) {
		default:
			assert(!"Unhandled modulation data.");
			return color[0];

		case 1:
			// Weight: 3/8
			rgba.B = rgba.B * 8 / 3;
			rgba.G = rgba.G * 8 / 3;
			rgba.R = rgba.R * 8 / 3;
			rgba.A = rgba.A * 8 / 3;
			break;

		case 2:
			// Weight: 5/8
			rgba.B = rgba.B * 8 / 5;
			rgba.G = rgba.G * 8 / 5;
			rgba.R = rgba.R * 8 / 5;
			rgba.A = rgba.A * 8 / 5;
			break;

		case 3:
			// Weight: 1
			break;
	}

	rgba.B += argb[0].b;
	rgba.G += argb[0].g;
	rgba.R += argb[0].r;
	rgba.A += argb[0].a;

	// Clamp the color components.
	return clamp_ColorRGBA(rgba);
}

// Pixels are reordered (twiddled) in PVRTC. Bits of x coordinate are interleaved with bits of y.
// TODO: Optimize into lookup table.
// Reference: https://gist.github.com/andreysm/bf835e634de37c2ee48d
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)|((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

/**
 * Convert a PVRTC 2bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTC_2bpp(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(img_siz >= ((width * height) / 4));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < ((width * height) / 4))
	{
		return nullptr;
	}

	// PVRTC 2bpp uses 8x4 tiles.
	assert(width % 8 == 0);
	assert(height % 4 == 0);
	if (width % 8 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 8);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// NOTE: PVRTC block indexes are twiddled.
	const pvrtc_block *const pvrtc_src = reinterpret_cast<const pvrtc_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[8*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++) {
		// TODO: Endianness conversion?
		const pvrtc_block *src = &pvrtc_src[TWIDOUT(x, y)];

		// Get the two color values.
		uint32_t color[2];
		color[0] = colorAtoARGB32(src->colorA);
		color[1] = colorBtoARGB32(src->colorB);

		uint32_t mod_data = src->mod_data;
		if (!(src->colorB & 0x01)) {
			// Modulation mode 0: Each bit is 0 for A, 1 for B.
			for (unsigned int i = 0; i < 32; i++, mod_data >>= 1) {
				tileBuf[i] = color[mod_data & 1];
			}
		} else {
			// Modulation mode 1: Each bit represents two pixels,
			// which allows interpolation to be used.
			// TODO: Verify this. There's probably some other
			// interpolation between the two pixels...
			// TODO: Is the interpolation correct?
			// NOTE: Should be checkerboard pattern, with interpolation.
			for (unsigned int i = 0; i < 32; i += 2, mod_data >>= 2) {
				const uint32_t interp = interp_colors_mode1(color, mod_data & 3);
				tileBuf[i+0] = interp;
				tileBuf[i+1] = interp;
			}
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 8, 4>(img, tileBuf, x, y);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

/**
 * Convert a PVRTC 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf ETC1 image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/2]
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTC_4bpp(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz)
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

	// PVRTC 4bpp uses 4x4 tiles.
	assert(width % 4 == 0);
	assert(height % 4 == 0);
	if (width % 4 != 0 || height % 4 != 0)
		return nullptr;

	// Calculate the total number of tiles.
	const unsigned int tilesX = (unsigned int)(width / 4);
	const unsigned int tilesY = (unsigned int)(height / 4);

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// NOTE: PVRTC block indexes are twiddled.
	const pvrtc_block *const pvrtc_src = reinterpret_cast<const pvrtc_block*>(img_buf);

	// Temporary tile buffer.
	uint32_t tileBuf[4*4];

	for (unsigned int y = 0; y < tilesY; y++) {
	for (unsigned int x = 0; x < tilesX; x++) {
		// TODO: Endianness conversion?
		const pvrtc_block *src = &pvrtc_src[TWIDOUT(x, y)];

		// Get the two color values.
		uint32_t color[2];
		color[0] = colorAtoARGB32(src->colorA);
		color[1] = colorBtoARGB32(src->colorB);

		uint32_t mod_data = src->mod_data;
		if (!(src->colorB & 0x01)) {
			// Modulation mode 0.
			for (unsigned int i = 0; i < 16; i++, mod_data >>= 2) {
				tileBuf[i] = interp_colors_mode0(color, mod_data & 3);
			}
		} else {
			// Modulation mode 1.
			for (unsigned int i = 0; i < 16; i++, mod_data >>= 2) {
				tileBuf[i] = interp_colors_mode1(color, mod_data & 3);
			}
		}

		// Blit the tile to the main image buffer.
		ImageDecoderPrivate::BlitTile<uint32_t, 4, 4>(img, tileBuf, x, y);
	} }

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

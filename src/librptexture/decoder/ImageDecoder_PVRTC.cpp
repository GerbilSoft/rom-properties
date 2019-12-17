/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_PVRTC.cpp: Image decoding functions. (PVRTC)               *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "ImageDecoder.hpp"
#include "ImageDecoder_p.hpp"

#include "PVRTDecompress.h"

// References:
// - https://www.khronos.org/registry/OpenGL/extensions/IMG/IMG_texture_compression_pvrtc.txt
// - https://gist.github.com/andreysm/bf835e634de37c2ee48d
// - http://downloads.isee.biz/pub/files/igep-dsp-gst-framework-3_40_00/Graphics_SDK_4_05_00_03/GFX_Linux_SDK/OGLES/SDKPackage/Utilities/PVRTC/Documentation/PVRTC%20Texture%20Compression.Usage%20Guide.1.4f.External.pdf
// - https://s3.amazonaws.com/pvr-sdk-live/sdk-documentation/PVRTC-and-Texture-Compression-User-Guide.pdf
// - http://cdn2.imgtec.com/documentation/PVRTextureCompression.pdf

namespace LibRpTexture { namespace ImageDecoder {

/**
 * Convert a PVRTC 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTC(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	uint8_t mode)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// Expected size to be read by the PowerVR Native SDK.
	const uint32_t expected_size_in = ((width * height) /
		(((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) ? 4 : 2));

	assert(img_siz >= static_cast<int>(expected_size_in));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < static_cast<int>(expected_size_in))
	{
		return nullptr;
	}

	// PVRTC 2bpp uses 8x4 tiles.
	// PVRTC 4bpp uses 4x4 tiles.
	if ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) {
		// PVRTC 2bpp
		assert(width % 8 == 0);
		assert(height % 4 == 0);
		if (width % 8 != 0 || height % 4 != 0)
			return nullptr;
	} else {
		// PVRTC 4bpp
		assert(width % 4 == 0);
		assert(height % 4 == 0);
		if (width % 4 != 0 || height % 4 != 0)
			return nullptr;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Use the PowerVR Native SDK to decompress the texture.
	// Return value is the size of the *input* data that was decompressed.
	// TODO: Row padding?
	uint32_t size = pvr::PVRTDecompressPVRTC(img_buf, ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP), width, height,
		static_cast<uint8_t*>(img->bits()));
	assert(size == expected_size_in);
	if (size != expected_size_in) {
		// Read error...
		delete img;
		return nullptr;
	}

	// TODO: If !has_alpha, make sure the alpha channel is all 0xFF.

	// Set the sBIT metadata.
	static const rp_image::sBIT_t sBIT_alpha  = {8,8,8,0,8};
	static const rp_image::sBIT_t sBIT_opaque = {8,8,8,0,0};
	img->set_sBIT(((mode & PVRTC_ALPHA_MASK) == PVRTC_ALPHA_YES) ? &sBIT_alpha : &sBIT_opaque);

	// Image has been converted.
	return img;
}

/**
 * Convert a PVRTC 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
rp_image *fromPVRTCII(int width, int height,
	const uint8_t *RESTRICT img_buf, int img_siz,
	uint8_t mode)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// Expected size to be read by the PowerVR Native SDK.
	const uint32_t expected_size_in = ((width * height) /
		(((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) ? 4 : 2));

	assert(img_siz >= static_cast<int>(expected_size_in));
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < static_cast<int>(expected_size_in))
	{
		return nullptr;
	}

	// FIXME: PVRTC-II supports non-power-of-two textures,
	// including sizes that aren't a multiple of the tile size,
	// in which case, we'll need to adjust it. For now, we'll
	// keep the assertion in here.

	// PVRTC-II 2bpp uses 8x4 tiles.
	// PVRTC-II 4bpp uses 4x4 tiles.
	if ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) {
		// PVRTC 2bpp
		assert(width % 8 == 0);
		assert(height % 4 == 0);
		if (width % 8 != 0 || height % 4 != 0)
			return nullptr;
	} else {
		// PVRTC 4bpp
		assert(width % 4 == 0);
		assert(height % 4 == 0);
		if (width % 4 != 0 || height % 4 != 0)
			return nullptr;
	}

	// Create an rp_image.
	rp_image *img = new rp_image(width, height, rp_image::FORMAT_ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		delete img;
		return nullptr;
	}

	// Use the PowerVR Native SDK to decompress the texture.
	// Return value is the size of the *input* data that was decompressed.
	// TODO: Row padding?
	uint32_t size = pvr::PVRTDecompressPVRTCII(img_buf, ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP), width, height,
		static_cast<uint8_t*>(img->bits()));
	assert(size == expected_size_in);
	if (size != expected_size_in) {
		// Read error...
		delete img;
		return nullptr;
	}

	// TODO: If !has_alpha, make sure the alpha channel is all 0xFF.

	// Set the sBIT metadata.
	// NOTE: Assuming PVRTC-II always has alpha for now.
	static const rp_image::sBIT_t sBIT  = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

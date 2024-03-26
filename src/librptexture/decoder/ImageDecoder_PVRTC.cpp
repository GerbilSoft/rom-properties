/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ImageDecoder_PVRTC.cpp: Image decoding functions: PVRTC                 *
 *                                                                         *
 * Copyright (c) 2019-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"

#include "ImageDecoder_PVRTC.hpp"
#include "PVRTDecompress.h"

// librptexture
#include "img/rp_image.hpp"

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
rp_image_ptr fromPVRTC(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	uint8_t mode)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// PVRTC uses 4x4 tiles (4bpp) or 8x4 tiles (2bpp), but
	// some container formats allow the last tile to be cut off.
	// NOTE: PVRTC *requires* power-of-2 textures.
	// Minimum image size: 8x8 (4bpp); 16x8 (2bpp) [based on PVRTC-II]
	if ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) {
		// PVRTC 2bpp
		assert(width >= 16);
		assert(height >= 8);
		if (width < 16 || height < 8)
			return nullptr;
	} else {
		// PVRTC 4bpp
		assert(width >= 8);
		assert(height >= 8);
		if (width < 8 || height < 8)
			return nullptr;
	}
	int physWidth = width, physHeight = height;
	if (!isPow2(physWidth)) {
		physWidth = nextPow2(physWidth);
	}
	if (!isPow2(physHeight)) {
		physHeight = nextPow2(physHeight);
	}

	// Expected size to be read by the PowerVR Native SDK.
	const uint32_t expected_size_in = ((physWidth * physHeight) /
		(((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) ? 4 : 2));

	assert(img_siz >= expected_size_in);
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < expected_size_in)
	{
		return nullptr;
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Use the PowerVR Native SDK to decompress the texture.
	// Return value is the size of the *input* data that was decompressed.
	// TODO: Row padding?
	uint32_t size = pvr::PVRTDecompressPVRTC(img_buf, ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP),
		physWidth, physHeight, static_cast<uint8_t*>(img->bits()));
	assert(size == expected_size_in);
	if (size != expected_size_in) {
		// Read error...
		return nullptr;
	}

	// TODO: If !has_alpha, make sure the alpha channel is all 0xFF.

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	static constexpr rp_image::sBIT_t sBIT_alpha  = {8,8,8,0,8};
	static constexpr rp_image::sBIT_t sBIT_opaque = {8,8,8,0,0};
	img->set_sBIT(((mode & PVRTC_ALPHA_MASK) == PVRTC_ALPHA_YES) ? &sBIT_alpha : &sBIT_opaque);

	// Image has been converted.
	return img;
}

/**
 * Convert a PVRTC-II 2bpp or 4bpp image to rp_image.
 * @param width Image width.
 * @param height Image height.
 * @param img_buf PVRTC image buffer.
 * @param img_siz Size of image data. [must be >= (w*h)/4]
 * @param mode Mode bitfield. (See PVRTC_Mode_e.)
 * @return rp_image, or nullptr on error.
 */
rp_image_ptr fromPVRTCII(int width, int height,
	const uint8_t *RESTRICT img_buf, size_t img_siz,
	uint8_t mode)
{
	// Verify parameters.
	assert(img_buf != nullptr);
	assert(width > 0);
	assert(height > 0);

	// PVRTC-II uses 4x4 tiles (4bpp) or 8x4 tiles (2bpp), but
	// PVRTC-II allows the last tile to be cut off.
	// NOTE: PVRTC-II does not require power-of-2 textures, but
	// we're using a slightly-modified PVRTC-I decoder, which *does*
	// requires power-of-2 textures.
	// Minimum image size: 8x8 (4bpp); 16x8 (2bpp)
	// TODO: PVRTDecompress supports images smaller than the minimum
	// image size, but it will copy to a temporary buffer, and the
	// return value will match the temporary buffer size.
	if ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) {
		// PVRTC-II 2bpp
		assert(height >= 16);
		if (width < 16 || height < 8)
			return nullptr;
	} else {
		// PVRTC-II 4bpp
		assert(height >= 8);
		if (width < 8 || height < 8)
			return nullptr;
	}
	// TODO: PVRTC-II NPOT support.
	int physWidth = width, physHeight = height;
	if (!isPow2(physWidth)) {
		physWidth = nextPow2(physWidth);
	}
	if (!isPow2(physHeight)) {
		physHeight = nextPow2(physHeight);
	}

	// Expected size to be read by the PowerVR Native SDK.
	const uint32_t expected_size_in = ((physWidth * physHeight) /
		(((mode & PVRTC_BPP_MASK) == PVRTC_2BPP) ? 4 : 2));

	assert(img_siz >= expected_size_in);
	if (!img_buf || width <= 0 || height <= 0 ||
	    img_siz < expected_size_in)
	{
		return nullptr;
	}

	// Create an rp_image.
	rp_image_ptr img = std::make_shared<rp_image>(physWidth, physHeight, rp_image::Format::ARGB32);
	if (!img->isValid()) {
		// Could not allocate the image.
		return nullptr;
	}

	// Use the PowerVR Native SDK to decompress the texture.
	// Return value is the size of the *input* data that was decompressed.
	// TODO: Row padding?
	uint32_t size = pvr::PVRTDecompressPVRTCII(img_buf, ((mode & PVRTC_BPP_MASK) == PVRTC_2BPP),
		physWidth, physHeight, static_cast<uint8_t*>(img->bits()));
	assert(size == expected_size_in);
	if (size != expected_size_in) {
		// Read error...
		return nullptr;
	}

	// TODO: If !has_alpha, make sure the alpha channel is all 0xFF.

	if (width < physWidth || height < physHeight) {
		// Shrink the image.
		img->shrink(width, height);
	}

	// Set the sBIT metadata.
	// NOTE: Assuming PVRTC-II always has alpha for now.
	static constexpr rp_image::sBIT_t sBIT = {8,8,8,0,8};
	img->set_sBIT(&sBIT);

	// Image has been converted.
	return img;
}

} }

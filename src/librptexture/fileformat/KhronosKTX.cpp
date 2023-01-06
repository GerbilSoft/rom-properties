/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * KhronosKTX.cpp: Khronos KTX image reader.                               *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
 */

#include "stdafx.h"
#include "config.librptexture.h"

#include "KhronosKTX.hpp"
#include "FileFormat_p.hpp"

#include "ktx_structs.h"
#include "gl_defs.h"
#include "data/GLenumStrings.hpp"

// librpbase, librpfile
#include "libi18n/i18n.h"
using LibRpBase::RomFields;
using LibRpFile::IRpFile;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"
#include "decoder/ImageDecoder_ETC1.hpp"
#include "decoder/ImageDecoder_BC7.hpp"
#include "decoder/ImageDecoder_PVRTC.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"

// C++ STL classes.
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpTexture {

class KhronosKTXPrivate final : public FileFormatPrivate
{
	public:
		KhronosKTXPrivate(KhronosKTX *q, IRpFile *file);
		~KhronosKTXPrivate();

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(KhronosKTXPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// KTX header.
		KTX_Header ktxHeader;

		// Is byteswapping needed?
		// (KTX file has the opposite endianness.)
		bool isByteswapNeeded;

		// Is HFlip/VFlip needed?
		// Some textures may be stored upside-down due to
		// the way GL texture coordinates are interpreted.
		// Default without KTXorientation is HFlip=false, VFlip=true
		rp_image::FlipOp flipOp;

		// Texture data start address.
		unsigned int texDataStartAddr;

		// Decoded image.
		rp_image *img;

		// Invalid pixel format message.
		char invalid_pixel_format[24];

		// Key/Value data.
		// NOTE: Stored as vector<vector<string> > instead of
		// vector<pair<string, string> > for compatibility with
		// RFT_LISTDATA.
		RomFields::ListData_t kv_data;

		/**
		 * Load the image.
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(void);

		/**
		 * Load key/value data.
		 */
		void loadKeyValueData(void);
};

FILEFORMAT_IMPL(KhronosKTX)

/** KhronosKTXPrivate **/

/* TextureInfo */
const char *const KhronosKTXPrivate::exts[] = {
	".ktx",

	nullptr
};
const char *const KhronosKTXPrivate::mimeTypes[] = {
	// Official MIME types.
	"image/ktx",

	nullptr
};
const TextureInfo KhronosKTXPrivate::textureInfo = {
	exts, mimeTypes
};

KhronosKTXPrivate::KhronosKTXPrivate(KhronosKTX *q, IRpFile *file)
	: super(q, file, &textureInfo)
	, isByteswapNeeded(false)
	, flipOp(rp_image::FLIP_V)
	, texDataStartAddr(0)
	, img(nullptr)
{
	// Clear the KTX header struct.
	memset(&ktxHeader, 0, sizeof(ktxHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

KhronosKTXPrivate::~KhronosKTXPrivate()
{
	UNREF(img);
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
const rp_image *KhronosKTXPrivate::loadImage(void)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `pixelHeight == 0` is allowed here. (1D texture)
	assert(ktxHeader.pixelWidth > 0);
	assert(ktxHeader.pixelWidth <= 32768);
	assert(ktxHeader.pixelHeight <= 32768);
	if (ktxHeader.pixelWidth == 0 || ktxHeader.pixelWidth > 32768 ||
	    ktxHeader.pixelHeight > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Texture cannot start inside of the KTX header.
	assert(texDataStartAddr >= sizeof(ktxHeader));
	if (texDataStartAddr < sizeof(ktxHeader)) {
		// Invalid texture data start address.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: KTX files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (ktxHeader.pixelHeight > 0 ? ktxHeader.pixelHeight : 1);

	// Calculate the expected size.
	// NOTE: Scanlines are 4-byte aligned.
	size_t expected_size;
	int stride = 0;
	switch (ktxHeader.glFormat) {
		case GL_RGB:
			// 24-bit RGB
			stride = ALIGN_BYTES(4, ktxHeader.pixelWidth * 3);
			expected_size = static_cast<unsigned int>(stride * height);
			break;

		case GL_RGBA:
			// 32-bit RGBA
			stride = ktxHeader.pixelWidth * 4;
			expected_size = static_cast<unsigned int>(stride * height);
			break;

		case GL_LUMINANCE:
			// 8-bit luminance
			stride = ALIGN_BYTES(4, ktxHeader.pixelWidth);
			expected_size = static_cast<unsigned int>(stride * height);
			break;

		case GL_RGB9_E5:
			// Uncompressed "special" 32bpp formats
			// TODO: Does KTX handle GL_RGB9_E5 as compressed?
			stride = ktxHeader.pixelWidth * 4;
			expected_size = static_cast<unsigned int>(stride * height);
			break;

		case 0:
		default:
			// May be a compressed format.
			// TODO: Stride calculations?
			switch (ktxHeader.glInternalFormat) {
				case GL_RGB8:
					// 24-bit RGB
					stride = ALIGN_BYTES(4, ktxHeader.pixelWidth * 3);
					expected_size = static_cast<unsigned int>(stride * height);
					break;

				case GL_RGBA8:
					// 32-bit RGBA
					stride = ktxHeader.pixelWidth * 4;
					expected_size = static_cast<unsigned int>(stride * height);
					break;

				case GL_R8:
					// 8-bit "Red"
					stride = ktxHeader.pixelWidth;
					expected_size = static_cast<unsigned int>(stride * height);
					break;

#ifdef ENABLE_PVRTC
				case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
				case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
					// 32 pixels compressed into 64 bits. (2bpp)
					// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
					expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(
						ktxHeader.pixelWidth, height);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
					// 32 pixels compressed into 64 bits. (2bpp)
					// NOTE: Width and height must be rounded to the nearest tile. (8x4)
					// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
					//expected_size = ALIGN_BYTES(8, ktxHeader.pixelWidth) *
					//                ALIGN_BYTES(4, (int)height) / 4;
					expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>
						(ktxHeader.pixelWidth, height);
					break;

				case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
				case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
					// 16 pixels compressed into 64 bits. (4bpp)
					// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
					expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(
						ktxHeader.pixelWidth, height);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
					// 16 pixels compressed into 64 bits. (4bpp)
					// NOTE: Width and height must be rounded to the nearest tile. (4x4)
					// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
					//expected_size = ALIGN_BYTES(4, ktxHeader.pixelWidth) *
					//                ALIGN_BYTES(4, (int)height) / 2;
					expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>
						(ktxHeader.pixelWidth, height);
					break;
#endif /* ENABLE_PVRTC */

				case GL_RGB_S3TC:
				case GL_RGB4_S3TC:
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				case GL_ETC1_RGB8_OES:
				case GL_COMPRESSED_R11_EAC:
				case GL_COMPRESSED_SIGNED_R11_EAC:
				case GL_COMPRESSED_RGB8_ETC2:
				case GL_COMPRESSED_SRGB8_ETC2:
				case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				case GL_COMPRESSED_RED_RGTC1:
				case GL_COMPRESSED_SIGNED_RED_RGTC1:
				case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
				case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
					// 16 pixels compressed into 64 bits. (4bpp)
					// NOTE: Width and height must be rounded to the nearest tile. (4x4)
					expected_size = ImageSizeCalc::T_calcImageSize(
						ALIGN_BYTES(4, ktxHeader.pixelWidth), ALIGN_BYTES(4, height)) / 2;
					break;

				//case GL_RGBA_S3TC:	// TODO
				//case GL_RGBA4_S3TC:	// TODO
				case GL_RGBA_DXT5_S3TC:
				case GL_RGBA4_DXT5_S3TC:
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				case GL_COMPRESSED_RG11_EAC:
				case GL_COMPRESSED_SIGNED_RG11_EAC:
				case GL_COMPRESSED_RGBA8_ETC2_EAC:
				case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
				case GL_COMPRESSED_RG_RGTC2:
				case GL_COMPRESSED_SIGNED_RG_RGTC2:
				case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
				case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
				case GL_COMPRESSED_RGBA_BPTC_UNORM:
				case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
					// 16 pixels compressed into 128 bits. (8bpp)
					// NOTE: Width and height must be rounded to the nearest tile. (4x4)
					expected_size = ImageSizeCalc::T_calcImageSize(
						ALIGN_BYTES(4, ktxHeader.pixelWidth), ALIGN_BYTES(4, height));
					break;

				case GL_RGB9_E5:
					// Uncompressed "special" 32bpp formats.
					// TODO: Does KTX handle GL_RGB9_E5 as compressed?
					expected_size = ImageSizeCalc::T_calcImageSize(ktxHeader.pixelWidth, height, sizeof(uint32_t));
					break;

				default: {
#ifdef ENABLE_ASTC
					unsigned int astc_idx;
					if (ktxHeader.glInternalFormat >= GL_COMPRESSED_RGBA_ASTC_4x4_KHR &&
					    ktxHeader.glInternalFormat <= GL_COMPRESSED_RGBA_ASTC_12x12_KHR)
					{
						static_assert((GL_COMPRESSED_RGBA_ASTC_12x12_KHR - GL_COMPRESSED_RGBA_ASTC_4x4_KHR) + 1 ==
							ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
						astc_idx = ktxHeader.glInternalFormat - GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
					}
					else if (ktxHeader.glInternalFormat >= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR &&
					         ktxHeader.glInternalFormat <= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR)
					{
						static_assert((GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR - GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR) + 1 ==
							ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
						astc_idx = ktxHeader.glInternalFormat - GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
					} else {
						// Not supported.
						return nullptr;
					}

					expected_size = ImageSizeCalc::calcImageSizeASTC(
						ktxHeader.pixelWidth, height,
						ImageDecoder::astc_lkup_tbl[astc_idx][0],
						ImageDecoder::astc_lkup_tbl[astc_idx][1]);
					break;
#else /* !ENABLE_ASTC */
					// Not supported.
					return nullptr;
#endif /* ENABLE_ASTC */
				}
			}
			break;
	}

	// Verify file size.
	if (texDataStartAddr + expected_size > file_sz) {
		// File is too small.
		return nullptr;
	}

	// Read the image size field.
	// NOTE: Divide image size by # of layers to get the expected size.
	uint32_t imageSize;
	size_t size = file->read(&imageSize, sizeof(imageSize));
	if (size != sizeof(imageSize)) {
		// Unable to read the image size field.
		return nullptr;
	}
	if (isByteswapNeeded) {
		imageSize = __swab32(imageSize);
	}
	if (ktxHeader.numberOfArrayElements <= 1) {
		// Single array element.
		if (imageSize != expected_size) {
			// Size is incorrect.
			return nullptr;
		}
	} else {
		// Multiple array elements.
		if (imageSize / ktxHeader.numberOfArrayElements != expected_size) {
			// Size is incorrect.
			return nullptr;
		}
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	// TODO: Byteswapping.
	// TODO: Handle variants. Check for channel sizes in glInternalFormat?
	// TODO: Handle sRGB post-processing? (for e.g. GL_SRGB8)
	switch (ktxHeader.glFormat) {
		case GL_RGB:
			// 24-bit RGB
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::BGR888,
				ktxHeader.pixelWidth, height,
				buf.get(), expected_size, stride);
			break;

		case GL_RGBA:
			// 32-bit RGBA
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::ABGR8888,
				ktxHeader.pixelWidth, height,
				reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
			break;

		case GL_LUMINANCE:
			// 8-bit Luminance
			img = ImageDecoder::fromLinear8(
				ImageDecoder::PixelFormat::L8,
				ktxHeader.pixelWidth, height,
				buf.get(), expected_size, stride);
			break;

		case GL_RGB9_E5:
			// Uncompressed "special" 32bpp formats
			// TODO: Does KTX handle GL_RGB9_E5 as compressed?
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::RGB9_E5,
				ktxHeader.pixelWidth, height,
				reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
			break;

		case 0:
		default:
			// May be a compressed format.
			// TODO: sRGB post-processing for sRGB formats?
			switch (ktxHeader.glInternalFormat) {
				case GL_RGB8:
					// 24-bit RGB
					img = ImageDecoder::fromLinear24(
						ImageDecoder::PixelFormat::BGR888,
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size, stride);
					break;

				case GL_RGBA8:
					// 32-bit RGBA
					img = ImageDecoder::fromLinear32(
						ImageDecoder::PixelFormat::ABGR8888,
						ktxHeader.pixelWidth, height,
						reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
					break;

				case GL_R8:
					// 8-bit "Red"
					img = ImageDecoder::fromLinear8(
						ImageDecoder::PixelFormat::R8,
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size, stride);
					break;

				case GL_RGB_S3TC:
				case GL_RGB4_S3TC:
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
					// DXT1-compressed texture.
					img = ImageDecoder::fromDXT1(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
					// DXT1-compressed texture with 1-bit alpha.
					img = ImageDecoder::fromDXT1_A1(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
					// DXT3-compressed texture.
					img = ImageDecoder::fromDXT3(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_RGBA_DXT5_S3TC:
				case GL_RGBA4_DXT5_S3TC:
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
					// DXT5-compressed texture.
					img = ImageDecoder::fromDXT5(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_ETC1_RGB8_OES:
					// ETC1-compressed texture.
					img = ImageDecoder::fromETC1(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RGB8_ETC2:
				case GL_COMPRESSED_SRGB8_ETC2:
					// ETC2-compressed RGB texture.
					// TODO: Handle sRGB.
					img = ImageDecoder::fromETC2_RGB(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
					// ETC2-compressed RGB texture
					// with punchthrough alpha.
					// TODO: Handle sRGB.
					img = ImageDecoder::fromETC2_RGB_A1(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RGBA8_ETC2_EAC:
				case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
					// ETC2-compressed RGB texture
					// with EAC-compressed alpha channel.
					// TODO: Handle sRGB.
					img = ImageDecoder::fromETC2_RGBA(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_R11_EAC:
				case GL_COMPRESSED_SIGNED_R11_EAC:
					// EAC-compressed R11 texture.
					// TODO: Does the signed version get decoded differently?
					img = ImageDecoder::fromEAC_R11(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RG11_EAC:
				case GL_COMPRESSED_SIGNED_RG11_EAC:
					// EAC-compressed RG11 texture.
					// TODO: Does the signed version get decoded differently?
					img = ImageDecoder::fromEAC_RG11(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RED_RGTC1:
				case GL_COMPRESSED_SIGNED_RED_RGTC1:
					// RGTC, one component. (BC4)
					// TODO: Handle signed properly.
					img = ImageDecoder::fromBC4(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_RG_RGTC2:
				case GL_COMPRESSED_SIGNED_RG_RGTC2:
					// RGTC, two components. (BC5)
					// TODO: Handle signed properly.
					img = ImageDecoder::fromBC5(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

				case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
				case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
					// LATC, one component. (BC4)
					// TODO: Handle signed properly.
					img = ImageDecoder::fromBC4(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					// TODO: If this fails, return it anyway or return nullptr?
					ImageDecoder::fromRed8ToL8(img);
					break;

				case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
				case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
					// LATC, two components. (BC5)
					// TODO: Handle signed properly.
					img = ImageDecoder::fromBC5(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					// TODO: If this fails, return it anyway or return nullptr?
					ImageDecoder::fromRG8ToLA8(img);
					break;

				case GL_COMPRESSED_RGBA_BPTC_UNORM:
				case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
					// BPTC-compressed RGBA texture. (BC7)
					img = ImageDecoder::fromBC7(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size);
					break;

#ifdef ENABLE_PVRTC
				case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
					// PVRTC, 2bpp, no alpha.
					img = ImageDecoder::fromPVRTC(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_NONE);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
					// PVRTC, 2bpp, has alpha.
					img = ImageDecoder::fromPVRTC(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
					break;

				case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
					// PVRTC, 4bpp, no alpha.
					img = ImageDecoder::fromPVRTC(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_NONE);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
					// PVRTC, 4bpp, has alpha.
					img = ImageDecoder::fromPVRTC(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
					// PVRTC-II, 2bpp.
					// NOTE: Assuming this has alpha.
					img = ImageDecoder::fromPVRTCII(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
					break;

				case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
					// PVRTC-II, 4bpp.
					// NOTE: Assuming this has alpha.
					img = ImageDecoder::fromPVRTCII(ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
					break;
#endif /* ENABLE_PVRTC */

				case GL_RGB9_E5:
					// Uncompressed "special" 32bpp formats.
					// TODO: Does KTX handle GL_RGB9_E5 as compressed?
					img = ImageDecoder::fromLinear32(
						ImageDecoder::PixelFormat::RGB9_E5,
						ktxHeader.pixelWidth, height,
						reinterpret_cast<const uint32_t*>(buf.get()), expected_size);
					break;

				default: {
#ifdef ENABLE_ASTC
					unsigned int astc_idx;
					if (ktxHeader.glInternalFormat >= GL_COMPRESSED_RGBA_ASTC_4x4_KHR &&
					    ktxHeader.glInternalFormat <= GL_COMPRESSED_RGBA_ASTC_12x12_KHR)
					{
						static_assert((GL_COMPRESSED_RGBA_ASTC_12x12_KHR - GL_COMPRESSED_RGBA_ASTC_4x4_KHR) + 1 ==
							ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
						astc_idx = ktxHeader.glInternalFormat - GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
					}
					else if (ktxHeader.glInternalFormat >= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR &&
					         ktxHeader.glInternalFormat <= GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR)
					{
						static_assert((GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR - GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR) + 1 ==
							ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
						astc_idx = ktxHeader.glInternalFormat - GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
					} else {
						// Not supported.
						break;
					}

					// TODO: sRGB handling?
					img = ImageDecoder::fromASTC(
						ktxHeader.pixelWidth, height,
						buf.get(), expected_size,
						ImageDecoder::astc_lkup_tbl[astc_idx][0],
						ImageDecoder::astc_lkup_tbl[astc_idx][1]);
#endif /* ENABLE_ASTC */
					break;
				}
			}
			break;
	}

	// Post-processing: Check if a flip is needed.
	if (img && flipOp != rp_image::FLIP_NONE) {
		rp_image *const flipimg = img->flip(flipOp);
		if (flipimg) {
			img->unref();
			img = flipimg;
		}
	}

	return img;
}

/**
 * Load key/value data.
 */
void KhronosKTXPrivate::loadKeyValueData(void)
{
	if (!kv_data.empty()) {
		// Key/value data is already loaded.
		return;
	} else if (ktxHeader.bytesOfKeyValueData < 5) {
		// No key/value data is present, or
		// there isn't enough data to be valid.
		return;
	} else if (ktxHeader.bytesOfKeyValueData > 512*1024) {
		// Sanity check: More than 512 KB is usually wrong.
		return;
	}

	// Load the data.
	unique_ptr<char[]> buf(new char[ktxHeader.bytesOfKeyValueData]);
	size_t size = file->seekAndRead(sizeof(ktxHeader), buf.get(), ktxHeader.bytesOfKeyValueData);
	if (size != ktxHeader.bytesOfKeyValueData) {
		// Seek and/or read error.
		return;
	}

	// Key/value data format:
	// - uint32_t: keyAndValueByteSize
	// - Byte: keyAndValue[keyAndValueByteSize] (UTF-8)
	// - Byte: valuePadding (4-byte alignment)
	const char *p = buf.get();
	const char *const p_end = p + ktxHeader.bytesOfKeyValueData;
	bool hasKTXorientation = false;

	while (p < p_end-3) {
		// Check the next key/value size.
		uint32_t sz = *((const uint32_t*)p);
		if (isByteswapNeeded) {
			sz = __swab32(sz);
		}

		if (sz < 2) {
			// Must be at least 2 bytes for an empty key and its NULL terminator.
			// TODO: Show an error?
			break;
		} else if (p + 4 + sz > p_end) {
			// Out of range.
			// TODO: Show an error?
			break;
		}

		p += 4;

		// keyAndValue consists of two sections:
		// - key: UTF-8 string terminated by a NUL byte.
		// - value: Arbitrary data terminated by a NUL byte. (usually UTF-8)

		// kv_end: Points past the end of the string.
		const char *const kv_end = p + sz;

		// Find the key.
		const char *const k_end = static_cast<const char*>(memchr(p, 0, kv_end - p));
		if (!k_end) {
			// NUL byte not found.
			// TODO: Show an error?
			break;
		}

		// Make sure the value ends at kv_end - 1.
		const char *const v_end = static_cast<const char*>(memchr(k_end + 1, 0, kv_end - k_end - 1));
		if (v_end != kv_end - 1) {
			// Either the NUL byte was not found,
			// or it's not at the end of the value.
			// TODO: Show an error?
			break;
		}

		vector<string> data_row;
		data_row.reserve(2);
		data_row.emplace_back(p, k_end - p);
		data_row.emplace_back(k_end + 1, kv_end - k_end - 2);
		kv_data.emplace_back(std::move(data_row));

		// Check if this is KTXorientation.
		// NOTE: Only the first instance is used.
		// NOTE 2: Specification says it's case-sensitive, but some files
		// have "KTXOrientation", so use a case-insensitive comparison.
		if (!hasKTXorientation && !strcasecmp(p, "KTXorientation")) {
			hasKTXorientation = true;
			// Check for known values.
			// NOTE: Ignoring the R component.
			// NOTE: str[7] does NOT have a NULL terminator.
			const char *const v = k_end + 1;

			static const struct {
				char str[7];
				rp_image::FlipOp flipOp;
			} orientation_lkup_tbl[] = {
				{{'S','=','r',',','T','=','d'}, rp_image::FLIP_NONE},
				{{'S','=','r',',','T','=','u'}, rp_image::FLIP_V},
				{{'S','=','l',',','T','=','d'}, rp_image::FLIP_H},
				{{'S','=','l',',','T','=','u'}, rp_image::FLIP_VH},
			};

			for (const auto &p : orientation_lkup_tbl) {
				if (!strncmp(v, p.str, 7)) {
					// Found a match.
					flipOp = p.flipOp;
					break;
				}
			}
		}

		// Next key/value pair.
		p += ALIGN_BYTES(4, sz);
	}
}

/** KhronosKTX **/

/**
 * Read a Khronos KTX image file.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
KhronosKTX::KhronosKTX(IRpFile *file)
	: super(new KhronosKTXPrivate(this, file))
{
	RP_D(KhronosKTX);
	d->mimeType = "image/ktx";	// official

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the KTX header.
	d->file->rewind();
	size_t size = d->file->read(&d->ktxHeader, sizeof(d->ktxHeader));
	if (size != sizeof(d->ktxHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this KTX texture is supported.
	const DetectInfo info = {
		{0, sizeof(d->ktxHeader), reinterpret_cast<const uint8_t*>(&d->ktxHeader)},
		nullptr,	// ext (not needed for KhronosKTX)
		file->size()	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if the header needs to be byteswapped.
	if (d->ktxHeader.endianness != KTX_ENDIAN_MAGIC) {
		// Byteswapping is required.
		// NOTE: Keeping `endianness` unswapped in case
		// the actual image data needs to be byteswapped.
		d->ktxHeader.glType			= __swab32(d->ktxHeader.glType);
		d->ktxHeader.glTypeSize			= __swab32(d->ktxHeader.glTypeSize);
		d->ktxHeader.glFormat			= __swab32(d->ktxHeader.glFormat);
		d->ktxHeader.glInternalFormat		= __swab32(d->ktxHeader.glInternalFormat);
		d->ktxHeader.glBaseInternalFormat	= __swab32(d->ktxHeader.glBaseInternalFormat);
		d->ktxHeader.pixelWidth			= __swab32(d->ktxHeader.pixelWidth);
		d->ktxHeader.pixelHeight		= __swab32(d->ktxHeader.pixelHeight);
		d->ktxHeader.pixelDepth			= __swab32(d->ktxHeader.pixelDepth);
		d->ktxHeader.numberOfArrayElements	= __swab32(d->ktxHeader.numberOfArrayElements);
		d->ktxHeader.numberOfFaces		= __swab32(d->ktxHeader.numberOfFaces);
		d->ktxHeader.numberOfMipmapLevels	= __swab32(d->ktxHeader.numberOfMipmapLevels);
		d->ktxHeader.bytesOfKeyValueData	= __swab32(d->ktxHeader.bytesOfKeyValueData);

		// Convenience flag.
		d->isByteswapNeeded = true;
	}

	// Texture data start address.
	// NOTE: Always 4-byte aligned.
	d->texDataStartAddr = ALIGN_BYTES(4, sizeof(d->ktxHeader) + d->ktxHeader.bytesOfKeyValueData);

	// Load key/value data.
	// This function also checks for KTXorientation
	// and sets the HFlip/VFlip values as necessary.
	d->loadKeyValueData();

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->ktxHeader.pixelWidth;
	d->dimensions[1] = d->ktxHeader.pixelHeight;
	if (d->ktxHeader.pixelDepth > 1) {
		d->dimensions[2] = d->ktxHeader.pixelDepth;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int KhronosKTX::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(KTX_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the KTX magic.
	const KTX_Header *const ktxHeader =
		reinterpret_cast<const KTX_Header*>(info->header.pData);
	if (!memcmp(ktxHeader->identifier, KTX_IDENTIFIER, sizeof(KTX_IDENTIFIER)-1)) {
		// KTX magic is present.
		// Check the endianness value.
		if (ktxHeader->endianness == KTX_ENDIAN_MAGIC ||
		    ktxHeader->endianness == __swab32(KTX_ENDIAN_MAGIC))
		{
			// Endianness value is either correct for this architecture
			// or correct for byteswapped.
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *KhronosKTX::textureFormatName(void) const
{
	RP_D(const KhronosKTX);
	if (!d->isValid)
		return nullptr;

	return "Khronos KTX";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *KhronosKTX::pixelFormat(void) const
{
	RP_D(const KhronosKTX);
	if (!d->isValid)
		return nullptr;

	// Using glInternalFormat.
	const char *const glInternalFormat_str = GLenumStrings::lookup_glEnum(d->ktxHeader.glInternalFormat);
	if (glInternalFormat_str) {
		return glInternalFormat_str;
	}

	// Invalid pixel format.
	if (d->invalid_pixel_format[0] == '\0') {
		// TODO: Localization?
		snprintf(const_cast<KhronosKTXPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (0x%04X)", d->ktxHeader.glInternalFormat);
	}
	return d->invalid_pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int KhronosKTX::mipmapCount(void) const
{
	RP_D(const KhronosKTX);
	if (!d->isValid)
		return -1;

	return d->ktxHeader.numberOfMipmapLevels;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int KhronosKTX::getFields(LibRpBase::RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(KhronosKTX);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 8);	// Maximum of 8 fields.

	// KTX header.
	const KTX_Header *const ktxHeader = &d->ktxHeader;

	// Endianness.
	// TODO: Save big vs. little in the constructor instead of just "needs byteswapping"?
	const char *endian_str;
	if (ktxHeader->endianness == KTX_ENDIAN_MAGIC) {
		// Matches host-endian.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		endian_str = C_("FileFormat", "Little-Endian");
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		endian_str = C_("FileFormat", "Big-Endian");
#endif
	} else {
		// Does not match host-endian.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		endian_str = C_("FileFormat", "Big-Endian");
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		endian_str = C_("FileFormat", "Little-Endian");
#endif
	}
	fields->addField_string(C_("FileFormat", "Endianness"), endian_str);

	// NOTE: GL field names should not be localized.

	// glType
	const char *const glType_str = GLenumStrings::lookup_glEnum(ktxHeader->glType);
	if (glType_str) {
		fields->addField_string("glType", glType_str);
	} else {
		fields->addField_string_numeric("glType", ktxHeader->glType, RomFields::Base::Hex);
	}

	// glFormat
	const char *const glFormat_str = GLenumStrings::lookup_glEnum(ktxHeader->glFormat);
	if (glFormat_str) {
		fields->addField_string("glFormat", glFormat_str);
	} else {
		fields->addField_string_numeric("glFormat", ktxHeader->glFormat, RomFields::Base::Hex);
	}

	// glInternalFormat
	const char *const glInternalFormat_str = GLenumStrings::lookup_glEnum(ktxHeader->glInternalFormat);
	if (glInternalFormat_str) {
		fields->addField_string("glInternalFormat", glInternalFormat_str);
	} else {
		fields->addField_string_numeric("glInternalFormat",
			ktxHeader->glInternalFormat, RomFields::Base::Hex);
	}

	// glBaseInternalFormat (only if != glFormat)
	if (ktxHeader->glBaseInternalFormat != ktxHeader->glFormat) {
		const char *const glBaseInternalFormat_str =
			GLenumStrings::lookup_glEnum(ktxHeader->glBaseInternalFormat);
		if (glBaseInternalFormat_str) {
			fields->addField_string("glBaseInternalFormat", glBaseInternalFormat_str);
		} else {
			fields->addField_string_numeric("glBaseInternalFormat",
				ktxHeader->glBaseInternalFormat, RomFields::Base::Hex);
		}
	}

	// # of array elements (for texture arrays)
	if (ktxHeader->numberOfArrayElements > 0) {
		fields->addField_string_numeric(C_("KhronosKTX", "# of Array Elements"),
			ktxHeader->numberOfArrayElements);
	}

	// # of faces (for cubemaps)
	if (ktxHeader->numberOfFaces > 1) {
		fields->addField_string_numeric(C_("KhronosKTX", "# of Faces"),
			ktxHeader->numberOfFaces);
	}

	// Key/Value data.
	d->loadKeyValueData();
	if (!d->kv_data.empty()) {
		static const char *const kv_field_names[] = {
			NOP_C_("KhronosKTX|KeyValue", "Key"),
			NOP_C_("KhronosKTX|KeyValue", "Value"),
		};

		// NOTE: Making a copy.
		RomFields::ListData_t *const p_kv_data = new RomFields::ListData_t(d->kv_data);
		vector<string> *const v_kv_field_names = RomFields::strArrayToVector_i18n(
			"KhronosKTX|KeyValue", kv_field_names, ARRAY_SIZE(kv_field_names));

		RomFields::AFLD_PARAMS params;
		params.headers = v_kv_field_names;
		params.data.single = p_kv_data;
		fields->addField_listData(C_("KhronosKTX", "Key/Value Data"), &params);
	}

	// Finished reading the field data.
	return (fields->count() - initial_count);
}
#endif /* ENABLE_LIBRPBASE_ROMFIELDS */

/** Image accessors **/

/**
 * Get the image.
 * For textures with mipmaps, this is the largest mipmap.
 * The image is owned by this object.
 * @return Image, or nullptr on error.
 */
const rp_image *KhronosKTX::image(void) const
{
	// The full image is mipmap 0.
	return this->mipmap(0);
}

/**
 * Get the image for the specified mipmap.
 * Mipmap 0 is the largest image.
 * @param mip Mipmap number.
 * @return Image, or nullptr on error.
 */
const rp_image *KhronosKTX::mipmap(int mip) const
{
	RP_D(const KhronosKTX);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// FIXME: Support decoding mipmaps.
	if (mip == 0) {
		return const_cast<KhronosKTXPrivate*>(d)->loadImage();
	}
	return nullptr;
}

}

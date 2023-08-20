/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * KhronosKTX2.cpp: Khronos KTX2 image reader.                             *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://github.khronos.org/KTX-Specification/
 * - https://github.com/KhronosGroup/KTX-Specification
 */

#include "stdafx.h"
#include "config.librptexture.h"

#include "KhronosKTX2.hpp"
#include "FileFormat_p.hpp"

#include "ktx2_structs.h"
#include "vk_defs.h"
#include "data/VkEnumStrings.hpp"

// Other rom-properties libraries
#include "libi18n/i18n.h"
using namespace LibRpFile;
using LibRpBase::RomFields;
using LibRpText::rp_sprintf;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"
#include "decoder/ImageDecoder_ETC1.hpp"
#include "decoder/ImageDecoder_BC7.hpp"
#include "decoder/ImageDecoder_PVRTC.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::vector;

// Uninitialized vector class.
// Reference: http://andreoffringa.org/?q=uvector
#include "uvector.h"

namespace LibRpTexture {

class KhronosKTX2Private final : public FileFormatPrivate
{
	public:
		KhronosKTX2Private(KhronosKTX2 *q, const IRpFilePtr &file);
		~KhronosKTX2Private() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(KhronosKTX2Private)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// KTX2 header
		KTX2_Header ktx2Header;

		// Is HFlip/VFlip needed?
		// Some textures may be stored upside-down due to
		// the way GL texture coordinates are interpreted.
		// Default without KTXorientation is HFlip=false, VFlip=true
		rp_image::FlipOp flipOp;

		// Mipmap offsets
		ao::uvector<KTX2_Mipmap_Index> mipmap_data;

		// Decoded mipmaps
		// Mipmap 0 is the full image.
		vector<rp_image_ptr > mipmaps;

		// Invalid pixel format message
		char invalid_pixel_format[24];

		// Key/Value data
		// NOTE: Stored as vector<vector<string> > instead of
		// vector<pair<string, string> > for compatibility with
		// RFT_LISTDATA.
		RomFields::ListData_t kv_data;

		// KTXswizzle, if specified.
		// Four bytes indicate the values of each channel.
		// Each byte can be: [rgba01], where rgba corresponds
		// to each channel, 0 is 0, and 1 is 1.
		// If byte 0 is a literal \0, no KTXswizzle tag was found.
		char ktx_swizzle[4];

		/**
		 * Load the image.
		 * @param mip Mipmap number. (0 == full image)
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(int mip);

		/**
		 * Load key/value data.
		 */
		void loadKeyValueData(void);
};

FILEFORMAT_IMPL(KhronosKTX2)

/** KhronosKTX2Private **/

/* TextureInfo */
const char *const KhronosKTX2Private::exts[] = {
	// TODO: Include ".ktx" too?
	".ktx2",

	nullptr
};
const char *const KhronosKTX2Private::mimeTypes[] = {
	// Official MIME types.
	"image/ktx2",

	nullptr
};
const TextureInfo KhronosKTX2Private::textureInfo = {
	exts, mimeTypes
};

KhronosKTX2Private::KhronosKTX2Private(KhronosKTX2 *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, flipOp(rp_image::FLIP_V)
{
	// Clear the KTX2 header struct.
	memset(&ktx2Header, 0, sizeof(ktx2Header));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
	memset(ktx_swizzle, 0, sizeof(ktx_swizzle));
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr KhronosKTX2Private::loadImage(int mip)
{
	int mipmapCount = ktx2Header.levelCount;
	if (mipmapCount <= 0) {
		// No mipmaps == one image.
		mipmapCount = 1;
	}

	assert(mip >= 0);
	assert(mip < mipmapCount);
	if (mip < 0 || mip >= mipmapCount) {
		// Invalid mipmap number.
		return nullptr;
	}

	if (!mipmaps.empty() && mipmaps[mip] != nullptr) {
		// Image has already been loaded.
		return mipmaps[mip];
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `pixelHeight == 0` is allowed here. (1D texture)
	assert(ktx2Header.pixelWidth > 0);
	assert(ktx2Header.pixelWidth <= 32768);
	assert(ktx2Header.pixelHeight <= 32768);
	if (ktx2Header.pixelWidth == 0 || ktx2Header.pixelWidth > 32768 ||
	    ktx2Header.pixelHeight > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// TODO: Support supercompression.
	if (ktx2Header.supercompressionScheme != 0) {
		return nullptr;
	}

	// TODO: For VK_FORMAT_UNDEFINED, parse the DFD.
	if (ktx2Header.vkFormat == VK_FORMAT_UNDEFINED) {
		return nullptr;
	}

	// Get the mipmap info.
	assert(mip < (int)mipmap_data.size());
	if (mip >= (int)mipmap_data.size()) {
		// Not enough mipmap data loaded...
		return nullptr;
	}
	const auto &mipinfo = mipmap_data.at(mip);

	// Adjust width/height for the mipmap level.
	int width = ktx2Header.pixelWidth;
	int height = ktx2Header.pixelHeight;
	if (mip > 0) {
		width >>= mip;
		height >>= mip;
	}

	// NOTE: If this is a 1D texture, height might be 0.
	// This might also happen on certain mipmap levels.
	// TODO: Make sure we have the correct minimum block size.
	if (width <= 0) width = 1;
	if (height <= 0) height = 1;

	// Texture cannot start inside of the KTX2 header.
	assert(mipinfo.byteOffset >= sizeof(ktx2Header));
	if (mipinfo.byteOffset < sizeof(ktx2Header)) {
		// Invalid texture data start address.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: KTX files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Seek to the start of the texture data.
	int ret = file->seek(mipinfo.byteOffset);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Calculate the expected size.
	// NOTE: Scanlines are 4-byte aligned.
	// TODO: Differences between UNORM, UINT, SRGB; handle SNORM, SINT.
	size_t expected_size;
	int stride = 0;
	switch (ktx2Header.vkFormat) {
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SRGB:
			// 24-bit RGB
			stride = ALIGN_BYTES(4, width * 3);
			expected_size = ImageSizeCalc::T_calcImageSize(stride, height);
			break;

		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
			// 32-bit RGBA
			stride = width * 4;
			expected_size = ImageSizeCalc::T_calcImageSize(stride, height);
			break;

		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SRGB:
			// 8-bit (red)
			stride = ALIGN_BYTES(4, width);
			expected_size = ImageSizeCalc::T_calcImageSize(stride, height);
			break;

		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SRGB:
			// 16-bit (red/green; may also be luminance/alpha)
			stride = ALIGN_BYTES(4, width * 2);
			expected_size = ImageSizeCalc::T_calcImageSize(stride, height);
			break;

		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			// Uncompressed "special" 32bpp formats.
			stride = width * 4;
			expected_size = ImageSizeCalc::T_calcImageSize(stride, height);
			break;

		// Compressed formats.
		// NOTE: These were handled separately in KTX1 due to OpenGL
		// differentiating between "format" and "internal format".

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			// 16 pixels compressed into 64 bits. (4bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (4x4)
			expected_size = ImageSizeCalc::T_calcImageSize(
				ALIGN_BYTES(4, width), ALIGN_BYTES(4, height) / 2);
			break;

		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			// 16 pixels compressed into 128 bits. (8bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (4x4)
			expected_size = ImageSizeCalc::T_calcImageSize(
				ALIGN_BYTES(4, width), ALIGN_BYTES(4, height));
			break;

#ifdef ENABLE_PVRTC
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
			// 32 pixels compressed into 64 bits. (2bpp)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(width, height);
			break;

		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			// 32 pixels compressed into 64 bits. (2bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (8x4)
			// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
			//expected_size = ALIGN_BYTES(8, width) *
			//                ALIGN_BYTES(4, height) / 4;
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<true>(width, height);
			break;

		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
			// 16 pixels compressed into 64 bits. (4bpp)
			// NOTE: Image dimensions must be a power of 2 for PVRTC-I.
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(width, height);
			break;

		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
			// 16 pixels compressed into 64 bits. (4bpp)
			// NOTE: Width and height must be rounded to the nearest tile. (8x4)
			// FIXME: Our PVRTC-II decoder requires power-of-2 textures right now.
			//expected_size = ALIGN_BYTES(4, width) *
			//                ALIGN_BYTES(4, height) / 2;
			expected_size = ImageSizeCalc::T_calcImageSizePVRTC_PoT<false>(width, height);
			break;
#endif /* ENABLE_PVRTC */

		default:
#ifdef ENABLE_ASTC
			if (ktx2Header.vkFormat >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK &&
			    ktx2Header.vkFormat <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
			{
				static_assert(((VK_FORMAT_ASTC_12x12_UNORM_BLOCK - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) / 2) + 1 ==
					ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
				const unsigned int astc_idx = (ktx2Header.vkFormat - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) / 2;
				expected_size = ImageSizeCalc::calcImageSizeASTC(width, height,
					ImageDecoder::astc_lkup_tbl[astc_idx][0],
					ImageDecoder::astc_lkup_tbl[astc_idx][1]);
				break;
			}
#endif /* ENABLE_ASTC */

			// Not supported.
			return nullptr;
	}

	// Verify mipmap size.
	if (mipinfo.byteLength < expected_size) {
		// Mipmap level is too small.
		// TODO: Should we require the exact size?
		return nullptr;
	}

	// Verify file size.
	if (mipinfo.byteOffset + expected_size > file_sz) {
		// File is too small.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	// TODO: Handle sRGB post-processing? (for e.g. GL_SRGB8)
	rp_image_ptr img;
	switch (ktx2Header.vkFormat) {
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SRGB:
			// 24-bit RGB
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::BGR888, width, height,
				buf.get(), expected_size, stride);
			break;

		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SRGB:
			// 24-bit RGB (R/B swapped)
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::RGB888, width, height,
				buf.get(), expected_size, stride);
			break;

		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
			// 32-bit RGBA
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::ABGR8888, width, height,
				reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
			break;

		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
			// 32-bit RGBA (R/B swapped)
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::ARGB8888, width, height,
				reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
			break;

		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SRGB:
			// 8-bit (red)
			img = ImageDecoder::fromLinear8(
				ImageDecoder::PixelFormat::R8, width, height,
				buf.get(), expected_size, stride);
			break;

		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SRGB:
			// 16-bit (red/green; may also be luminance/alpha)
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::GR88, width, height,
				reinterpret_cast<const uint16_t*>(buf.get()), expected_size, stride);
			break;

		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			// Uncompressed "special" 32bpp formats.
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::RGB9_E5, width, height,
				reinterpret_cast<const uint32_t*>(buf.get()), expected_size, stride);
			break;

		// Compressed formats.
		// NOTE: These were handled separately in KTX1 due to OpenGL
		// differentiating between "format" and "internal format".

		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			// DXT1-compressed texture.
			img = ImageDecoder::fromDXT1(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
			// DXT1-compressed texture with 1-bit alpha.
			img = ImageDecoder::fromDXT1_A1(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_BC2_UNORM_BLOCK:
		case VK_FORMAT_BC2_SRGB_BLOCK:
			// DXT3-compressed texture.
			img = ImageDecoder::fromDXT3(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
			// DXT5-compressed texture.
			img = ImageDecoder::fromDXT5(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
			// ETC2-compressed RGB texture.
			// TODO: Handle sRGB.
			img = ImageDecoder::fromETC2_RGB(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
			// ETC2-compressed RGB texture
			// with punchthrough alpha.
			// TODO: Handle sRGB.
			img = ImageDecoder::fromETC2_RGB_A1(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
			// ETC2-compressed RGB texture
			// with EAC-compressed alpha channel.
			// TODO: Handle sRGB.
			img = ImageDecoder::fromETC2_RGBA(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			// EAC-compressed R11 texture.
			// TODO: Does the signed version get decoded differently?
			img = ImageDecoder::fromEAC_R11(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			// EAC-compressed RG11 texture.
			// TODO: Does the signed version get decoded differently?
			img = ImageDecoder::fromEAC_RG11(
				width, height,
				buf.get(), expected_size);
			break;

		case VK_FORMAT_BC7_UNORM_BLOCK:
		case VK_FORMAT_BC7_SRGB_BLOCK:
			// BPTC-compressed RGBA texture. (BC7)
			img = ImageDecoder::fromBC7(
				width, height,
				buf.get(), expected_size);
			break;

#ifdef ENABLE_PVRTC
		// NOTE: KTX2 doesn't have a way to specify "no alpha" for PVRTC.
		// We'll assume all PVRTC KTX2 textures have alpha.

		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
			// PVRTC, 2bpp.
			img = ImageDecoder::fromPVRTC(width, height,
				buf.get(), expected_size,
				ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;

		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
			// PVRTC, 4bpp.
			img = ImageDecoder::fromPVRTC(width, height,
				buf.get(), expected_size,
				ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;

		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
			// PVRTC-II, 2bpp.
			img = ImageDecoder::fromPVRTCII(width, height,
				buf.get(), expected_size,
				ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;

		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
			// PVRTC-II, 4bpp.
			// NOTE: Assuming this has alpha.
			img = ImageDecoder::fromPVRTCII(width, height,
				buf.get(), expected_size,
				ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;
#endif /* ENABLE_PVRTC */

		default:
#ifdef ENABLE_ASTC
			if (ktx2Header.vkFormat >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK &&
			    ktx2Header.vkFormat <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
			{
				// TODO: sRGB handling?
				static_assert(((VK_FORMAT_ASTC_12x12_UNORM_BLOCK - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) / 2) + 1 ==
					ARRAY_SIZE(ImageDecoder::astc_lkup_tbl), "ASTC lookup table size is wrong!");
				const unsigned int astc_idx = (ktx2Header.vkFormat - VK_FORMAT_ASTC_4x4_UNORM_BLOCK) / 2;
				img = ImageDecoder::fromASTC(width, height,
					buf.get(), expected_size,
					ImageDecoder::astc_lkup_tbl[astc_idx][0],
					ImageDecoder::astc_lkup_tbl[astc_idx][1]);
				break;
			}
#endif /* ENABLE_ASTC */

			// Not supported.
			break;
	}

	// Post-processing
	if (img) {
		// Check if a flip is needed.
		if (flipOp != rp_image::FLIP_NONE) {
			// TODO: Assert that img dimensions match ktx2Header?
			const rp_image_ptr flipimg = img->flip(flipOp);
			if (flipimg) {
				img = flipimg;
			}
		}

		// Check if swizzling is needed.
		if (this->ktx_swizzle[0] != '\0') {
			// Swizzle is needed.
			img->swizzle(this->ktx_swizzle);
		}
	}

	mipmaps[mip] = img;
	return img;
}

/**
 * Load key/value data.
 */
void KhronosKTX2Private::loadKeyValueData(void)
{
	if (!kv_data.empty()) {
		// Key/value data is already loaded.
		return;
	} else if (ktx2Header.kvdByteOffset <= sizeof(ktx2Header)) {
		// Offset is within the KTX2 header.
		return;
	} else if (ktx2Header.kvdByteLength == 0) {
		// No key/value data is present.
		return;
	} else if (ktx2Header.kvdByteLength > 512*1024) {
		// Sanity check: More than 512 KB is usually wrong.
		return;
	}

	// Load the data.
	unique_ptr<char[]> buf(new char[ktx2Header.kvdByteLength]);
	size_t size = file->seekAndRead(ktx2Header.kvdByteOffset, buf.get(), ktx2Header.kvdByteLength);
	if (size != ktx2Header.kvdByteLength) {
		// Seek and/or read error.
		return;
	}

	// Key/value data format:
	// - uint32_t: keyAndValueByteLength
	// - Byte: keyAndValue[keyAndValueByteLength] (UTF-8)
	// - Byte: valuePadding (4-byte alignment)
	const char *p = buf.get();
	const char *const p_end = p + ktx2Header.kvdByteLength;
	bool hasKTXorientation = false;
	bool hasKTXswizzle = false;

	while (p < p_end-3) {
		// Check the next key/value size.
		const uint32_t sz = le32_to_cpu(*((const uint32_t*)p));
		if (sz < 2) {
			// Must be at least 2 bytes for an empty key and its NULL terminator.
			// TODO: Show an error?
			break;
		} else if ((p_end - p) < 4+static_cast<intptr_t>(sz)) {
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
			// For KTX2, this field has one character per dimension.
			// - X: rl
			// - Y: du
			// - Z: oi
			const char *const v = k_end + 1;
			flipOp = rp_image::FLIP_NONE;

			if (v[0] == 'l') {
				flipOp = rp_image::FLIP_H;
			}
			if (v[0] != '\0' && v[1] == 'u') {
				flipOp = static_cast<rp_image::FlipOp>(flipOp | rp_image::FLIP_V);
			}
		}

		// Check if this is KTXswizzle.
		// NOTE: Only the first instance is used.
		if (!hasKTXswizzle && !strcasecmp(p, "KTXswizzle")) {
			// Value must match the regex [rgba01]{4}.
			const char *const v = k_end + 1;
			bool isOK = true;
			for (unsigned int i = 0; isOK && i < 4; i++) {
				switch (v[i]) {
					case '\0':
					default:
						// Invalid character!
						isOK = false;
						break;
					case 'r': case 'g': case 'b': case 'a':
					case '0': case '1':
						break;
				}
			}

			if (isOK) {
				// Make sure the fifth byte *is* NULL.
				if (v[4] != '\0') {
					// Invalid character!
					isOK = false;
				}
			}

			if (isOK) {
				memcpy(this->ktx_swizzle, v, 4);
				hasKTXswizzle = true;
			}
		}

		// Next key/value pair.
		p += ALIGN_BYTES(4, (uint32_t)sz);
	}
}

/** KhronosKTX2 **/

/**
 * Read a Khronos KTX2 image file.
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
KhronosKTX2::KhronosKTX2(const IRpFilePtr &file)
	: super(new KhronosKTX2Private(this, file))
{
	RP_D(KhronosKTX2);
	d->mimeType = "image/ktx2";	// official

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the KTX2 header.
	d->file->rewind();
	size_t size = d->file->read(&d->ktx2Header, sizeof(d->ktx2Header));
	if (size != sizeof(d->ktx2Header)) {
		d->file.reset();
		return;
	}

	// Check if this KTX2 texture is supported.
	const DetectInfo info = {
		{0, sizeof(d->ktx2Header), reinterpret_cast<const uint8_t*>(&d->ktx2Header)},
		nullptr,	// ext (not needed for KhronosKTX2)
		file->size()	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Byteswap the header.
	d->ktx2Header.vkFormat			= le32_to_cpu(d->ktx2Header.vkFormat);
	d->ktx2Header.typeSize			= le32_to_cpu(d->ktx2Header.typeSize);
	d->ktx2Header.pixelWidth		= le32_to_cpu(d->ktx2Header.pixelWidth);
	d->ktx2Header.pixelHeight		= le32_to_cpu(d->ktx2Header.pixelHeight);
	d->ktx2Header.pixelDepth		= le32_to_cpu(d->ktx2Header.pixelDepth);
	d->ktx2Header.layerCount		= le32_to_cpu(d->ktx2Header.layerCount);
	d->ktx2Header.faceCount			= le32_to_cpu(d->ktx2Header.faceCount);
	d->ktx2Header.levelCount		= le32_to_cpu(d->ktx2Header.levelCount);
	d->ktx2Header.supercompressionScheme	= le32_to_cpu(d->ktx2Header.supercompressionScheme);

	d->ktx2Header.dfdByteOffset		= le32_to_cpu(d->ktx2Header.dfdByteOffset);
	d->ktx2Header.dfdByteLength		= le32_to_cpu(d->ktx2Header.dfdByteLength);
	d->ktx2Header.kvdByteOffset		= le32_to_cpu(d->ktx2Header.kvdByteOffset);
	d->ktx2Header.kvdByteLength		= le32_to_cpu(d->ktx2Header.kvdByteLength);
	d->ktx2Header.sgdByteOffset		= le32_to_cpu(d->ktx2Header.sgdByteOffset);
	d->ktx2Header.sgdByteLength		= le32_to_cpu(d->ktx2Header.sgdByteLength);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Read the mipmap info.
	int mipmapCount = d->ktx2Header.levelCount;
	if (mipmapCount <= 0) {
		// No mipmaps == one image.
		mipmapCount = 1;
	} else if (mipmapCount > 128) {
		// Too many mipmaps.
		d->isValid = false;
		d->file.reset();
		return;
	}
	d->mipmaps.resize(mipmapCount);
	d->mipmap_data.resize(mipmapCount);
	const size_t mipdata_size = mipmapCount * sizeof(KTX2_Mipmap_Index);
	size = d->file->read(d->mipmap_data.data(), mipdata_size);
	if (size != mipdata_size) {
		d->isValid = false;
		d->file.reset();
		return;
	}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	for (KTX2_Mipmap_Index &mipdata : d->mipmap_data) {
		mipdata.byteOffset = le64_to_cpu(mipdata.byteOffset);
		mipdata.byteLength = le64_to_cpu(mipdata.byteLength);
		mipdata.uncompressedByteLength = le64_to_cpu(mipdata.uncompressedByteLength);
	}
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Load key/value data.
	// This function also checks for KTXorientation
	// and sets the HFlip/VFlip values as necessary.
	d->loadKeyValueData();

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->ktx2Header.pixelWidth;
	d->dimensions[1] = d->ktx2Header.pixelHeight;
	if (d->ktx2Header.pixelDepth > 1) {
		d->dimensions[2] = d->ktx2Header.pixelDepth;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int KhronosKTX2::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(KTX2_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Verify the KTX2 magic.
	const KTX2_Header *const ktx2Header =
		reinterpret_cast<const KTX2_Header*>(info->header.pData);
	if (!memcmp(ktx2Header->identifier, KTX2_IDENTIFIER, sizeof(KTX2_IDENTIFIER)-1)) {
		// KTX magic is present.
		return 0;
	}

	// Not supported.
	return -1;
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *KhronosKTX2::textureFormatName(void) const
{
	RP_D(const KhronosKTX2);
	if (!d->isValid)
		return nullptr;

	return "Khronos KTX2";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *KhronosKTX2::pixelFormat(void) const
{
	RP_D(const KhronosKTX2);
	if (!d->isValid)
		return nullptr;

	// Using vkFormat.
	const char *const vkFormat_str = VkEnumStrings::lookup_vkFormat(d->ktx2Header.vkFormat);
	if (vkFormat_str) {
		return vkFormat_str;
	}

	// Invalid pixel format.
	if (d->invalid_pixel_format[0] == '\0') {
		// TODO: Localization?
		snprintf(const_cast<KhronosKTX2Private*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (%u)", d->ktx2Header.vkFormat);
	}
	return d->invalid_pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int KhronosKTX2::mipmapCount(void) const
{
	RP_D(const KhronosKTX2);
	if (!d->isValid)
		return -1;

	return d->ktx2Header.levelCount;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int KhronosKTX2::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(KhronosKTX2);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 5);	// Maximum of 5 fields.

	// KTX2 header.
	const KTX2_Header *const ktx2Header = &d->ktx2Header;

	// Supercompression.
	static const char *const supercompression_tbl[] = {
		"None",			// TODO: Localize?
		"BasisLZ",
		"Zstandard",
		"ZLIB",
	};
	if (ktx2Header->supercompressionScheme < ARRAY_SIZE(supercompression_tbl)) {
		fields->addField_string(C_("KhronosKTX2", "Supercompression"),
			supercompression_tbl[ktx2Header->supercompressionScheme]);
	} else {
		fields->addField_string(C_("KhronosKTX2", "Supercompression"),
			rp_sprintf(C_("RomData", "Unknown (%u)"),
				ktx2Header->supercompressionScheme));
	}

	// NOTE: Vulkan field names should not be localized.

	// vkFormat
	const char *const vkFormat_str = VkEnumStrings::lookup_vkFormat(ktx2Header->vkFormat);
	if (vkFormat_str) {
		fields->addField_string("vkFormat", vkFormat_str);
	} else {
		fields->addField_string_numeric("vkFormat", ktx2Header->vkFormat, RomFields::Base::Hex);
	}

	// # of layers (for texture arrays)
	if (ktx2Header->layerCount > 1) {
		fields->addField_string_numeric(C_("KhronosKTX2", "# of Layers"),
			ktx2Header->layerCount);
	}

	// # of faces (for cubemaps)
	if (ktx2Header->faceCount > 1) {
		fields->addField_string_numeric(C_("KhronosKTX2", "# of Faces"),
			ktx2Header->faceCount);
	}

	// Key/Value data.
	d->loadKeyValueData();
	if (!d->kv_data.empty()) {
		static const char *const kv_field_names[] = {
			NOP_C_("KhronosKTX2|KeyValue", "Key"),
			NOP_C_("KhronosKTX2|KeyValue", "Value"),
		};

		// NOTE: Making a copy.
		RomFields::ListData_t *const p_kv_data = new RomFields::ListData_t(d->kv_data);
		vector<string> *const v_kv_field_names = RomFields::strArrayToVector_i18n(
			"KhronosKTX2|KeyValue", kv_field_names, ARRAY_SIZE(kv_field_names));

		RomFields::AFLD_PARAMS params;
		params.headers = v_kv_field_names;
		params.data.single = p_kv_data;
		fields->addField_listData(C_("KhronosKTX2", "Key/Value Data"), &params);
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
rp_image_const_ptr KhronosKTX2::image(void) const
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
rp_image_const_ptr KhronosKTX2::mipmap(int mip) const
{
	RP_D(const KhronosKTX2);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<KhronosKTX2Private*>(d)->loadImage(mip);
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * GodotSTEX.cpp: Godot STEX image reader.                                 *
 *                                                                         *
 * Copyright (c) 2017-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librptexture.h"

#include "GodotSTEX.hpp"
#include "FileFormat_p.hpp"

#include "godot_stex_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpbase/img/RpPng.hpp"
#include "librpfile/MemFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// librptexture
#include "ImageSizeCalc.hpp"
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder_Linear.hpp"
#include "decoder/ImageDecoder_S3TC.hpp"
#include "decoder/ImageDecoder_BC7.hpp"
#include "decoder/ImageDecoder_PVRTC.hpp"
#include "decoder/ImageDecoder_ETC1.hpp"
#include "decoder/ImageDecoder_ASTC.hpp"
using LibRpTexture::ImageSizeCalc::OpCode;

// C++ STL classes
using std::array;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRpTexture {

class GodotSTEXPrivate final : public FileFormatPrivate
{
	public:
		GodotSTEXPrivate(GodotSTEX *q, const IRpFilePtr &file);

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(GodotSTEXPrivate)

	public:
		/** TextureInfo **/
		static const array<const char*, 2+1> exts;
		static const array<const char*, 2+1> mimeTypes;
		static const TextureInfo textureInfo;

	public:
		// Godot STEX header
		union {
			uint32_t magic;		// [0x000] 'GDST' (v3) or 'GST2' (v4)
			STEX3_Header v3;
			STEX4_Header v4;
		} stexHeader;
		unsigned int stexVersion;	// 3 or 4 (TODO: romType equivalent)
		STEX_Format_e pixelFormat;	// flags are NOT included here
		uint32_t format_flags;		// format flags

		// Embedded file header (PNG/WebP)
		bool hasEmbeddedFile;
		STEX_Embed_Header embedHeader;

		// Decoded mipmaps
		// Mipmap 0 is the full image.
		vector<rp_image_ptr > mipmaps;

		// Mipmap sizes and start addresses.
		struct mipmap_data_t {
			uint32_t addr;		// start address
			uint32_t size;		// in bytes
			uint16_t width;		// width
			uint16_t height;	// height
		};
		vector<mipmap_data_t> mipmap_data;

		// Invalid pixel format message
		mutable string invalid_pixel_format;

	public:
		// Image format tables
		static const array<const char*, 0x26> img_format_tbl_v3;
		static const array<const char*, 0x27> img_format_tbl_v4;

		// ImageSizeCalc opcode tables
		static const array<ImageSizeCalc::OpCode, 0x26> op_tbl_v3;
		static const array<ImageSizeCalc::OpCode, 0x27> op_tbl_v4;

	public:
		/**
		 * Get mipmap information.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int getMipmapInfo(void);

		/**
		 * Load the image.
		 * @param mip Mipmap number. (0 == full image)
		 * @return Image, or nullptr on error.
		 */
		rp_image_const_ptr loadImage(int mip);
};

FILEFORMAT_IMPL(GodotSTEX)

/** GodotSTEXPrivate **/

/* TextureInfo */
const array<const char*, 2+1> GodotSTEXPrivate::exts = {{
	".stex",
	".ctex",

	nullptr
}};
const array<const char*, 2+1> GodotSTEXPrivate::mimeTypes = {{
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-godot-stex",
	"image/x-godot-ctex",

	nullptr
}};
const TextureInfo GodotSTEXPrivate::textureInfo = {
	exts.data(), mimeTypes.data()
};

// Image format table (STEX v3)
const array<const char*, 0x26> GodotSTEXPrivate::img_format_tbl_v3 = {{
	// 0x00
	"L8", "LA8", "R8", "RG8",
	"RGB8", "RGBA8", "RGBA4444", "RGB565",

	// 0x08
	"RF", "RGF", "RGBF", "RGBAF",
	"RH", "RGH", "RGBH", "RGBAH",

	// 0x10
	"RGBE9995", "DXT1", "DXT3", "DXT5",
	"RGTC_R", "RGTC_RG", "BPTC_RGBA", "BPTC_RGBF",

	// 0x18
	"BPTC_RGBFU", "PVRTC1_2", "PVRTC1_2A", "PVRTC1_4",
	"PVRTC1_4A", "ETC", "ETC2_R11", "ETC2_R11S",

	// 0x20
	"ETC2_RG11", "ETC2_RG11S", "ETC2_RGB8", "ETC2_RGBA8",
	"ETC2_RGB8A1",

	// Proprietary formats used in Sonic Colors Ultimate.
	// NOTE: There's extra formats here in Godot 4.0 that
	// may conflict, so check the version number once
	// Godot 4.0 is out.
	"ASTC_8x8",
}};

// Image format table (STEX v4)
const array<const char*, 0x27> GodotSTEXPrivate::img_format_tbl_v4 = {{
	// 0x00
	"L8", "LA8", "R8", "RG8",
	"RGB8", "RGBA8", "RGBA4444", "RGB565",

	// 0x08
	"RF", "RGF", "RGBF", "RGBAF",
	"RH", "RGH", "RGBH", "RGBAH",

	// 0x10
	"RGBE9995", "DXT1", "DXT3", "DXT5",
	"RGTC_R", "RGTC_RG", "BPTC_RGBA", "BPTC_RGBF",

	// 0x18
	"BPTC_RGBFU", "ETC", "ETC2_R11", "ETC2_R11S",
	"ETC2_RG11", "ETC2_RG11S", "ETC2_RGB8", "ETC2_RGBA8",

	// 0x20
	"ETC2_RGB8A1", "ETC2_RA_AS_RG", "DXT5_RA_AS_RG", "ASTC_4x4",
	"ASTC_4x4_HDR", "ASTC_8x8", "ASTC_8x8_HDR",
}};

// ImageSizeCalc opcode table (STEX v3)
const array<ImageSizeCalc::OpCode, 0x26> GodotSTEXPrivate::op_tbl_v3 = {{
	// 0x00
	OpCode::None,		// STEX_FORMAT_L8
	OpCode::Multiply2,	// STEX_FORMAT_LA8
	OpCode::None,		// STEX_FORMAT_R8
	OpCode::Multiply2,	// STEX_FORMAT_RG8
	OpCode::Multiply3,	// STEX_FORMAT_RGB8
	OpCode::Multiply4,	// STEX_FORMAT_RGBA8
	OpCode::Multiply2,	// STEX_FORMAT_RGBA4444
	OpCode::Multiply2,	// STEX_FORMAT_RGB565

	// 0x08
	OpCode::Multiply4,	// STEX_FORMAT_RF
	OpCode::Multiply8,	// STEX_FORMAT_RGF
	OpCode::Multiply12,	// STEX_FORMAT_RGBF	// TODO: Verify that it's not RGBxF.
	OpCode::Multiply16,	// STEX_FORMAT_RGBAF
	OpCode::Multiply2,	// STEX_FORMAT_RH
	OpCode::Multiply4,	// STEX_FORMAT_RGH
	OpCode::Multiply6,	// STEX_FORMAT_RGBH	// TODO: Verify that it's not RGBxH.
	OpCode::Multiply8,	// STEX_FORMAT_RGBAH

	// 0x10
	OpCode::Multiply4,	// STEX_FORMAT_RGBE9995
	OpCode::Align4Divide2,	// STEX_FORMAT_DXT1
	OpCode::Align4,		// STEX_FORMAT_DXT3
	OpCode::Align4,		// STEX_FORMAT_DXT5
	OpCode::Align4Divide2,	// STEX_FORMAT_RGTC_R
	OpCode::Align4,		// STEX_FORMAT_RGTC_RG
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBA
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBF	// TODO: Verify

	// 0x18
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBFU	// TODO: Verify
	OpCode::Divide4,	// STEX_FORMAT_PVRTC1_2
	OpCode::Divide4,	// STEX_FORMAT_PVRTC1_2A
	OpCode::Divide2,	// STEX_FORMAT_PVRTC1_4
	OpCode::Divide2,	// STEX_FORMAT_PVRTC1_4A
	OpCode::Divide2,	// STEX_FORMAT_ETC
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11S

	// 0x20
	OpCode::None,		// STEX_FORMAT_ETC2_RG11
	OpCode::None,		// STEX_FORMAT_ETC2_RG11S
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8	// TODO: Verify?
	OpCode::Align4,		// STEX_FORMAT_ETC2_RGBA8	// TODO: Verify?
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8A1	// TODO: Verify?

	// Proprietary format used in Sonic Colors Ultimate.
	OpCode::Align8Divide4,	// STEX_FORMAT_SCU_ASTC_8x8	// 8x8 == 2bpp
}};

// ImageSizeCalc opcode table (STEX v3)
const array<ImageSizeCalc::OpCode, 0x27> GodotSTEXPrivate::op_tbl_v4 = {{
	// 0x00
	OpCode::None,		// STEX_FORMAT_L8
	OpCode::Multiply2,	// STEX_FORMAT_LA8
	OpCode::None,		// STEX_FORMAT_R8
	OpCode::Multiply2,	// STEX_FORMAT_RG8
	OpCode::Multiply3,	// STEX_FORMAT_RGB8
	OpCode::Multiply4,	// STEX_FORMAT_RGBA8
	OpCode::Multiply2,	// STEX_FORMAT_RGBA4444
	OpCode::Multiply2,	// STEX_FORMAT_RGB565

	// 0x08
	OpCode::Multiply4,	// STEX_FORMAT_RF
	OpCode::Multiply8,	// STEX_FORMAT_RGF
	OpCode::Multiply12,	// STEX_FORMAT_RGBF	// TODO: Verify that it's not RGBxF.
	OpCode::Multiply16,	// STEX_FORMAT_RGBAF
	OpCode::Multiply2,	// STEX_FORMAT_RH
	OpCode::Multiply4,	// STEX_FORMAT_RGH
	OpCode::Multiply6,	// STEX_FORMAT_RGBH	// TODO: Verify that it's not RGBxH.
	OpCode::Multiply8,	// STEX_FORMAT_RGBAH

	// 0x10
	OpCode::Multiply4,	// STEX_FORMAT_RGBE9995
	OpCode::Align4Divide2,	// STEX_FORMAT_DXT1
	OpCode::Align4,		// STEX_FORMAT_DXT3
	OpCode::Align4,		// STEX_FORMAT_DXT5
	OpCode::Align4Divide2,	// STEX_FORMAT_RGTC_R
	OpCode::Align4,		// STEX_FORMAT_RGTC_RG
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBA
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBF	// TODO: Verify

	// 0x18
	OpCode::Align4,		// STEX_FORMAT_BPTC_RGBFU	// TODO: Verify
	OpCode::Divide2,	// STEX_FORMAT_ETC
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11S
	OpCode::None,		// STEX_FORMAT_ETC2_RG11
	OpCode::None,		// STEX_FORMAT_ETC2_RG11S
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8	// TODO: Verify?
	OpCode::Align4,		// STEX_FORMAT_ETC2_RGBA8	// TODO: Verify?

	// 0x20
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8A1	// TODO: Verify?
	OpCode::Align4,		// STEX4_FORMAT_ETC2_RA_AS_RG	// TODO: Verify?
	OpCode::Align4,		// STEX4_FORMAT_DXT5_RA_AS_RG	// TODO: Verify?
	OpCode::None,		// STEX4_FORMAT_ASTC_4x4	// 4x4 == 8bpp
	OpCode::None,		// STEX4_FORMAT_ASTC_4x4_HDR	// 4x4 == 8bpp
	OpCode::Align8Divide4,	// STEX4_FORMAT_ASTC_8x8	// 8x8 == 2bpp
	OpCode::Align8Divide4,	// STEX4_FORMAT_ASTC_8x8_HDR	// 8x8 == 2bpp
}};

GodotSTEXPrivate::GodotSTEXPrivate(GodotSTEX *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, stexVersion(0)
	, pixelFormat(static_cast<STEX_Format_e>(~0U))
	, format_flags(~0U)
	, hasEmbeddedFile(false)
{
	static_assert(GodotSTEXPrivate::img_format_tbl_v3.size() == STEX3_FORMAT_MAX,
		"GodotSTEXPrivate::img_format_tbl_v3[] is not the correct size.");
	static_assert(GodotSTEXPrivate::img_format_tbl_v4.size() == STEX4_FORMAT_MAX,
		"GodotSTEXPrivate::img_format_tbl_v4[] is not the correct size.");

	static_assert(GodotSTEXPrivate::op_tbl_v3.size() == STEX3_FORMAT_MAX,
		"GodotSTEXPrivate::op_tbl_v3[] is not the correct size.");
	static_assert(GodotSTEXPrivate::op_tbl_v4.size() == STEX4_FORMAT_MAX,
		"GodotSTEXPrivate::op_tbl_v4[] is not the correct size.");

	// Clear the structs and arrays.
	memset(&stexHeader, 0, sizeof(stexHeader));
	memset(&embedHeader, 0, sizeof(embedHeader));
}

/**
 * Get mipmap information.
 * @return 0 on success; non-zero on error.
 */
int GodotSTEXPrivate::getMipmapInfo(void)
{
	if (!mipmaps.empty()) {
		// Mipmap info was already obtained.
		return 0;
	}

	// NOTE: Using dimensions[] instead of accessing stexHeader directly
	// due to differences in the v3 and v4 formats.

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(dimensions[0] > 0);
	assert(dimensions[0] <= 32768);
	assert(dimensions[1] <= 32768);
	if (dimensions[0] == 0 || dimensions[0] > 32768 || dimensions[1] > 32768) {
		// Invalid image dimensions.
		return -EIO;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: STEX files shouldn't be more than 128 MB.
		return -ENOMEM;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Add the main image as the first mipmap.
	// TODO: Mipmap alignment?
	// TODO: Reserve space maybe?
	mipmaps.resize(1);
	mipmap_data.resize(1);

	uint32_t addr;
	switch (stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			return -EIO;
		case 3:
			addr = sizeof(stexHeader.v3);
			break;
		case 4:
			addr = sizeof(stexHeader.v4);
			break;
	}

	unsigned int width = dimensions[0];
	unsigned int height = (dimensions[1] > 0 ? dimensions[1] : 1);

	if (hasEmbeddedFile) {
		// Lossless (PNG/WebP) or lossy (WebP).
		// We'll use the size from the embedded file header.
		// TODO: Verify the embedded image's dimensions?
		// TODO: Mipmap support? STEX3 has a mipmap count in the header,
		// though we're excluding it right now.

		// NOTE: embedHeader.size includes the fourCC.
		assert(embedHeader.size > 4);
		if (embedHeader.size <= 4) {
			// Invalid size.
			mipmaps.clear();
			mipmap_data.clear();
			return -EIO;
		}

		const uint32_t embed_size = embedHeader.size - 4;
		// Sanity check: Maximum of 16 MB for the embedded image.
		assert(embedHeader.size <= 16*1024*1024);
		if (embedHeader.size >= 16*1024*1024) {
			// Invalid embedded file size.
			mipmaps.clear();
			mipmap_data.clear();
			return -EIO;
		}

		mipmap_data[0].addr = addr + sizeof(embedHeader);
		if (stexVersion == 3) {
			mipmap_data[0].addr += sizeof(uint32_t);
		}
		mipmap_data[0].size = embed_size;
		mipmap_data[0].width = width;
		mipmap_data[0].height = height;
		return 0;
	}

	const ImageSizeCalc::OpCode *op_tbl;
	size_t op_tbl_sz;
	switch (stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			return -EIO;
		case 3:
			op_tbl = op_tbl_v3.data();
			op_tbl_sz = op_tbl_v3.size();
			break;
		case 4:
			op_tbl = op_tbl_v4.data();
			op_tbl_sz = op_tbl_v4.size();
			break;
	}
	unsigned int expected_size = ImageSizeCalc::calcImageSize_tbl(
		op_tbl, op_tbl_sz, pixelFormat, width, height);
	if (expected_size == 0 || expected_size + addr > file_sz) {
		// Invalid image size.
		mipmaps.clear();
		mipmap_data.clear();
		return -EIO;
	}

	mipmap_data[0].addr = addr;
	mipmap_data[0].size = expected_size;
	mipmap_data[0].width = width;
	mipmap_data[0].height = height;

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	// NOTE: Using the internal image size, not the rescale size.
	if (height <= 1) {
		// This is a 1D texture.
		// We're done here.
		return 0;
	}

	// Check the mipmap flag and/or count.
	switch (stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			return -EIO;
		case 3:
			if (!(stexHeader.v3.format & STEX_FORMAT_FLAG_HAS_MIPMAPS)) {
				// No mipmaps.
				return 0;
			}
			break;
		case 4:
			if (stexHeader.v4.mipmap_count <= 1) {
				// No mipmaps.
				return 0;
			}
			break;
	}

	// Next mipmaps.
	// The header doesn't store a mipmap count,
	// so we'll need to figure it out ourselves.
	// TODO: STEX 4 has a mipmap count. We should make use of it;
	// otherwise, the number of mipmaps might not match...
	int i = 1;
	for (addr += expected_size; addr < file_sz; addr += expected_size, i++) {
		// Divide width/height by two.
		// TODO: Any alignment or minimum sizes?
		width /= 2;
		height /= 2;
		if (width <= 0 || height <= 0) {
			// We're done here.
			// NOTE: There seems to be more mipmaps in some files...
			break;
		}

		expected_size = calcImageSize_tbl(op_tbl, op_tbl_sz, pixelFormat, width, height);
		if (expected_size == 0 || expected_size + addr > file_sz) {
			// Invalid image size.
			break;
		}

		// Add a mipmap.
		mipmaps.resize(mipmaps.size() + 1);
		mipmap_data.resize(mipmap_data.size() + 1);
		mipmap_data_t &mdata = mipmap_data[mipmap_data.size() - 1];
		mdata.addr = addr;
		mdata.size = expected_size;
		mdata.width = width;
		mdata.height = height;
	}

	// Done calculating mipmaps.
	return 0;
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr GodotSTEXPrivate::loadImage(int mip)
{
	// Make sure the mipmap information is loaded.
	int ret = getMipmapInfo();
	assert(ret == 0);
	assert(!mipmap_data.empty());
	if (ret != 0 || mipmap_data.empty()) {
		return nullptr;
	}

	assert(mip >= 0);
	assert(mip < (int)mipmaps.size());
	if (mip < 0 || mip >= (int)mipmaps.size()) {
		// Invalid mipmap number.
		return nullptr;
	}

	if (!mipmaps.empty() && mipmaps[mip] != nullptr) {
		// Image has already been loaded.
		return mipmaps[mip];
	} else if (!this->isValid || !this->file) {
		// Can't load the image.
		return nullptr;
	}

	const mipmap_data_t &mdata = mipmap_data[mip];

	// Sanity check: Verify that the rescale dimensions,
	// if present, don't exceed 32768x32768.
	// TODO: Rescale dimensions for mipmaps?
	assert(rescale_dimensions[0] <= 32768);
	assert(rescale_dimensions[1] <= 32768);
	if (rescale_dimensions[0] > 32768 ||
	    rescale_dimensions[1] > 32768)
	{
		// Invalid rescale dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: STEX files shouldn't be more than 128 MB.
		return nullptr;
	}

	// TODO: Support WebP images, and maybe Basis Universal.
	if (hasEmbeddedFile) {
		// If it's PNG, load it.
		if (embedHeader.fourCC != cpu_to_be32(STEX_FourCC_PNG)) {
			// Not PNG.
			return nullptr;
		}
		if (stexVersion == 4 && stexHeader.v4.data_format != STEX4_DATA_FORMAT_PNG) {
			// FourCC is PNG, but the data format isn't...
			return nullptr;
		}

		// Load the PNG data.
		unique_ptr<uint8_t[]> buf(new uint8_t[mdata.size]);
		size_t size = file->seekAndRead(mdata.addr, buf.get(), mdata.size);
		if (size != mdata.size) {
			// Seek and/or read error.
			return nullptr;
		}

		// FIXME: Move RpPng to librptexture.
		// Requires moving IconAnimData and some other stuff...
		// TODO: Make use of PartitionFile instead of loading it into memory?
		MemFile f_mem(buf.get(), mdata.size);
		mipmaps[mip] = RpPng::load(&f_mem);

		return mipmaps[mip];
	}

	// Seek to the start of the texture data.
	ret = file->seek(mdata.addr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, mdata.size);
	size_t size = file->read(buf.get(), mdata.size);
	if (size != mdata.size) {
		// Read error.
		return nullptr;
	}

	// Decode the image.
	// TODO: More formats.
	rp_image_ptr img;
	if (pixelFormat <= STEX_FORMAT_BPTC_RGBFU) {
		// Common format between v3 and v4.
		switch (pixelFormat) {
			default:
				break;

			case STEX_FORMAT_L8:
				img = ImageDecoder::fromLinear8(
					ImageDecoder::PixelFormat::L8,
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_LA8:
				// TODO: Verify byte-order.
				img = ImageDecoder::fromLinear16(
					ImageDecoder::PixelFormat::L8A8,
					mdata.width, mdata.height,
					reinterpret_cast<const uint16_t*>(buf.get()),
					mdata.size);
				break;

			case STEX_FORMAT_R8:
				img = ImageDecoder::fromLinear8(
					ImageDecoder::PixelFormat::R8,
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_RG8:
				// TODO: Verify byte-order.
				img = ImageDecoder::fromLinear16(
					ImageDecoder::PixelFormat::GR88,
					mdata.width, mdata.height,
					reinterpret_cast<const uint16_t*>(buf.get()),
					mdata.size);
				break;
			case STEX_FORMAT_RGB8:
				img = ImageDecoder::fromLinear24(
					ImageDecoder::PixelFormat::BGR888,
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_RGBA8:
				img = ImageDecoder::fromLinear32(
					ImageDecoder::PixelFormat::ABGR8888,
					mdata.width, mdata.height,
					reinterpret_cast<const uint32_t*>(buf.get()),
					mdata.size);
				break;
			case STEX_FORMAT_RGBA4444:
				img = ImageDecoder::fromLinear16(
					ImageDecoder::PixelFormat::RGBA4444,
					mdata.width, mdata.height,
					reinterpret_cast<const uint16_t*>(buf.get()),
					mdata.size);
				break;

			case STEX_FORMAT_RGBE9995:
				img = ImageDecoder::fromLinear32(
					ImageDecoder::PixelFormat::RGB9_E5,
					mdata.width, mdata.height,
					reinterpret_cast<const uint32_t*>(buf.get()),
					mdata.size);
				break;

			// NOTE: Godot 4's DXTn encoding is broken if the
			// image width isn't a multiple of 4.
			// - https://github.com/godotengine/godot/issues/49981
			// - https://github.com/godotengine/godot/issues/51943
			case STEX_FORMAT_DXT1:
				img = ImageDecoder::fromDXT1(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_DXT3:
				img = ImageDecoder::fromDXT3(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_DXT5:
				img = ImageDecoder::fromDXT5(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;

			case STEX_FORMAT_RGTC_R:
				// RGTC, one component. (BC4)
				img = ImageDecoder::fromBC4(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_RGTC_RG:
				// RGTC, two components. (BC5)
				img = ImageDecoder::fromBC5(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX_FORMAT_BPTC_RGBA:
				// BPTC-compressed RGBA texture. (BC7)
				img = ImageDecoder::fromBC7(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
		}
	} else if (stexVersion == 3) {
		// NOTE: Godot 4 removed the PVRTC formats.
		// Godot commit 40be15920f849100dbf5bf94a1d09e81bf05c6e4,
		// 2021/12/29 02:06:12 +0100
		// Remove support for PVRTC texture encoding and decoding
		// Pixel format numbering diverges at this point.
		switch (pixelFormat) {
			default:
				break;

#ifdef ENABLE_PVRTC
			case STEX3_FORMAT_PVRTC1_2:
				img = ImageDecoder::fromPVRTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_NONE);
				break;
			case STEX3_FORMAT_PVRTC1_2A:
				img = ImageDecoder::fromPVRTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size,
					ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;

			case STEX3_FORMAT_PVRTC1_4:
				img = ImageDecoder::fromPVRTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_NONE);
				break;
			case STEX3_FORMAT_PVRTC1_4A:
				img = ImageDecoder::fromPVRTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size,
					ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
				break;
#endif /* ENABLE_PVRTC */

			// NOTE: Godot 4 uses swapped R and B channels in ETC textures.
			case STEX3_FORMAT_ETC:
				img = ImageDecoder::fromETC1(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX3_FORMAT_ETC2_RGB8:
				// NOTE: If the ETC2 texture has mipmaps,
				// it's stored as a Power-of-2 texture.
				img = ImageDecoder::fromETC2_RGB(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX3_FORMAT_ETC2_RGBA8:
				img = ImageDecoder::fromETC2_RGBA(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX3_FORMAT_ETC2_RGB8A1:
				img = ImageDecoder::fromETC2_RGB_A1(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;

			case STEX3_FORMAT_ETC2_R11:
			case STEX3_FORMAT_ETC2_R11S:
				// EAC-compressed R11 texture.
				// TODO: Does the signed version get decoded differently?
				img = ImageDecoder::fromEAC_R11(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX3_FORMAT_ETC2_RG11:
			case STEX3_FORMAT_ETC2_RG11S:
				// EAC-compressed RG11 texture.
				// TODO: Does the signed version get decoded differently?
				img = ImageDecoder::fromEAC_RG11(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;

#ifdef ENABLE_ASTC
			case STEX3_FORMAT_SCU_ASTC_8x8:
				// Sonic Colors Ultimate: ASTC 8x8
				img = ImageDecoder::fromASTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size, 8, 8);
				break;
#endif /* ENABLE_ASTC */
		}
	} else if (stexVersion == 4) {
		// NOTE: Godot 4 removed the PVRTC formats.
		// Godot commit 40be15920f849100dbf5bf94a1d09e81bf05c6e4,
		// 2021/12/29 02:06:12 +0100
		// Remove support for PVRTC texture encoding and decoding
		// Pixel format numbering diverges at this point.
		switch (pixelFormat) {
			default:
				break;

			// NOTE: Godot 4 previously used swapped R and B channels in ETC textures,
			// but this was fixed in commit ebec23d8d807e2a785075aac12466b88f7cbd3c1.
			// 2022/12/02 01:34:05 +1100
			// ETCPAK expects BGRA data for ETC
			case STEX4_FORMAT_ETC:
				img = ImageDecoder::fromETC1(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX4_FORMAT_ETC2_RGB8:
				// NOTE: If the ETC2 texture has mipmaps,
				// it's stored as a Power-of-2 texture.
				img = ImageDecoder::fromETC2_RGB(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX4_FORMAT_ETC2_RGBA8:
				img = ImageDecoder::fromETC2_RGBA(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX4_FORMAT_ETC2_RGB8A1:
				img = ImageDecoder::fromETC2_RGB_A1(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;

			case STEX4_FORMAT_ETC2_R11:
			case STEX4_FORMAT_ETC2_R11S:
				// EAC-compressed R11 texture.
				// TODO: Does the signed version get decoded differently?
				img = ImageDecoder::fromEAC_R11(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;
			case STEX4_FORMAT_ETC2_RG11:
			case STEX4_FORMAT_ETC2_RG11S:
				// EAC-compressed RG11 texture.
				// TODO: Does the signed version get decoded differently?
				img = ImageDecoder::fromEAC_RG11(
					mdata.width, mdata.height,
					buf.get(), mdata.size);
				break;

			case STEX4_FORMAT_ETC2_RA_AS_RG:
			case STEX4_FORMAT_DXT5_RA_AS_RG:
				// TODO
				break;

			case STEX4_FORMAT_ASTC_4x4:
				img = ImageDecoder::fromASTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size, 4, 4);
				break;
			case STEX4_FORMAT_ASTC_8x8:
				img = ImageDecoder::fromASTC(
					mdata.width, mdata.height,
					buf.get(), mdata.size, 8, 8);
				break;
		}
	} else {
		assert(!"Unsupported stexVersion value.");
	}

	// Image rescaling is handled by the UI frontend.
	mipmaps[mip] = img;
	return img;
}

/** GodotSTEX **/

/**
 * Read a Godot STEX image file.
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
GodotSTEX::GodotSTEX(const IRpFilePtr &file)
	: super(new GodotSTEXPrivate(this, file))
{
	RP_D(GodotSTEX);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the STEX header.
	d->file->rewind();
	size_t size = d->file->read(&d->stexHeader, sizeof(d->stexHeader));
	if (size != sizeof(d->stexHeader)) {
		d->file.reset();
		return;
	}

	// Verify the STEX magic.
	if (d->stexHeader.magic == cpu_to_be32(STEX3_MAGIC)) {
		// Godot 3 texture
		d->stexVersion = 3;

		// Read the embedded file header if it's present.
		if ((le32_to_cpu(d->stexHeader.v3.format) & (STEX_FORMAT_FLAG_LOSSLESS |
		                                             STEX_FORMAT_FLAG_LOSSY)) != 0)
		{
			// TODO: STEX3 has another uint32_t before the embedded file header.
			// This is the mipmap count. We're ignoring PNG/WebP mipmaps for now.
			size = d->file->seekAndRead(sizeof(d->stexHeader.v3) + sizeof(uint32_t),
				&d->embedHeader, sizeof(d->embedHeader));
			if (size != sizeof(d->embedHeader)) {
				// Seek and/or read error.
				d->file.reset();
				return;
			}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
			d->embedHeader.size = le32_to_cpu(d->embedHeader.size);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
			d->hasEmbeddedFile = true;
		}
	} else if (d->stexHeader.magic == cpu_to_be32(STEX4_MAGIC)) {
		// Godot 4 texture
		if (le32_to_cpu(d->stexHeader.v4.version) > STEX4_FORMAT_VERSION) {
			// Unsupported format version.
			d->file.reset();
			return;
		}
		d->stexVersion = 4;

		// Read the embedded file header if it's present.
		switch (le32_to_cpu(d->stexHeader.v4.data_format)) {
			case STEX4_DATA_FORMAT_PNG:
			case STEX4_DATA_FORMAT_WEBP:
				size = d->file->seekAndRead(sizeof(d->stexHeader.v4),
					&d->embedHeader, sizeof(d->embedHeader));
				if (size != sizeof(d->embedHeader)) {
					// Seek and/or read error.
					d->file.reset();
					return;
				}
#if SYS_BYTEORDER == SYS_BIG_ENDIAN
				d->embedHeader.size = le32_to_cpu(d->embedHeader.size);
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
				d->hasEmbeddedFile = true;
				break;
			default:
				break;
		}
	} else {
		// Incorrect magic.
		d->file.reset();
		return;
	}

	// File is valid.
	d->isValid = true;

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Header is stored in little-endian, so it always
	// needs to be byteswapped on big-endian.
	// NOTE: Signature is *not* byteswapped.
	switch (d->stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			d->file.reset();
			return;

		case 3:
			d->stexHeader.v3.width		= le16_to_cpu(d->stexHeader.v3.width);
			d->stexHeader.v3.width_rescale	= le16_to_cpu(d->stexHeader.v3.width_rescale);
			d->stexHeader.v3.height		= le16_to_cpu(d->stexHeader.v3.height);
			d->stexHeader.v3.height_rescale	= le16_to_cpu(d->stexHeader.v3.height_rescale);
			d->stexHeader.v3.flags		= le32_to_cpu(d->stexHeader.v3.flags);
			d->stexHeader.v3.format		= le32_to_cpu(d->stexHeader.v3.format);
			break;

		case 4:
			d->stexHeader.v4.version	= le32_to_cpu(d->stexHeader.v4.version);
			d->stexHeader.v4.width		= le32_to_cpu(d->stexHeader.v4.width);
			d->stexHeader.v4.height		= le32_to_cpu(d->stexHeader.v4.height);
			d->stexHeader.v4.format_flags	= le32_to_cpu(d->stexHeader.v4.format_flags);
			d->stexHeader.v4.mipmap_limit	= le32_to_cpu(d->stexHeader.v4.mipmap_limit);

			d->stexHeader.v4.data_format	= le32_to_cpu(d->stexHeader.v4.data_format);
			d->stexHeader.v4.img_width	= le16_to_cpu(d->stexHeader.v4.img_width);
			d->stexHeader.v4.img_height	= le16_to_cpu(d->stexHeader.v4.img_height);
			d->stexHeader.v4.mipmap_count	= le32_to_cpu(d->stexHeader.v4.mipmap_count);
			d->stexHeader.v4.pixel_format	= le32_to_cpu(d->stexHeader.v4.pixel_format);
			break;
	}
#endif /* SYS_BYTEORDER == SYS_BIG_ENDIAN */

	// Cache the dimensions for the FileFormat base class.
	// Also cache the pixel format and mipmap count.
	// TODO: 3D textures?
	// TODO: Don't set rescale dimensions if they match the regular dimensions.
	switch (d->stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			d->file.reset();
			return;

		case 3:
			d->mimeType = "image/x-godot-stex";	// unofficial, not on fd.o
			d->textureFormatName = "Godot STEX";

			d->pixelFormat = static_cast<STEX_Format_e>(d->stexHeader.v3.format & STEX_FORMAT_MASK);
			d->format_flags = (d->stexHeader.v3.format & ~STEX_FORMAT_MASK);
			d->dimensions[0] = d->stexHeader.v3.width;
			d->dimensions[1] = d->stexHeader.v3.height;
			if (d->stexHeader.v3.width_rescale  != d->dimensions[0] ||
			    d->stexHeader.v3.height_rescale != d->dimensions[1])
			{
				// Rescaling is needed.
				d->rescale_dimensions[0] = d->stexHeader.v3.width_rescale;
				d->rescale_dimensions[1] = d->stexHeader.v3.height_rescale;
			}

			if (d->stexHeader.v3.format & STEX_FORMAT_FLAG_HAS_MIPMAPS) {
				// Make sure the mipmap info is loaded.
				// TODO: Some way to get the mipmap count without loading mipmap info.
				int ret = const_cast<GodotSTEXPrivate*>(d)->getMipmapInfo();
				assert(ret == 0);
				assert(!d->mipmap_data.empty());
				if (ret == 0 && !d->mipmap_data.empty()) {
					d->mipmapCount = static_cast<int>(d->mipmap_data.size());
				}
			}
			break;

		case 4:
			d->mimeType = "image/x-godot-ctex";	// unofficial, not on fd.o
			d->textureFormatName = "Godot CTEX";

			// FIXME: Verify rescale dimensions.
			d->pixelFormat = static_cast<STEX_Format_e>(d->stexHeader.v4.pixel_format);
			d->format_flags = d->stexHeader.v4.format_flags;
			d->dimensions[0] = d->stexHeader.v4.img_width;
			d->dimensions[1] = d->stexHeader.v4.img_height;
			if ((int)d->stexHeader.v4.width  != d->dimensions[0] ||
			    (int)d->stexHeader.v4.height != d->dimensions[1])
			{
				// Rescaling is needed.
				d->rescale_dimensions[0] = d->stexHeader.v4.width;
				d->rescale_dimensions[1] = d->stexHeader.v4.height;
			}

			// NOTE: STEX_FORMAT_FLAG_HAS_MIPMAPS isn't used.
			d->mipmapCount = d->stexHeader.v4.mipmap_count;
			break;
	}

	// Special case: Godot 3 doesn't set rescaling parameters for NPOT PVRTC textures.
	if (d->stexVersion == 3 && (d->pixelFormat >= STEX3_FORMAT_PVRTC1_2 && d->pixelFormat <= STEX3_FORMAT_PVRTC1_4A)) {
		if (d->rescale_dimensions[0] == 0 &&
		    (!isPow2(d->dimensions[0]) || !isPow2(d->dimensions[1])))
		{
			// NPOT PVRTC texture, and no rescaling dimensions are set.
			d->rescale_dimensions[0] = d->dimensions[0];
			d->rescale_dimensions[1] = d->dimensions[1];

			if (!isPow2(d->dimensions[0])) {
				d->dimensions[0] = nextPow2(d->dimensions[0]);
			}
			if (!isPow2(d->dimensions[1])) {
				d->dimensions[1] = nextPow2(d->dimensions[1]);
			}
		}
	}
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *GodotSTEX::pixelFormat(void) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid)
		return nullptr;

	const char *const *img_format_tbl;
	STEX_Format_e pixelFormatMax;
	switch (d->stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			return nullptr;
		case 3:
			// Godot 3: Pixel format is always L8 (0) if an embedded
			// PNG or WebP image is present.
			if (d->hasEmbeddedFile)
				return nullptr;
			img_format_tbl = d->img_format_tbl_v3.data();
			pixelFormatMax = STEX3_FORMAT_MAX;
			break;
		case 4:
			img_format_tbl = d->img_format_tbl_v4.data();
			pixelFormatMax = STEX4_FORMAT_MAX;
			break;
	}

	if (d->pixelFormat >= 0 && d->pixelFormat < pixelFormatMax) {
		return img_format_tbl[d->pixelFormat];
	}

	// Invalid pixel format.
	// Store an error message instead.
	if (d->invalid_pixel_format.empty()) {
		d->invalid_pixel_format = fmt::format(
			FRUN(C_("RomData", "Unknown ({:d})")),
			static_cast<int>(d->pixelFormat));
	}
	return d->invalid_pixel_format.c_str();
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int GodotSTEX::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const GodotSTEX);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const int initial_count = fields->count();
	fields->reserve(initial_count + 4);	// Maximum of 4 fields.

	// STEX version (NOT STEX4's version field!)
	fields->addField_string_numeric(C_("GodotSTEX", "STEX Version"), d->stexVersion);
	

	switch (d->stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			break;
		case 3: {
			// Embedded data format (if present)
			const char *data_format = nullptr;
			if (d->hasEmbeddedFile) {
				switch (be32_to_cpu(d->embedHeader.fourCC)) {
					case STEX_FourCC_PNG:
						data_format = "PNG";
						break;
					case STEX_FourCC_WEBP:
						data_format = "WebP";
						break;
					default:
						break;
				}
			}
			if (data_format) {
				fields->addField_string(C_("GodotSTEX", "Data Format"), data_format);
			}

			// Flags (Godot 3 only)
			static const array<const char*, 13> flags_bitfield_names = {{
				NOP_C_("GodotSTEX|Flags", "Mipmaps"),
				NOP_C_("GodotSTEX|Flags", "Repeat"),
				NOP_C_("GodotSTEX|Flags", "Filter"),
				NOP_C_("GodotSTEX|Flags", "Anisotropic"),
				NOP_C_("GodotSTEX|Flags", "To Linear"),
				NOP_C_("GodotSTEX|Flags", "Mirrored Repeat"),
				nullptr, nullptr, nullptr, nullptr, nullptr,
				NOP_C_("GodotSTEX|Flags", "Cubemap"),
				NOP_C_("GodotSTEX|Flags", "For Streaming"),
			}};
			vector<string> *const v_flags_bitfield_names = RomFields::strArrayToVector_i18n("GodotSTEX|Flags", flags_bitfield_names);
			fields->addField_bitfield(C_("GodotSTEX", "Flags"),
				v_flags_bitfield_names, 3, d->stexHeader.v3.flags);
			break;
		}
		case 4: {
			// Data Format (Godot 4 only)
			static const array<const char*, 4> data_format_tbl = {{
				NOP_C_("GodotSTEX|DataFormat", "Image"),
				"PNG",	// Not translatable!
				"WebP",	// Not translatable!
				NOP_C_("GodotSTEX|DataFormat", "Basis Universal"),
			}};
			const char *const s_title = C_("GodotSTEX", "Data Format");
			if (d->stexHeader.v4.data_format < data_format_tbl.size()) {
				fields->addField_string(s_title,
					pgettext_expr("GodotSTEX|DataFormat",
						data_format_tbl[d->stexHeader.v4.data_format]));
			} else {
				fields->addField_string(s_title,
					fmt::format(FRUN(C_("RomData", "Unknown ({:d})")),
						d->stexHeader.v4.data_format));
			}
			break;
		}
	}

	// Format flags (v3) (starting at bit 20)
	static const array<const char*, 7> format_flags_bitfield_names_v3 = {{
		NOP_C_("GodotSTEX|FormatFlags", "Lossless"),		// 20
		NOP_C_("GodotSTEX|FormatFlags", "Lossy"),		// 21
		NOP_C_("GodotSTEX|FormatFlags", "Stream"),		// 22
		NOP_C_("GodotSTEX|FormatFlags", "Has Mipmaps"),		// 23
		NOP_C_("GodotSTEX|FormatFlags", "Detect 3D"),		// 24
		NOP_C_("GodotSTEX|FormatFlags", "Detect sRGB"),		// 25
		NOP_C_("GodotSTEX|FormatFlags", "Detect Normal"),	// 26
	}};
	// Format flags (v4) (starting at bit 20)
	static const array<const char*, 8> format_flags_bitfield_names_v4 = {{
		nullptr,						// 20
		nullptr,						// 21
		NOP_C_("GodotSTEX|FormatFlags", "Stream"),		// 22
		nullptr,						// 23
		NOP_C_("GodotSTEX|FormatFlags", "Detect 3D"),		// 24
		nullptr,						// 25
		NOP_C_("GodotSTEX|FormatFlags", "Detect Normal"),	// 26
		NOP_C_("GodotSTEX|FormatFlags", "Detect Roughness"),	// 27
	}};

	vector<string> *v_format_flags_bitfield_names = nullptr;
	switch (d->stexVersion) {
		default:
			assert(!"Invalid STEX version.");
			break;
		case 3:
			v_format_flags_bitfield_names = RomFields::strArrayToVector_i18n(
				"GodotSTEX|FormatFlags", format_flags_bitfield_names_v3);
			break;
		case 4:
			v_format_flags_bitfield_names = RomFields::strArrayToVector_i18n(
				"GodotSTEX|FormatFlags", format_flags_bitfield_names_v4);
			break;
	}

	if (v_format_flags_bitfield_names) {
		fields->addField_bitfield(C_("GodotSTEX", "Format Flags"),
			v_format_flags_bitfield_names, 3, d->format_flags >> 20);
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
rp_image_const_ptr GodotSTEX::image(void) const
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
rp_image_const_ptr GodotSTEX::mipmap(int mip) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<GodotSTEXPrivate*>(d)->loadImage(mip);
}

} // namespace LibRpTexture

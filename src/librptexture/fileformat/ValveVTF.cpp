/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * ValveVTF.cpp: Valve VTF image reader.                                   *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - https://developer.valvesoftware.com/wiki/Valve_Texture_Format
 */

#include "stdafx.h"
#include "ValveVTF.hpp"
#include "FileFormat_p.hpp"

#include "vtf_structs.h"

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
using LibRpTexture::ImageSizeCalc::OpCode;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRpTexture {

class ValveVTFPrivate final : public FileFormatPrivate
{
	public:
		ValveVTFPrivate(ValveVTF *q, const IRpFilePtr &file);
		~ValveVTFPrivate() final = default;

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(ValveVTFPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// VTF header
		VTFHEADER vtfHeader;

		// Texture data start address
		uint32_t texDataStartAddr;

		// Decoded mipmaps
		// Mipmap 0 is the full image.
		vector<rp_image_ptr > mipmaps;

		// Mipmap sizes and start addresses.
		struct mipmap_data_t {
			uint32_t addr;		// start address
			uint32_t size;		// in bytes
			uint16_t width;		// width
			uint16_t height;	// height

			uint16_t row_width;	// Row width. (must be a power of 2)
		};
		vector<mipmap_data_t> mipmap_data;

		// Invalid pixel format message
		char invalid_pixel_format[24];

	public:
		// Image format table
		static const std::array<const char*, VTF_IMAGE_FORMAT_MAX> img_format_tbl;

		// ImageSizeCalc opcode table
		static const std::array<ImageSizeCalc::OpCode, VTF_IMAGE_FORMAT_MAX> op_tbl;

	public:
		/**
		 * Get the minimum block size for the specified format.
		 * @param format VTF image format.
		 * @return Minimum block size, or 0 if invalid.
		 */
		static unsigned int getMinBlockSize(VTF_IMAGE_FORMAT format);

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

FILEFORMAT_IMPL(ValveVTF)

/** ValveVTFPrivate **/

/* TextureInfo */
const char *const ValveVTFPrivate::exts[] = {
	".vtf",
	//".vtx",	// TODO: Some files might use the ".vtx" extension.

	nullptr
};
const char *const ValveVTFPrivate::mimeTypes[] = {
	// Vendor-specific MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/vnd.valve.source.texture",

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-vtf",

	nullptr
};
const TextureInfo ValveVTFPrivate::textureInfo = {
	exts, mimeTypes
};

// Image format table
const std::array<const char*, VTF_IMAGE_FORMAT_MAX> ValveVTFPrivate::img_format_tbl = {
	"RGBA8888",
	"ABGR8888",
	"RGB888",
	"BGR888",
	"RGB565",
	"I8",
	"IA88",
	"P8",
	"A8",
	"RGB888 (Bluescreen)",	// FIXME: Localize?
	"BGR888 (Bluescreen)",	// FIXME: Localize?
	"ARGB8888",
	"BGRA8888",
	"DXT1",
	"DXT3",
	"DXT5",
	"BGRx8888",
	"BGR565",
	"BGRx5551",
	"BGRA4444",
	"DXT1_A1",
	"BGRA5551",
	"UV88",
	"UVWQ8888",
	"RGBA16161616F",
	"RGBA16161616",
	"UVLX8888",
};

// ImageSizeCalc opcode table
const std::array<ImageSizeCalc::OpCode, VTF_IMAGE_FORMAT_MAX> ValveVTFPrivate::op_tbl = {
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_RGBA8888
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_ABGR8888
	OpCode::Multiply3,	// VTF_IMAGE_FORMAT_RGB888
	OpCode::Multiply3,	// VTF_IMAGE_FORMAT_BGR888
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_RGB565
	OpCode::None,		// VTF_IMAGE_FORMAT_I8
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_IA88
	OpCode::None,		// VTF_IMAGE_FORMAT_P8
	OpCode::None,		// VTF_IMAGE_FORMAT_A8
	OpCode::Multiply3,	// VTF_IMAGE_FORMAT_RGB888_BLUESCREEN
	OpCode::Multiply3,	// VTF_IMAGE_FORMAT_BGR888_BLUESCREEN
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_ARGB8888
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_BGRA8888
	OpCode::Align4Divide2,	// VTF_IMAGE_FORMAT_DXT1
	OpCode::Align4,		// VTF_IMAGE_FORMAT_DXT3
	OpCode::Align4,		// VTF_IMAGE_FORMAT_DXT5
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_BGRx8888
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_BGR565
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_BGRx5551
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_BGRA4444
	OpCode::Align4Divide2,	// VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_BGRA5551
	OpCode::Multiply2,	// VTF_IMAGE_FORMAT_UV88
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_UVWQ8888
	OpCode::Multiply8,	// VTF_IMAGE_FORMAT_RGBA16161616F
	OpCode::Multiply8,	// VTF_IMAGE_FORMAT_RGBA16161616
	OpCode::Multiply4,	// VTF_IMAGE_FORMAT_UVLX8888
};

ValveVTFPrivate::ValveVTFPrivate(ValveVTF *q, const IRpFilePtr &file)
	: super(q, file, &textureInfo)
	, texDataStartAddr(0)
{
	// Clear the structs and arrays.
	memset(&vtfHeader, 0, sizeof(vtfHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

/**
* Get the minimum block size for the specified format.
* @param format VTF image format.
* @return Minimum block size, or 0 if invalid.
*/
unsigned int ValveVTFPrivate::getMinBlockSize(VTF_IMAGE_FORMAT format)
{
	static const std::array<uint8_t, VTF_IMAGE_FORMAT_MAX> block_size_tbl = {
		4,	// VTF_IMAGE_FORMAT_RGBA8888
		4,	// VTF_IMAGE_FORMAT_ABGR8888
		3,	// VTF_IMAGE_FORMAT_RGB888
		3,	// VTF_IMAGE_FORMAT_BGR888
		2,	// VTF_IMAGE_FORMAT_RGB565
		1,	// VTF_IMAGE_FORMAT_I8
		2,	// VTF_IMAGE_FORMAT_IA88
		1,	// VTF_IMAGE_FORMAT_P8
		1,	// VTF_IMAGE_FORMAT_A8
		3,	// VTF_IMAGE_FORMAT_RGB888_BLUESCREEN
		3,	// VTF_IMAGE_FORMAT_BGR888_BLUESCREEN
		4,	// VTF_IMAGE_FORMAT_ARGB8888
		4,	// VTF_IMAGE_FORMAT_BGRA8888
		8,	// VTF_IMAGE_FORMAT_DXT1
		16,	// VTF_IMAGE_FORMAT_DXT3
		16,	// VTF_IMAGE_FORMAT_DXT5
		4,	// VTF_IMAGE_FORMAT_BGRx8888
		2,	// VTF_IMAGE_FORMAT_BGR565
		2,	// VTF_IMAGE_FORMAT_BGRx5551
		2,	// VTF_IMAGE_FORMAT_BGRA4444
		8,	// VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA
		2,	// VTF_IMAGE_FORMAT_BGRA5551
		2,	// VTF_IMAGE_FORMAT_UV88
		4,	// VTF_IMAGE_FORMAT_UVWQ8888
		8,	// VTF_IMAGE_FORMAT_RGBA16161616F
		8,	// VTF_IMAGE_FORMAT_RGBA16161616
		4,	// VTF_IMAGE_FORMAT_UVLX8888
	};

	assert(format >= 0 && format < static_cast<int>(block_size_tbl.size()));
	if (format < 0 || format >= static_cast<int>(block_size_tbl.size())) {
		// Invalid format.
		return 0;
	}

	return block_size_tbl[format];
}

/**
 * Get mipmap information.
 * @return 0 on success; non-zero on error.
 */
int ValveVTFPrivate::getMipmapInfo(void)
{
	if (!mipmaps.empty()) {
		// Mipmap info was already obtained.
		return 0;
	}

	// Resize based on mipmap count.

	// Starting address.
	uint32_t addr = texDataStartAddr;

	// Skip the low-resolution image.
	if (vtfHeader.lowResImageFormat >= 0) {
		addr += ImageSizeCalc::calcImageSize_tbl(
			op_tbl.data(), op_tbl.size(), vtfHeader.lowResImageFormat,
			vtfHeader.lowResImageWidth,
			(vtfHeader.lowResImageHeight > 0 ? vtfHeader.lowResImageHeight : 1));
	}

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (vtfHeader.height > 0 ? vtfHeader.height : 1);

	// NOTE: VTF specifications say the image size must be a power of two.
	// Some malformed images may have a smaller width in the header,
	// so calculate the row width here.
	int row_width = vtfHeader.width;
	if (!isPow2(row_width)) {
		// Adjust to the next power of two.
		// We need to calculate the actual stride in order to
		// prevent crashes in the SSE2 code.
		row_width = 1 << (uilog2(row_width) + 1);
	}

	// Calculate the size of the full image.
	unsigned int mipmap_size = ImageSizeCalc::calcImageSize_tbl(
		op_tbl.data(), op_tbl.size(), vtfHeader.highResImageFormat,
		row_width, height);
	if (mipmap_size == 0) {
		// Invalid image size.
		return -EIO;
	}

	const unsigned int minBlockSize = getMinBlockSize(
		static_cast<VTF_IMAGE_FORMAT>(vtfHeader.highResImageFormat));
	if (minBlockSize == 0) {
		// Invalid minimum block size.
		return -EIO;
	}

	// Set up mipmap arrays.
	mipmaps.resize(mipmapCount > 0 ? mipmapCount : 1);
	mipmap_data.resize(mipmaps.size());

	// Mipmaps are stored from smallest to largest.
	// Calculate mipmap sizes and width/height first.
	int w = vtfHeader.width, h = height;
	int rw = row_width;
	for (int mip = 0; mip < static_cast<int>(mipmaps.size()); mip++) {
		auto &mdata = mipmap_data[mip];
		mdata.size = mipmap_size;
		mdata.width = w;
		mdata.height = h;
		mdata.row_width = rw;

		// Next mipmap is half the size.
		mipmap_size /= 4;
		w /= 2;
		h /= 2;
		rw /= 2;
		if (mipmap_size < minBlockSize) {
			// Mipmap is smaller than the minimum block size for this format.
			// NOTE: This might be an error...
			mipmap_size = minBlockSize;
		}
	}

	// Calculate the addresses.
	for (int mip = static_cast<int>(mipmaps.size())-1; mip >= 0; mip--) {
		auto &mdata = mipmap_data[mip];
		mdata.addr = addr;
		addr += mdata.size;
	}

	// Done calculating mipmaps.
	return 0;
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr ValveVTFPrivate::loadImage(int mip)
{
	// TODO: Option to load the low-res image instead?

	// Make sure the mipmap info is loaded.
	if (mipmap_data.empty()) {
		int ret = getMipmapInfo();
		assert(ret == 0);
		assert(!mipmap_data.empty());
		assert(mipmaps.size() == mipmap_data.size());
		if (ret != 0 || mipmap_data.empty()) {
			// Error getting the mipmap info.
			return nullptr;
		}
	}

	assert(mip >= 0);
	assert(mip < static_cast<int>(mipmaps.size()));
	if (mip < 0 || mip >= static_cast<int>(mipmaps.size())) {
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

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(vtfHeader.width > 0);
	assert(vtfHeader.width <= 32768);
	assert(vtfHeader.height <= 32768);
	if (vtfHeader.width == 0 || vtfHeader.width > 32768 ||
	    vtfHeader.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: VTF files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Mipmap data for this mipmap level
	const auto &mdata = mipmap_data[mip];

	// TODO: Handle environment maps (6-faced cube map) and volumetric textures.

	// Verify file size.
	if (mdata.addr + mdata.size > file_sz) {
		// File is too small.
		return nullptr;
	}

	// Texture cannot start inside of the VTF header.
	assert(mdata.addr >= sizeof(vtfHeader));
	if (mdata.addr < sizeof(vtfHeader)) {
		// Invalid texture data start address.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, mdata.size);
	size_t size = file->seekAndRead(mdata.addr, buf.get(), mdata.size);
	if (size != mdata.size) {
		// Read error.
		return nullptr;
	}

	// FIXME: Smaller mipmaps have read errors if encoded with e.g. DXTn,
	// since the width is smaller than 4.

	// Decode the image.
	// NOTE: VTF channel ordering does NOT match ImageDecoder channel ordering.
	// (The channels appear to be backwards.)
	// TODO: Lookup table to convert to PXF constants?
	// TODO: Verify on big-endian?
	rp_image_ptr img;
	switch (vtfHeader.highResImageFormat) {
		/* 32-bit */
		case VTF_IMAGE_FORMAT_RGBA8888:
		case VTF_IMAGE_FORMAT_UVWQ8888:	// handling as RGBA8888
		case VTF_IMAGE_FORMAT_UVLX8888:	// handling as RGBA8888
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::ABGR8888,
				mdata.width, mdata.height,
				reinterpret_cast<const uint32_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint32_t));
			break;
		case VTF_IMAGE_FORMAT_ABGR8888:
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::RGBA8888,
				mdata.width, mdata.height,
				reinterpret_cast<const uint32_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint32_t));
			break;
		case VTF_IMAGE_FORMAT_ARGB8888:
			// This is stored as RAGB for some reason...
			// FIXME: May be a bug in VTFEdit. (Tested versions: 1.2.5, 1.3.3)
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::RABG8888,
				mdata.width, mdata.height,
				reinterpret_cast<const uint32_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint32_t));
			break;
		case VTF_IMAGE_FORMAT_BGRA8888:
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::ARGB8888,
				mdata.width, mdata.height,
				reinterpret_cast<const uint32_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint32_t));
			break;
		case VTF_IMAGE_FORMAT_BGRx8888:
			img = ImageDecoder::fromLinear32(
				ImageDecoder::PixelFormat::xRGB8888,
				mdata.width, mdata.height,
				reinterpret_cast<const uint32_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint32_t));
			break;

		/* 24-bit */
		case VTF_IMAGE_FORMAT_RGB888:
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::BGR888,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width * 3);
			break;
		case VTF_IMAGE_FORMAT_BGR888:
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::RGB888,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width * 3);
			break;
		case VTF_IMAGE_FORMAT_RGB888_BLUESCREEN:
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::BGR888,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width * 3);
			img->apply_chroma_key(0xFF0000FF);
			break;
		case VTF_IMAGE_FORMAT_BGR888_BLUESCREEN:
			img = ImageDecoder::fromLinear24(
				ImageDecoder::PixelFormat::RGB888,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width * 3);
			img->apply_chroma_key(0xFF0000FF);
			break;

		/* 16-bit */
		case VTF_IMAGE_FORMAT_RGB565:
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::BGR565,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_BGR565:
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::RGB565,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_BGRx5551:
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::RGB555,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_BGRA4444:
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::ARGB4444,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_BGRA5551:
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::ARGB1555,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_IA88:
			// FIXME: I8 might have the alpha channel set to the I channel,
			// whereas L8 has A=1.0.
			// https://www.opengl.org/discussion_boards/showthread.php/151701-GL_LUMINANCE-vs-GL_INTENSITY
			// NOTE: Using A8L8 format, not IA8, which is GameCube-specific.
			// (Channels are backwards.)
			// TODO: Add ImageDecoder::fromLinear16() support for IA8 later.
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::A8L8,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;
		case VTF_IMAGE_FORMAT_UV88:
			// We're handling this as a GR88 texture.
			img = ImageDecoder::fromLinear16(
				ImageDecoder::PixelFormat::GR88,
				mdata.width, mdata.height,
				reinterpret_cast<const uint16_t*>(buf.get()), mdata.size,
				mdata.row_width * sizeof(uint16_t));
			break;

		/* 8-bit */
		case VTF_IMAGE_FORMAT_I8:
			// FIXME: I8 might have the alpha channel set to the I channel,
			// whereas L8 has A=1.0.
			// https://www.opengl.org/discussion_boards/showthread.php/151701-GL_LUMINANCE-vs-GL_INTENSITY
			img = ImageDecoder::fromLinear8(
				ImageDecoder::PixelFormat::L8,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width);
			break;
		case VTF_IMAGE_FORMAT_A8:
			img = ImageDecoder::fromLinear8(
				ImageDecoder::PixelFormat::A8,
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				mdata.row_width);
			break;

		/* Compressed */
		case VTF_IMAGE_FORMAT_DXT1:
			img = ImageDecoder::fromDXT1(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case VTF_IMAGE_FORMAT_DXT1_ONEBITALPHA:
			img = ImageDecoder::fromDXT1_A1(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case VTF_IMAGE_FORMAT_DXT3:
			img = ImageDecoder::fromDXT3(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case VTF_IMAGE_FORMAT_DXT5:
			img = ImageDecoder::fromDXT5(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;

		case VTF_IMAGE_FORMAT_P8:
		case VTF_IMAGE_FORMAT_RGBA16161616F:
		case VTF_IMAGE_FORMAT_RGBA16161616:
		default:
			// Not supported.
			break;
	}

	mipmaps[mip] = img;
	return img;
}

/** ValveVTF **/

/**
 * Read a Valve VTF image file.
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
ValveVTF::ValveVTF(const IRpFilePtr &file)
	: super(new ValveVTFPrivate(this, file))
{
	RP_D(ValveVTF);
	d->mimeType = "image/vnd.valve.source.texture";	// vendor-specific, not on fd.o
	d->textureFormatName = "Valve VTF";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the VTF header.
	d->file->rewind();
	size_t size = d->file->read(&d->vtfHeader, sizeof(d->vtfHeader));
	if (size != sizeof(d->vtfHeader)) {
		d->file.reset();
		return;
	}

	// Verify the VTF magic.
	if (d->vtfHeader.signature != cpu_to_be32(VTF_SIGNATURE)) {
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
	d->vtfHeader.version[0]		= le32_to_cpu(d->vtfHeader.version[0]);
	d->vtfHeader.version[1]		= le32_to_cpu(d->vtfHeader.version[1]);
	d->vtfHeader.headerSize		= le32_to_cpu(d->vtfHeader.headerSize);
	d->vtfHeader.width		= le16_to_cpu(d->vtfHeader.width);
	d->vtfHeader.height		= le16_to_cpu(d->vtfHeader.height);
	d->vtfHeader.flags		= le32_to_cpu(d->vtfHeader.flags);
	d->vtfHeader.frames		= le16_to_cpu(d->vtfHeader.frames);
	d->vtfHeader.firstFrame		= le16_to_cpu(d->vtfHeader.firstFrame);
	d->vtfHeader.reflectivity[0]	= lef32_to_cpu(d->vtfHeader.reflectivity[0]);
	d->vtfHeader.reflectivity[1]	= lef32_to_cpu(d->vtfHeader.reflectivity[1]);
	d->vtfHeader.reflectivity[2]	= lef32_to_cpu(d->vtfHeader.reflectivity[2]);
	d->vtfHeader.bumpmapScale	= lef32_to_cpu(d->vtfHeader.bumpmapScale);
	d->vtfHeader.highResImageFormat	= le32_to_cpu(d->vtfHeader.highResImageFormat);
	d->vtfHeader.lowResImageFormat	= le32_to_cpu(d->vtfHeader.lowResImageFormat);
	d->vtfHeader.depth		= le16_to_cpu(d->vtfHeader.depth);
	d->vtfHeader.numResources	= le32_to_cpu(d->vtfHeader.numResources);
#endif

	// Texture data start address.
	// Note that this is the start of *all* texture data,
	// including the low-res texture and mipmaps.
	// TODO: Should always be 16-byte aligned?
	// TODO: Verify header size against sizeof(VTFHEADER).
	// Test VTFs are 7.2 with 80-byte headers; sizeof(VTFHEADER) is 72...
	d->texDataStartAddr = d->vtfHeader.headerSize;

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->vtfHeader.width;
	d->dimensions[1] = d->vtfHeader.height;
	// 7.2+ supports 3D textures.
	if ((d->vtfHeader.version[0] > 7 ||
	    (d->vtfHeader.version[0] == 7 && d->vtfHeader.version[1] >= 2)))
	{
		if (d->vtfHeader.depth > 1) {
			d->dimensions[2] = d->vtfHeader.depth;
		}
	}

	// Save the mipmap count.
	// TODO: Differentiate between files that have 0 vs. 1?
	d->mipmapCount = d->vtfHeader.mipmapCount;
	assert(d->mipmapCount >= 0);
	assert(d->mipmapCount <= 128);
	if (d->mipmapCount > 128) {
		// Too many mipmaps...
		// Clamp it to 128.
		d->mipmapCount = 128;
	}
}

/** Property accessors **/

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *ValveVTF::pixelFormat(void) const
{
	RP_D(const ValveVTF);
	if (!d->isValid)
		return nullptr;

	const int fmt = d->vtfHeader.highResImageFormat;

	if (fmt >= 0 && fmt < static_cast<int>(d->img_format_tbl.size())) {
		return d->img_format_tbl[fmt];
	} else if (fmt < 0) {
		// Negative == none (usually -1)
		return C_("ValveVTF|ImageFormat", "None");
	}

	// Invalid pixel format.
	if (d->invalid_pixel_format[0] == '\0') {
		// TODO: Localization?
		snprintf(const_cast<ValveVTFPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (%d)", fmt);
	}
	return d->invalid_pixel_format;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int ValveVTF::getFields(RomFields *fields) const
{
	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(const ValveVTF);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// TODO: Move to RomFields?
#ifdef _WIN32
	// Windows: 6 visible rows per RFT_LISTDATA.
	static const int rows_visible = 6;
#else
	// Linux: 4 visible rows per RFT_LISTDATA.
	static const int rows_visible = 4;
#endif

	const int initial_count = fields->count();
	fields->reserve(initial_count + 9);	// Maximum of 9 fields.

	// VTF header.
	const VTFHEADER *const vtfHeader = &d->vtfHeader;

	// VTF version.
	fields->addField_string(C_("ValveVTF", "VTF Version"),
		rp_sprintf("%u.%u", vtfHeader->version[0], vtfHeader->version[1]));

	// Flags.
	// TODO: Show "deprecated" flags for older versions.
	static const char *const flags_names[] = {
		// 0x1-0x8
		NOP_C_("ValveVTF|Flags", "Point Sampling"),
		NOP_C_("ValveVTF|Flags", "Trilinear Sampling"),
		NOP_C_("ValveVTF|Flags", "Clamp S"),
		NOP_C_("ValveVTF|Flags", "Clamp T"),
		// 0x10-0x80
		NOP_C_("ValveVTF|Flags", "Anisotropic Sampling"),
		NOP_C_("ValveVTF|Flags", "Hint DXT5"),
		NOP_C_("ValveVTF|Flags", "PWL Corrected"),	// "No Compress" (deprecated)
		NOP_C_("ValveVTF|Flags", "Normal Map"),
		// 0x100-0x800
		NOP_C_("ValveVTF|Flags", "No Mipmaps"),
		NOP_C_("ValveVTF|Flags", "No Level of Detail"),
		NOP_C_("ValveVTF|Flags", "No Minimum Mipmap"),
		NOP_C_("ValveVTF|Flags", "Procedural"),
		// 0x1000-0x8000
		NOP_C_("ValveVTF|Flags", "1-bit Alpha"),
		NOP_C_("ValveVTF|Flags", "8-bit Alpha"),
		NOP_C_("ValveVTF|Flags", "Environment Map"),
		NOP_C_("ValveVTF|Flags", "Render Target"),
		// 0x10000-0x80000
		NOP_C_("ValveVTF|Flags", "Depth Render Target"),
		NOP_C_("ValveVTF|Flags", "No Debug Override"),
		NOP_C_("ValveVTF|Flags", "Single Copy"),
		NOP_C_("ValveVTF|Flags", "Pre SRGB"),		// "One Over Mipmap Level in Alpha" (deprecated)
		// 0x100000-0x800000
		NOP_C_("ValveVTF|Flags", "Premult Color by 1/mipmap"),
		NOP_C_("ValveVTF|Flags", "Normal to DuDv"),
		NOP_C_("ValveVTF|Flags", "Alpha Test Mipmap Gen"),
		NOP_C_("ValveVTF|Flags", "No depth Buffer"),
		// 0x1000000-0x8000000
		NOP_C_("ValveVTF|Flags", "Nice Filtered"),
		NOP_C_("ValveVTF|Flags", "Clamp U"),
		NOP_C_("ValveVTF|Flags", "Vertex Texture"),
		NOP_C_("ValveVTF|Flags", "SSBump"),
		// 0x10000000-0x20000000
		nullptr,
		NOP_C_("ValveVTF|Flags", "Border"),
	};

	// Convert to ListData_t for RFT_LISTDATA.
	auto vv_flags = new RomFields::ListData_t();
	vv_flags->reserve(ARRAY_SIZE(flags_names));
	for (const char *pFlagName : flags_names) {
		if (!pFlagName)
			continue;

		const size_t j = vv_flags->size()+1;
		vv_flags->resize(j);
		auto &data_row = vv_flags->at(j-1);
		// TODO: Localization.
		//data_row.emplace_back(dpgettext_expr(RP_I18N_DOMAIN, "ValveVTF|Flags", pFlagName));
		data_row.emplace_back(pFlagName);
	}

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_CHECKBOXES, rows_visible);
	params.headers = nullptr;
	params.data.single = vv_flags;
	params.mxd.checkboxes = vtfHeader->flags;
	fields->addField_listData(C_("ValveVTF", "Flags"), &params);

	// Number of frames.
	fields->addField_string_numeric(C_("ValveVTF", "# of Frames"), vtfHeader->frames);
	if (vtfHeader->frames > 1) {
		fields->addField_string_numeric(C_("ValveVTF", "First Frame"), vtfHeader->firstFrame);
	}

	// Reflectivity vector.
	fields->addField_string(C_("ValveVTF", "Reflectivity Vector"),
		rp_sprintf("(%0.1f, %0.1f, %0.1f)",
			vtfHeader->reflectivity[0],
			vtfHeader->reflectivity[1],
			vtfHeader->reflectivity[2]));

	// Bumpmap scale.
	fields->addField_string(C_("ValveVTF", "Bumpmap Scale"),
		rp_sprintf("%0.1f", vtfHeader->bumpmapScale));

	// Low-resolution image format.
	const char *img_format;
	if (vtfHeader->lowResImageFormat >= 0 &&
	    vtfHeader->lowResImageFormat < static_cast<int>(d->img_format_tbl.size()))
	{
		img_format = d->img_format_tbl[vtfHeader->lowResImageFormat];
	} else if (vtfHeader->lowResImageFormat < 0) {
		// Negative == none (usually -1)
		img_format = NOP_C_("ValveVTF|ImageFormat", "None");
	} else {
		// Unknown format.
		img_format = nullptr;
	}

	const char *const low_res_image_format_title = C_("ValveVTF", "Low-Res Image Format");
	if (img_format) {
		// TODO: Localization.
		fields->addField_string(low_res_image_format_title, img_format);
			//dpgettext_expr(RP_I18N_DOMAIN, "ValveVTF|ImageFormat", img_format));
		// Low-res image size.
		fields->addField_dimensions(C_("ValveVTF", "Low-Res Size"),
			vtfHeader->lowResImageWidth,
			vtfHeader->lowResImageHeight);
	} else {
		fields->addField_string(low_res_image_format_title,
			rp_sprintf(C_("RomData", "Unknown (%d)"), vtfHeader->lowResImageFormat));
	}

	if (vtfHeader->version[0] > 7 ||
	    (vtfHeader->version[0] == 7 && vtfHeader->version[1] >= 3))
	{
		// 7.3+: Resources.
		// TODO: Display the resources as RFT_LISTDATA?
		fields->addField_string_numeric(C_("ValveVTF", "# of Resources"),
			vtfHeader->numResources);
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
rp_image_const_ptr ValveVTF::image(void) const
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
rp_image_const_ptr ValveVTF::mipmap(int mip) const
{
	RP_D(const ValveVTF);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<ValveVTFPrivate*>(d)->loadImage(mip);
}

}

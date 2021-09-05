/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * GodotSTEX.cpp: Godot STEX image reader.                                 *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.librptexture.h"

#include "GodotSTEX.hpp"
#include "FileFormat_p.hpp"

#include "godot_stex_structs.h"

// librpbase, librpfile
#include "libi18n/i18n.h"
using LibRpBase::RomFields;
using LibRpFile::IRpFile;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder.hpp"
#include "decoder/ImageSizeCalc.hpp"
using LibRpTexture::ImageSizeCalc::OpCode;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRpTexture {

class GodotSTEXPrivate final : public FileFormatPrivate
{
	public:
		GodotSTEXPrivate(GodotSTEX *q, IRpFile *file);
		~GodotSTEXPrivate();

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(GodotSTEXPrivate)

	public:
		/** TextureInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const TextureInfo textureInfo;

	public:
		// Godot STEX header.
		STEX_30_Header stexHeader;

		// Decoded mipmaps.
		// Mipmap 0 is the full image.
		vector<rp_image*> mipmaps;

		// Mipmap sizes and start addresses.
		struct mipmap_data_t {
			uint32_t addr;		// start address
			uint32_t size;		// in bytes
			uint16_t width;		// width
			uint16_t height;	// height
		};
		vector<mipmap_data_t> mipmap_data;

		// Invalid pixel format message
		char invalid_pixel_format[24];

	public:
		// Image format table
		static const char *const img_format_tbl[];

		// ImageSizeCalc opcode table
		static const ImageSizeCalc::OpCode op_tbl[];

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
		const rp_image *loadImage(int mip);
};

FILEFORMAT_IMPL(GodotSTEX)

/** GodotSTEXPrivate **/

/* TextureInfo */
const char *const GodotSTEXPrivate::exts[] = {
	".stex",

	nullptr
};
const char *const GodotSTEXPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-godot-stex",

	nullptr
};
const TextureInfo GodotSTEXPrivate::textureInfo = {
	exts, mimeTypes
};

// Image format table
const char *const GodotSTEXPrivate::img_format_tbl[] = {
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
};

// ImageSizeCalc opcode table
const ImageSizeCalc::OpCode GodotSTEXPrivate::op_tbl[] = {
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
	OpCode::Divide4,	// STEX_FORMAT_PVRTC1_2		// TODO: Alignment for PVRTC1?
	OpCode::Divide4,	// STEX_FORMAT_PVRTC1_2A
	OpCode::Divide2,	// STEX_FORMAT_PVRTC1_4
	OpCode::Divide2,	// STEX_FORMAT_PVRTC1_4A
	OpCode::Divide2,	// STEX_FORMAT_ETC
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11		// TODO: Verify; Align4?
	OpCode::Divide2,	// STEX_FORMAT_ETC2_R11S	// TODO: Verify; Align4?

	// 0x20
	OpCode::Divide2,	// STEX_FORMAT_ETC2_RG11	// TODO: Verify; Align4?
	OpCode::Divide2,	// STEX_FORMAT_ETC2_RG11S	// TODO: Verify; Align4?
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8	// TODO: Verify?
	OpCode::Align4,		// STEX_FORMAT_ETC2_RGBA8	// TODO: Verify?
	OpCode::Align4Divide2,	// STEX_FORMAT_ETC2_RGB8A1	// TODO: Verify?

	// Proprietary formats used in Sonic Colors Ultimate.
	// FIXME: Other ASTC variants need a more complicated calculation.
	OpCode::Align8Divide4,	// STEX_FORMAT_SCU_ASTC_8x8	// 8x8 == 2bpp
};

GodotSTEXPrivate::GodotSTEXPrivate(GodotSTEX *q, IRpFile *file)
	: super(q, file, &textureInfo)
{
	static_assert(ARRAY_SIZE(GodotSTEXPrivate::op_tbl) == STEX_FORMAT_MAX,
		"GodotSTEXPrivate::op_tbl[] is not the correct size.");

	// Clear the structs and arrays.
	memset(&stexHeader, 0, sizeof(stexHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

GodotSTEXPrivate::~GodotSTEXPrivate()
{
	std::for_each(mipmaps.begin(), mipmaps.end(), [](rp_image *img) { UNREF(img); });
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

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(stexHeader.width > 0);
	assert(stexHeader.width <= 32768);
	assert(stexHeader.height <= 32768);
	if (stexHeader.width == 0 || stexHeader.width > 32768 || stexHeader.height > 32768) {
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

	int64_t addr = sizeof(stexHeader);
	unsigned int width = stexHeader.width;
	unsigned int height = (stexHeader.height > 0 ? stexHeader.height : 1);
	unsigned int expected_size = ImageSizeCalc::calcImageSize(
		op_tbl, ARRAY_SIZE(op_tbl),
		(stexHeader.format & STEX_FORMAT_MASK), width, height);
	if (expected_size == 0 || expected_size + addr > file_sz) {
		// Invalid image size.
		return -EIO;
	}

	mipmap_data[0].addr = addr;
	mipmap_data[0].size = expected_size;
	mipmap_data[0].width = width;
	mipmap_data[0].height = (height > 0 ? height : 1);

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	// NOTE: Using the internal image size, not the rescale size.
	if (height <= 1 || !(stexHeader.format & STEX_FORMAT_FLAG_HAS_MIPMAPS)) {
		// No mipmaps, or this is a 1D texture.
		// We're done here.
		return 0;
	}

	// Next mipmaps.
	// The header doesn't store a mipmap count,
	// so we'll need to figure it out ourselves.
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

		expected_size = calcImageSize(
			op_tbl, ARRAY_SIZE(op_tbl),
			(stexHeader.format & STEX_FORMAT_MASK), width, height);
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
const rp_image *GodotSTEXPrivate::loadImage(int mip)
{
	// Make sure the mipmap information is loaded.
	int ret = getMipmapInfo();
	if (ret != 0) {
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
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	const mipmap_data_t &mdata = mipmap_data[mip];

	// Sanity check: Verify that the rescale dimensions,
	// if present, don't exceed 32768x32768.
	// TODO: Rescale dimensions for mipmaps?
	assert(stexHeader.width_rescale <= 32768);
	assert(stexHeader.height_rescale <= 32768);
	if (stexHeader.width_rescale > 32768 ||
	    stexHeader.height_rescale > 32768)
	{
		// Invalid rescale dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: STEX files shouldn't be more than 128 MB.
		return nullptr;
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
	rp_image *img = nullptr;
	switch (stexHeader.format & STEX_FORMAT_MASK) {
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

#ifdef ENABLE_PVRTC
		case STEX_FORMAT_PVRTC1_2:
			img = ImageDecoder::fromPVRTC(
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_NONE);
			break;
		case STEX_FORMAT_PVRTC1_2A:
			img = ImageDecoder::fromPVRTC(
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				ImageDecoder::PVRTC_2BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;

		case STEX_FORMAT_PVRTC1_4:
			img = ImageDecoder::fromPVRTC(
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_NONE);
			break;
		case STEX_FORMAT_PVRTC1_4A:
			img = ImageDecoder::fromPVRTC(
				mdata.width, mdata.height,
				buf.get(), mdata.size,
				ImageDecoder::PVRTC_4BPP | ImageDecoder::PVRTC_ALPHA_YES);
			break;
#endif /* ENABLE_PVRTC */

		case STEX_FORMAT_ETC:
			img = ImageDecoder::fromETC1(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case STEX_FORMAT_ETC2_RGB8:
			// NOTE: If the ETC2 texture has mipmaps,
			// it's stored as a Power-of-2 texture.
			img = ImageDecoder::fromETC2_RGB(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case STEX_FORMAT_ETC2_RGBA8:
			img = ImageDecoder::fromETC2_RGBA(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;
		case STEX_FORMAT_ETC2_RGB8A1:
			img = ImageDecoder::fromETC2_RGB_A1(
				mdata.width, mdata.height,
				buf.get(), mdata.size);
			break;

#ifdef ENABLE_ASTC
		case STEX_FORMAT_SCU_ASTC_8x8:
			img = ImageDecoder::fromASTC(
				mdata.width, mdata.height,
				buf.get(), mdata.size, 8, 8);
			break;
#endif /* ENABLE_ASTC */
	}

	// FIXME: Rescale the image if necessary.
	// This might need to be done in the UI.

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
GodotSTEX::GodotSTEX(IRpFile *file)
	: super(new GodotSTEXPrivate(this, file))
{
	RP_D(GodotSTEX);
	d->mimeType = "image/x-godot-stex";	// unofficial, not on fd.o

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the STEX header.
	d->file->rewind();
	size_t size = d->file->read(&d->stexHeader, sizeof(d->stexHeader));
	if (size != sizeof(d->stexHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Verify the STEX magic.
	if (d->stexHeader.magic != cpu_to_be32(STEX_30_MAGIC)) {
		// Incorrect magic.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// File is valid.
	d->isValid = true;

#if SYS_BYTEORDER == SYS_BIG_ENDIAN
	// Header is stored in little-endian, so it always
	// needs to be byteswapped on big-endian.
	// NOTE: Signature is *not* byteswapped.
	d->stexHeader.width		= le16_to_cpu(d->stexHeader.width);
	d->stexHeader.width_rescale	= le16_to_cpu(d->stexHeader.width_rescale);
	d->stexHeader.height		= le16_to_cpu(d->stexHeader.height);
	d->stexHeader.height_rescale	= le16_to_cpu(d->stexHeader.height_rescale);
	d->stexHeader.flags		= le32_to_cpu(d->stexHeader.flags);
	d->stexHeader.format		= le32_to_cpu(d->stexHeader.format);
#endif

	// Cache the dimensions for the FileFormat base class.
	// TODO: 3D textures?
	d->dimensions[0] = d->stexHeader.width;
	d->dimensions[1] = d->stexHeader.height;
	d->rescale_dimensions[0] = d->stexHeader.width_rescale;
	d->rescale_dimensions[1] = d->stexHeader.height_rescale;

	// Initialize the mipmap data.
	d->getMipmapInfo();
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *GodotSTEX::textureFormatName(void) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid)
		return nullptr;

	// TODO: Version disambiguation when Godot 4.0 is released.
	return "Godot STEX";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *GodotSTEX::pixelFormat(void) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid)
		return nullptr;

	const unsigned int fmt = (d->stexHeader.format & STEX_FORMAT_MASK);
	if (fmt < ARRAY_SIZE(d->img_format_tbl)) {
		return d->img_format_tbl[fmt];
	}

	// Invalid pixel format.
	if (d->invalid_pixel_format[0] == '\0') {
		// TODO: Localization?
		snprintf(const_cast<GodotSTEXPrivate*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (%d)", fmt);
	}
	return d->invalid_pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int GodotSTEX::mipmapCount(void) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid)
		return -1;

	if (!(d->stexHeader.format & STEX_FORMAT_FLAG_HAS_MIPMAPS)) {
		return 0;
	}

	return static_cast<int>(d->mipmaps.size());
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int GodotSTEX::getFields(LibRpBase::RomFields *fields) const
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
	fields->reserve(initial_count + 2);	// Maximum of 2 fields.

	// Flags
	static const char *const flags_bitfield_names[] = {
		NOP_C_("GodotSTEX|Flags", "Mipmaps"),
		NOP_C_("GodotSTEX|Flags", "Repeat"),
		NOP_C_("GodotSTEX|Flags", "Filter"),
		NOP_C_("GodotSTEX|Flags", "Anisotropic"),
		NOP_C_("GodotSTEX|Flags", "To Linear"),
		NOP_C_("GodotSTEX|Flags", "Mirrored Repeat"),
		nullptr, nullptr, nullptr, nullptr, nullptr,
		NOP_C_("GodotSTEX|Flags", "Cubemap"),
		NOP_C_("GodotSTEX|Flags", "For Streaming"),
	};
	vector<string> *const v_flags_bitfield_names = RomFields::strArrayToVector_i18n(
		"GodotSTEX|Flags", flags_bitfield_names, ARRAY_SIZE(flags_bitfield_names));
	fields->addField_bitfield(C_("GodotSTEX", "Flags"),
		v_flags_bitfield_names, 3, d->stexHeader.flags);

	// Format flags (starting at bit 21)
	static const char *const format_flags_bitfield_names[] = {
		NOP_C_("GodotSTEX|FormatFlags", "Lossless"),
		NOP_C_("GodotSTEX|FormatFlags", "Lossy"),
		NOP_C_("GodotSTEX|FormatFlags", "Stream"),
		NOP_C_("GodotSTEX|FormatFlags", "Has Mipmaps"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect 3D"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect sRGB"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect Normal"),
	};
	vector<string> *const v_format_flags_bitfield_names = RomFields::strArrayToVector_i18n(
		"GodotSTEX|FormatFlags", format_flags_bitfield_names, ARRAY_SIZE(format_flags_bitfield_names));
	fields->addField_bitfield(C_("GodotSTEX", "Format Flags"),
		v_format_flags_bitfield_names, 3, d->stexHeader.format >> 20);

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
const rp_image *GodotSTEX::image(void) const
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
const rp_image *GodotSTEX::mipmap(int mip) const
{
	RP_D(const GodotSTEX);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<GodotSTEXPrivate*>(d)->loadImage(mip);
}

}

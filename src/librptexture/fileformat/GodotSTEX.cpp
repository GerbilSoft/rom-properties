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

		// Invalid pixel format message
		char invalid_pixel_format[24];

	public:
		// Image format table
		static const char *const img_format_tbl[];

	public:
		/**
		 * Calculate an image size.
		 * @param format STEX image format
		 * @param width Image width
		 * @param height Image height
		 * @return Image size, in bytes
		 */
		static unsigned int calcImageSize(STEX_Format_e format, unsigned int width, unsigned int height);

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

GodotSTEXPrivate::GodotSTEXPrivate(GodotSTEX *q, IRpFile *file)
	: super(q, file, &textureInfo)
{
	// Clear the structs and arrays.
	memset(&stexHeader, 0, sizeof(stexHeader));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

GodotSTEXPrivate::~GodotSTEXPrivate()
{
	std::for_each(mipmaps.begin(), mipmaps.end(), [](rp_image *img) { UNREF(img); });
}

/**
 * Calculate an image size.
 * @param format STEX image format
 * @param width Image width
 * @param height Image height
 * @return Image size, in bytes
 */
unsigned int GodotSTEXPrivate::calcImageSize(STEX_Format_e format, unsigned int width, unsigned int height)
{
	enum class OpCode : uint8_t {
		Unknown = 0,
		None,
		Multiply2,
		Multiply3,
		Multiply4,
		Multiply6,
		Multiply8,
		Multiply12,
		Multiply16,
		Divide2,
		Divide4,

		// DXTn requires aligned blocks.
		Align4Divide2,
		Align4,

		// ASTC requires aligned blocks.
		Align8Divide4,

		Max
	};

	static const OpCode mul_tbl[] = {
		// 0x00
		OpCode::None,		// STEX_FORMAT_L8
		OpCode::Multiply2,	// STEX_FORMAT_LA8
		OpCode::None,		// STEX_FORMAT_R8
		OpCode::Multiply2,	// STEX_FORMAT_RG8
		OpCode::Multiply3,	// STEX_FORMAT_RGB8	// TODO: Verify that it's not RGBx8.
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
		OpCode::Align4Divide2,	// STEX_FORMAT_RGTC_R		// TODO: Verify
		OpCode::Align4Divide2,	// STEX_FORMAT_RGTC_RG		// TODO: Verify
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
		OpCode::Divide2,	// STEX_FORMAT_ETC2_RGB8	// TODO: Verify; Align4?
		OpCode::None,		// STEX_FORMAT_ETC2_RGBA8	// TODO: Verify; Align4?
		OpCode::Divide2,	// STEX_FORMAT_ETC2_RGB8A1	// TODO: Verify; Align4?

		// Proprietary formats used in Sonic Colors Ultimate.
		// FIXME: Other ASTC variants need a more complicated calculation.
		OpCode::Align8Divide4,	// STEX_FORMAT_SCU_ASTC_8x8	// 8x8 == 2bpp
	};
	static_assert(ARRAY_SIZE(mul_tbl) == STEX_FORMAT_MAX,
		"mul_tbl[] is not the correct size.");

	format = static_cast<STEX_Format_e>(
		static_cast<unsigned int>(format) & STEX_FORMAT_MASK);
	assert(format >= 0 && format < ARRAY_SIZE_I(mul_tbl));
	if (format < 0 || format >= ARRAY_SIZE_I(mul_tbl)) {
		// Invalid format.
		return 0;
	}

	unsigned int expected_size = 0;
	if (mul_tbl[format] < OpCode::Align4Divide2) {
		expected_size = width * height;
	}

	switch (mul_tbl[format]) {
		default:
		case OpCode::Unknown:
			// Invalid opcode.
			return 0;

		case OpCode::None:					break;
		case OpCode::Multiply2:		expected_size *= 2;	break;
		case OpCode::Multiply3:		expected_size *= 3;	break;
		case OpCode::Multiply4:		expected_size *= 4;	break;
		case OpCode::Multiply6:		expected_size *= 6;	break;
		case OpCode::Multiply8:		expected_size *= 8;	break;
		case OpCode::Multiply12:	expected_size *= 12;	break;
		case OpCode::Multiply16:	expected_size *= 16;	break;
		case OpCode::Divide2:		expected_size /= 2;	break;
		case OpCode::Divide4:		expected_size /= 4;	break;

		case OpCode::Align4Divide2:
			expected_size = ALIGN_BYTES(4, width) * ALIGN_BYTES(4, height) / 2;
			break;

		case OpCode::Align4:
			expected_size = ALIGN_BYTES(4, width) * ALIGN_BYTES(4, height);
			break;

		case OpCode::Align8Divide4:
			expected_size = ALIGN_BYTES(8, width) * ALIGN_BYTES(8, height) / 4;
			break;
	}

	return expected_size;
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
const rp_image *GodotSTEXPrivate::loadImage(int mip)
{
	// TODO: Determine the mipmap count.
	int mipmapCount = 0;//vtfHeader.mipmapCount;
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

	if (mipmaps.size() != static_cast<size_t>(mipmapCount)) {
		mipmaps.resize(mipmapCount);
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(stexHeader.width > 0);
	assert(stexHeader.width <= 32768);
	assert(stexHeader.height <= 32768);
	if (stexHeader.width == 0 || stexHeader.width > 32768 ||
	    stexHeader.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: STEX files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = static_cast<uint32_t>(file->size());

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const unsigned int height = (stexHeader.height > 0 ? stexHeader.height : 1);

	// Determine the image data size based on format.
	// TODO: Stride adjustment?
	unsigned int expected_size = calcImageSize(
		static_cast<STEX_Format_e>(stexHeader.format),
		stexHeader.width, height);
	if (expected_size == 0 || expected_size > file_sz) {
		// Invalid image size.
		return nullptr;
	}

	// TODO: Adjust for mipmaps.
	// For now, assuming the main texture is directly after the header.
	const unsigned int texDataStartAddr = static_cast<unsigned int>(sizeof(stexHeader));

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// Read the texture data.
	auto buf = aligned_uptr<uint8_t>(16, expected_size);
	size_t size = file->read(buf.get(), expected_size);
	if (size != expected_size) {
		// Read error.
		return nullptr;
	}

	// Decode the image.
	// TODO: More formats.
	rp_image *img = nullptr;
	switch (stexHeader.format & STEX_FORMAT_MASK) {
		default:
			break;

		case STEX_FORMAT_DXT1:
			img = ImageDecoder::fromDXT1(
				stexHeader.width, height,
				buf.get(), expected_size);
			break;
		case STEX_FORMAT_DXT3:
			img = ImageDecoder::fromDXT3(
				stexHeader.width, height,
				buf.get(), expected_size);
			break;
		case STEX_FORMAT_DXT5:
			img = ImageDecoder::fromDXT5(
				stexHeader.width, height,
				buf.get(), expected_size);
			break;

#ifdef ENABLE_ASTC
		case STEX_FORMAT_SCU_ASTC_8x8:
			img = ImageDecoder::fromASTC(
				stexHeader.width, height,
				buf.get(), expected_size, 8, 8);
			break;
#endif /* ENABLE_ASTC */
	}

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
	d->stexHeader.width	= be32_to_cpu(d->stexHeader.width);
	d->stexHeader.height	= be32_to_cpu(d->stexHeader.height);
	d->stexHeader.flags	= be32_to_cpu(d->stexHeader.flags);
	d->stexHeader.format	= be32_to_cpu(d->stexHeader.format);
#endif

	// Cache the dimensions for the FileFormat base class.
	// TODO: 3D textures?
	d->dimensions[0] = d->stexHeader.width;
	d->dimensions[1] = d->stexHeader.height;
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

	// TODO: Count the number of mipmaps.
	return 0;
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
		NOP_C_("GodotSTEX|FormatFlags", "Has Mipmaps"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect 3D"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect sRGB"),
		NOP_C_("GodotSTEX|FormatFlags", "Detect Normal"),
	};
	vector<string> *const v_format_flags_bitfield_names = RomFields::strArrayToVector_i18n(
		"GodotSTEX|FormatFlags", format_flags_bitfield_names, ARRAY_SIZE(format_flags_bitfield_names));
	fields->addField_bitfield(C_("GodotSTEX", "Format Flags"),
		v_format_flags_bitfield_names, 3, d->stexHeader.format >> 21);

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

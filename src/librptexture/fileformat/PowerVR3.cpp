/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * PowerVR3.cpp: PowerVR 3.0.0 texture image reader.                       *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * References:
 * - http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.pdf
 */

#include "PowerVR3.hpp"
#include "FileFormat_p.hpp"

#include "pvr3_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/RomFields.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// librptexture
#include "img/rp_image.hpp"
#include "decoder/ImageDecoder.hpp"

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cstring>

namespace LibRpTexture {

FILEFORMAT_IMPL(PowerVR3)

class PowerVR3Private : public FileFormatPrivate
{
	public:
		PowerVR3Private(PowerVR3 *q, IRpFile *file);
		~PowerVR3Private();

	private:
		typedef FileFormatPrivate super;
		RP_DISABLE_COPY(PowerVR3Private)

	public:
		// PVR3 header.
		PowerVR3_Header pvr3Header;

		// Decoded image.
		rp_image *img;

		// Invalid pixel format message.
		char invalid_pixel_format[40];

		// Is byteswapping needed?
		// (PVR3 file has the opposite endianness.)
		bool isByteswapNeeded;

		// TODO: Metadata.

		// Texture data start address.
		unsigned int texDataStartAddr;

		/**
		 * Load the image.
		 * @param mip Mipmap number. (0 == full image)
		 * @return Image, or nullptr on error.
		 */
		const rp_image *loadImage(int mip);
};

/** PowerVR3Private **/

PowerVR3Private::PowerVR3Private(PowerVR3 *q, IRpFile *file)
	: super(q, file)
	, img(nullptr)
	, isByteswapNeeded(false)
	, texDataStartAddr(0)
{
	// Clear the PowerVR3 header struct.
	memset(&pvr3Header, 0, sizeof(pvr3Header));
	memset(invalid_pixel_format, 0, sizeof(invalid_pixel_format));
}

PowerVR3Private::~PowerVR3Private()
{
	delete img;
}

/**
 * Load the image.
 * @param mip Mipmap number. (0 == full image)
 * @return Image, or nullptr on error.
 */
const rp_image *PowerVR3Private::loadImage(int mip)
{
	if (img) {
		// Image has already been loaded.
		return img;
	} else if (!this->file || !this->isValid) {
		// Can't load the image.
		return nullptr;
	}

	// Sanity check: Maximum image dimensions of 32768x32768.
	// NOTE: `height == 0` is allowed here. (1D texture)
	assert(pvr3Header.width > 0);
	assert(pvr3Header.width <= 32768);
	assert(pvr3Header.height <= 32768);
	if (pvr3Header.width == 0 || pvr3Header.width > 32768 ||
	    pvr3Header.height > 32768)
	{
		// Invalid image dimensions.
		return nullptr;
	}

	// Texture cannot start inside of the PowerVR3 header.
	assert(texDataStartAddr >= sizeof(pvr3Header));
	if (texDataStartAddr < sizeof(pvr3Header)) {
		// Invalid texture data start address.
		return nullptr;
	}

	if (file->size() > 128*1024*1024) {
		// Sanity check: PowerVR3 files shouldn't be more than 128 MB.
		return nullptr;
	}
	const uint32_t file_sz = (uint32_t)file->size();

	// Seek to the start of the texture data.
	int ret = file->seek(texDataStartAddr);
	if (ret != 0) {
		// Seek error.
		return nullptr;
	}

	// NOTE: Mipmaps are stored *after* the main image.
	// Hence, no mipmap processing is necessary.

	// TODO: Support mipmaps.
	if (mip != 0) {
		return nullptr;
	}

	// Handle a 1D texture as a "width x 1" 2D texture.
	// NOTE: Handling a 3D texture as a single 2D texture.
	const int height = (pvr3Header.height > 0 ? pvr3Header.height : 1);

	// TODO
	return nullptr;
}

/** PowerVR3 **/

/**
 * Read a PowerVR 3.0.0 texture image file.
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
PowerVR3::PowerVR3(IRpFile *file)
	: super(new PowerVR3Private(this, file))
{
	RP_D(PowerVR3);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PowerVR3 header.
	d->file->rewind();
	size_t size = d->file->read(&d->pvr3Header, sizeof(d->pvr3Header));
	if (size != sizeof(d->pvr3Header)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Verify the PVR3 magic/version.
	printf("ver: %08X, host: %08X\n", d->pvr3Header.version, PVR3_VERSION_HOST);
	if (d->pvr3Header.version == PVR3_VERSION_HOST) {
		// Host-endian. Byteswapping is not needed.
		d->isByteswapNeeded = false;
	} else if (d->pvr3Header.version == PVR3_VERSION_SWAP) {
		// Swap-endian. Byteswapping is needed.
		// NOTE: Keeping `version` unswapped in case
		// the actual image data needs to be byteswapped.
		d->pvr3Header.flags		= __swab32(d->pvr3Header.flags);

		// Pixel format is technically 64-bit, so we have to
		// byteswap *and* swap both DWORDs.
		const uint32_t channel_depth	= __swab32(d->pvr3Header.pixel_format);
		const uint32_t pixel_format	= __swab32(d->pvr3Header.channel_depth);
		d->pvr3Header.pixel_format	= pixel_format;
		d->pvr3Header.channel_depth	= channel_depth;

		d->pvr3Header.color_space	= __swab32(d->pvr3Header.color_space);
		d->pvr3Header.channel_type	= __swab32(d->pvr3Header.channel_type);
		d->pvr3Header.height		= __swab32(d->pvr3Header.height);
		d->pvr3Header.width		= __swab32(d->pvr3Header.width);
		d->pvr3Header.depth		= __swab32(d->pvr3Header.depth);
		d->pvr3Header.num_surfaces	= __swab32(d->pvr3Header.num_surfaces);
		d->pvr3Header.num_faces		= __swab32(d->pvr3Header.num_faces);
		d->pvr3Header.mipmap_count	= __swab32(d->pvr3Header.mipmap_count);
		d->pvr3Header.metadata_size	= __swab32(d->pvr3Header.metadata_size);

		// Convenience flag.
		d->isByteswapNeeded = true;
	} else {
		// Invalid magic.
		d->isValid = false;
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// File is valid.
	d->isValid = true;

	// Texture data start address.
	d->texDataStartAddr = sizeof(d->pvr3Header) + d->pvr3Header.metadata_size;

	// Cache the dimensions for the FileFormat base class.
	d->dimensions[0] = d->pvr3Header.width;
	d->dimensions[1] = d->pvr3Header.height;
	d->dimensions[2] = d->pvr3Header.depth;
}

/** Class-specific functions that can be used even if isValid() is false. **/

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *PowerVR3::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".pvr",		// NOTE: Same as SegaPVR.
		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *PowerVR3::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		// TODO: Get these upstreamed on FreeDesktop.org.
		"image/x-pvr",

		nullptr
	};
	return mimeTypes;
}

/** Property accessors **/

/**
 * Get the texture format name.
 * @return Texture format name, or nullptr on error.
 */
const char *PowerVR3::textureFormatName(void) const
{
	RP_D(const PowerVR3);
	if (!d->isValid)
		return nullptr;

	return "PowerVR";
}

/**
 * Get the pixel format, e.g. "RGB888" or "DXT1".
 * @return Pixel format, or nullptr if unavailable.
 */
const char *PowerVR3::pixelFormat(void) const
{
	// TODO: Localization.
#define C_(ctx, str) str
#define NOP_C_(ctx, str) str

	RP_D(const PowerVR3);
	if (!d->isValid)
		return nullptr;

	if (d->invalid_pixel_format[0] != '\0') {
		return d->invalid_pixel_format;
	}

	// TODO: Localization?
	if (d->pvr3Header.channel_depth == 0) {
		// Compressed texture format.
		static const char *const pvr3PxFmt_tbl[] = {
			// 0
			"PVRTC 2bpp RGB", "PVRTC 2bpp RGBA",
			"PVRTC 4bpp RGB", "PVRTC 4bpp RGBA",
			"PVRTC-II 2bpp", "PVRTC-II 4bpp",
			"ETC1", "DXT1", "DXT2", "DXT3", "DXT4", "DXT5",
			"BC4", "BC5", "BC6", "BC7",

			// 16
			"UYVY", "YUY2", "BW1bpp", "R9G9B9E5 Shared Exponent",
			"RGBG8888", "GRGB8888", "ETC2 RGB", "ETC2 RGBA",
			"ETC2 RGB A1", "EAC R11", "EAC RG11",

			// 27
			"ASTC_4x4", "ASTC_5x4", "ASTC_5x5", "ASTC_6x5", "ATC_6x6",

			// 32
			"ASTC_8x5", "ASTC_8x6", "ASTC_8x8", "ASTC_10x5",
			"ASTC_10x6", "ASTC_10x8", "ASTC_10x10", "ASTC_12x10",
			"ASTC_12x12",

			// 41
			"ASTC_3x3x3", "ASTC_4x3x3", "ASTC_4x4x3", "ASTC_4x4x4",
			"ASTC_5x4x4", "ASTC_5x5x4", "ASTC_5x5x5", "ASTC_6x5x5",
			"ASTC_6x6x5", "ASTC_6x6x6",
		};
		static_assert(ARRAY_SIZE(pvr3PxFmt_tbl) == PVR3_PXF_MAX, "pvr3PxFmt_tbl[] needs to be updated!");

		if (d->pvr3Header.pixel_format < ARRAY_SIZE(pvr3PxFmt_tbl)) {
			return pvr3PxFmt_tbl[d->pvr3Header.pixel_format];
		}

		// Not valid.
		snprintf(const_cast<PowerVR3Private*>(d)->invalid_pixel_format,
			sizeof(d->invalid_pixel_format),
			"Unknown (Compressed: 0x%08X)", d->pvr3Header.pixel_format);
		return d->invalid_pixel_format;
	}

	// Uncompressed pixel formats.
	// These are literal channel identifiers, e.g. 'rgba',
	// followed by a color depth value for each channel.

	// NOTE: Pixel formats are stored in literal order in
	// little-endian files, so the low byte is the first channel.
	// TODO: Verify big-endian.

	char s_pxf[8], s_chcnt[16];
	char *p_pxf = s_pxf;
	char *p_chcnt = s_chcnt;

	uint32_t pixel_format = d->pvr3Header.pixel_format;
	uint32_t channel_depth = d->pvr3Header.channel_depth;
	for (unsigned int i = 0; i < 4; i++, pixel_format >>= 8, channel_depth >>= 8) {
		uint8_t pxf = (pixel_format & 0xFF);
		if (pxf == 0)
			break;

		*p_pxf++ = TOUPPER(pxf);
		p_chcnt += sprintf(p_chcnt, "%u", channel_depth & 0xFF);
	}
	*p_pxf = '\0';
	*p_chcnt = '\0';

	if (s_pxf[0] != '\0') {
		snprintf(const_cast<PowerVR3Private*>(d)->invalid_pixel_format,
			 sizeof(d->invalid_pixel_format),
			 "%s%s", s_pxf, s_chcnt);
	} else {
		strcpy(const_cast<PowerVR3Private*>(d)->invalid_pixel_format, C_("RomData", "Unknown"));
	}
	return d->invalid_pixel_format;
}

/**
 * Get the mipmap count.
 * @return Number of mipmaps. (0 if none; -1 if format doesn't support mipmaps)
 */
int PowerVR3::mipmapCount(void) const
{
	RP_D(const PowerVR3);
	if (!d->isValid)
		return -1;

	// Mipmap count.
	return d->pvr3Header.mipmap_count;
}

#ifdef ENABLE_LIBRPBASE_ROMFIELDS
/**
 * Get property fields for rom-properties.
 * @param fields RomFields object to which fields should be added.
 * @return Number of fields added, or 0 on error.
 */
int PowerVR3::getFields(LibRpBase::RomFields *fields) const
{
	// TODO: Localization.
#define C_(ctx, str) str
#define NOP_C_(ctx, str) str

	assert(fields != nullptr);
	if (!fields)
		return 0;

	RP_D(PowerVR3);
	if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	const PowerVR3_Header *const pvr3Header = &d->pvr3Header;
	const int initial_count = fields->count();
	fields->reserve(initial_count + 6);	// Maximum of 6 fields. (TODO)

	// TODO: Handle PVR 1.0 and 2.0 headers.
	fields->addField_string(C_("PowerVR3", "Version"), "3.0.0");

	// Endianness.
	// TODO: Save big vs. little in the constructor instead of just "needs byteswapping"?
	const char *endian_str;
	if (pvr3Header->version == PVR3_VERSION_HOST) {
		// Matches host-endian.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		endian_str = C_("PowerVR3", "Little-Endian");
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		endian_str = C_("PowerVR3", "Big-Endian");
#endif
	} else {
		// Does not match host-endian.
#if SYS_BYTEORDER == SYS_LIL_ENDIAN
		endian_str = C_("PowerVR3", "Big-Endian");
#else /* SYS_BYTEORDER == SYS_BIG_ENDIAN */
		endian_str = C_("PowerVR3", "Little-Endian");
#endif
	}
	fields->addField_string(C_("PowerVR3", "Endianness"), endian_str);

	// Color space.
	static const char *const pvr3_colorspace_tbl[] = {
		NOP_C_("PowerVR3|ColorSpace", "Linear RGB"),
		NOP_C_("PowerVR3|ColorSpace", "sRGB"),
	};
	static_assert(ARRAY_SIZE(pvr3_colorspace_tbl) == PVR3_COLOR_SPACE_MAX, "pvr3_colorspace_tbl[] needs to be updated!");
	if (pvr3Header->color_space < ARRAY_SIZE(pvr3_colorspace_tbl)) {
		fields->addField_string(C_("PowerVR3", "Color Space"),
			pvr3_colorspace_tbl[pvr3Header->color_space]);
			//dpgettext_expr(RP_I18N_DOMAIN, "PowerVR3|ColorSpace", pvr3_colorspace[pvr3Header->color_space]));
	} else {
		fields->addField_string_numeric(C_("PowerVR3", "Color Space"),
			pvr3Header->color_space);
	}

	// Channel type.
	static const char *const pvr3_chtype_tbl[] = {
		NOP_C_("PowerVR3|ChannelType", "Unsigned Byte (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Byte (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Byte"),
		NOP_C_("PowerVR3|ChannelType", "Signed Byte"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Short (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Short (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Short"),
		NOP_C_("PowerVR3|ChannelType", "Signed Short"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Integer (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Signed Integer (normalized)"),
		NOP_C_("PowerVR3|ChannelType", "Unsigned Integer"),
		NOP_C_("PowerVR3|ChannelType", "Signed Integer"),
		NOP_C_("PowerVR3|ChannelType", "Float"),
	};
	static_assert(ARRAY_SIZE(pvr3_chtype_tbl) == PVR3_CHTYPE_MAX, "pvr3_chtype_tbl[] needs to be updated!");
	if (pvr3Header->channel_type < ARRAY_SIZE(pvr3_chtype_tbl)) {
		fields->addField_string(C_("PowerVR3", "Channel Type"),
			pvr3_chtype_tbl[pvr3Header->channel_type]);
			//dpgettext_expr(RP_I18N_DOMAIN, "PowerVR3|ChannelType", pvr3_chtype_tbl[pvr3Header->channel_type]));
	} else {
		fields->addField_string_numeric(C_("PowerVR3", "Channel Type"),
			pvr3Header->channel_type);
	}

	// Other numeric fields.
	fields->addField_string_numeric(C_("PowerVR3", "# of Surfaces"),
		pvr3Header->num_surfaces);
	fields->addField_string_numeric(C_("PowerVR3", "# of Faces"),
		pvr3Header->num_faces);

	// TODO: Additional metadata.

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
const rp_image *PowerVR3::image(void) const
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
const rp_image *PowerVR3::mipmap(int mip) const
{
	RP_D(const PowerVR3);
	if (!d->isValid) {
		// Unknown file type.
		return nullptr;
	}

	// Load the image.
	return const_cast<PowerVR3Private*>(d)->loadImage(mip);
}

}

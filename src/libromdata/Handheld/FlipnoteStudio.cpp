/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * FlipnoteStudio.cpp: Flipnote Studio animation reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "FlipnoteStudio.hpp"
#include "RomData_p.hpp"

#include "flipnote_structs.h"
#include "../Console/gcn_card.h"	// for GC_UNIX_TIME_DIFF

// Other rom-properties libraries
#include "librptexture/decoder/ImageDecoder_NDS.hpp"
#include "bitstuff.h"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;
using namespace LibRpTexture;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class FlipnoteStudioPrivate final : public RomDataPrivate
{
public:
	explicit FlipnoteStudioPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
public:
	RP_DISABLE_COPY(FlipnoteStudioPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 1+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	// .ppm header
	FlipnoteStudio_PPM_Header_t ppmHeader;

	// Icon
	rp_image_ptr img_icon;

	/**
	 * Load the image.
	 * @return Image, or nullptr on error.
	 */
	rp_image_const_ptr loadImage(void);

private:
	// PPM preview palette
	// TODO: Get the exact palette from Flipnote Studio.
	// Using the palette from: https://www.dsibrew.org/wiki/Flipnote_Files/PPM
	// NOTE: Stored in NDS BGR555 format because fromNDS_CI4() requires it.
	static const array<uint16_t, 16> ppm_palette;
};

ROMDATA_IMPL(FlipnoteStudio)
ROMDATA_IMPL_IMG(FlipnoteStudio)

/** FlipnoteStudioPrivate **/

/* RomDataInfo */
const array<const char*, 1+1> FlipnoteStudioPrivate::exts = {{
	".ppm",	// Flipnote Studio (DSi)

	nullptr
}};
const array<const char*, 2+1> FlipnoteStudioPrivate::mimeTypes = {{
	// Unofficial MIME types.
	"image/x-portable-pixmap",	// HACK due to .ppm extension conflict

	// Unofficial MIME types.
	// TODO: Get these upstreamed on FreeDesktop.org.
	"image/x-flipnote-studio",

	nullptr
}};
const RomDataInfo FlipnoteStudioPrivate::romDataInfo = {
	"FlipnoteStudio", exts.data(), mimeTypes.data()
};

// PPM preview palette
// TODO: Get the exact palette from Flipnote Studio.
// Using the palette from: https://www.dsibrew.org/wiki/Flipnote_Files/PPM
// NOTE: Stored in NDS BGR555 format because fromNDS_CI4() requires it.
#define NDS_BGR555(r, g, b) (((b) << 10) | ((g) << 5) | (r))
const array<uint16_t, 16> FlipnoteStudioPrivate::ppm_palette = {{
	NDS_BGR555(31, 31, 31),	// TODO: Transparent?
	NDS_BGR555(10, 10, 10),	// dark gray
	NDS_BGR555(31, 31, 31),	// white
	NDS_BGR555(20, 20, 20),	// light gray
	NDS_BGR555(31,  0,  0),	// pure red
	NDS_BGR555(15,  0,  0),	// dark red
	NDS_BGR555(31, 15, 15),	// light red / pink
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0,  0, 31),	// pure blue
	NDS_BGR555( 0,  0, 15),	// dark blue
	NDS_BGR555(15, 15, 31),	// light blue
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555(31,  0, 31),	// magenta / purple
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0, 31,  0),	// pure green
	NDS_BGR555( 0, 31,  0),	// pure green
}};

FlipnoteStudioPrivate::FlipnoteStudioPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&ppmHeader, 0, sizeof(ppmHeader));
}

/**
 * Load the image.
 * @return Image, or nullptr on error.
 */
rp_image_const_ptr FlipnoteStudioPrivate::loadImage(void)
{
	rp_image_const_ptr icon;

	if (img_icon) {
		// Icon has already been loaded.
		// NOTE: Assigning to `icon` for named-return-value optimization.
		icon = img_icon;
		return icon;
	} else if (!this->isValid || !this->file) {
		// Can't load the icon.
		return icon;
	}

	// PPM images are stored in the header in NDS 4bpp tiled format.
	rp_image_ptr tmp_icon = ImageDecoder::fromNDS_CI4(
		FLIPNOTE_PPM_WIDTH, FLIPNOTE_PPM_HEIGHT,
		ppmHeader.preview_bitmap, sizeof(ppmHeader.preview_bitmap),
		ppm_palette.data(), sizeof(ppm_palette));
	if (!tmp_icon) {
		// Unable to decode the icon.
		return icon;
	}

	// Set the sBIT metadata.
	// TODO: Transparency?
	static const rp_image::sBIT_t sBIT = {5,5,5,0,0};
	tmp_icon->set_sBIT(sBIT);

	// Save and return the icon.
	// NOTE: Assigning to `icon` for named-return-value optimization.
	this->img_icon = tmp_icon;
	icon = std::move(tmp_icon);
	return icon;
}

/** FlipnoteStudio **/

/**
 * Read a Tiger game.com ROM image.
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
FlipnoteStudio::FlipnoteStudio(const IRpFilePtr &file)
	: super(new FlipnoteStudioPrivate(file))
{
	RP_D(FlipnoteStudio);
	d->mimeType = "image/x-flipnote-studio";	// unofficial, not on fd.o
	d->fileType = FileType::AnimationFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PPM header.
	d->file->rewind();
	size_t size = d->file->read(&d->ppmHeader, sizeof(d->ppmHeader));
	if (size != sizeof(d->ppmHeader)) {
		d->file.reset();
		return;
	}

	DetectInfo info = {
		{0, sizeof(d->ppmHeader), reinterpret_cast<const uint8_t*>(&d->ppmHeader)},
		nullptr,	// ext (not needed for FlipnoteStudio)
		0		// szFile (not needed for FlipnoteStudio)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int FlipnoteStudio::isRomSupported_static(const DetectInfo *info)
{
	// NOTE: The official game.com emulator requires the header to be at 0x40000.
	// Some ROMs have the header at 0, though.
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(FlipnoteStudio_PPM_Header_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the magic number.
	// TODO: Check other things too?
	const FlipnoteStudio_PPM_Header_t *const ppmHeader =
		reinterpret_cast<const FlipnoteStudio_PPM_Header_t*>(info->header.pData);
	if (ppmHeader->magic == cpu_to_be32(FLIPNOTE_PPM_MAGIC)) {
		// Magic number is correct.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *FlipnoteStudio::systemName(unsigned int type) const
{
	RP_D(const FlipnoteStudio);
	if (!d->isValid || !isSystemNameTypeValid(type)) {
		return nullptr;
	}

	// game.com has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"FlipnoteStudio::systemName() array index optimization needs to be updated.");

	static const array<const char*, 4> sysNames = {{
		"Flipnote Studio", "Flipnote", "Flipnote", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t FlipnoteStudio::supportedImageTypes_static(void)
{
	return IMGBF_INT_IMAGE;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> FlipnoteStudio::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	if (imageType != IMG_INT_IMAGE) {
		// Only IMG_INT_IMAGE is supported.
		return {};
	}

	return {{nullptr, FLIPNOTE_PPM_WIDTH, FLIPNOTE_PPM_HEIGHT, 0}};
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t FlipnoteStudio::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	switch (imageType) {
		case IMG_INT_IMAGE:
			// Use nearest-neighbor scaling.
			return IMGPF_RESCALE_NEAREST;
		default:
			break;
	}
	return 0;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int FlipnoteStudio::loadFieldData(void)
{
	RP_D(FlipnoteStudio);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Add more fields?

	const FlipnoteStudio_PPM_Header_t *const ppmHeader = &d->ppmHeader;
	d->fields.reserve(5);	// Maximum of 5 fields. (TESTING)

	// # of frames
	d->fields.addField_string_numeric(C_("FlipnoteStudio", "# of Frames"),
		le32_to_cpu(ppmHeader->num_frames));

	// Author names
	// TODO: U+E000 private use area from Nintendo fonts.
	if (ppmHeader->orig_author_name[0] != 0) {
		d->fields.addField_string(C_("FlipnoteStudio", "Original Author"),
			utf16le_to_utf8(ppmHeader->orig_author_name, ARRAY_SIZE(ppmHeader->orig_author_name)));
	}
	if (ppmHeader->last_edited_author_name[0] != 0) {
		d->fields.addField_string(C_("FlipnoteStudio", "Last Edited By"),
			utf16le_to_utf8(ppmHeader->last_edited_author_name, ARRAY_SIZE(ppmHeader->last_edited_author_name)));
	}
	if (ppmHeader->user_name[0] != 0) {
		d->fields.addField_string(C_("FlipnoteStudio", "Username"),
			utf16le_to_utf8(ppmHeader->user_name, ARRAY_SIZE(ppmHeader->user_name)));
	}

	// Last Modified timestamp
	d->fields.addField_dateTime(C_("RomData", "Timestamp"),
		static_cast<rp_time_t>(ppmHeader->timestamp) + static_cast<rp_time_t>(GC_UNIX_TIME_DIFF),
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_HAS_TIME |
		RomFields::RFT_DATETIME_IS_UTC	// DSi doesn't support timezones.
		);

	// Finished reading the field data.
	return d->fields.count();
}

/**
 * Load an internal image.
 * Called by RomData::image().
 * @param imageType	[in] Image type to load.
 * @param pImage	[out] Reference to rp_image_const_ptr to store the image in.
 * @return 0 on success; negative POSIX error code on error.
 */
int FlipnoteStudio::loadInternalImage(ImageType imageType, rp_image_const_ptr &pImage)
{
	ASSERT_loadInternalImage(imageType, pImage);
	RP_D(FlipnoteStudio);

	ROMDATA_loadInternalImage_single(
		IMG_INT_IMAGE,	// ourImageType
		d->file,	// file
		d->isValid,	// isValid
		0,		// romType
		d->img_icon,	// imgCache
		d->loadImage);	// func
}

} // namespace LibRomData

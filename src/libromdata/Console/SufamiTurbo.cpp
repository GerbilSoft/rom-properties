/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SufamiTurbo.cpp: Sufami Turbo ROM image reader.                         *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SufamiTurbo.hpp"
#include "st_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class SufamiTurboPrivate final : public RomDataPrivate
{
public:
	SufamiTurboPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(SufamiTurboPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// ROM header
	// NOTE: Must be byteswapped on access.
	ST_RomHeader romHeader;

	/**
	 * Get the ROM title.
	 *
	 * The ROM title length depends on type, and encoding
	 * depends on type and region.
	 *
	 * @return ROM title.
	 */
	string getRomTitle(void) const;
};

ROMDATA_IMPL(SufamiTurbo)
ROMDATA_IMPL_IMG(SufamiTurbo)

/** SufamiTurboPrivate **/

/* RomDataInfo */
// NOTE: Handling Sufami Turbo ROMs as if they're Super NES.
const char *const SufamiTurboPrivate::exts[] = {
	// NOTE: Not including ".smc" here.
	".st",

	nullptr
};
const char *const SufamiTurboPrivate::mimeTypes[] = {
	// Vendor-specific MIME types from FreeDesktop.org.
	"application/vnd.nintendo.snes.rom",

	// Unofficial MIME types from FreeDesktop.org.
	"application/x-snes-rom",
	"application/x-sufami-turbo-rom",

	nullptr
};
const RomDataInfo SufamiTurboPrivate::romDataInfo = {
	"SNES", exts, mimeTypes
};

SufamiTurboPrivate::SufamiTurboPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get the ROM title.
 *
 * The ROM title length depends on type, and encoding
 * depends on type and region.
 *
 * @return ROM title.
 */
string SufamiTurboPrivate::getRomTitle(void) const
{
	// Find the start of the title.
	size_t start;
	for (start = 0; start < sizeof(romHeader.title); start++) {
		if (romHeader.title[start] != ' ')
			break;
	}
	if (start >= sizeof(romHeader.title)) {
		// Empty title...
		return {};
	}

	// Trim spaces at the end of the title.
	size_t len = sizeof(romHeader.title) - start;
	bool done = false;
	for (const char *p = &romHeader.title[sizeof(romHeader.title)-1]; !done && (len > 0); p--) {
		switch (static_cast<uint8_t>(*p)) {
			case 0:
			case ' ':
			case 0xFF:
				// Blank character. Trim it.
				len--;
				break;

			default:
				// Not a blank character.
				done = true;
				break;
		}
	}
	if (len == 0) {
		// Empty title...
		return {};
	}

	// Convert the title from cp1252 and/or Shift-JIS.
	return cp1252_sjis_to_utf8(&romHeader.title[start], static_cast<int>(len));
}

/** SufamiTurbo **/

/**
 * Read a Sufami Turbo ROM image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
SufamiTurbo::SufamiTurbo(const IRpFilePtr &file)
	: super(new SufamiTurboPrivate(file))
{
	// NOTE: Handling Sufami Turbo ROMs as if they're Super NES.
	RP_D(SufamiTurbo);
	d->mimeType = "application/x-sufami-turbo-rom";	// unofficial, not on fd.o

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		d->file.reset();
		return;
	}

	// Check if this ROM is supported.
	const DetectInfo info = {
		{0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for SufamiTurbo)
		0		// szFile (not needed for SufamiTurbo)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Verify that this isn't the Sufami Turbo BIOS.
	// The BIOS should be handled as a Super Famicom ROM image.
	if (!memcmp(d->romHeader.title, ST_BIOS_TITLE, sizeof(d->romHeader.title))) {
		// This is the Sufami Turbo BIOS.
		d->isValid = false;
		d->file.reset();
		return;
	}

	// ROM is valid.
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SufamiTurbo::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(ST_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the magic number.
	const ST_RomHeader *const romHeader =
		reinterpret_cast<const ST_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->magic, ST_MAGIC, sizeof(romHeader->magic))) {
		// Found the magic number.
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
const char *SufamiTurbo::systemName(unsigned int type) const
{
	RP_D(const SufamiTurbo);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"SufamiTurbo::systemName() array index optimization needs to be updated.");

	// Sufami Turbo was only released in Japan, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SufamiTurbo::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const array<const char*, 4> sysNames = {{
		"Sufami Turbo", "ST", "ST", nullptr
	}};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t SufamiTurbo::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> SufamiTurbo::supportedImageSizes_static(ImageType imageType)
{
	// NOTE: This matches SNES.
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// NOTE: Some images might use high-resolution mode.
			// 292 = floor((256 * 8) / 7)
			return {{nullptr, 292, 224, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
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
uint32_t SufamiTurbo::imgpf(ImageType imageType) const
{
	// NOTE: This matches SNES.
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Rescaling is required for the 8:7 pixel aspect ratio.
			ret = IMGPF_RESCALE_ASPECT_8to7;
			break;

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SufamiTurbo::loadFieldData(void)
{
	RP_D(SufamiTurbo);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown save file type.
		return -EIO;
	}

	// ROM header is read in the constructor.
	const ST_RomHeader *const romHeader = &d->romHeader;
	d->fields.reserve(4); // Maximum of 4 fields.

	// Title
	d->fields.addField_string(C_("RomData", "Title"), d->getRomTitle());

	// Game ID
	// FIXME: This seems useless, so not including it for now...
#if 0
	char gameID[16];
	snprintf(gameID, sizeof(gameID), "%02X%02X%02X",
		romHeader->game_id[0], romHeader->game_id[1], romHeader->game_id[2]);
	d->fields.addField_string(C_("RomData", "Game ID"), gameID,
		RomFields::STRF_MONOSPACE);
#endif

	// Features
	static const char *const features_bitfield_names[] = {
		"SlowROM", "FastROM", "SRAM", "Special"
	};
	vector<string> *const v_features_bitfield_names = RomFields::strArrayToVector(
		features_bitfield_names, ARRAY_SIZE(features_bitfield_names));
	uint32_t features = 0;
	switch (romHeader->rom_speed) {
		default:
			break;
		case ST_RomSpeed_SlowROM:
			features = (1U << 0);
			break;
		case ST_RomSpeed_FastROM:
			features = (1U << 1);
			break;
	}
	switch (romHeader->features) {
		default:
		case ST_Feature_Simple:
			break;
		case ST_Feature_SRAM:
			features |= (1U << 2);
			break;
		case ST_Feature_Special:
			features |= (1U << 3);
			break;
	}
	d->fields.addField_bitfield(C_("SufamiTurbo", "Features"),
		v_features_bitfield_names, 4, features);

	// ROM size
	d->fields.addField_string(C_("SufamiTurbo", "ROM Size"),
		formatFileSizeKiB(romHeader->rom_size * 128*1024));

	// RAM size
	d->fields.addField_string(C_("SufamiTurbo", "SRAM Size"),
		formatFileSizeKiB(romHeader->sram_size * 2*1024));

	// TODO: Get the Sufami Turbo game code?

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int SufamiTurbo::loadMetaData(void)
{
	RP_D(SufamiTurbo);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// ROM header is read in the constructor.
	//const ST_RomHeader *const romHeader = &d->romHeader;

	// Title
	d->metaData->addMetaData_string(Property::Title, d->getRomTitle());

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int SufamiTurbo::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const SufamiTurbo);
	if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// RPDB directory is "snes", since Sufami Turbo is an SNES adapter.
	// Region code is "ST".

	// Filename is based on the title.
	const string s_title = d->getRomTitle();
	if (s_title.empty()) {
		// Empty title...
		return -ENOENT;
	}

	// NOTE: We only have one size for SufamiTurbo right now.
	RP_UNUSED(size);
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	assert(sizeDefs.size() == 1);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// NOTE: RPDB's title screen database only has one size.
	// There's no need to check image sizes, but we need to
	// get the image size for the extURLs struct.

	// Determine the image type name.
	const char *imageTypeName;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			imageTypeName = "title";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB("snes", imageTypeName, "ST", s_title.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("snes", imageTypeName, "ST", s_title.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

} // namespace LibRomData

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameBoyAdvance.hpp: Nintendo Game Boy Advance ROM reader.               *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "GameBoyAdvance.hpp"
#include "data/NintendoPublishers.hpp"
#include "gba_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(GameBoyAdvance)
ROMDATA_IMPL_IMG(GameBoyAdvance)

class GameBoyAdvancePrivate : public RomDataPrivate
{
	public:
		GameBoyAdvancePrivate(GameBoyAdvance *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(GameBoyAdvancePrivate)

	public:
		enum class RomType {
			Unknown	= -1,

			GBA		= 0,	// Standard GBA ROM.
			GBA_PassThru	= 1,	// Unlicensed GBA pass-through cartridge.
			NDS_Expansion	= 2,	// Non-bootable NDS expansion ROM.

			Max
		};
		RomType romType;

		// ROM header.
		GBA_RomHeader romHeader;

	public:
		/**
		 * Get the publisher.
		 * @return Publisher, or "Unknown (xxx)" if unknown.
		 */
		string getPublisher(void) const;
};

/** GameBoyAdvancePrivate **/

GameBoyAdvancePrivate::GameBoyAdvancePrivate(GameBoyAdvance *q, IRpFile *file)
	: super(q, file)
	, romType(RomType::Unknown)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get the publisher.
 * @return Publisher, or "Unknown (xxx)" if unknown.
 */
string GameBoyAdvancePrivate::getPublisher(void) const
{
	const char *const publisher = NintendoPublishers::lookup(romHeader.company);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		if (ISALNUM(romHeader.company[0]) &&
		    ISALNUM(romHeader.company[1]))
		{
			s_publisher = rp_sprintf(C_("GameBoyAdvance", "Unknown (%.2s)"),
				romHeader.company);
		} else {
			s_publisher = rp_sprintf(C_("GameBoyAdvance", "Unknown (%02X %02X)"),
				static_cast<uint8_t>(romHeader.company[0]),
				static_cast<uint8_t>(romHeader.company[1]));
		}
	}

	return s_publisher;
}

/** GameBoyAdvance **/

/**
 * Read a Nintendo Game Boy Advance ROM image.
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
GameBoyAdvance::GameBoyAdvance(IRpFile *file)
	: super(new GameBoyAdvancePrivate(this, file))
{
	RP_D(GameBoyAdvance);
	d->className = "GameBoyAdvance";
	d->mimeType = "application/x-gba-rom";	// unofficial

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	d->file->rewind();
	size_t size = d->file->read(&d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for GBA.
	info.szFile = 0;	// Not needed for GBA.
	d->romType = static_cast<GameBoyAdvancePrivate::RomType>(isRomSupported_static(&info));

	d->isValid = ((int)d->romType >= 0);
	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameBoyAdvance::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(GBA_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(GameBoyAdvancePrivate::RomType::Unknown);
	}

	// Check the first 16 bytes of the Nintendo logo.
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};

	GameBoyAdvancePrivate::RomType romType = GameBoyAdvancePrivate::RomType::Unknown;

	const GBA_RomHeader *const gba_header =
		reinterpret_cast<const GBA_RomHeader*>(info->header.pData);
	if (!memcmp(gba_header->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
		// Nintendo logo is present at the correct location.
		romType = GameBoyAdvancePrivate::RomType::GBA;
	} else if (gba_header->fixed_96h == 0x96 && gba_header->device_type == 0x00) {
		// This may be an expansion cartridge for a DS game.
		// These cartridges don't have the logo data, so they
		// aren't bootable as a GBA game.

		// Verify the header checksum.
		uint8_t chk = 0;
		for (int i = 0xA0; i <= 0xBC; i++) {
			chk -= info->header.pData[i];
		}
		chk -= 0x19;
		if (chk == gba_header->checksum) {
			// Header checksum is correct.
			// This is either a Nintendo DS expansion cartridge
			// or an unlicensed pass-through cartridge, e.g.
			// "Action Replay".

			// The entry point for expansion cartridges is 0xFFFFFFFF.
			if (gba_header->entry_point == cpu_to_le32(0xFFFFFFFF)) {
				// This is a Nintendo DS expansion cartridge.
				romType = GameBoyAdvancePrivate::RomType::NDS_Expansion;
			} else {
				// This is an unlicensed pass-through cartridge.
				romType = GameBoyAdvancePrivate::RomType::GBA_PassThru;
			}
		}
	}

	return static_cast<int>(romType);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *GameBoyAdvance::systemName(unsigned int type) const
{
	RP_D(const GameBoyAdvance);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses AGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameBoyAdvance::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Nintendo Game Boy Advance",
		"Game Boy Advance",
		"GBA",
		nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

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
const char *const *GameBoyAdvance::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gba",	// Most common
		".agb",	// Less common
		".mb",	// Multiboot (may conflict with AutoDesk Maya)
		".srl",	// Official SDK extension

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
const char *const *GameBoyAdvance::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org.
		"application/x-gba-rom",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t GameBoyAdvance::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> GameBoyAdvance::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			static const ImageSizeDef sz_EXT_TITLE_SCREEN[] = {
				{nullptr, 240, 160, 0},
			};
			return vector<ImageSizeDef>(sz_EXT_TITLE_SCREEN,
				sz_EXT_TITLE_SCREEN + ARRAY_SIZE(sz_EXT_TITLE_SCREEN));
		}
		default:
			break;
	}

	// Unsupported image type.
	return vector<ImageSizeDef>();
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
uint32_t GameBoyAdvance::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Use nearest-neighbor scaling when resizing.
			ret = IMGPF_RESCALE_NEAREST;
			break;

		default:
			// GameTDB's Nintendo DS cover scans have alpha transparency.
			// Hence, no image processing is required.
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameBoyAdvance::loadFieldData(void)
{
	RP_D(GameBoyAdvance);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// GBA ROM header
	const GBA_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(7);	// Maximum of 7 fields.

	// Title
	d->fields->addField_string(C_("RomData", "Title"),
		cpN_to_utf8(437, romHeader->title, sizeof(romHeader->title)));

	// Game ID
	// Replace any non-printable characters with underscores.
	// (Action Replay has ID6 "\0\0\0\001".)
	char id6[7];
	for (int i = 0; i < 6; i++) {
		id6[i] = (ISPRINT(romHeader->id6[i])
			? romHeader->id6[i]
			: '_');
	}
	id6[6] = 0;
	d->fields->addField_string(C_("GameBoyAdvance", "Game ID"), latin1_to_utf8(id6, 6));

	// Publisher
	d->fields->addField_string(C_("RomData", "Publisher"), d->getPublisher());

	// ROM version
	d->fields->addField_string_numeric(C_("RomData", "Revision"),
		romHeader->rom_version, RomFields::Base::Dec, 2);

	// Entry point
	const char *const entry_point_title = C_("GameBoyAdvance", "Entry Point");
	switch (d->romType) {
		case GameBoyAdvancePrivate::RomType::GBA:
		case GameBoyAdvancePrivate::RomType::GBA_PassThru:
			if (romHeader->entry_point_bytes[3] == 0xEA) {
				// Unconditional branch instruction.
				// NOTE: Due to pipelining, the actual branch is 2 words
				// after the specified branch offset.
				uint32_t entry_point = ((le32_to_cpu(romHeader->entry_point) + 2) & 0xFFFFFF) << 2;
				// Handle signed values.
				if (entry_point & 0x02000000) {
					entry_point |= 0xFC000000;
				}
				d->fields->addField_string_numeric(entry_point_title,
					entry_point, RomFields::Base::Hex, 8,
					RomFields::STRF_MONOSPACE);
			} else {
				// Non-standard entry point instruction.
				d->fields->addField_string_hexdump(entry_point_title,
					romHeader->entry_point_bytes, 4,
					RomFields::STRF_MONOSPACE);
			}
			break;

		case GameBoyAdvancePrivate::RomType::NDS_Expansion:
			// Not bootable.
			d->fields->addField_string(entry_point_title,
				C_("GameBoyAdvance", "Not bootable (Nintendo DS expansion)"));
			break;

		default:
			// Unknown ROM type.
			d->fields->addField_string(entry_point_title,
				C_("RomData", "Unknown"));
			break;
	}

	// Debugging enabled?
	// Reference: https://problemkaputt.de/gbatek.htm#gbacartridgeheader
	if (d->romType == GameBoyAdvancePrivate::RomType::GBA) {
		const uint8_t debug_enable = romHeader->nintendo_logo[0x9C-4];
		d->fields->addField_string(C_("GameBoyAdvance", "Enable Debug"),
			(debug_enable & 0xA5) == 0xA5
				// tr: Debugging is enabled.
				? C_("GameBoyAdvance", "Yes")
				// tr: Debugging is disabled.
				: C_("GameBoyAdvance", "No"));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int GameBoyAdvance::loadMetaData(void)
{
	RP_D(GameBoyAdvance);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// GBA ROM header
	const GBA_RomHeader *const romHeader = &d->romHeader;

	// Title
	d->metaData->addMetaData_string(Property::Title,
		cpN_to_utf8(437, romHeader->title, sizeof(romHeader->title)),
		RomMetaData::STRF_TRIM_END);

	// Publisher
	d->metaData->addMetaData_string(Property::Publisher, d->getPublisher());

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
int GameBoyAdvance::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	// Check for GBA ROMs that don't have external images.
	RP_D(const GameBoyAdvance);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	} else if (!memcmp(d->romHeader.id4, "AGBJ", 4) ||
	           !memcmp(d->romHeader.id4, "    ", 4))
	{
		// This is a prototype.
		// No external images are available.
		// TODO: CRC32-based images?
		return -ENOENT;
	} else if (d->romType == GameBoyAdvancePrivate::RomType::NDS_Expansion) {
		// This is a Nintendo DS expansion cartridge.
		// No external images are available.
		return -ENOENT;
	}

	// NOTE: We only have one size for GBA right now.
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

	// Game ID. (RPDB uses ID6 for Game Boy Advance.)
	// The ID6 cannot have non-printable characters.
	char id6[7];
	for (int i = ARRAY_SIZE(d->romHeader.id6)-1; i >= 0; i--) {
		if (!ISPRINT(d->romHeader.id6[i])) {
			// Non-printable character found.
			return -ENOENT;
		}
		id6[i] = d->romHeader.id6[i];
	}
	// NULL-terminated ID6 is needed for the
	// RPDB URL functions.
	id6[6] = 0;

	// Region code is id6[3].
	const char region_code[2] = {id6[3], '\0'};

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB("gba", imageTypeName, region_code, id6, ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("gba", imageTypeName, region_code, id6, ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}

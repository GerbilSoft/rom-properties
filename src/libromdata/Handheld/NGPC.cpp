/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NGPC.cpp: Neo Geo Pocket (Color) ROM reader.                            *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NGPC.hpp"
#include "ngpc_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::shared_ptr;
using std::string;
using std::vector;

namespace LibRomData {

class NGPCPrivate final : public RomDataPrivate
{
	public:
		NGPCPrivate(const shared_ptr<IRpFile> &file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NGPCPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		/** RomFields **/

		// ROM type.
		enum class RomType {
			Unknown	= -1,

			NGP	= 0,	// Neo Geo Pocket
			NGPC	= 1,	// Neo Geo Pocket Color

			Max
		};
		RomType romType;

	public:
		// ROM header.
		NGPC_RomHeader romHeader;
};

ROMDATA_IMPL(NGPC)
ROMDATA_IMPL_IMG(NGPC)

/** NGPCPrivate **/

/* RomDataInfo */
const char *const NGPCPrivate::exts[] = {
	".ngp",  ".ngc", ".ngpc",

	nullptr
};
const char *const NGPCPrivate::mimeTypes[] = {
	// NOTE: Ordering matches RomType.

	// Unofficial MIME types from FreeDesktop.org.
	"application/x-neo-geo-pocket-rom",
	"application/x-neo-geo-pocket-color-rom",

	nullptr
};
const RomDataInfo NGPCPrivate::romDataInfo = {
	"NGPC", exts, mimeTypes
};

NGPCPrivate::NGPCPrivate(const shared_ptr<IRpFile> &file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
{
	// Clear the various structs.
	memset(&romHeader, 0, sizeof(romHeader));
}

/** NGPC **/

/**
 * Read a Neo Geo Pocket (Color) ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
NGPC::NGPC(const shared_ptr<IRpFile> &file)
	: super(new NGPCPrivate(file))
{
	RP_D(NGPC);
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
		nullptr,	// ext (not needed for NGPC)
		0		// szFile (not needed for NGPC)
	};
	d->romType = static_cast<NGPCPrivate::RomType>(isRomSupported_static(&info));
	d->isValid = ((int)d->romType >= 0);

	if (!d->isValid) {
		d->file.reset();
		return;
	}

	// Set the MIME type.
	if ((int)d->romType < ARRAY_SIZE_I(d->mimeTypes)-1) {
		d->mimeType = d->mimeTypes[(int)d->romType];
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NGPC::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(NGPC_RomHeader))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return static_cast<int>(NGPCPrivate::RomType::Unknown);
	}

	NGPCPrivate::RomType romType = NGPCPrivate::RomType::Unknown;

	// Check the copyright/license string.
	const NGPC_RomHeader *const romHeader =
		reinterpret_cast<const NGPC_RomHeader*>(info->header.pData);
	if (!memcmp(romHeader->copyright, NGPC_COPYRIGHT_STR, sizeof(romHeader->copyright)) ||
	    !memcmp(romHeader->copyright, NGPC_LICENSED_STR,  sizeof(romHeader->copyright)))
	{
		// Valid copyright/license string.
		// Check the machine type.
		switch (romHeader->machine_type) {
			case NGPC_MACHINETYPE_MONOCHROME:
				romType = NGPCPrivate::RomType::NGP;
				break;
			case NGPC_MACHINETYPE_COLOR:
				romType = NGPCPrivate::RomType::NGPC;
				break;
			default:
				// Invalid.
				break;
		}
	}

	return static_cast<int>(romType);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NGPC::systemName(unsigned int type) const
{
	RP_D(const NGPC);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// NGPC has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NGPC::systemName() array index optimization needs to be updated.");
	static_assert((int)NGPCPrivate::RomType::Max == 2,
		"NGPC::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	// Bit 2: Machine type. (0 == NGP, 1 == NGPC)
	static const char *const sysNames[2][4] = {
		{"Neo Geo Pocket", "NGP", "NGP", nullptr},
		{"Neo Geo Pocket Color", "NGPC", "NGPC", nullptr}
	};

	// NOTE: This might return an incorrect system name if
	// d->romType is RomType::TYPE_UNKNOWN.
	return sysNames[(int)d->romType & 1][type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NGPC::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> NGPC::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			static const ImageSizeDef sz_EXT_TITLE_SCREEN[] = {
				{nullptr, 160, 152, 0},
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
uint32_t NGPC::imgpf(ImageType imageType) const
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
int NGPC::loadFieldData(void)
{
	RP_D(NGPC);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// ROM header is read in the constructor.
	const NGPC_RomHeader *const romHeader = &d->romHeader;
	d->fields.reserve(6);	// Maximum of 6 fields.

	// Title
	// NOTE: It's listed as ASCII. We'll use Latin-1.
	d->fields.addField_string(C_("RomData", "Title"),
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomFields::STRF_TRIM_END);

	// Product ID
	d->fields.addField_string(C_("RomData", "Product ID"),
		rp_sprintf("NEOP%02X%02X", romHeader->id_code[1], romHeader->id_code[0]));

	// Revision
	d->fields.addField_string_numeric(C_("RomData", "Revision"),
		romHeader->version, RomFields::Base::Dec, 2);

	// System
	static const char *const system_bitfield_names[] = {
		"NGP (Monochrome)", "NGP Color"
	};
	vector<string> *const v_system_bitfield_names = RomFields::strArrayToVector(
		system_bitfield_names, ARRAY_SIZE(system_bitfield_names));
	d->fields.addField_bitfield(C_("NGPC", "System"),
		v_system_bitfield_names, 0,
			(d->romType == NGPCPrivate::RomType::NGPC ? 3 : 1));

	// Entry point
	const uint32_t entry_point = le32_to_cpu(romHeader->entry_point);
	d->fields.addField_string_numeric(C_("RomData", "Entry Point"),
		entry_point, RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);

	// Debug enabled?
	const char *s_debug = nullptr;
	switch (entry_point >> 24) {
		case NGPC_DEBUG_MODE_OFF:
			s_debug = C_("NGPC|DebugMode", "Off");
			break;
		case NGPC_DEBUG_MODE_ON:
			s_debug = C_("NGPC|DebugMode", "On");
			break;
		default:
			break;
	}
	if (s_debug) {
		d->fields.addField_string(C_("NGPC", "Debug Mode"), s_debug);
	} else {
		d->fields.addField_string(C_("NGPC", "Debug Mode"),
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), entry_point >> 24));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int NGPC::loadMetaData(void)
{
	RP_D(NGPC);
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

	// ROM header is read in the constructor.
	const NGPC_RomHeader *const romHeader = &d->romHeader;

	// Title
	// NOTE: It's listed as ASCII. We'll use Latin-1.
	d->metaData->addMetaData_string(Property::Title,
		latin1_to_utf8(romHeader->title, sizeof(romHeader->title)),
		RomMetaData::STRF_TRIM_END);

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
int NGPC::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const NGPC);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NOTE: We only have one size for NGPC right now.
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

	// ROM header is read in the constructor.
	const NGPC_RomHeader *const romHeader = &d->romHeader;

	// Game ID and subdirectory.
	// For game ID, RPDB uses "NEOPxxxx" for NGPC.
	// TODO: Special cases for duplicates?
	const uint16_t id_code = (romHeader->id_code[1] << 8) | romHeader->id_code[0];
	const char *p_extra_subdir = nullptr;
	char extra_subdir[12];
	char game_id[13];	// original size is 12

	switch (id_code) {
		default:
			// No special handling for tihs game.
			snprintf(game_id, sizeof(game_id), "NEOP%04X", id_code);
			break;

		case 0x0000:	// Homebrew
		case 0x1234:	// Some samples
			// Use the game ID as the extra subdirectory,
			// and the ROM title as the game ID.
			memcpy(game_id, romHeader->title, sizeof(romHeader->title));
			game_id[sizeof(game_id)-1] = '\0';
			// Trim spaces from the game ID.
			for (int i = (int)sizeof(game_id)-2; i > 0; i--) {
				if (game_id[i] != '\0' && game_id[i] != ' ')
					break;
				game_id[i] = '\0';
			}
			if (game_id[0] == '\0') {
				// Title is empty.
				return -ENOENT;
			}

			snprintf(extra_subdir, sizeof(extra_subdir), "NEOP%04X", id_code);
			p_extra_subdir = extra_subdir;
			break;
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB("ngpc", imageTypeName, p_extra_subdir, game_id, ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("ngpc", imageTypeName, p_extra_subdir, game_id, ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}

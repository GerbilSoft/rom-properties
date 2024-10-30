/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwan.cpp: Bandai WonderSwan (Color) ROM reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WonderSwan.hpp"
#include "data/WonderSwanPublishers.hpp"
#include "ws_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using namespace LibRpFile;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

class WonderSwanPrivate final : public RomDataPrivate
{
public:
	explicit WonderSwanPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(WonderSwanPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	enum class RomType {
		Unknown	= -1,

		Original	= 0,	// WonderSwan
		Color		= 1,	// WonderSwan Color

		Max
	};
	RomType romType;

	// ROM footer
	WS_RomFooter romFooter;

	// Force the game ID's system ID character to '0'?
	bool forceGameIDSysIDTo0;

public:
	/**
	 * Get the game ID. (SWJ-PUBx01)
	 * @return Game ID, or empty string on error.
	 */
	string getGameID(void) const;
};

ROMDATA_IMPL(WonderSwan)

/** WonderSwanPrivate **/

/* RomDataInfo */
const char *const WonderSwanPrivate::exts[] = {
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.

	".ws",
	".wsc",
	".pc2",	// Pocket Challenge V2

	nullptr
};
const char *const WonderSwanPrivate::mimeTypes[] = {
	// NOTE: Ordering matches RomType.

	// Unofficial MIME types from FreeDesktop.org.
	"application/x-wonderswan-rom",
	"application/x-wonderswan-color-rom",

	// Unofficial MIME types.
	// TODO: Get this upstreamed on FreeDesktop.org.
	"application/x-pocket-challenge-v2-rom",

	nullptr
};
const RomDataInfo WonderSwanPrivate::romDataInfo = {
	"WonderSwan", exts, mimeTypes
};

WonderSwanPrivate::WonderSwanPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, romType(RomType::Unknown)
	, forceGameIDSysIDTo0(false)
{
	// Clear the ROM footer struct.
	memset(&romFooter, 0, sizeof(romFooter));
}

/**
 * Get the game ID. (SWJ-PUBx01)
 * @return Game ID, or empty string on error.
 */
string WonderSwanPrivate::getGameID(void) const
{
	const char *publisher_code = WonderSwanPublishers::lookup_code(romFooter.publisher);
	if (!publisher_code) {
		// Invalid publisher code.
		// Use "???" as a placeholder.
		publisher_code = "???";
	}

	char sys_id;
	if (romType == RomType::Original || forceGameIDSysIDTo0) {
		sys_id = '0';
	} else /*if (romType == RomType::Color)*/ {
		sys_id = 'C';
	}

	char game_id[16];
	snprintf(game_id, sizeof(game_id), "SWJ-%s%c%02X",
		publisher_code, sys_id, romFooter.game_id);
	return {game_id};
}

/** WonderSwan **/

/**
 * Read a WonderSwan (Color) ROM image.
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
WonderSwan::WonderSwan(const IRpFilePtr &file)
	: super(new WonderSwanPrivate(file))
{
	RP_D(WonderSwan);
	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the footer.
	const off64_t fileSize = d->file->size();
	// File must be at least 1024 bytes,
	// and cannot be larger than 16 MB.
	if (fileSize < 1024 || fileSize > (16*1024*1024)) {
		// File size is out of range.
		d->file.reset();
		return;
	}

	// Read the ROM footer.
	const unsigned int footer_addr = static_cast<unsigned int>(fileSize - sizeof(WS_RomFooter));
	d->file->seek(footer_addr);
	size_t size = d->file->read(&d->romFooter, sizeof(d->romFooter));
	if (size != sizeof(d->romFooter)) {
		d->file.reset();
		return;
	}

	// File extension is needed.
	const char *const filename = file->filename();
	const char *const ext = FileSystem::file_ext(filename);
	if (!ext) {
		// Unable to get the file extension.
		d->file.reset();
		return;
	}

	// Make sure this is actually a WonderSwan ROM.
	const DetectInfo info = {
		{footer_addr, sizeof(d->romFooter),
			reinterpret_cast<const uint8_t*>(&d->romFooter)},
		ext,		// ext
		fileSize	// szFile
	};
	d->romType = static_cast<WonderSwanPrivate::RomType>(isRomSupported(&info));
	d->isValid = ((int)d->romType >= 0);

	if (!d->isValid) {
		d->file.reset();
	}

	// Check for certain ROMs that have incorrect footers.
	switch (d->romFooter.publisher) {
		default:
			break;

		case 0x00:	// Unlicensed
			if (d->romType == WonderSwanPrivate::RomType::Original) {
				if (d->romFooter.game_id == 0x80 &&
				    d->romFooter.revision == 0x80 &&
				    d->romFooter.checksum == le16_to_cpu(0x0004))
				{
					// RUN=DIM Return to Earth
					// Published by Digital Dream, though they don't seem to have
					// a publisher code assigned.
					// System ID should be Color.
					d->romFooter.game_id = 0x01;
					d->romFooter.publisher = 0x40;	// fake, used internally only
					d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
					d->romType = WonderSwanPrivate::RomType::Color;
				} else if (d->romFooter.game_id == 0 &&
				           d->romFooter.revision == 0 &&
				           d->romFooter.checksum == le16_to_cpu(0x7F73))
				{
					// SD Gundam G Generation - Gather Beat
					// NOTE: This game has two IDs: SWJ-BAN030 and SWJ-BAN031
					d->romFooter.game_id = 0x30;
					d->romFooter.publisher = 0x01;	// Bandai
				}
			} else /*if (d->romType == WonderSwanPrivate::RomType::Color)*/ {
				if (d->romFooter.game_id == 0x17 &&
				    d->romFooter.checksum == le16_to_cpu(0x7C1D))
				{
					// Turntablist - DJ Battle
					// Published by Bandai, but publisher ID is 0.
					// System ID should be Original (Mono).
					d->romFooter.publisher = 0x01;
					d->romFooter.system_id = WS_SYSTEM_ID_ORIGINAL;
					d->romType = WonderSwanPrivate::RomType::Original;
				}
			}
			break;

		case 0x01:	// Bandai ("BAN")
			if (d->romType == WonderSwanPrivate::RomType::Original) {
				switch (d->romFooter.game_id) {
					default:
						break;

					case 0x01:
						// Some Digimon games incorrectly have this ID.
						if (d->romFooter.checksum == le16_to_cpu(0x54A8)) {
							// Digimon Digital Monsters (As) [M]
							// FIXME: Need to find the correct game ID for the Asian version here.
							//d->romFooter.game_id = 0x05;
						} else if (d->romFooter.checksum == le16_to_cpu(0xC4C9)) {
							// Digimon Digital Monsters - Anode & Cathode Tamer - Veedramon Version (As) [!]
							// System ID should be Color.
							// NOTE: Game ID is SWJ-BAN01C, even though it's Color.
							d->romFooter.game_id = 0x1C;
							d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
							d->romType = WonderSwanPrivate::RomType::Color;
							d->forceGameIDSysIDTo0 = true;
						}
						break;

					case 0x14:
						// Digimon Tamers: Digimon Medley
						// System ID should be Color.
						if (d->romFooter.checksum == le16_to_cpu(0x698F)) {
							d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
							d->romType = WonderSwanPrivate::RomType::Color;
						}
						break;
				}
			}
			break;

		case 0x0B:	// Sammy ("SUM")
			if (d->romType == WonderSwanPrivate::RomType::Original) {
				if (d->romFooter.game_id == 0x07) {
					// Guilty Gear Petit
					// System ID should be Color.
					d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
					d->romType = WonderSwanPrivate::RomType::Color;
				}
			}
			break;

		case 0x18:	// Kaga Tech ("KGT")
			if (d->romType == WonderSwanPrivate::RomType::Original) {
				if (d->romFooter.game_id == 0x09) {
					// Soroban Gu
					// System ID should be Color.
					d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
					d->romType = WonderSwanPrivate::RomType::Color;
				}
			}
			break;

		case 0x28:	// Square Enix ("SQR")
			if (d->romType == WonderSwanPrivate::RomType::Original) {
				switch (d->romFooter.game_id) {
					default:
						break;
					case 0x01:
						// Final Fantasy
						// System ID should be Color.
						d->romType = WonderSwanPrivate::RomType::Color;
						break;
					case 0x04:
						// Hataraku Chocobo
						// System ID should be Color.
						// NOTE: Game ID is SWJ-BAN004, even though it's Color.
						d->romFooter.system_id = WS_SYSTEM_ID_COLOR;
						d->romType = WonderSwanPrivate::RomType::Color;
						d->forceGameIDSysIDTo0 = true;
						break;
				}
			}
			break;
	}

	// MIME type.
	// TODO: Set to application/x-pocket-challenge-v2-rom if the extension is .pc2?
	if ((int)d->romType >= 0) {
		d->mimeType = d->mimeTypes[static_cast<int>(d->romType)];
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int WonderSwan::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData)
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);

	// File extension must be ".ws", ".wsc", or ".pc2".
	// TODO: Gzipped ROMs?
	if (!info->ext || (
	     strcasecmp(info->ext, ".ws") != 0 &&
	     strcasecmp(info->ext, ".wsc") != 0 &&
	     strcasecmp(info->ext, ".pc2") != 0))
	{
		// Not a supported file extension.
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}

	// File size constraints:
	// - Must be at least 16 KB.
	// - Cannot be larger than 16 MB.
	// - Must be a power of two.
	// NOTE: The only retail ROMs were 512 KB, 1 MB, and 2 MB,
	// but the system supports up to 16 MB, and some homebrew
	// is less than 512 KB.
	if (info->szFile < 16*1024 ||
	    info->szFile > 16*1024*1024 ||
	    ((info->szFile & (info->szFile - 1)) != 0))
	{
		// File size is not valid.
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}

	// Virtual Boy header is located at the end of the file.
	if (info->szFile < static_cast<off64_t>(sizeof(WS_RomFooter))) {
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}
	const uint32_t header_addr_expected = static_cast<uint32_t>(info->szFile - sizeof(WS_RomFooter));
	if (info->header.addr > header_addr_expected) {
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	} else if (info->header.addr + info->header.size < header_addr_expected + sizeof(WS_RomFooter)) {
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}

	// Determine the offset.
	const unsigned int offset = header_addr_expected - info->header.addr;

	// Get the ROM footer.
	const WS_RomFooter *const romFooter =
		reinterpret_cast<const WS_RomFooter*>(&info->header.pData[offset]);

	// Sanity checks for some values.
	if (romFooter->zero != 0) {
		// Not supported.
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}

	// This is probably a WonderSwan ROM.
	switch (romFooter->system_id) {
		case WS_SYSTEM_ID_ORIGINAL:
			return static_cast<int>(WonderSwanPrivate::RomType::Original);
		case WS_SYSTEM_ID_COLOR:
			return static_cast<int>(WonderSwanPrivate::RomType::Color);
		default:
			return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	}
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const char *WonderSwan::systemName(unsigned int type) const
{
	RP_D(const WonderSwan);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// WonderSwan has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"WonderSwan::systemName() array index optimization needs to be updated.");
	
	static const array<array<const char*, 4>, 2> sysNames = {{
		{{"Bandai WonderSwan", "WonderSwan", "WS", nullptr}},
		{{"Bandai WonderSwan Color", "WonderSwan Color", "WSC", nullptr}},
	}};

	return sysNames[d->romFooter.system_id & 1][type & SYSNAME_TYPE_MASK];
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t WonderSwan::supportedImageTypes(void) const
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> WonderSwan::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Assuming horizontal orientation by default.
			return {{nullptr, 224, 144, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> WonderSwan::supportedImageSizes(ImageType imageType) const
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			RP_D(const WonderSwan);
			if ((d->romFooter.flags & WS_FLAG_DISPLAY_MASK) == WS_FLAG_DISPLAY_VERTICAL) {
				return {{nullptr, 144, 224, 0}};
			} else {
				return {{nullptr, 224, 144, 0}};
			}
		}
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
uint32_t WonderSwan::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Use nearest-neighbor scaling when resizing.
			ret = IMGPF_RESCALE_NEAREST;
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
int WonderSwan::loadFieldData(void)
{
	RP_D(WonderSwan);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// WonderSwan ROM footer
	const WS_RomFooter *const romFooter = &d->romFooter;
	d->fields.reserve(10);	// Maximum of 10 fields.

	// Game ID
	const char *const game_id_title = C_("RomData", "Game ID");
	const string game_id = d->getGameID();
	if (!game_id.empty()) {
		d->fields.addField_string(game_id_title, game_id);
	} else {
		d->fields.addField_string(game_id_title, C_("WonderSwan", "None"));
	}

	// Revision
	d->fields.addField_string_numeric(C_("RomData", "Revision"), romFooter->revision);

	// Publisher
	const char *const publisher = WonderSwanPublishers::lookup_name(romFooter->publisher);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		s_publisher = rp_sprintf(C_("RomData", "Unknown (0x%02X)"), romFooter->publisher);
	}
	d->fields.addField_string(C_("RomData", "Publisher"), s_publisher);

	// System
	static const array<const char*, 2> system_bitfield_names = {{
		"WonderSwan", "WonderSwan Color"
	}};
	// TODO: Localize?
	vector<string> *const v_system_bitfield_names = RomFields::strArrayToVector(system_bitfield_names);
	const uint32_t ws_system = (romFooter->system_id & 1) ? 3 : 1;
	d->fields.addField_bitfield(C_("WonderSwan", "System"),
		v_system_bitfield_names, 0, ws_system);

	// ROM size
	static constexpr array<uint16_t, 10> rom_size_tbl = {{
		128, 256, 512, 1024,
		2048, 3072, 4096, 6144,
		8192, 16384,
	}};
	const char *const rom_size_title = C_("WonderSwan", "ROM Size");
	if (romFooter->rom_size < rom_size_tbl.size()) {
		d->fields.addField_string(rom_size_title,
			formatFileSizeKiB(rom_size_tbl[romFooter->rom_size]));
	} else {
		d->fields.addField_string(rom_size_title,
			rp_sprintf(C_("RomData", "Unknown (%u)"), romFooter->publisher));
	}

	// Save size and type
	static constexpr array<uint16_t, 6> sram_size_tbl = {{
		0, 8, 32, 128, 256, 512,
	}};
	const char *const save_memory_title = C_("WonderSwan", "Save Memory");
	if (romFooter->save_type == 0) {
		d->fields.addField_string(save_memory_title, C_("WonderSwan|SaveMemory", "None"));
	} else if (romFooter->save_type < sram_size_tbl.size()) {
		d->fields.addField_string(save_memory_title,
			// tr: Parameter 2 indicates the save type, e.g. "SRAM" or "EEPROM".
			rp_sprintf_p(C_("WonderSwan|SaveMemory", "%1$u KiB (%2$s)"),
				sram_size_tbl[romFooter->save_type],
				C_("WonderSwan|SaveMemory", "SRAM")));
	} else {
		// EEPROM save
		unsigned int eeprom_bytes;
		switch (romFooter->save_type) {
			case 0x10:	eeprom_bytes = 128; break;
			case 0x20:	eeprom_bytes = 2048; break;
			case 0x50:	eeprom_bytes = 1024; break;
			default:	eeprom_bytes = 0; break;
		}
		if (eeprom_bytes == 0) {
			d->fields.addField_string(save_memory_title, C_("WonderSwan|SaveMemory", "None"));
		} else {
			const char *fmtstr;
			if (eeprom_bytes >= 1024) {
				// tr: Parameter 2 indicates the save type, e.g. "SRAM" or "EEPROM".
				fmtstr = C_("WonderSwan|SaveMemory", "%1$u KiB (%2$s)");
				eeprom_bytes /= 1024;
			} else {
				// tr: Parameter 2 indicates the save type, e.g. "SRAM" or "EEPROM".
				fmtstr = C_("WonderSwan|SaveMemory", "%1$u bytes (%2$s)");
			}
			d->fields.addField_string(save_memory_title,
				rp_sprintf_p(fmtstr, eeprom_bytes, C_("WonderSwan|SaveMemory", "EEPROM")));
		}
	}

	// Features (aka RTC Present)
	static const array<const char*, 1> ws_feature_bitfield_names = {{
		NOP_C_("WonderSwan|Features", "RTC Present"),
	}};
	vector<string> *const v_ws_feature_bitfield_names = RomFields::strArrayToVector_i18n(
		"WonderSwan|Features", ws_feature_bitfield_names);
	d->fields.addField_bitfield(C_("WonderSwan", "Features"),
		v_ws_feature_bitfield_names, 0, romFooter->rtc_present);

	// Flags: Display orientation
	d->fields.addField_string(C_("WonderSwan", "Orientation"),
		((romFooter->flags & WS_FLAG_DISPLAY_MASK) == WS_FLAG_DISPLAY_VERTICAL)
			? C_("WonderSwan|Orientation", "Vertical")
			: C_("WonderSwan|Orientation", "Horizontal"));

	// Flags: Bus width
	d->fields.addField_string(C_("WonderSwan", "Bus Width"),
		((romFooter->flags & WS_FLAG_ROM_BUS_WIDTH_MASK) == WS_FLAG_ROM_BUS_WIDTH_8_BIT)
			? C_("WonderSwan|BusWidth", "8-bit")
			: C_("WonderSwan|BusWidth", "16-bit"));

	// Flags: ROM access speed
	d->fields.addField_string(C_("WonderSwan", "ROM Access Speed"),
		((romFooter->flags & WS_FLAG_ROM_ACCESS_SPEED_MASK) == WS_FLAG_ROM_ACCESS_SPEED_1_CYCLE)
			? C_("WonderSwan|ROMAccessSpeed", "1 cycle")
			: C_("WonderSwan|ROMAccessSpeed", "3 cycles"));

	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int WonderSwan::loadMetaData(void)
{
	RP_D(WonderSwan);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || static_cast<int>(d->romType) < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(1);	// Maximum of 1 metadata property.

	// WonderSwan ROM footer
	const WS_RomFooter *const romFooter = &d->romFooter;

	// Publisher
	const char *const publisher = WonderSwanPublishers::lookup_name(romFooter->publisher);
	if (publisher) {
		d->metaData->addMetaData_string(Property::Publisher, publisher);
	}

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
int WonderSwan::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	// "Pocket Challenge v2" ROMs don't have a publisher or
	// game ID set, so we can't get a title screen.
	RP_D(const WonderSwan);
	if (d->romFooter.publisher == 0 || d->romFooter.game_id == 0) {
		return -ENOENT;
	}

	// Get the game ID.
	const string game_id = d->getGameID();
	if (game_id.empty()) {
		// No game ID.
		return -ENOENT;
	}

	// NOTE: We only have one size for WonderSwan right now.
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

	// Subdirectory is 'C' for color or 'M' for original/mono.
	char subdir[2];
	subdir[0] = (d->romType == WonderSwanPrivate::RomType::Color) ? 'C' : 'M';
	subdir[1] = '\0';

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB("ws", imageTypeName, subdir, game_id.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("ws", imageTypeName, subdir, game_id.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

} // namespace LibRomData

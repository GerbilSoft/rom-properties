/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WonderSwan.cpp: Bandai WonderSwan (Color) ROM reader.                   *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WonderSwan.hpp"
#include "data/WonderSwanPublishers.hpp"
#include "ws_structs.h"

// librpbase, librpfile
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(WonderSwan)

class WonderSwanPrivate final : public RomDataPrivate
{
	public:
		WonderSwanPrivate(WonderSwan *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(WonderSwanPrivate)

	public:
		enum class RomType {
			Unknown	= -1,

			Original	= 0,	// WonderSwan
			Color		= 1,	// WonderSwan Color

			Max
		};
		RomType romType;

		// ROM footer.
		WS_RomFooter romFooter;

	public:
		/**
		 * Get the game ID. (SWJ-PUBx01)
		 * @return Game ID, or empty string on error.
		 */
		string getGameID(void) const;
};

/** WonderSwanPrivate **/

WonderSwanPrivate::WonderSwanPrivate(WonderSwan *q, IRpFile *file)
	: super(q, file)
	, romType(RomType::Unknown)
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
	const char *const publisher_code = WonderSwanPublishers::lookup_code(romFooter.publisher);
	if (!publisher_code) {
		// Invalid publisher code.
		return string();
	}

	char game_id[16];
	if (romType == WonderSwanPrivate::RomType::Original) {
		snprintf(game_id, sizeof(game_id), "SWJ-%s%03X",
			publisher_code, romFooter.game_id);
	} else {
		snprintf(game_id, sizeof(game_id), "SWJ-%sC%02X",
			publisher_code, romFooter.game_id);
	}

	return string(game_id);
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
WonderSwan::WonderSwan(IRpFile *file)
	: super(new WonderSwanPrivate(this, file))
{
	RP_D(WonderSwan);
	d->className = "WonderSwan";
	d->mimeType = "application/x-wonderswan-color-rom";

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the footer.
	const off64_t filesize = d->file->size();
	// File must be at least 1024 bytes,
	// and cannot be larger than 16 MB.
	if (filesize < 1024 || filesize > (16*1024*1024)) {
		// File size is out of range.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Read the ROM footer.
	const unsigned int footer_addr = static_cast<unsigned int>(filesize - sizeof(WS_RomFooter));
	d->file->seek(footer_addr);
	size_t size = d->file->read(&d->romFooter, sizeof(d->romFooter));
	if (size != sizeof(d->romFooter)) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// File extension is needed.
	const char *ext = nullptr;
	const string filename = file->filename();
	if (!filename.empty()) {
		ext = FileSystem::file_ext(filename);
	}
	if (!ext) {
		// Unable to get the file extension.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Make sure this is actually a WonderSwan ROM.
	DetectInfo info;
	info.header.addr = footer_addr;
	info.header.size = sizeof(d->romFooter);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romFooter);
	info.ext = ext;
	info.szFile = filesize;
	d->romType = static_cast<WonderSwanPrivate::RomType>(isRomSupported(&info));
	d->isValid = ((int)d->romType >= 0);

	if (!d->isValid) {
		UNREF_AND_NULL_NOCHK(d->file);
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

	// File extension must be ".ws" or ".wsc".
	// TODO: Gzipped ROMs?
	if (!info->ext || (strcasecmp(info->ext, ".ws") != 0 && strcasecmp(info->ext, ".wsc") != 0)) {
		// Not ".ws" or ".wsc".
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
	if (info->szFile < static_cast<int64_t>(sizeof(WS_RomFooter)))
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	const uint32_t header_addr_expected = static_cast<uint32_t>(info->szFile - sizeof(WS_RomFooter));
	if (info->header.addr > header_addr_expected)
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);
	else if (info->header.addr + info->header.size < header_addr_expected + sizeof(WS_RomFooter))
		return static_cast<int>(WonderSwanPrivate::RomType::Unknown);

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
	
	static const char *const sysNames[2][4] = {
		{"Bandai WonderSwan", "WonderSwan", "WS", nullptr},
		{"Bandai WonderSwan Color", "WonderSwan Color", "WSC", nullptr},
	};

	return sysNames[d->romFooter.system_id & 1][type & SYSNAME_TYPE_MASK];
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
const char *const *WonderSwan::supportedFileExtensions_static(void)
{
	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	static const char *const exts[] = {
		".ws",
		".wsc",

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
const char *const *WonderSwan::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types from FreeDesktop.org.
		"application/x-wonderswan-rom",
		"application/x-wonderswan-color-rom",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int WonderSwan::loadFieldData(void)
{
	RP_D(WonderSwan);
	if (!d->fields->empty()) {
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
	d->fields->reserve(10);	// Maximum of 10 fields.

	// Game ID
	const char *const game_id_title = C_("RomData", "Game ID");
	const string game_id = d->getGameID();
	if (!game_id.empty()) {
		d->fields->addField_string(game_id_title, game_id);
	} else {
		d->fields->addField_string(game_id_title, C_("WonderSwan", "None"));
	}

	// Revision
	d->fields->addField_string_numeric(C_("RomData", "Revision"), romFooter->revision);

	// Publisher
	const char *const publisher = WonderSwanPublishers::lookup_name(romFooter->publisher);
	string s_publisher;
	if (publisher) {
		s_publisher = publisher;
	} else {
		s_publisher = rp_sprintf(C_("RomData", "Unknown (%u)"), romFooter->publisher);
	}
	d->fields->addField_string(C_("RomData", "Publisher"), s_publisher);

	// System
	static const char *const system_bitfield_names[] = {
		"WonderSwan", "WonderSwan Color"
	};
	vector<string> *const v_system_bitfield_names = RomFields::strArrayToVector(
		system_bitfield_names, ARRAY_SIZE(system_bitfield_names));
	const uint32_t ws_system = (romFooter->system_id & 1 ? 3 : 1);
	d->fields->addField_bitfield(C_("WonderSwan", "System"),
		v_system_bitfield_names, 0, ws_system);

	// ROM size
	static const unsigned int rom_size_tbl[] = {
		128, 256, 512, 1024,
		2048, 3072, 4096, 6144,
		8192, 16384,
	};
	const char *const rom_size_title = C_("WonderSwan", "ROM Size");
	if (romFooter->rom_size < ARRAY_SIZE(rom_size_tbl)) {
		d->fields->addField_string(rom_size_title,
			// TODO: Format with commas?
			rp_sprintf(C_("WonderSwan", "%u KiB"), rom_size_tbl[romFooter->rom_size]));
	} else {
		d->fields->addField_string(rom_size_title,
			rp_sprintf(C_("RomData", "Unknown (%u)"), romFooter->publisher));
	}

	// Save size and type
	static const unsigned int sram_size_tbl[] = {
		0, 8, 32, 128, 256, 512,
	};
	const char *const save_memory_title = C_("WonderSwan", "Save Memory");
	if (romFooter->save_type == 0) {
		d->fields->addField_string(save_memory_title, C_("WonderSwan|SaveMemory", "None"));
	} else if (romFooter->save_type < ARRAY_SIZE(sram_size_tbl)) {
		d->fields->addField_string(save_memory_title,
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
			d->fields->addField_string(save_memory_title, C_("WonderSwan|SaveMemory", "None"));
		} else {
			const char *fmtstr;
			if (eeprom_bytes >= 1024) {
				fmtstr = C_("WonderSwan|SaveMemory", "%1$u KiB (%2$s)");
				eeprom_bytes /= 1024;
			} else {
				fmtstr = C_("WonderSwan|SaveMemory", "%1$u bytes (%2$s)");
			}
			d->fields->addField_string(save_memory_title,
				rp_sprintf_p(fmtstr, eeprom_bytes, C_("WonderSwan|SaveMemory", "EEPROM")));
		}
	}

	// Features (aka RTC Present)
	static const char *const ws_feature_bitfield_names[] = {
		NOP_C_("WonderSwan|Features", "RTC Present"),
	};
	vector<string> *const v_ws_feature_bitfield_names = RomFields::strArrayToVector_i18n(
		"WonderSwan|Features", ws_feature_bitfield_names, ARRAY_SIZE(ws_feature_bitfield_names));
	d->fields->addField_bitfield(C_("WonderSwan", "Features"),
		v_ws_feature_bitfield_names, 0, romFooter->rtc_present);

	// Flags: Display orientation
	d->fields->addField_string(C_("WonderSwan", "Orientation"),
		(romFooter->flags & WS_FLAG_DISPLAY_MASK)
			? C_("WonderSwan|Orientation", "Vertical")
			: C_("WonderSwan|Orientation", "Horizontal"));

	// Flags: Bus width
	d->fields->addField_string(C_("WonderSwan", "Bus Width"),
		(romFooter->flags & WS_FLAG_ROM_BUS_WIDTH_MASK)
			? C_("WonderSwan|BusWidth", "8-bit")
			: C_("WonderSwan|BusWidth", "16-bit"));

	// Flags: ROM access speed
	d->fields->addField_string(C_("WonderSwan", "ROM Access Speed"),
		(romFooter->flags & WS_FLAG_ROM_ACCESS_SPEED_MASK)
			? C_("WonderSwan|ROMAccessSpeed", "1 cycle")
			: C_("WonderSwan|ROMAccessSpeed", "3 cycles"));

	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
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
	} else if (!d->isValid || (int)d->romType < 0) {
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

}

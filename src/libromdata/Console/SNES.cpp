/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SNES.cpp: Super Nintendo ROM image reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SNES.hpp"
#include "data/NintendoPublishers.hpp"
#include "snes_structs.h"
#include "CopierFormats.h"

// librpbase, librpfile
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(SNES)
ROMDATA_IMPL_IMG(SNES)

class SNESPrivate : public RomDataPrivate
{
	public:
		SNESPrivate(SNES *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SNESPrivate)

	public:
		/**
		 * Is the specified SNES/SFC ROM header valid?
		 * @param romHeader SNES/SFC ROM header to check.
		 * @param isHiROM True if the header was read from a HiROM address; false if not.
		 * @return True if the SNES/SFC ROM header is valid; false if not.
		 */
		static bool isSnesRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM);

		/**
		 * Is the specified BS-X ROM header valid?
		 * @param romHeader BS-X ROM header to check.
		 * @param isHiROM True if the header was read from a HiROM address; false if not.
		 * @return True if the BS-X ROM header is valid; false if not.
		 */
		static bool isBsxRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM);

	public:
		enum SNES_RomType {
			ROM_UNKNOWN = -1,	// Unknown ROM type.
			ROM_SNES = 0,		// SNES/SFC ROM image.
			ROM_BSX = 1,		// BS-X ROM image.
		};

		// ROM type.
		int romType;

		// ROM header.
		// NOTE: Must be byteswapped on access.
		SNES_RomHeader romHeader;
		uint32_t header_address;

		/**
		 * Get the ROM title.
		 *
		 * The ROM title length depends on type, and encoding
		 * depends on type and region.
		 *
		 * @return ROM title.
		 */
		string getROMTitle(void) const;

		/**
		 * Is a character a valid game ID character?
		 * @return True if it is; false if it isn't.
		 */
		static inline bool isValidGameIDChar(char x)
		{
			return (x >= '0' && x <= '9') || (x >= 'A' && x <= 'Z');
		}

		/**
		 * Get the game ID.
		 * @param doFake If true, return a fake ID using the ROM's title.
		 * @return Game ID if available; empty string if not.
		 */
		string getGameID(bool doFake = false) const;
};

/** SNESPrivate **/

SNESPrivate::SNESPrivate(SNES *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
	, header_address(0)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Is the specified SNES/SFC ROM header valid?
 * @param romHeader SNES/SFC ROM header to check.
 * @param isHiROM True if the header was read from a HiROM address; false if not.
 * @return True if the SNES/SFC ROM header is valid; false if not.
 */
bool SNESPrivate::isSnesRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM)
{
	// Game title: Should be ASCII.
	// NOTE: Japanese ROMs may be Shift-JIS. (TODO: China, Korea?)
	// We're only disallowing control codes for now.
	// Check: Final Fantasy V - Expert v0.947 by JCE3000GT (Hack) [a1].smc
	// - Zero out the low 0x7F00 bytes.
	// - ROM is incorrectly detected as LoROM.
	for (int i = 0; i < ARRAY_SIZE(romHeader->snes.title); i++) {
		const uint8_t chr = static_cast<uint8_t>(romHeader->snes.title[i]);
		if (chr == 0) {
			if (i == 0) {
				// First character is NULL. Not allowed.
				return false;
			}
			break;
		}

		// Check for control characters.
		if (!(chr & 0xE0)) {
			// Control character. Not allowed.
			return false;
		}
	}

	// Is the ROM mapping byte valid?
	switch (romHeader->snes.rom_mapping) {
		case SNES_ROMMAPPING_LoROM:
		case SNES_ROMMAPPING_LoROM_S_DD1:
		case SNES_ROMMAPPING_LoROM_SA_1:
		case SNES_ROMMAPPING_LoROM_FastROM:
		case SNES_ROMMAPPING_ExLoROM_FastROM:
			if (isHiROM) {
				// LoROM mapping at a HiROM address.
				// Not valid.
				return false;
			}
			// Valid ROM mapping byte.
			break;

		case SNES_ROMMAPPING_HiROM:
		case SNES_ROMMAPPING_HiROM_FastROM:
		case SNES_ROMMAPPING_ExHiROM:
		case SNES_ROMMAPPING_ExHiROM_FastROM:
			if (!isHiROM) {
				// HiROM mapping at a LoROM address.
				// Not valid.
				return false;
			}

			// Valid ROM mapping byte.
			break;

		case 'A':
			// Some ROMs incorrectly extend the title into the mapping byte:
			// - WWF Super WrestleMania
			if (romHeader->snes.title[20] == 'I') {
				// Assume this ROM is valid.
				break;
			}
			// Not valid.
			return false;

		case 'E':
			// Some ROMs incorrectly extend the title into the mapping byte:
			// - Krusty's Super Fun House (some versions)
			// - Space Football - One on One
			if (romHeader->snes.title[20] == 'S' ||
			    romHeader->snes.title[20] == 'N')
			{
				// Assume this ROM is valid.
				break;
			}
			// Not valid.
			return false;

		case 'S':
			// Some ROMs incorrectly extend the title into the mapping byte:
			// - Contra III - The Alien Wars (U)
			if (romHeader->snes.title[20] == 'R') {
				// Assume this ROM is valid.
				break;
			}
			// Not valid.
			return false;

		default:
			// Not valid.
			return false;
	}

	// Is the ROM type byte valid?
	// TODO: Check if any other types exist.
	if ( (romHeader->snes.rom_type & SNES_ROMTYPE_ROM_MASK) > SNES_ROMTYPE_ROM_BATT_ENH ||
	    ((romHeader->snes.rom_type & SNES_ROMTYPE_ENH_MASK) >= 0x50 &&
	     (romHeader->snes.rom_type & SNES_ROMTYPE_ENH_MASK) <= 0xD0))
	{
		// Not a valid ROM type.
		return false;
	}

	// Check the extended header.
	if (romHeader->snes.old_publisher_code == 0x33) {
		// Extended header should be present.
		// New publisher code and game ID must be alphanumeric.
		if (!ISALNUM(romHeader->snes.ext.new_publisher_code[0]) ||
		    !ISALNUM(romHeader->snes.ext.new_publisher_code[1]))
		{
			// New publisher code is invalid.
			return false;
		}

		// Game ID must contain alphanumeric characters or a space.
		for (int i = 0; i < ARRAY_SIZE(romHeader->snes.ext.id4); i++) {
			// ID4 should be in the format "SMWJ" or "MW  ".
			if (ISALNUM(romHeader->snes.ext.id4[i])) {
				// Alphanumeric character.
				continue;
			} else if (i >= 2 && romHeader->snes.ext.id4[i] == ' ') {
				// Some game IDs are two characters,
				// and the remaining characters are spaces.
				continue;
			}

			// Invalid character.
			return false;
		}
	}

	// ROM header appears to be valid.
	return true;
}

/**
 * Is the specified BS-X ROM header valid?
 * @param romHeader BS-X ROM header to check.
 * @param isHiROM True if the header was read from a HiROM address; false if not.
 * @return True if the BS-X ROM header is valid; false if not.
 */
bool SNESPrivate::isBsxRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM)
{
	// TODO: Game title may be ASCII or Shift-JIS.
	// For now, just make sure the first byte isn't 0.
	if (romHeader->bsx.title[0] == 0) {
		// Title is empty.
		return false;
	}

	// Is the ROM mapping byte valid?
	switch (romHeader->bsx.rom_mapping) {
		case SNES_ROMMAPPING_LoROM:
		case SNES_ROMMAPPING_LoROM_S_DD1:
		case SNES_ROMMAPPING_LoROM_SA_1:
		case SNES_ROMMAPPING_LoROM_FastROM:
			if (isHiROM) {
				// LoROM mapping at a HiROM address.
				// Not valid.
				return false;
			}
			// Valid ROM mapping byte.
			break;

		case SNES_ROMMAPPING_HiROM:
		case SNES_ROMMAPPING_HiROM_FastROM:
			if (!isHiROM) {
				// HiROM mapping at a LoROM address.
				// Not valid.
				return false;
			}

			// Valid ROM mapping byte.
			break;

		case SNES_ROMMAPPING_ExHiROM:
		case SNES_ROMMAPPING_ExLoROM_FastROM:
		case SNES_ROMMAPPING_ExHiROM_FastROM:
		default:
			// Not valid.
			// (ExLoROM/ExHiROM is not valid for BS-X.)
			return false;
	}

	// Old publisher code must be either 0x33 or 0x00.
	// 0x00 indicates the file was deleted.
	if (romHeader->bsx.old_publisher_code != 0x33 &&
	    romHeader->bsx.old_publisher_code != 0x00)
	{
		// Invalid old publisher code.
		return false;
	}

	// New publisher code must be alphanumeric.
	if (!ISALNUM(romHeader->bsx.ext.new_publisher_code[0]) ||
	    !ISALNUM(romHeader->bsx.ext.new_publisher_code[1]))
	{
		// New publisher code is invalid.
		return false;
	}

	// ROM header appears to be valid.
	// TODO: Check other BS-X fields.
	return true;
}

/**
 * Get the ROM title.
 *
 * The ROM title length depends on type, and encoding
 * depends on type and region.
 *
 * @return ROM title.
 */
string SNESPrivate::getROMTitle(void) const
{
	// NOTE: If the region code is JPN, the title might be encoded in Shift-JIS.
	// TODO: Space elimination; China, Korea encodings?
	// TODO: Remove leading spaces? (Capcom NFL Football; symlinked on the server for now.)

	bool doSJIS = false;
	const char *title;
	size_t len;
	switch (romType) {
		case ROM_SNES:
			doSJIS = (romHeader.snes.destination_code == SNES_DEST_JAPAN);
			title = romHeader.snes.title;
			len = sizeof(romHeader.snes.title);
			break;
		case ROM_BSX:
			doSJIS = true;
			title = romHeader.bsx.title;
			len = sizeof(romHeader.bsx.title);
			break;
		default:
			// Should not get here...
			assert(!"Invalid ROM type.");
			break;
	}

	// Trim the end of the title before converting it.
	bool done = false;
	while (!done && (len > 0)) {
		switch (static_cast<uint8_t>(title[len-1])) {
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

	string s_title;
	if (doSJIS) {
		s_title = cp1252_sjis_to_utf8(title, static_cast<int>(len));
	} else {
		s_title = cp1252_to_utf8(title, static_cast<int>(len));
	}
	return s_title;
}

/**
 * Get the game ID.
 * This returns a *full* game ID if available, e.g. SNS-YI-USA.
 * @param doFake If true, return a fake ID using the ROM's title.
 * @return Game ID if available; empty string if not.
 */
string SNESPrivate::getGameID(bool doFake) const
{
	string gameID;

	// Game ID is only available for SNES, not BS-X.
	// TODO: Are we sure this is the case?
	if (romType != ROM_SNES && !doFake) {
		return gameID;
	}

	char id4[5];
	id4[0] = '\0';

	// NOTE: The game ID field is Only valid if the old publisher code is 0x33.
	if (romHeader.snes.old_publisher_code == 0x33) {
		// Do we have a valid two-digit game ID?
		if (isValidGameIDChar(romHeader.snes.ext.id4[0]) &&
		    isValidGameIDChar(romHeader.snes.ext.id4[1]))
		{
			// Valid two-digit game ID.
			id4[0] = romHeader.snes.ext.id4[0];
			id4[1] = romHeader.snes.ext.id4[1];
			id4[2] = '\0';

			// Do we have a valid four-digit game ID?
			if (isValidGameIDChar(romHeader.snes.ext.id4[2]) &&
			    isValidGameIDChar(romHeader.snes.ext.id4[3]))
			{
				// Valid four-digit game ID.
				id4[2] = romHeader.snes.ext.id4[2];
				id4[3] = romHeader.snes.ext.id4[3];
				id4[4] = '\0';
			}
		}
	}

	// Check the region value to determine the template.
	// NOTE: BS-X might have BRA for some reason.
	const char *prefix, *suffix;
	const uint8_t region = ((romType != ROM_BSX)
		? romHeader.snes.destination_code
		: static_cast<uint8_t>(SNES_DEST_JAPAN));

	// Prefix/suffix table.
	struct PrefixSuffixTbl_t {
		char prefix[8];
		char suffix[8];
	};
	static const PrefixSuffixTbl_t region_ps[] = {
		// 0x00
		{"SHVC-", "-JPN"},	// Japan
		{"SNS-",  "-USA"},	// North America
		{"SNSP-", "-EUR"},	// Europe
		{"SNSP-", "-SCN"},	// Scandinavia
		{"",      ""},
		{"",      ""},
		{"SNSP-", "-FRA"},	// France
		{"SNSP-", "-HOL"},	// Netherlands

		// 0x08
		{"SNSP-", "-ESP"},	// Spain
		{"SNSP-", "-NOE"},	// Germany
		{"SNSP-", "-ITA"},	// Italy
		{"SNSN-", "-ROC"},	// China
		{"",      ""},
		{"SNSN-", "-KOR"},	// South Korea
		{"",      ""},		// ALL region?
		{"SNS-",  "-CAN"},	// Canada

		// 0x10
		{"SNS-",  "-BRA"},	// Brazil
		{"SNSP-", "-AUS"},	// Australia
		{"SNSP-", "-SCN"},	// Scandinavia
	};
	if (romType == ROM_BSX) {
		// Separate BS-X titles from regular SNES titles.
		prefix = "BSX-";
		suffix = "-JPN";
	} else if (region < ARRAY_SIZE(region_ps)) {
		prefix = region_ps[region].prefix;
		suffix = region_ps[region].suffix;
	} else {
		prefix = "";
		suffix = "";
	}

	// Do we have an ID2 or ID4?
	if (id4[0] != '\0') {
		// ID2/ID4 is present. Use it.
		gameID.reserve(13);
		gameID = prefix;
		gameID += id4;
		gameID += suffix;
	} else {
		// ID2/ID4 is not present. Use the ROM title.
		string s_title = getROMTitle();
		if (s_title.empty()) {
			// No title...
			return gameID;
		}

		// Manually filter out characters that are rejected by CacheKeys.
		for (size_t n = 0; n < s_title.size(); n++) {
			switch (s_title[n]) {
				case ':':
				case '/':
				case '\\':
				case '*':
				case '?':
					s_title[n] = '_';
					break;
				default:
					break;
			}
		}

		gameID.reserve(5 + s_title.size() + 4);
		gameID = prefix;
		gameID += s_title;
		gameID += suffix;
	}

	return gameID;
}

/** SNES **/

/**
 * Read a Super Nintendo ROM image.
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
SNES::SNES(IRpFile *file)
	: super(new SNESPrivate(this, file))
{
	RP_D(SNES);
	d->className = "SNES";
	d->mimeType = "application/vnd.nintendo.snes.rom";	// vendor-specific

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// TODO: Only allow supported file extensions.
	bool isCopierHeader = false;

	// TODO: BS-X heuristics.
	// For now, assuming that if the file extension starts with
	// ".b", it's a BS-X ROM image.
	const string filename = file->filename();
	if (!filename.empty()) {
		const char *const ext = FileSystem::file_ext(filename);
		if (ext && ext[0] == '.' && tolower(ext[1]) == 'b') {
			// BS-X ROM image.
			d->romType = SNESPrivate::ROM_BSX;
		}
	}

	if (d->romType == SNESPrivate::ROM_UNKNOWN) {
		// Check for BS-X "Memory Pack" headers.
		static const uint16_t bsx_addrs[2] = {0x7F00, 0xFF00};
		static const uint8_t bsx_mempack_magic[6] = {'M', 0, 'P', 0, 0, 0};
		uint8_t buf[7];

		for (unsigned int i = 0; i < 2; i++) {
			size_t size = d->file->seekAndRead(bsx_addrs[i], buf, sizeof(buf));
			if (size != sizeof(buf)) {
				// Read error.
				d->file->unref();
				d->file = nullptr;
				return;
			}

			if (!memcmp(buf, bsx_mempack_magic, sizeof(bsx_mempack_magic))) {
				// Found BS-X memory pack magic.
				// Check the memory pack type.
				// (7 is ROM; 1 to 4 is FLASH.)
				if ((buf[6] & 0xF0) == 0x70) {
					// ROM cartridge
					// TODO: Use the size value.
					// Size is (1024 << (buf[6] & 0x0F))
					d->romType = SNESPrivate::ROM_BSX;
					break;
				}
			}
		}
	}

	if (d->romType == SNESPrivate::ROM_UNKNOWN) {
		// SNES ROMs don't necessarily have a header at the start of the file.
		// Therefore, we need to do a few reads and guessing.

		// Check if a copier header is present.
		SMD_Header smdHeader;
		d->file->rewind();
		size_t size = d->file->read(&smdHeader, sizeof(smdHeader));
		if (size != sizeof(smdHeader)) {
			d->file->unref();
			d->file = nullptr;
			return;
		}

		if (smdHeader.id[0] == 0xAA && smdHeader.id[1] == 0xBB) {
			// TODO: Check page count?

			// Check that reserved fields are 0.
			bool areFieldsZero = true;
			for (int i = ARRAY_SIZE(smdHeader.reserved1)-1; i >= 0; i--) {
				if (smdHeader.reserved1[i] != 0) {
					areFieldsZero = false;
					break;
				}
			}
			if (!areFieldsZero) {
				for (int i = ARRAY_SIZE(smdHeader.reserved2)-1; i >= 0; i--) {
					if (smdHeader.reserved2[i] != 0) {
						areFieldsZero = false;
						break;
					}
				}
			}

			// If the fields are zero, this is a copier header.
			isCopierHeader = areFieldsZero;
		}

		if (!isCopierHeader) {
			// Check for "GAME DOCTOR SF ".
			// (UCON64 uses "GAME DOCTOR SF 3", but there's multiple versions.)
			static const char gdsf3[] = "GAME DOCTOR SF ";
			if (!memcmp(&smdHeader, gdsf3, sizeof(gdsf3)-1)) {
				// Game Doctor ROM header.
				isCopierHeader = true;
			} else {
				// Check for "SUPERUFO".
				static const char superufo[] = "SUPERUFO";
				const uint8_t *u8ptr = reinterpret_cast<const uint8_t*>(&smdHeader);
				if (!memcmp(&u8ptr[8], superufo, sizeof(superufo)-1)) {
					// Super UFO ROM header.
					isCopierHeader = true;
				}
			}
		}
	}

	// Header addresses to check.
	// If a copier header is detected, use index 1,
	// which checks +512 offsets first.
	static const uint32_t all_header_addresses[2][4] = {
		// Non-headered first.
		{0x7FB0, 0xFFB0, 0x7FB0+512, 0xFFB0+512},
		// Headered first.
		{0x7FB0+512, 0xFFB0+512, 0x7FB0, 0xFFB0},
	};

	d->header_address = 0;
	const uint32_t *pHeaderAddress = &all_header_addresses[isCopierHeader][0];
	for (int i = 0; i < 4; i++, pHeaderAddress++) {
		if (*pHeaderAddress == 0)
			break;

		size_t size = d->file->seekAndRead(*pHeaderAddress, &d->romHeader, sizeof(d->romHeader));
		if (size != sizeof(d->romHeader)) {
			// Seek and/or read error.
			continue;
		}

		if (d->romType == SNESPrivate::ROM_BSX) {
			// Check for a valid BS-X ROM header first.
			if (d->isBsxRomHeaderValid(&d->romHeader, (i & 1))) {
				// BS-X ROM header is valid.
				d->header_address = *pHeaderAddress;
				break;
			} else if (d->isSnesRomHeaderValid(&d->romHeader, (i & 1))) {
				// SNES/SFC ROM header is valid.
				d->header_address = *pHeaderAddress;
				d->romType = SNESPrivate::ROM_SNES;
				break;
			}
		} else {
			// Check for a valid SNES/SFC ROM header.
			if (d->isSnesRomHeaderValid(&d->romHeader, (i & 1))) {
				// SNES/SFC ROM header is valid.
				d->header_address = *pHeaderAddress;
				d->romType = SNESPrivate::ROM_SNES;
				break;
			} else if (d->isBsxRomHeaderValid(&d->romHeader, (i & 1))) {
				// BS-X ROM header is valid.
				d->header_address = *pHeaderAddress;
				d->romType = SNESPrivate::ROM_BSX;
				break;
			}
		}
	}

	if (d->header_address == 0) {
		// No ROM header.
		d->file->unref();
		d->file = nullptr;
		d->romType = SNESPrivate::ROM_UNKNOWN;
		d->isValid = false;
		return;
	}

	// ROM header found.
	d->isValid = true;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SNES::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	if (!info) {
		// Either no detection information was specified,
		// or the file extension is missing.
		return -1;
	}

	// SNES ROMs don't necessarily have a header at the start of the file.
	// Therefore, we're using the file extension.
	if (info->ext && info->ext[0] != 0) {
		const char *const *exts = supportedFileExtensions_static();
		if (!exts) {
			// Should not happen...
			return SNESPrivate::ROM_UNKNOWN;
		}
		for (; *exts != nullptr; exts++) {
			if (!strcasecmp(info->ext, *exts)) {
				// File extension is supported.
				if (*exts[1] == 'b') {
					// BS-X extension.
					return SNESPrivate::ROM_BSX;
				} else {
					// SNES/SFC extension.
					return SNESPrivate::ROM_SNES;
				}
			}
		}

		// Extra check for ".ic1", used by MAME for Nintendo Super System.
		if (!strcasecmp(info->ext, ".ic1")) {
			// File extension is supported.
			// TODO: Special handling for NSS?
			return SNESPrivate::ROM_SNES;
		}
	}

	// TODO: BS-X heuristics.

	if (info->header.addr == 0 && info->header.size >= 0x200) {
		// Check for "GAME DOCTOR SF ".
		// (UCON64 uses "GAME DOCTOR SF 3", but there's multiple versions.)
		static const char gdsf3[] = "GAME DOCTOR SF ";
		if (!memcmp(info->header.pData, gdsf3, sizeof(gdsf3)-1)) {
			// Game Doctor ROM header.
			return 0;
		}

		// Check for "SUPERUFO".
		static const char superufo[] = "SUPERUFO";
		if (!memcmp(&info->header.pData[8], superufo, sizeof(superufo)-8)) {
			// Super UFO ROM header.
			return 0;
		}
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *SNES::systemName(unsigned int type) const
{
	RP_D(const SNES);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	static_assert(SYSNAME_TYPE_MASK == 3,
		"SNES::systemName() array index optimization needs to be updated.");

	// sysNames[] bitfield:
	// - Bits 0-1: Type. (long, short, abbreviation)
	// - Bits 2-3: Region.
	unsigned int idx = (type & SYSNAME_TYPE_MASK);

	// Localized SNES/SFC system names.
	static const char *const sysNames[16] = {
		// Japan: Super Famicom
		"Nintendo Super Famicom", "Super Famicom", "SFC", nullptr,
		// South Korea: Super Comboy
		"Hyundai Super Comboy", "Super Comboy", "SCB", nullptr,
		// Worldwide: Super NES
		"Super Nintendo Entertainment System", "Super NES", "SNES", nullptr,
		// Reserved.
		nullptr, nullptr, nullptr, nullptr
	};

	// BS-X system names.
	static const char *const sysNames_BSX[4] = {
		"Satellaview BS-X", "Satellaview", "BS-X", nullptr
	};

	switch (d->romType) {
		case SNESPrivate::ROM_SNES:
			// SNES/SFC ROM image. Handled later.
			break;
		case SNESPrivate::ROM_BSX:
			// BS-X was only released in Japan, so no
			// localization is necessary.
			return sysNames_BSX[idx];
		default:
			// Should not get here...
			assert(!"Invalid ROM type.");
			return nullptr;
	}

	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_ROM_LOCAL) {
		// Localized region name is requested.
		bool foundRegion = false;
		switch (d->romHeader.snes.destination_code) {
			case SNES_DEST_JAPAN:
				idx |= (0U << 2);
				foundRegion = true;
				break;
			case SNES_DEST_SOUTH_KOREA:
				idx |= (1U << 2);
				foundRegion = true;
				break;

			case SNES_DEST_ALL:
			case SNES_DEST_OTHER_X:
			case SNES_DEST_OTHER_Y:
			case SNES_DEST_OTHER_Z:
				// Use the system locale.
				break;

			default:
				if (d->romHeader.snes.destination_code <= SNES_DEST_AUSTRALIA) {
					idx |= (2 << 2);
					foundRegion = true;
				}
				break;
		}

		if (!foundRegion) {
			// Check the system locale.
			switch (SystemRegion::getCountryCode()) {
				case 'JP':
					idx |= (0U << 2);
					break;
				case 'KR':
					idx |= (1U << 2);
					break;
				default:
					idx |= (2U << 2);
					break;
			}
		}
	}

	return sysNames[idx];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *SNES::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".smc", ".swc", ".sfc",
		".fig", ".ufo", ".mgd",

		// BS-X
		".bs", ".bsx",

		// Nintendo Super System (MAME) (TODO)
		//".ic1",

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
const char *const *SNES::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Vendor-specific MIME types from FreeDesktop.org.
		"application/vnd.nintendo.snes.rom",

		// Unofficial MIME types from FreeDesktop.org.
		"application/x-snes-rom",

		nullptr
	};
	return mimeTypes;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t SNES::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> SNES::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			// NOTE: Some images might use high-resolution mode.
			static const ImageSizeDef sz_EXT_TITLE_SCREEN[] = {
				{nullptr, 256, 224, 0},
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
uint32_t SNES::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Use nearest-neighbor scaling when resizing.
			// FIXME: Add 256->320 / 512->640 rescaling.
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
int SNES::loadFieldData(void)
{
	RP_D(SNES);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown save file type.
		return -EIO;
	}

	// ROM file header is read and byteswapped in the constructor.
	const SNES_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(8); // Maximum of 8 fields.

	// Cartridge HW.
	// TODO: Make this translatable.
	static const char *const hw_base_tbl[16] = {
		"ROM", "ROM, RAM", "ROM, RAM, Battery",
		"ROM, ", "ROM, RAM, ", "ROM, RAM, Battery, ",
		"ROM, Battery, ", nullptr,

		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr
	};
	static const char *const hw_enh_tbl[16] = {
		"DSP-1", "Super FX", "OBC-1", "SA-1",
		"S-DD1", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Other", "Custom Chip"
	};

	string cart_hw;
	const char *publisher = nullptr;
	uint8_t rom_mapping;

	// Get the field data based on ROM type.
	switch (d->romType) {
		case SNESPrivate::ROM_SNES: {
			// Super NES / Super Famicom ROM image.

			// Publisher.
			if (romHeader->snes.old_publisher_code == 0x33) {
				publisher = NintendoPublishers::lookup(romHeader->snes.ext.new_publisher_code);
			} else {
				publisher = NintendoPublishers::lookup_old(romHeader->snes.old_publisher_code);
			}

			// ROM mapping.
			rom_mapping = romHeader->snes.rom_mapping;

			// Some ROMs incorrectly extend the title into the mapping byte.
			// TODO: Extend the title based on this?
			switch (rom_mapping) {
				case 'A':
					if (romHeader->snes.title[20] == 'I' && d->header_address < 0x8000) {
						// WWF Super WrestleMania
						// Assume LoROM. (TODO: Is it FastROM?)
						rom_mapping = SNES_ROMMAPPING_LoROM;
					}
					break;
				case 'E':
					if ((romHeader->snes.title[20] == 'S' || romHeader->snes.title[20] == 'N') &&
					     d->header_address < 0x8000)
					{
						// Krusty's Super Fun House
						// Space Football - One on One
						// Assume LoROM. (TODO: Is it FastROM?)
						rom_mapping = SNES_ROMMAPPING_LoROM;
					}
					break;

				case 'S':
					if (romHeader->snes.title[20] == 'R' && d->header_address < 0x8000) {
						// Contra III - The Alien Wars (U)
						// Assume LoROM. (TODO: Is it FastROM?)
						break;
					}
					break;

				default:
					break;
			}

			// Cartridge HW.
			const char *const hw_base = hw_base_tbl[romHeader->snes.rom_type & SNES_ROMTYPE_ROM_MASK];
			if (hw_base) {
				cart_hw = hw_base;
				if ((romHeader->snes.rom_type & SNES_ROMTYPE_ROM_MASK) >= SNES_ROMTYPE_ROM_ENH) {
					// Enhancement chip.
					cart_hw += hw_enh_tbl[(romHeader->snes.rom_type & SNES_ROMTYPE_ENH_MASK) >> 4];
				}
			} else {
				// Unknown cartridge HW.
				cart_hw = C_("RomData", "Unknown");
			}

			break;
		}

		case SNESPrivate::ROM_BSX: {
			// Satellaview BS-X ROM image.

			// Publisher.
			// NOTE: Old publisher code is always 0x33 or 0x00,
			// so use the new publisher code.
			publisher = NintendoPublishers::lookup(romHeader->bsx.ext.new_publisher_code);

			// ROM mapping.
			rom_mapping = romHeader->bsx.rom_mapping;

			break;
		}

		default:
			// Should not get here...
			assert(!"Invalid ROM type.");
			return 0;
	}

	/** Add the field data. **/

	// Title
	d->fields->addField_string(C_("SNES", "Title"), d->getROMTitle());

	// Game ID
	const char *const game_id_title = C_("SNES", "Game ID");
	string gameID = d->getGameID();
	if (!gameID.empty()) {
		d->fields->addField_string(game_id_title, gameID);
	} else if (d->romType == SNESPrivate::ROM_SNES) {
		// Unknown game ID.
		d->fields->addField_string(game_id_title, C_("RomData", "Unknown"));
	}

	// Publisher
	// TODO: Print the publisher code if the lookup returns nullptr.
	d->fields->addField_string(C_("RomData", "Publisher"),
		publisher ? publisher : C_("RomData", "Unknown"));

	// ROM mapping
	// NOTE: Not translatable!
	static const struct {
		uint8_t rom_mapping;
		const char *s_rom_mapping;
	} rom_mapping_tbl[] = {
		{SNES_ROMMAPPING_LoROM,			"LoROM"},
		{SNES_ROMMAPPING_HiROM,			"HiROM"},
		{SNES_ROMMAPPING_LoROM_S_DD1,		"LoROM + S-DD1"},
		{SNES_ROMMAPPING_LoROM_SA_1,		"LoROM + SA-1"},
		{SNES_ROMMAPPING_ExHiROM,		"ExHiROM"},
		{SNES_ROMMAPPING_LoROM_FastROM,		"LoROM + FastROM"},
		{SNES_ROMMAPPING_HiROM_FastROM,		"HiROM + FastROM"},
		{SNES_ROMMAPPING_ExLoROM_FastROM,	"ExLoROM + FastROM"},
		{SNES_ROMMAPPING_ExHiROM_FastROM,	"ExHiROM + FastROM"},

		{0, nullptr}
	};

	const char *s_rom_mapping = nullptr;
	for (const auto *p = rom_mapping_tbl; p->rom_mapping != 0; p++) {
		if (p->rom_mapping == rom_mapping) {
			// Found a match.
			s_rom_mapping = p->s_rom_mapping;
			break;
		}
	}

	const char *const rom_mapping_title = C_("SNES", "ROM Mapping");
	if (s_rom_mapping) {
		d->fields->addField_string(rom_mapping_title, s_rom_mapping);
	} else {
		// Unknown ROM mapping.
		d->fields->addField_string(rom_mapping_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), rom_mapping));
	}

	// Cartridge HW
	if (!cart_hw.empty()) {
		d->fields->addField_string(C_("SNES", "Cartridge HW"), cart_hw);
	}

	// Region
	// NOTE: Not listed for BS-X because BS-X is Japan only.
	static const char *const region_tbl[0x15] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "North America"),
		NOP_C_("Region", "Europe"),
		NOP_C_("Region", "Scandinavia"),
		nullptr, nullptr,
		NOP_C_("Region", "France"),
		NOP_C_("Region", "Netherlands"),
		NOP_C_("Region", "Spain"),
		NOP_C_("Region", "Germany"),
		NOP_C_("Region", "Italy"),
		NOP_C_("Region", "China"),
		nullptr,
		NOP_C_("Region", "South Korea"),
		NOP_C_("Region", "All"),
		NOP_C_("Region", "Canada"),
		NOP_C_("Region", "Brazil"),
		NOP_C_("Region", "Australia"),
		NOP_C_("Region", "Other"),
		NOP_C_("Region", "Other"),
		NOP_C_("Region", "Other"),
	};
	const char *const region_lkup = (romHeader->snes.destination_code < ARRAY_SIZE(region_tbl)
					? region_tbl[romHeader->snes.destination_code]
					: nullptr);

	switch (d->romType) {
		case SNESPrivate::ROM_SNES: {
			// Region
			const char *const region_title = C_("RomData", "Region Code");
			if (region_lkup) {
				d->fields->addField_string(region_title,
					dpgettext_expr(RP_I18N_DOMAIN, "Region", region_lkup));
			} else {
				d->fields->addField_string(region_title,
					rp_sprintf(C_("RomData", "Unknown (0x%02X)"),
						romHeader->snes.destination_code));
			}

			// Revision
			d->fields->addField_string_numeric(C_("SNES", "Revision"),
				romHeader->snes.version, RomFields::FB_DEC, 2);

			break;
		}

		case SNESPrivate::ROM_BSX: {
			// Date
			// Verify that the date field is valid.
			// NOTE: Not verifying the low bits. (should be 0)
			time_t unixtime = -1;
			const uint8_t month = romHeader->bsx.date.month >> 4;
			const uint8_t day = romHeader->bsx.date.day >> 3;
			if (month > 0 && month <= 12 && day > 0 && day <= 31) {
				// Date field is valid.
				// Convert to Unix time.
				// NOTE: struct tm has some oddities:
				// - tm_year: year - 1900
				// - tm_mon: 0 == January
				struct tm bsxtime;
				bsxtime.tm_year = 1980 - 1900;	// Use 1980 to make errors more obvious.
				bsxtime.tm_mon = month - 1;
				bsxtime.tm_mday = day;

				bsxtime.tm_hour = 0;
				bsxtime.tm_min = 0;
				bsxtime.tm_sec = 0;

				// tm_wday and tm_yday are output variables.
				bsxtime.tm_wday = 0;
				bsxtime.tm_yday = 0;
				bsxtime.tm_isdst = 0;

				// Convert to Unix time.
				// If this fails, unixtime will be equal to -1.
				unixtime = timegm(&bsxtime);
			}

			d->fields->addField_dateTime(C_("SNES", "Date"), unixtime,
				RomFields::RFT_DATETIME_HAS_DATE |	// Date only.
				RomFields::RFT_DATETIME_IS_UTC   |	// No timezone.
				RomFields::RFT_DATETIME_NO_YEAR		// No year.
			);

			// Program type
			const char *program_type;
			switch (le32_to_cpu(romHeader->bsx.ext.program_type)) {
				case SNES_BSX_PRG_65c816:
					program_type = "65c816";
					break;
				case SNES_BSX_PRG_SCRIPT:
					program_type = C_("SNES|ProgramType", "BS-X Script");
					break;
				case SNES_BSX_PRG_SA_1:
					program_type = C_("SNES|ProgramType", "SA-1");
					break;
				default:
					program_type = nullptr;
					break;
			}
			const char *const program_type_title = C_("SNES", "Program Type");
			if (program_type) {
				d->fields->addField_string(program_type_title,
					dpgettext_expr(RP_I18N_DOMAIN, "SNES|ProgramType", program_type));
			} else {
				d->fields->addField_string(program_type_title,
					rp_sprintf(C_("RomData", "Unknown (0x%08X)"),
						le32_to_cpu(romHeader->bsx.ext.program_type)));
			}

			// TODO: block_alloc

			// Limited starts
			// Bit 15: 0 for unlimited; 1 for limited.
			// If limited:
			// - Bits 14-0: 1 for playthrough allowed, 0 for not.
			// - Each bit counts as a playthrough, so up to 15
			//   plays are possible. After bootup, the bits are
			//   cleared in MSB to LSB order.
			const char *const limited_starts_title = C_("SNES", "Limited Starts");
			const uint16_t limited_starts = le16_to_cpu(romHeader->bsx.limited_starts);
			if (limited_starts & 0x8000U) {
				// Limited.
				const unsigned int n = popcount(limited_starts & ~0x8000U);
				d->fields->addField_string_numeric(limited_starts_title, n);
			} else {
				// Unlimited.
				d->fields->addField_string(limited_starts_title, C_("SNES", "Unlimited"));
			}

			// TODO: Show region == Japan?
			// (Implied by BS-X, which was only released in Japan.)
			break;
		}

		default:
			// Should not get here...
			assert(!"Invalid ROM type.");
			return 0;
	}


	// TODO: Other fields.

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
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
int SNES::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const SNES);
	if (!d->isValid || d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// Determine the region code based on the destination code.
	char region_code[4] = {'\0', '\0', '\0', '\0'};
	static const char RegionCode_tbl[] = {
		'J', 'E', 'P', 'X', '\0', '\0', 'F', 'H',
		'S', 'D', 'I', 'C', '\0',  'K', 'A', 'N',
		'B', 'U', 'X', 'Y', 'Z'
	};
	if (d->romType == SNESPrivate::ROM_BSX) {
		// BS-X. Use a separate "region".
		region_code[0] = 'B';
		region_code[1] = 'S';
	} else if (d->romHeader.snes.destination_code < ARRAY_SIZE(RegionCode_tbl)) {
		// SNES region code is in range.
		region_code[0] = RegionCode_tbl[d->romHeader.snes.destination_code];
	} else {
		// Unable to determine the region code.
		// Assume a default value.
		region_code[0] = 'U';
		region_code[1] = 'n';
		region_code[2] = 'k';
	}

	if (region_code[0] == '\0') {
		// Invalid region code.
		return -ENOENT;
	}

	// Get the game ID.
	string gameID = d->getGameID(true);
	if (gameID.empty()) {
		// No game ID. Image is not available.
		return -ENOENT;
	}

	// NOTE: We only have one size for SNES right now.
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
	extURL_iter->url = d->getURL_RPDB("snes", imageTypeName, region_code, gameID.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB("snes", imageTypeName, region_code, gameID.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

}

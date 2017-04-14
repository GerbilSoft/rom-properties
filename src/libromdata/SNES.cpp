/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SNES.cpp: Super Nintendo ROM image reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include "SNES.hpp"
#include "RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "snes_structs.h"
#include "CopierFormats.h"
#include "SystemRegion.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class SNESPrivate : public RomDataPrivate
{
	public:
		SNESPrivate(SNES *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SNESPrivate)

	public:
		/**
		 * Is the specified ROM header valid?
		 * @param romHeader SNES ROM header to check.
		 * @param isHiROM True if the header was read from a HiROM address; false if not.
		 * @return True if the ROM header is valid; false if not.
		 */
		static bool isRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM);

	public:
		// ROM header.
		// NOTE: Must be byteswapped on access.
		SNES_RomHeader romHeader;
		uint32_t header_address;
};

/** SNESPrivate **/

SNESPrivate::SNESPrivate(SNES *q, IRpFile *file)
	: super(q, file)
	, header_address(0)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Is the specified ROM header valid?
 * @param romHeader SNES ROM header to check.
 * @param isHiROM True if the header was read from a HiROM address; false if not.
 * @return True if the ROM header is valid; false if not.
 */
bool SNESPrivate::isRomHeaderValid(const SNES_RomHeader *romHeader, bool isHiROM)
{
	// Game title: Should be ASCII.
	for (int i = 0; i < ARRAY_SIZE(romHeader->title); i++) {
		if (romHeader->title[i] & 0x80) {
			// Invalid character.
			return false;
		}
	}

	// Is the ROM mapping byte valid?
	switch (romHeader->rom_mapping) {
		case SNES_ROMMAPPING_LoROM:
		case SNES_ROMMAPPING_LoROM_FastROM:
		case SNES_ROMMAPPING_ExLoROM:
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
			if (!isHiROM) {
				// HiROM mapping at a LoROM address.
				// Not valid.
				return false;
			}

			// Valid ROM mapping byte.
			break;

		default:
			// Not valid.
			return false;
	}

	// Is the ROM type byte valid?
	// TODO: Check if any other types exist.
	if ( (romHeader->rom_type & SNES_ROMTYPE_ROM_MASK) > SNES_ROMTYPE_ROM_BATT_ENH ||
	    ((romHeader->rom_type & SNES_ROMTYPE_ENH_MASK) >= 0x50 &&
	     (romHeader->rom_type & SNES_ROMTYPE_ENH_MASK) <= 0xD0))
	{
		// Not a valid ROM type.
		return false;
	}

	// Check the extended header.
	if (romHeader->old_publisher_code == 0x33) {
		// Extended header should be present.
		// New publisher code and game ID must be alphanumeric.
		if (!isalnum(romHeader->ext.new_publisher_code[0]) ||
		    !isalnum(romHeader->ext.new_publisher_code[1]))
		{
			// New publisher code is invalid.
			return false;
		}

		// Game ID must contain alphanumeric characters or a space.
		for (int i = 0; i < ARRAY_SIZE(romHeader->ext.id4); i++) {
			// ID4 should be in the format "SMWJ" or "MW  ".
			if (isalnum(romHeader->ext.id4[i])) {
				// Alphanumeric character.
				continue;
			} else if (romHeader->ext.id4[i] == ' ' && i >= 2) {
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

/** SNES **/

/**
 * Read a Super Nintendo ROM image.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
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
	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// TODO: Only allow supported file extensions.

	// SNES ROMs don't necessarily have a header at the start of the file.
	// Therefore, we need to do a few reads and guessing.

	// Check if a copier header is present.
	SMD_Header smdHeader;
	d->file->rewind();
	size_t size = d->file->read(&smdHeader, sizeof(smdHeader));
	if (size != sizeof(smdHeader))
		return;

	bool isCopierHeader = false;
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
			if (!memcmp(&u8ptr[8], superufo, sizeof(superufo)-8)) {
				// Super UFO ROM header.
				isCopierHeader = true;
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

		d->file->seek(*pHeaderAddress);
		size = d->file->read(&d->romHeader, sizeof(d->romHeader));
		if (size != sizeof(d->romHeader))
			continue;

		if (d->isRomHeaderValid(&d->romHeader, (i & 1))) {
			// ROM header is valid.
			d->header_address = *pHeaderAddress;
			break;
		}
	}

	if (d->header_address == 0) {
		// No ROM header.
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
		const rp_char *const *exts = supportedFileExtensions_static();
		if (!exts) {
			// Should not happen...
			return -1;
		}
		for (; *exts != nullptr; exts++) {
			if (!rp_strcasecmp(info->ext, *exts)) {
				// File extension is supported.
				return 0;
			}
		}
	}

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
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SNES::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *SNES::systemName(uint32_t type) const
{
	RP_D(const SNES);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// sysNames[] bitfield:
	// - Bits 0-1: Type. (short, long, abbreviation)
	// - Bits 2-3: Region.
	uint32_t idx = (type & SYSNAME_TYPE_MASK);

	static const rp_char *const sysNames[16] = {
		// Japan: Super Famicom
		_RP("Nintendo Super Famicom"), _RP("Super Famicom"), _RP("SFC"), nullptr,
		// South Korea: Super Comboy
		_RP("Hyundai Super Comboy"), _RP("Super Comboy"), _RP("SCB"), nullptr,
		// Worldwide: Super NES
		_RP("Super Nintendo Entertainment System"), _RP("Super NES"), _RP("SNES"), nullptr,
		// Reserved.
		nullptr, nullptr, nullptr, nullptr
	};

	bool foundRegion = false;
	switch (d->romHeader.destination_code) {
		case SNES_DEST_JAPAN:
			idx |= (0 << 2);
			foundRegion = true;
			break;
		case SNES_DEST_SOUTH_KOREA:
			idx |= (1 << 2);
			foundRegion = true;
			break;

		case SNES_DEST_ALL:
		case SNES_DEST_OTHER_X:
		case SNES_DEST_OTHER_Y:
		case SNES_DEST_OTHER_Z:
			// Use the system locale.
			break;

		default: 
			if (d->romHeader.destination_code <= SNES_DEST_AUSTRALIA) {
				idx |= (2 << 2);
				foundRegion = true;
			}
			break;
	}

	if (!foundRegion) {
		// Check the system locale.
		switch (SystemRegion::getCountryCode()) {
			case 'JP':
				idx |= (0 << 2);
				break;
			case 'KR':
				idx |= (1 << 2);
				break;
			default:
				idx |= (2 << 2);
				break;
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
const rp_char *const *SNES::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".smc"), _RP(".swc"), _RP(".sfc"),
		_RP(".fig"), _RP(".ufo"),

		nullptr
	};
	return exts;
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
const rp_char *const *SNES::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SNES::loadFieldData(void)
{
	RP_D(SNES);
	if (d->fields->isDataLoaded()) {
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
	d->fields->reserve(7); // Maximum of 7 fields.

	// Title.
	// TODO: Space elimination.
	d->fields->addField_string(_RP("Title"),
		latin1_to_rp_string(romHeader->title, sizeof(romHeader->title)));

	// Game ID.
	// NOTE: Only valid if the old publisher code is 0x33.
	if (romHeader->old_publisher_code == 0x33) {
		string id6(romHeader->ext.id4, sizeof(romHeader->ext.id4));
		if (romHeader->ext.id4[2] == ' ' && romHeader->ext.id4[3] == ' ') {
			// Two-character ID.
			// Don't append the publisher.
			id6.resize(2);
		} else {
			// Four-character ID.
			// Append the publisher.
			id6 += string(romHeader->ext.new_publisher_code, sizeof(romHeader->ext.new_publisher_code));
		}
		d->fields->addField_string(_RP("Game ID"),
			latin1_to_rp_string(id6.data(), (int)id6.size()));
	} else {
		// No game ID.
		d->fields->addField_string(_RP("Game ID"), _RP("Unknown"));
	}

	// Publisher.
	const rp_char* publisher;
	if (romHeader->old_publisher_code == 0x33) {
		publisher = NintendoPublishers::lookup(romHeader->ext.new_publisher_code);
	} else {
		publisher = NintendoPublishers::lookup_old(romHeader->old_publisher_code);
	}
	d->fields->addField_string(_RP("Publisher"), publisher ? publisher : _RP("Unknown"));

	// ROM mapping.
	const rp_char *rom_mapping;
	switch (romHeader->rom_mapping) {
		case SNES_ROMMAPPING_LoROM:
			rom_mapping = _RP("LoROM");
			break;
		case SNES_ROMMAPPING_HiROM:
			rom_mapping = _RP("HiROM");
			break;
		case SNES_ROMMAPPING_LoROM_FastROM:
			rom_mapping = _RP("LoROM+FastROM");
			break;
		case SNES_ROMMAPPING_HiROM_FastROM:
			rom_mapping = _RP("HiROM+FastROM");
			break;
		case SNES_ROMMAPPING_ExLoROM:
			rom_mapping = _RP("ExLoROM");
			break;
		case SNES_ROMMAPPING_ExHiROM:
			rom_mapping = _RP("ExHiROM");
			break;
		default:
			rom_mapping = nullptr;
			break;
	}
	if (rom_mapping) {
		d->fields->addField_string(_RP("ROM Mapping"), rom_mapping);
	} else {
		// Unknown ROM mapping.
		char buf[20];
		int len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", romHeader->rom_mapping);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		d->fields->addField_string(_RP("ROM Mapping"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// Cartridge HW.
	static const rp_char *const hw_base_tbl[16] = {
		_RP("ROM"), _RP("ROM, RAM"), _RP("ROM, RAM, Battery"),
		_RP("ROM, "), _RP("ROM, RAM, "), _RP("ROM, RAM, Battery, "),
		_RP("ROM, Battery, "), nullptr,

		nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr
	};
	static const rp_char *const hw_enh_tbl[16] = {
		_RP("DSP-1"), _RP("Super FX"), _RP("OBC-1"), _RP("SA-1"),
		_RP("S-DD1"), _RP("Unknown"), _RP("Unknown"), _RP("Unknown"),
		_RP("Unknown"), _RP("Unknown"), _RP("Unknown"), _RP("Unknown"),
		_RP("Unknown"), _RP("Unknown"), _RP("Other"), _RP("Custom Chip")
	};

	const rp_char *const hw_base = hw_base_tbl[romHeader->rom_type & SNES_ROMTYPE_ROM_MASK];
	if (hw_base) {
		if ((romHeader->rom_type & SNES_ROMTYPE_ROM_MASK) >= SNES_ROMTYPE_ROM_ENH) {
			// Enhancement chip.
			const rp_char *const hw_enh = hw_enh_tbl[(romHeader->rom_type & SNES_ROMTYPE_ENH_MASK) >> 4];
			rp_string rps(hw_base);
			rps.append(hw_enh);
			d->fields->addField_string(_RP("Cartridge HW"), rps);
		} else {
			// No enhancement chip.
			d->fields->addField_string(_RP("Cartridge HW"), hw_base);
		}
	} else {
		// Unknown cartridge HW.
		d->fields->addField_string(_RP("Cartridge HW"), _RP("Unknown"));
	}

	// Region
	static const rp_char *const region_tbl[0x15] = {
		_RP("Japan"), _RP("North America"), _RP("Europe"), _RP("Scandinavia"),
		nullptr, nullptr, _RP("France"), _RP("Netherlands"),
		_RP("Spain"), _RP("Germany"), _RP("Italy"), _RP("China"),
		nullptr, _RP("South Korea"), _RP("All"), _RP("Canada"),
		_RP("Brazil"), _RP("Australia"), _RP("Other"), _RP("Other"),
		_RP("Other")
	};
	const rp_char *region = (romHeader->destination_code < ARRAY_SIZE(region_tbl)
		? region_tbl[romHeader->destination_code]
		: nullptr);
	d->fields->addField_string(_RP("Region"), region ? region : _RP("Unknown"));

	// Revision
	d->fields->addField_string_numeric(_RP("Revision"),
		romHeader->version, RomFields::FB_DEC, 2);

	// TODO: Other fields.

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

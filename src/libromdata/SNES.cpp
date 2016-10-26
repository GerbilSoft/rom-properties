/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SNES.cpp: Super Nintendo ROM image reader.                              *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
#include "snes_structs.h"
#include "CopierFormats.h"

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

class SNESPrivate
{
	public:
		SNESPrivate();

	private:
		SNESPrivate(const SNESPrivate &other);
		SNESPrivate &operator=(const SNESPrivate &other);

	public:
		// RomFields data.
		static const struct RomFields::Desc snes_fields[];

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

// ROM fields.
const struct RomFields::Desc SNESPrivate::snes_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("ROM Mapping"), RomFields::RFT_STRING, {nullptr}},
	// TODO: More fields.
};

SNESPrivate::SNESPrivate()
	: header_address(0)
{ }

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
	if ( (romHeader->rom_type & SNES_ROMTYPE_ROM_MASK) > SNES_ROMTYPE_ROM_SRAM_ENH ||
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
			if (!isalnum(romHeader->ext.id4[i]) &&
			    romHeader->ext.id4[i] != ' ')
			{
				// Game ID is invalid.
				return false;
			}
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
	: RomData(file, SNESPrivate::snes_fields, ARRAY_SIZE(SNESPrivate::snes_fields))
	, d(new SNESPrivate())
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// TODO: Only allow supported file extensions.

	// SNES ROMs don't necessarily have a header at the start of the file.
	// Therefore, we need to do a few reads and guessing.

	// Check if a copier header is present.
	SMD_Header smdHeader;
	m_file->rewind();
	size_t size = m_file->read(&smdHeader, sizeof(smdHeader));
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
			for (int i = ARRAY_SIZE(smdHeader.reserved2); i >= 0; i--) {
				if (smdHeader.reserved2[i] != 0) {
					areFieldsZero = false;
					break;
				}
			}
		}

		// If the fields are zero, this is a copier header.
		isCopierHeader = areFieldsZero;
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
		m_file->seek(*pHeaderAddress);
		size = m_file->read(&d->romHeader, sizeof(d->romHeader));
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
		m_isValid = false;
		return;
	}

	// ROM header found.
	m_isValid = true;
}

SNES::~SNES()
{
	delete d;
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
	assert(info->ext != nullptr);
	assert(info->ext[0] != 0);
	if (!info || !info->ext || info->ext[0] == 0) {
		// Either no detection information was specified,
		// or the file extension is missing.
		return -1;
	}

	// SNES ROMs don't necessarily have a header at the start of the file.
	// Therefore, we're using the file extension.
	vector<const rp_char*> exts = supportedFileExtensions_static();
	for (vector<const rp_char*>::const_iterator iter = exts.begin();
	     iter != exts.end(); ++iter)
	{
		if (!rp_strcmp(info->ext, *iter)) {
			// File extension is supported.
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
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		// FIXME: "NGC" in Japan?
		_RP("Super Nintendo Entertainment System"), _RP("Super NES"), _RP("SNES"), nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> SNES::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(4);
	ret.push_back(_RP(".smc"));
	ret.push_back(_RP(".swc"));
	ret.push_back(_RP(".sfc"));
	ret.push_back(_RP(".fig"));
	return ret;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions do not include the leading dot,
 * e.g. "bin" instead of ".bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> SNES::supportedFileExtensions(void) const
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
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// Unknown save file type.
		return -EIO;
	}

	// ROM file header is read and byteswapped in the constructor.
	const SNES_RomHeader *const romHeader = &d->romHeader;

	// Title.
	// TODO: Space elimination.
	m_fields->addData_string(latin1_to_rp_string(
		romHeader->title, sizeof(romHeader->title)));

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
		m_fields->addData_string(rom_mapping);
	} else {
		// Unknown ROM mapping.
		char buf[20];
		int len = snprintf(buf, sizeof(buf), "Unknown (0x%02X)", romHeader->rom_mapping);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));
	}

	// TODO: Other fields.

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

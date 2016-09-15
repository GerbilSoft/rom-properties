/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * GameBoyAdvance.hpp: Nintendo Game Boy Advance ROM reader.               *
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

#include "GameBoyAdvance.hpp"
#include "NintendoPublishers.hpp"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cstddef>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class GameBoyAdvancePrivate
{
	public:
		GameBoyAdvancePrivate() { }

	private:
		GameBoyAdvancePrivate(const GameBoyAdvancePrivate &other);
		GameBoyAdvancePrivate &operator=(const GameBoyAdvancePrivate &other);

	public:
		/** RomFields **/

		// Monospace string formatting.
		static const RomFields::StringDesc gba_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc gba_fields[];

		/** Internal ROM data. **/
		
		/**
		 * Game Boy Advance ROM header.
		 * This matches the GBA ROM header format exactly.
		 * Reference: http://problemkaputt.de/gbatek.htm#gbacartridgeheader
		 *
		 * All fields are in little-endian.
		 *
		 * NOTE: Strings are NOT null-terminated!
		 */
		#define GBA_RomHeader_SIZE 192
		#pragma pack(1)
		struct PACKED GBA_RomHeader {
			union {
				uint32_t entry_point;	// 32-bit ARM branch opcode.
				uint8_t entry_point_bytes[4];
			};
			uint8_t nintendo_logo[0x9C];	// Compressed logo.
			char title[12];
			union {
				char id6[6];	// Game code. (ID6)
				struct {
					char id4[4];		// Game code. (ID4)
					char company[2];	// Company code.
				};
			};
			uint8_t fixed_96h;	// Fixed value. (Must be 0x96)
			uint8_t unit_code;	// 0x00 for all GBA models.
			uint8_t device_type;	// 0x00. (bit 7 for debug?)
			uint8_t reserved1[7];
			uint8_t rom_version;
			uint8_t checksum;
			uint8_t reserved2[2];
		};
		#pragma pack()

	public:
		// ROM header.
		GBA_RomHeader romHeader;
};

/** GameBoyAdvancePrivate **/

// Monospace string formatting.
const RomFields::StringDesc GameBoyAdvancePrivate::gba_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// ROM fields.
const struct RomFields::Desc GameBoyAdvancePrivate::gba_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {&gba_string_monospace}},
};

/** GameBoyAdvance **/

/**
 * Read a Nintendo Game Boy Advance ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
GameBoyAdvance::GameBoyAdvance(IRpFile *file)
	: RomData(file, GameBoyAdvancePrivate::gba_fields, ARRAY_SIZE(GameBoyAdvancePrivate::gba_fields))
	, d(new GameBoyAdvancePrivate())
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	static_assert(sizeof(GameBoyAdvancePrivate::GBA_RomHeader) == GBA_RomHeader_SIZE,
		"GBA_RomHeader is the wrong size. (Should be 192 bytes.)");
	GameBoyAdvancePrivate::GBA_RomHeader romHeader;
	m_file->rewind();
	size_t size = m_file->read(&romHeader, sizeof(romHeader));
	if (size != sizeof(romHeader))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.pHeader = reinterpret_cast<const uint8_t*>(&romHeader);
	info.szHeader = sizeof(romHeader);
	info.ext = nullptr;	// Not needed for GBA.
	info.szFile = 0;	// Not needed for GBA.
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the ROM header.
		memcpy(&d->romHeader, &romHeader, sizeof(d->romHeader));
	}
}

GameBoyAdvance::~GameBoyAdvance()
{
	delete d;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameBoyAdvance::isRomSupported_static(const DetectInfo *info)
{
	if (!info || info->szHeader < sizeof(GameBoyAdvancePrivate::GBA_RomHeader)) {
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the first 16 bytes of the Nintendo logo.
	static const uint8_t nintendo_gba_logo[16] = {
		0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
		0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
	};

	const GameBoyAdvancePrivate::GBA_RomHeader *gba_header =
		reinterpret_cast<const GameBoyAdvancePrivate::GBA_RomHeader*>(info->pHeader);
	if (!memcmp(gba_header->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
		// Nintendo logo is present at the correct location.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int GameBoyAdvance::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *GameBoyAdvance::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"GameBoyAdvance::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Nintendo Game Boy Advance"),
		_RP("Game Boy Advance"),
		_RP("GBA"),
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
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameBoyAdvance::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(2);
	ret.push_back(_RP(".gba"));
	ret.push_back(_RP(".agb"));
	//ret.push_back(_RP(".mb"));	// TODO: Enable this?
	return ret;
}

/**
 * Get a list of all supported file extensions.
 * This is to be used for file type registration;
 * subclasses don't explicitly check the extension.
 *
 * NOTE: The extensions include the leading dot,
 * e.g. ".bin" instead of "bin".
 *
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> GameBoyAdvance::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int GameBoyAdvance::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// GBA ROM header.
	const GameBoyAdvancePrivate::GBA_RomHeader *romHeader = &d->romHeader;

	// Game title.
	m_fields->addData_string(latin1_to_rp_string(romHeader->title, sizeof(romHeader->title)));

	// Game ID and publisher.
	m_fields->addData_string(latin1_to_rp_string(romHeader->id6, sizeof(romHeader->id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(romHeader->company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	m_fields->addData_string_numeric(romHeader->rom_version, RomFields::FB_DEC, 2);

	// Entry point.
	if (romHeader->entry_point_bytes[3] == 0xEA) {
		// Unconditional branch instruction.
		const uint32_t entry_point = (le32_to_cpu(romHeader->entry_point) & 0xFFFFFF) << 2;
		m_fields->addData_string_numeric(entry_point, RomFields::FB_HEX, 8);
	} else {
		// Non-standard entry point instruction.
		m_fields->addData_string_hexdump(romHeader->entry_point_bytes, 4);
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

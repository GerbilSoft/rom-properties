/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMG.hpp: Game Boy (DMG/CGB/SGB) ROM reader.                             *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
 * Copyright (c) 2016 by Egor.                                             *
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

#include "DMG.hpp"
#include "NintendoPublishers.hpp"
#include "dmg_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

// C includes. (C++ namespace)
#include <cstring>
#include <cctype>

// C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

class DMGPrivate
{
	public:
		DMGPrivate() { }

	private:
		DMGPrivate(const DMGPrivate &other);
		DMGPrivate &operator=(const DMGPrivate &other);

	public:
		/** RomFields **/

		// System. (RFT_BITFIELD)
		enum DMG_System {
			DMG_SYSTEM_DMG		= (1 << 0),
			DMG_SYSTEM_CGB		= (1 << 1),
			DMG_SYSTEM_SGB		= (1 << 2),
		};
		static const rp_char *const dmg_system_bitfield_names[];
		static const RomFields::BitfieldDesc dmg_system_bitfield;

		// Cartridge hardware features. (RFT_BITFIELD)
		enum DMG_Feature {
			DMG_FEATURE_RAM		= (1 << 0),
			DMG_FEATURE_BATTERY	= (1 << 1),
			DMG_FEATURE_TIMER	= (1 << 2),
			DMG_FEATURE_RUMBLE	= (1 << 3),
		};
		static const rp_char *const dmg_feature_bitfield_names[];
		static const RomFields::BitfieldDesc dmg_feature_bitfield;

		// Monospace string formatting.
		static const RomFields::StringDesc dmg_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc dmg_fields[];

		/** Internal ROM data. **/

		// Cartridge hardware.
		enum DMG_Hardware {
			DMG_HW_UNK,
			DMG_HW_ROM,
			DMG_HW_MBC1,
			DMG_HW_MBC2,
			DMG_HW_MBC3,
			DMG_HW_MBC4,
			DMG_HW_MBC5,
			DMG_HW_MBC6,
			DMG_HW_MBC7,
			DMG_HW_MMM01,
			DMG_HW_HUC1,
			DMG_HW_HUC3,
			DMG_HW_TAMA5,
			DMG_HW_CAMERA
		};
		static const rp_char *const dmg_hardware_names[];

		struct dmg_cart_type {
			DMG_Hardware hardware;
			uint32_t features; //DMG_Feature
		};

	private:
		// Sparse array setup:
		// - "start" starts at 0x00.
		// - "end" ends at 0xFF.
		static const dmg_cart_type dmg_cart_types_start[];
		static const dmg_cart_type dmg_cart_types_end[];

	public:
		/**
		 * Get a dmg_cart_type struct describing a cartridge type byte.
		 * @param type Cartridge type byte.
		 * @return dmg_cart_type struct.
		 */
		static inline const dmg_cart_type& CartType(uint8_t type);

		/**
		 * Convert the ROM size value to an actual size.
		 * @param type ROM size value.
		 * @return ROM size, in kilobytes. (-1 on error)
		 */
		static inline int RomSize(uint8_t type);

	public:
		/**
		 * DMG RAM size array.
		 */
		static const uint8_t dmg_ram_size[];

		/**
		 * Nintendo's logo which is checked by bootrom.
		 * (Top half only.)
		 * 
		 * NOTE: CGB bootrom only checks the top half of the logo.
		 * (see 0x00D1 of CGB IPL)
		 */
		static const uint8_t dmg_nintendo[0x18];

	public:
		// ROM header.
		DMG_RomHeader romHeader;
};

/** DMGPrivate **/

// System.
const rp_char *const DMGPrivate::dmg_system_bitfield_names[] = {
	_RP("DMG"), _RP("CGB"), _RP("SGB")
};

const RomFields::BitfieldDesc DMGPrivate::dmg_system_bitfield = {
	ARRAY_SIZE(dmg_system_bitfield_names), 3, dmg_system_bitfield_names
};

// Cartrige hardware features.
const rp_char *const DMGPrivate::dmg_feature_bitfield_names[] = {
	_RP("RAM"), _RP("Battery"), _RP("Timer"), _RP("Rumble")
};

const RomFields::BitfieldDesc DMGPrivate::dmg_feature_bitfield = {
	ARRAY_SIZE(dmg_feature_bitfield_names), 4, dmg_feature_bitfield_names
};

// Monospace string formatting.
const RomFields::StringDesc DMGPrivate::dmg_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// ROM fields.
const struct RomFields::Desc DMGPrivate::dmg_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Game ID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("System"), RomFields::RFT_BITFIELD, {&dmg_system_bitfield}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {&dmg_string_monospace}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Hardware"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Features"), RomFields::RFT_BITFIELD, {&dmg_feature_bitfield}},
	{_RP("ROM Size"), RomFields::RFT_STRING, {nullptr}},
	{_RP("RAM Size"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Region"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Checksum"), RomFields::RFT_STRING, {&dmg_string_monospace}},
};

/** Internal ROM data. **/

// Cartrige hardware.
const rp_char *const DMGPrivate::dmg_hardware_names[] = {
	_RP("Unknown"),
	_RP("ROM"),
	_RP("MBC1"),
	_RP("MBC2"),
	_RP("MBC3"),
	_RP("MBC4"),
	_RP("MBC5"),
	_RP("MBC6"),
	_RP("MBC7"),
	_RP("MMM01"),
	_RP("HuC1"),
	_RP("HuC3"),
	_RP("TAMA5"),
	_RP("POCKET CAMERA"), // ???
};

const DMGPrivate::dmg_cart_type DMGPrivate::dmg_cart_types_start[] = {
	{DMG_HW_ROM,	0},
	{DMG_HW_MBC1,	0},
	{DMG_HW_MBC1,	DMG_FEATURE_RAM},
	{DMG_HW_MBC1,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC2,	0},
	{DMG_HW_MBC2,	DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_ROM,	DMG_FEATURE_RAM},
	{DMG_HW_ROM,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MMM01,	0},
	{DMG_HW_MMM01,	DMG_FEATURE_RAM},
	{DMG_HW_MMM01,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC3,	DMG_FEATURE_TIMER|DMG_FEATURE_BATTERY},
	{DMG_HW_MBC3,	DMG_FEATURE_TIMER|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_MBC3,	0},
	{DMG_HW_MBC3,	DMG_FEATURE_RAM},
	{DMG_HW_MBC3,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC4,	0},
	{DMG_HW_MBC4,	DMG_FEATURE_RAM},
	{DMG_HW_MBC4,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC5,	0},
	{DMG_HW_MBC5,	DMG_FEATURE_RAM},
	{DMG_HW_MBC5,	DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_MBC5,	DMG_FEATURE_RUMBLE},
	{DMG_HW_MBC5,	DMG_FEATURE_RUMBLE|DMG_FEATURE_RAM},
	{DMG_HW_MBC5,	DMG_FEATURE_RUMBLE|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC6,	0},
	{DMG_HW_UNK,	0},
	{DMG_HW_MBC7,	DMG_FEATURE_RUMBLE|DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
};

const DMGPrivate::dmg_cart_type DMGPrivate::dmg_cart_types_end[] = {
	{DMG_HW_CAMERA, 0},
	{DMG_HW_TAMA5, 0},
	{DMG_HW_HUC3, 0},
	{DMG_HW_HUC1, DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
};

/**
 * Get a dmg_cart_type struct describing a cartridge type byte.
 * @param type Cartridge type byte.
 * @return dmg_cart_type struct.
 */
inline const DMGPrivate::dmg_cart_type& DMGPrivate::CartType(uint8_t type)
{
	static const dmg_cart_type unk = {DMG_HW_UNK, 0};
	if (type < ARRAY_SIZE(dmg_cart_types_start)) {
		return dmg_cart_types_start[type];
	}
	const unsigned end_offset = 0x100u-ARRAY_SIZE(dmg_cart_types_end);
	if (type>=end_offset) {
		return dmg_cart_types_end[type-end_offset];
	}
	return unk;
}

/**
 * Convert the ROM size value to an actual size.
 * @param type ROM size value.
 * @return ROM size, in kilobytes. (-1 on error)
 */
inline int DMGPrivate::RomSize(uint8_t type)
{
	static const int rom_size[] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
	static const int rom_size_52[] = {1152, 1280, 1536};
	if (type < ARRAY_SIZE(rom_size)) {
		return rom_size[type];
	} else if (type >= 0x52 && type < 0x52+ARRAY_SIZE(rom_size_52)) {
		return rom_size_52[type-0x52];
	}
	return -1;
}

/**
 * DMG RAM size array.
 */
const uint8_t DMGPrivate::dmg_ram_size[] = {
	0, 2, 8, 32, 128, 64
};

/**
 * Nintendo's logo which is checked by bootrom.
 * (Top half only.)
 * 
 * NOTE: CGB bootrom only checks the top half of the logo.
 * (see 0x00D1 of CGB IPL)
 */
const uint8_t DMGPrivate::dmg_nintendo[0x18] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E
};

/** DMG **/

/**
 * Read a Game Boy ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
DMG::DMG(IRpFile *file)
	: RomData(file, DMGPrivate::dmg_fields, ARRAY_SIZE(DMGPrivate::dmg_fields))
	, d(new DMGPrivate())
{
	// TODO: Only validate that this is an DMG ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	m_file->rewind();

	// Read the ROM header. [0x150 bytes]
	static_assert(sizeof(DMG_RomHeader) == DMG_RomHeader_SIZE,
		"DMG_RomHeader_SIZE is not 80 bytes.");
	uint8_t header[0x150];
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for DMG.
	info.szFile = 0;	// Not needed for DMG.
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the header for later.
		// TODO: Save the RST table?
		memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
	}
}

DMG::~DMG()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DMG::isRomSupported_static(const DetectInfo *info)
{
	if (!info)
		return -1;
	
	if (info->szHeader >= 0x150) {
		// Check the system name.
		const DMG_RomHeader *romHeader =
			reinterpret_cast<const DMG_RomHeader*>(&info->pHeader[0x100]);
		if (!memcmp(romHeader->nintendo, DMGPrivate::dmg_nintendo, sizeof(DMGPrivate::dmg_nintendo))) {
			// Found a DMG ROM.
			if (romHeader->cgbflag & 0x80) {
				//TODO: Make this an enum, maybe
				return 1; // CGB supported
			}
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
int DMG::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *DMG::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GB/GBC have the same names worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"DMG::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bit 2: Game Boy Color. (DMG-specific)
	static const rp_char *const sysNames[8] = {
		_RP("Nintendo Game Boy"), _RP("Game Boy"), _RP("GB"), nullptr,
		_RP("Nintendo Game Boy Color"), _RP("Game Boy Color"), _RP("GBC"), nullptr
	};

	uint32_t idx = (type & SYSNAME_TYPE_MASK);
	if (d->romHeader.cgbflag & 0x80) {
		// ROM supports Game Boy Color functionality.
		// NOTE: Pokemon Yellow used a "Game Boy" box, even though
		// the US version supported GBC, but Pokemon Gold and Silver
		// use a "Game Boy Color" box.
		idx |= 4;
	}

	return sysNames[idx];
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
vector<const rp_char*> DMG::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(3);
	ret.push_back(_RP(".gb"));
	ret.push_back(_RP(".sgb"));
	ret.push_back(_RP(".sgb2"));
	ret.push_back(_RP(".gbc"));
	ret.push_back(_RP(".cgb"));
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
vector<const rp_char*> DMG::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int DMG::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// DMG ROM header, excluding the RST table.
	const DMG_RomHeader *romHeader = &d->romHeader;
	
	char buffer[64];
	int len;
	
	// Game title & Game ID
	/* NOTE: there are two approaches for doing this, when the 15 bytes are all used
	 * 1) prioritize id
	 * 2) prioritize title
	 * Both of those have counter examples:
	 * If you do the first, you will get "SUPER MARIO" and "LAND" on super mario land rom
	 * With the second one, you will get "MARIO DELUXAHYJ" and Unknown on super mario deluxe rom
	 * 
	 * Current method is the first one.
	 */
	if (romHeader->cgbflag < 0x80) {
		// Assuming 16-character title for non-CGB.
		m_fields->addData_string(latin1_to_rp_string(romHeader->title16, sizeof(romHeader->title16)));
		// Game ID is not present.
		m_fields->addData_string(_RP("Unknown"));
	} else {
		// Check if CGB flag is present.
		bool isGameID;
		if ((romHeader->cgbflag & 0x3F) == 0) {
		// Check if a Game ID is present.
			isGameID = true;
			for (int i = 11; i < 15; i++) {
				if (!isalnum(romHeader->title15[i])) {
					// Not a Game ID.
					isGameID = false;
					break;
				}
			}
		} else {
			// Not CGB. No Game ID.
			isGameID = false;
		}

		if (isGameID) {
			// Game ID is present.
			m_fields->addData_string(latin1_to_rp_string(romHeader->title11, sizeof(romHeader->title11)));

			// Append the publisher code to make an ID6.
			char id6[6];
			memcpy(id6, romHeader->id4, 4);
			if (romHeader->old_publisher_code == 0x33) {
				// New publisher code.
				id6[4] = romHeader->new_publisher_code[0];
				id6[5] = romHeader->new_publisher_code[1];
			} else {
				// Old publisher code.
				// FIXME: This probably won't ever happen,
				// since Game ID was added *after* CGB.
				static const char hex_lookup[16] = {
					'0','1','2','3','4','5','6','7',
					'8','9','A','B','C','D','E','F'
				};
				id6[4] = hex_lookup[romHeader->old_publisher_code >> 4];
				id6[5] = hex_lookup[romHeader->old_publisher_code & 0x0F];
			}
			m_fields->addData_string(latin1_to_rp_string(id6, sizeof(id6)));
		} else {
			// Game ID is not present.
			m_fields->addData_string(latin1_to_rp_string(romHeader->title15, sizeof(romHeader->title15)));
			m_fields->addData_string(_RP("Unknown"));
		}
	}

	// System
	uint32_t dmg_system = 0;
	if (romHeader->cgbflag & 0x80) {
		// Game supports CGB.
		dmg_system = DMGPrivate::DMG_SYSTEM_CGB;
		if (!(romHeader->cgbflag & 0x40)) {
			// Not CGB exclusive.
			dmg_system |= DMGPrivate::DMG_SYSTEM_DMG;
		}
	} else {
		// Game does not support CGB.
		dmg_system |= DMGPrivate::DMG_SYSTEM_DMG;
	}

	if (romHeader->old_publisher_code == 0x33 && romHeader->sgbflag==0x03) {
		// Game supports SGB.
		dmg_system |= DMGPrivate::DMG_SYSTEM_SGB;
	}
	m_fields->addData_bitfield(dmg_system);

	// Entry Point
	if(romHeader->entry[0] == 0 && romHeader->entry[1] == 0xC3){
		// this is the "standard" way of doing the entry point
		uint16_t entry_address;
		memcpy((void*)&entry_address,(void*)(romHeader->entry+2),sizeof(uint16_t)); //ugly hack, but avoids aliasing warnings
		entry_address = le16_to_cpu(entry_address);
		m_fields->addData_string_numeric(entry_address, RomFields::FB_HEX, 4);
	}
	else{
		m_fields->addData_string_hexdump(romHeader->entry,4);
	}
	
	// Publisher
	const rp_char* publisher;
	if (romHeader->old_publisher_code == 0x33) {
		publisher = NintendoPublishers::lookup(romHeader->new_publisher_code);
	} else {
		publisher = NintendoPublishers::lookup_old(romHeader->old_publisher_code);
	}
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));
	
	// Hardware
	m_fields->addData_string(
		DMGPrivate::dmg_hardware_names[DMGPrivate::CartType(romHeader->cart_type).hardware]);
	
	// Features
	m_fields->addData_bitfield(DMGPrivate::CartType(romHeader->cart_type).features);
	
	// ROM Size
	int rom_size = DMGPrivate::RomSize(romHeader->rom_size);
	if (rom_size < 0) {
		m_fields->addData_string(_RP("Unknown"));
	} else {
		if(rom_size>32){
			len = snprintf(buffer,sizeof(buffer),"%d KiB (%d banks)",rom_size,rom_size/16);
		}
		else{
			len = snprintf(buffer,sizeof(buffer),"%d KiB",rom_size);
		}
		m_fields->addData_string(latin1_to_rp_string(buffer,len));
	}
	
	// RAM Size
	if (romHeader->ram_size >= ARRAY_SIZE(DMGPrivate::dmg_ram_size)){
		m_fields->addData_string(_RP("Unknown"));
	} else {
		uint8_t ram_size = DMGPrivate::dmg_ram_size[romHeader->ram_size];
		if (ram_size == 0 &&
		    DMGPrivate::CartType(romHeader->cart_type).hardware == DMGPrivate::DMG_HW_MBC2)
		{
			// Not really RAM, but whatever
			m_fields->addData_string(_RP("512 x 4 bits"));
		} else if(ram_size == 0) {
			m_fields->addData_string(_RP("No RAM"));
		} else {
			if(ram_size>8){
				len = snprintf(buffer,sizeof(buffer),"%u KiB (%u banks)",ram_size,ram_size/8);
			}
			else{
				len = snprintf(buffer,sizeof(buffer),"%u KiB",ram_size);
			}
			m_fields->addData_string(latin1_to_rp_string(buffer,len));
		}
	}

	// Region
	switch (romHeader->region) {
		case 0:
			m_fields->addData_string(_RP("Japanese"));
			break;
		case 1:
			m_fields->addData_string(_RP("Non-Japanese"));
			break;
		default:
			// Invalid value.
			len = snprintf(buffer, sizeof(buffer), "0x%02X (INVALID)", romHeader->region);
			m_fields->addData_string(latin1_to_rp_string(buffer, len));
			break;
	}
	
	// Revision
	m_fields->addData_string_numeric(romHeader->version, RomFields::FB_DEC, 2);
	
	// Header checksum.
	// This is a checksum of ROM addresses 0x134-0x14D.
	// Note that romHeader is a copy of the ROM header
	// starting at 0x100, so the values are offset accordingly.
	uint8_t checksum = 0xE7; // -0x19
	const uint8_t *romHeader8 = reinterpret_cast<const uint8_t*>(romHeader);
	for(int i = 0x0034; i < 0x004D; i++){
		checksum -= romHeader8[i];
	}

	if (checksum - romHeader->header_checksum) {
		len = snprintf(buffer, sizeof(buffer), "0x%02X (INVALID; should be 0x%02X)",
			checksum, romHeader->header_checksum);
	} else {
		len = snprintf(buffer, sizeof(buffer), "0x%02X (valid)", checksum);
	}
	m_fields->addData_string(latin1_to_rp_string(buffer,len));
	
	
	return (int)m_fields->count();
}

}

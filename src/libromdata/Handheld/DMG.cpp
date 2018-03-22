/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * DMG.hpp: Game Boy (DMG/CGB/SGB) ROM reader.                             *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * Copyright (c) 2016-2017 by Egor.                                        *
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
#include "librpbase/RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "dmg_structs.h"

// librpbase
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cctype>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace LibRomData {

class DMGPrivate : public RomDataPrivate
{
	public:
		DMGPrivate(DMG *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DMGPrivate)

	public:
		/** RomFields **/

		// System. (RFT_BITFIELD)
		enum DMG_System {
			DMG_SYSTEM_DMG		= (1 << 0),
			DMG_SYSTEM_SGB		= (1 << 1),
			DMG_SYSTEM_CGB		= (1 << 2),
		};

		// Cartridge hardware features. (RFT_BITFIELD)
		enum DMG_Feature {
			DMG_FEATURE_RAM		= (1 << 0),
			DMG_FEATURE_BATTERY	= (1 << 1),
			DMG_FEATURE_TIMER	= (1 << 2),
			DMG_FEATURE_RUMBLE	= (1 << 3),
		};

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
		static const char *const dmg_hardware_names[];

		struct dmg_cart_type {
			uint8_t hardware;	// DMG_Hardware
			uint8_t features;	// DMG_Feature
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
		enum DMG_RomType {
			ROM_UNKNOWN	= -1,	// Unknown ROM type.
			ROM_DMG		= 0,	// Game Boy
			ROM_CGB		= 1,	// Game Boy Color
		};

		// ROM type.
		int romType;

	public:
		// ROM header.
		DMG_RomHeader romHeader;
};

/** DMGPrivate **/

/** Internal ROM data. **/

// Cartrige hardware.
const char *const DMGPrivate::dmg_hardware_names[] = {
	"Unknown",
	"ROM",
	"MBC1",
	"MBC2",
	"MBC3",
	"MBC4",
	"MBC5",
	"MBC6",
	"MBC7",
	"MMM01",
	"HuC1",
	"HuC3",
	"TAMA5",
	"POCKET CAMERA", // ???
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

DMGPrivate::DMGPrivate(DMG *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

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
	: super(new DMGPrivate(this, file))
{
	RP_D(DMG);
	d->className = "DMG";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header. [0x150 bytes]
	uint8_t header[0x150];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for DMG.
	info.szFile = 0;	// Not needed for DMG.
	d->romType = isRomSupported_static(&info);

	d->isValid = (d->romType >= 0);
	if (d->isValid) {
		// Save the header for later.
		// TODO: Save the RST table?
		memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int DMG::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 0x150)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check the system name.
	const DMG_RomHeader *const romHeader =
		reinterpret_cast<const DMG_RomHeader*>(&info->header.pData[0x100]);
	if (!memcmp(romHeader->nintendo, DMGPrivate::dmg_nintendo, sizeof(DMGPrivate::dmg_nintendo))) {
		// Found a DMG ROM.
		if (romHeader->cgbflag & 0x80) {
			return DMGPrivate::ROM_CGB; // CGB supported
		}
		return DMGPrivate::ROM_DMG;
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
const char *DMG::systemName(unsigned int type) const
{
	RP_D(const DMG);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GB/GBC have the same names worldwide, so we can
	// ignore the region selection.
	// TODO: Abbreviation might be different... (Japan uses DMG/CGB?)
	static_assert(SYSNAME_TYPE_MASK == 3,
		"DMG::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	// Bit 2: Game Boy Color. (DMG-specific)
	static const char *const sysNames[8] = {
		"Nintendo Game Boy", "Game Boy", "GB", nullptr,
		"Nintendo Game Boy Color", "Game Boy Color", "GBC", nullptr
	};

	unsigned int idx = (d->romType << 2) | (type & SYSNAME_TYPE_MASK);
	if (idx >= ARRAY_SIZE(sysNames)) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *DMG::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gb",  ".sgb", ".sgb2",
		".gbc", ".cgb",

		nullptr
	};
	return exts;
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
const char *const *DMG::supportedFileExtensions(void) const
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
	RP_D(DMG);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// DMG ROM header, excluding the RST table.
	const DMG_RomHeader *const romHeader = &d->romHeader;
	d->fields->reserve(12);	// Maximum of 12 fields.

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
		d->fields->addField_string(C_("DMG", "Title"),
			latin1_to_utf8(romHeader->title16, sizeof(romHeader->title16)));
		// Game ID is not present.
		d->fields->addField_string(C_("DMG", "Game ID"), C_("DMG", "Unknown"));
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
			d->fields->addField_string(C_("DMG", "Title"),
				latin1_to_utf8(romHeader->title11, sizeof(romHeader->title11)));

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
			d->fields->addField_string(C_("DMG", "Game ID"),
				latin1_to_utf8(id6, sizeof(id6)));
		} else {
			// Game ID is not present.
			d->fields->addField_string(C_("DMG", "Title"),
				latin1_to_utf8(romHeader->title15, sizeof(romHeader->title15)));
			d->fields->addField_string(C_("DMG", "Game ID"), C_("DMG", "Unknown"));
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

	static const char *const system_bitfield_names[] = {
		"DMG", "SGB", "CGB"
	};
	vector<string> *v_system_bitfield_names = RomFields::strArrayToVector(
		system_bitfield_names, ARRAY_SIZE(system_bitfield_names));
	d->fields->addField_bitfield(C_("DMG", "System"),
		v_system_bitfield_names, 0, dmg_system);

	// Entry Point
	if(romHeader->entry[0] == 0 && romHeader->entry[1] == 0xC3){
		// this is the "standard" way of doing the entry point
		const uint16_t entry_address = (romHeader->entry[2] | (romHeader->entry[3] << 8));
		d->fields->addField_string_numeric(C_("DMG", "Entry Point"),
			entry_address, RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);
	} else {
		d->fields->addField_string_hexdump(C_("DMG", "Entry Point"),
			romHeader->entry, 4, RomFields::STRF_MONOSPACE);
	}

	// Publisher
	const char* publisher;
	if (romHeader->old_publisher_code == 0x33) {
		publisher = NintendoPublishers::lookup(romHeader->new_publisher_code);
	} else {
		publisher = NintendoPublishers::lookup_old(romHeader->old_publisher_code);
	}
	d->fields->addField_string(C_("DMG", "Publisher"),
		publisher ? publisher : C_("DMG", "Unknown"));

	// Hardware
	d->fields->addField_string("Hardware",
		DMGPrivate::dmg_hardware_names[DMGPrivate::CartType(romHeader->cart_type).hardware]);

	// Features
	static const char *const feature_bitfield_names[] = {
		NOP_C_("DMG|Features", "RAM"),
		NOP_C_("DMG|Features", "Battery"),
		NOP_C_("DMG|Features", "Timer"),
		NOP_C_("DMG|Features", "Rumble"),
	};
	vector<string> *v_feature_bitfield_names = RomFields::strArrayToVector_i18n(
		"DMG|Features", feature_bitfield_names, ARRAY_SIZE(feature_bitfield_names));
	d->fields->addField_bitfield(C_("DMG", "Features"),
		v_feature_bitfield_names, 0, DMGPrivate::CartType(romHeader->cart_type).features);

	// ROM Size
	int rom_size = DMGPrivate::RomSize(romHeader->rom_size);
	if (rom_size < 0) {
		d->fields->addField_string(C_("DMG", "ROM Size"), C_("DMG", "Unknown"));
	} else {
		if (rom_size > 32) {
			const int banks = rom_size / 16;
			d->fields->addField_string("ROM Size",
				rp_sprintf_p(NC_("DMG", "%1$u KiB (%2$u bank)", "%1$u KiB (%2$u banks)", banks),
					(unsigned int)rom_size, (unsigned int)banks));
		} else {
			d->fields->addField_string("ROM Size",
				rp_sprintf(C_("DMG", "%u KiB"), (unsigned int)rom_size));
		}
	}

	// RAM Size
	if (romHeader->ram_size >= ARRAY_SIZE(DMGPrivate::dmg_ram_size)) {
		d->fields->addField_string(C_("DMG", "RAM Size"), C_("DMG", "Unknown"));
	} else {
		uint8_t ram_size = DMGPrivate::dmg_ram_size[romHeader->ram_size];
		if (ram_size == 0 &&
		    DMGPrivate::CartType(romHeader->cart_type).hardware == DMGPrivate::DMG_HW_MBC2)
		{
			d->fields->addField_string(C_("DMG", "RAM Size"),
				// tr: MBC2 internal memory - Not really RAM, but whatever.
				C_("DMG", "512 x 4 bits"));
		} else if(ram_size == 0) {
			d->fields->addField_string(C_("DMG", "RAM Size"), C_("DMG", "No RAM"));
		} else {
			if (ram_size > 8) {
				const int banks = ram_size / 16;
				d->fields->addField_string("RAM Size",
					rp_sprintf_p(NC_("DMG", "%1$u KiB (%2$u bank)", "%1$u KiB (%2$u banks)", banks),
						(unsigned int)ram_size, (unsigned int)banks));
			} else {
				d->fields->addField_string("RAM Size",
					rp_sprintf(C_("DMG", "%u KiB"), ram_size));
			}
		}
	}

	// Region
	switch (romHeader->region) {
		case 0:
			d->fields->addField_string(C_("DMG", "Region"), C_("Region|DMG", "Japanese"));
			break;
		case 1:
			d->fields->addField_string(C_("DMG", "Region"), C_("Region|DMG", "Non-Japanese"));
			break;
		default:
			// Invalid value.
			d->fields->addField_string(C_("DMG", "Region"),
				rp_sprintf(C_("DMG", "0x%02X (INVALID)"), romHeader->region));
			break;
	}

	// Revision
	d->fields->addField_string_numeric(C_("DMG", "Revision"),
		romHeader->version, RomFields::FB_DEC, 2);

	// Header checksum.
	// This is a checksum of ROM addresses 0x134-0x14D.
	// Note that romHeader is a copy of the ROM header
	// starting at 0x100, so the values are offset accordingly.
	uint8_t checksum = 0xE7; // -0x19
	const uint8_t *romHeader8 = reinterpret_cast<const uint8_t*>(romHeader);
	for (unsigned int i = 0x0034; i < 0x004D; i++){
		checksum -= romHeader8[i];
	}

	if (checksum - romHeader->header_checksum != 0) {
		d->fields->addField_string(C_("DMG", "Checksum"),
			rp_sprintf_p(C_("DMG", "0x%1$02X (INVALID; should be 0x%2$02X)"),
				romHeader->header_checksum, checksum));
	} else {
		d->fields->addField_string(C_("DMG", "Checksum"),
			rp_sprintf(C_("DMG", "0x%02X (valid)"), checksum));
	}

	return (int)d->fields->count();
}

}

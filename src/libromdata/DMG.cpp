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
#include "TextFuncs.hpp"
#include "byteswap.h"
#include "common.h"

// C includes. (C++ namespace)
#include <cstring>
#include <cctype>

// C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

/**
 * String length with limit
 * @param str The string itself
 * @param len Maximum length of the string
 * @returns equivivalent to min(strlen(str), len) without buffer overruns
 */
static inline size_t strnlen(const char *str, size_t len){
	size_t rv=0;
	for(rv=0;rv<len;rv++)
		if(!*(str++)) break;
	return rv;
}

// System
static const rp_char *const dmg_system_bitfield_names[] = {
	_RP("DMG"), _RP("CGB"), _RP("SGB")
};

enum DMG_System {
	DMG_SYSTEM_DMG		= (1 << 0),
	DMG_SYSTEM_CGB		= (1 << 1),
	DMG_SYSTEM_SGB		= (1 << 2),
};

static const RomFields::BitfieldDesc dmg_system_bitfield = {
	ARRAY_SIZE(dmg_system_bitfield_names), 3, dmg_system_bitfield_names
};


// Cartrige hardware features
static const rp_char *const dmg_feature_bitfield_names[] = {
	_RP("RAM"), _RP("Battery"), _RP("Timer"), _RP("Rumble")
};

enum DMG_Feature {
	DMG_FEATURE_RAM		= (1 << 0),
	DMG_FEATURE_BATTERY	= (1 << 1),
	DMG_FEATURE_TIMER	= (1 << 2),
	DMG_FEATURE_RUMBLE	= (1 << 3),
};

static const RomFields::BitfieldDesc dmg_feature_bitfield = {
	ARRAY_SIZE(dmg_feature_bitfield_names), 4, dmg_feature_bitfield_names
};

static const struct RomFields::Desc dmg_fields[] = {
	{_RP("Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("GameID"), RomFields::RFT_STRING, {nullptr}},
	{_RP("System"), RomFields::RFT_BITFIELD, {&dmg_system_bitfield}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Hardware"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Features"), RomFields::RFT_BITFIELD, {&dmg_feature_bitfield}},
	{_RP("ROM Size"), RomFields::RFT_STRING, {nullptr}},
	{_RP("RAM Size"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Region"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Revision"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Checksum"), RomFields::RFT_STRING, {nullptr}},
};

// Cartrige hardware
static const rp_char *const dmg_hardware_names[] = {
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

enum DMG_Hardware{
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

struct dmg_cart_type{
	DMG_Hardware hardware;
	uint32_t features; //DMG_Feature
};

static const dmg_cart_type dmg_cart_types_start[] = {
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
static const dmg_cart_type dmg_cart_types_end[] = {
	{DMG_HW_CAMERA, 0},
	{DMG_HW_TAMA5, 0},
	{DMG_HW_HUC3, 0},
	{DMG_HW_HUC1, DMG_FEATURE_RAM|DMG_FEATURE_BATTERY},
};

inline const dmg_cart_type& CartType(uint8_t type){
	static const dmg_cart_type unk= {DMG_HW_UNK, 0};
	if(type<ARRAY_SIZE(dmg_cart_types_start)){
		return dmg_cart_types_start[type];
	}
	const unsigned end_offset = 0x100u-ARRAY_SIZE(dmg_cart_types_end);
	if(type>=end_offset){
		return dmg_cart_types_end[type-end_offset];
	}
	return unk;
}

inline int RomSize(uint8_t type){
	static const int rom_size[] = { 32,64,128,256,512,1024,2048,4096 };
	static const int rom_size_52[] = { 1152,1280,1536 };
	if(type<ARRAY_SIZE(rom_size)){
		return rom_size[type];
	}
	if(type>=0x52 && type<0x52+ARRAY_SIZE(rom_size_52)){
		return rom_size_52[type-0x52];
	}
	return -1u;
}

static const unsigned dmg_ram_size[] = {
	0,2,8,32,128,64
};

/**
 * Nintendo's logo which is checked by bootrom.
 * (Top half only.)
 * 
 * NOTE: CGB bootrom only checks the top half of the logo.
 * (see 0x00D1 of CGB IPL)
 */
static const uint8_t dmg_nintendo[0x18] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E
};

/**
 * Game Boy ROM header.
 * 
 * NOTE: Strings are NOT null-terminated!
 */
struct DMG_RomHeader {
	uint8_t entry[4];
	uint8_t nintendo[0x30];
	
	// There are 3 variations on the next 16 bytes:
	// 1) title(16)
	// 2) title(15) cgbflag(1)
	// 3) title(11) gamecode(4) cgbflag(1)
	// In this struct we're assuming the 2nd
	
	char title[15]; // Padded with null
	uint8_t cgbflag;
	
	char new_publisher_code[2];
	uint8_t sgbflag;
	uint8_t cart_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t region;
	uint8_t old_publisher_code;
	uint8_t version;
	
	uint8_t header_checksum; // checked by bootrom
	uint8_t rom_checksum; // checked by no one
};

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
	: RomData(file, dmg_fields, ARRAY_SIZE(dmg_fields))
{
	// TODO: Only validate that this is an DMG ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the header.
	m_file->rewind();

	// Read the header. [0x150 bytes]
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
	m_isValid = isRomSupported(&info)>=0;
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
		if (!memcmp(romHeader->nintendo, dmg_nintendo, sizeof(dmg_nintendo))) {
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
 * @return System name, or nullptr if not supported.
 */
const rp_char *DMG::systemName(void) const
{
	// TODO: Store system ID.
	return _RP("Game Boy");
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

	// Read the header. [0x150 bytes]
	uint8_t header[0x150];
	m_file->rewind();
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header)) {
		// File isn't big enough for a Game Boy header...
		return -EIO;
	}

	// DMG ROM header, excluding the RST table.
	const DMG_RomHeader *romHeader = reinterpret_cast<const DMG_RomHeader*>(&header[0x100]);
	
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
	len = strnlen(romHeader->title,(romHeader->cgbflag < 0x80 ? 16 : 15));
	
	
	if ((romHeader->cgbflag&0x3F) == 0 &&		// gameid is only present when cgbflag is
	    4 == strnlen(romHeader->title+11,4)){	// gameid is always 4 characters long
		m_fields->addData_string(ascii_to_rp_string(romHeader->title,std::min(len,11)));
		m_fields->addData_string(ascii_to_rp_string(romHeader->title+11,4));
	} else{
		m_fields->addData_string(ascii_to_rp_string(romHeader->title,len));
		m_fields->addData_string(_RP("Unknown"));
	}

	// System
	uint32_t dmg_system = 0;
	if (romHeader->cgbflag & 0x80) {
		// Game supports CGB.
		dmg_system = DMG_SYSTEM_CGB;
		if (!(romHeader->cgbflag & 0x40)) {
			// Not CGB exclusive.
			dmg_system |= DMG_SYSTEM_DMG;
		}
	} else {
		// Game does not support CGB.
		dmg_system |= DMG_SYSTEM_DMG;
	}

	if (romHeader->old_publisher_code == 0x33 && romHeader->sgbflag==0x03) {
		// Game supports SGB.
		dmg_system |= DMG_SYSTEM_SGB;
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
	if(romHeader->old_publisher_code == 0x33){
		publisher = NintendoPublishers::lookup(romHeader->new_publisher_code);
	}else{
		publisher = NintendoPublishers::lookup_old(romHeader->old_publisher_code);
	}
	m_fields->addData_string(publisher);
	
	// Hardware
	m_fields->addData_string(dmg_hardware_names[CartType(romHeader->cart_type).hardware]);
	
	// Features
	m_fields->addData_bitfield(CartType(romHeader->cart_type).features);
	
	// ROM Size
	int rom_size = RomSize(romHeader->rom_size);
	if(rom_size<0){
		m_fields->addData_string(_RP("Unknown"));
	}
	else{
		if(rom_size>32){
			len = snprintf(buffer,sizeof(buffer),"%d KiB (%d banks)",rom_size,rom_size/16);
		}
		else{
			len = snprintf(buffer,sizeof(buffer),"%d KiB",rom_size);
		}
		m_fields->addData_string(ascii_to_rp_string(buffer,len));
	}
	
	// RAM Size
	if(romHeader->ram_size > ARRAY_SIZE(dmg_ram_size)){
		m_fields->addData_string(_RP("Unknown"));
	}
	else{
		int ram_size = dmg_ram_size[romHeader->ram_size];
		if(ram_size<0){
			m_fields->addData_string(_RP("Unknown"));
		}
		else if(ram_size==0 && CartType(romHeader->cart_type).hardware == DMG_HW_MBC2){
			// Not really RAM, but whatever
			m_fields->addData_string(_RP("512 x 4 bits"));
		}
		else if(ram_size==0){
			m_fields->addData_string(_RP("No RAM"));
		}
		else{
			if(ram_size>8){
				len = snprintf(buffer,sizeof(buffer),"%d KiB (%d banks)",ram_size,ram_size/8);
			}
			else{
				len = snprintf(buffer,sizeof(buffer),"%d KiB",ram_size);
			}
			m_fields->addData_string(ascii_to_rp_string(buffer,len));
		}
	}
	
	// Region
	m_fields->addData_string(romHeader->region?_RP("Non-Japanese"):_RP("Japanese"));
	
	// Revision
	m_fields->addData_string_numeric(romHeader->version, RomFields::FB_DEC, 2);
	
	// Checksum
	uint8_t checksum=0xE7; // -0x19
	for(int i=0x0134;i<0x014D;i++){
		checksum-=header[i];
	}
	if(checksum - romHeader->header_checksum){
		len = snprintf(buffer, sizeof(buffer), "0x%02X (INVALID; should be 0x%02X)",
			checksum, romHeader->header_checksum);
	}
	else{
		len = snprintf(buffer, sizeof(buffer), "0x%02X (valid)", checksum);
	}
	m_fields->addData_string(ascii_to_rp_string(buffer,len));
	
	
	return (int)m_fields->count();
}

}

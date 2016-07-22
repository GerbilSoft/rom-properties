/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NintendoDS.hpp: Nintendo DS(i) ROM reader.                              *
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

#include "NintendoDS.hpp"
#include "NintendoPublishers.hpp"
#include "TextFuncs.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cstring>

// TODO: Move this elsewhere.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#endif

namespace LibRomData {

// Hardware bitfield.
static const rp_char *ds_hw_bitfield[] = {
	_RP("Nintendo DS"), _RP("Nintendo DSi")
};

enum DS_HWType {
	DS_HW_DS	= (1 << 0),
	DS_HW_DSi	= (1 << 1),
};

// DS region bitfield.
static const rp_char *ds_region_bitfield[] = {
	_RP("Region-Free"), _RP("South Korea"), _RP("China")
};

enum DS_Region {
	DS_REGION_FREE		= (1 << 0),
	DS_REGION_SKOREA	= (1 << 1),
	DS_REGION_CHINA		= (1 << 2),
};

// DSi region bitfield.
static const rp_char *dsi_region_bitfield[] = {
	_RP("Japan"), _RP("USA"), _RP("Europe"),
	_RP("Australia"), _RP("China"), _RP("South Korea")
};

enum DSi_Region {
	DSi_REGION_JAPAN	= (1 << 0),
	DSi_REGION_USA		= (1 << 1),
	DSi_REGION_EUROPE	= (1 << 2),
	DSi_REGION_AUSTRALIA	= (1 << 3),
	DSi_REGION_CHINA	= (1 << 4),
	DSi_REGION_SKOREA	= (1 << 5),
};

// ROM fields.
// TODO: Private class?
static const struct RomFields::Desc md_fields[] = {
	// TODO: Banner?
	{_RP("Title"), RomFields::RFT_STRING, {}},
	{_RP("Game ID"), RomFields::RFT_STRING, {}},
	{_RP("Publisher"), RomFields::RFT_STRING, {}},
	{_RP("Revision"), RomFields::RFT_STRING, {}},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(ds_hw_bitfield), 2, ds_hw_bitfield}},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(ds_region_bitfield), 3, ds_region_bitfield}},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(dsi_region_bitfield), 3, dsi_region_bitfield}},

	// TODO: Icon, full game title.
};

/**
 * Nintendo DS ROM header.
 * This matches the ROM header format exactly.
 * Reference: http://problemkaputt.de/gbatek.htm#dscartridgeheader
 * 
 * NOTE: Strings are NOT null-terminated!
 */
#pragma pack(1)
struct PACKED DS_ROMHeader {
	char title[12];
	union {
		char id6[6];	// Game code. (ID6)
		struct {
			char id4[4];		// Game code. (ID4)
			char company[2];	// Company code.
		};
	};

	// 0x12
	uint8_t unitcode;	// 00h == NDS, 02h == NDS+DSi, 03h == DSi only
	uint8_t enc_seed_select;
	uint8_t device_capacity;
	uint8_t reserved1[7];
	uint8_t reserved2_dsi;
	uint8_t nds_region;	// 0x00 == normal, 0x80 == China, 0x40 == Korea
	uint8_t rom_version;
	uint8_t autostart;

	// 0x20
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm9;
	struct {
		uint32_t rom_offset;
		uint32_t entry_address;
		uint32_t ram_address;
		uint32_t size;
	} arm7;

	// 0x40
	uint32_t fnt_offset;	// File Name Table offset
	uint32_t fnt_size;	// File Name Table size
	uint32_t fat_offset;
	uint32_t fat_size;

	// 0x50
	uint32_t arm9_overlay_offset;
	uint32_t arm9_overlay_size;
	uint32_t arm7_overlay_offset;
	uint32_t arm7_overlay_size;

	// 0x60
	uint32_t cardControl13;	// Port 0x40001A4 setting for normal commands (usually 0x00586000)
	uint32_t cardControlBF;	// Port 0x40001A4 setting for KEY1 commands (usually 0x001808F8)

	// 0x68
        uint32_t icon_offset;
        uint16_t secure_area_checksum;		// CRC32 of 0x0020...0x7FFF
        uint16_t secure_area_delay;		// Delay, in 131 kHz units (0x051E=10ms, 0x0D7E=26ms)

        uint32_t arm9_auto_load_list_ram_address;
        uint32_t arm7_auto_load_list_ram_address;

        uint64_t secure_area_disable;

	// 0x80
        uint32_t total_used_rom_size;		// Excluding DSi area
        uint32_t rom_header_size;		// Usually 0x4000
        uint8_t reserved3[0x38];
        uint8_t nintendo_logo[0x9C];		// GBA-style Nintendo logo
        uint16_t nintendo_logo_checksum;	// CRC16 of nintendo_logo[] (always 0xCF56)
        uint16_t header_checksum;		// CRC16 of 0x0000...0x015D

        // 0x160
        struct {
                uint32_t rom_offset;
                uint32_t size;
                uint32_t ram_address;
        } debug;

	// 0x16C
        uint8_t reserved4[4];
        uint8_t reserved5[0x10];

	// 0x180 [DSi header]
	uint32_t dsi_region;	// DSi region flags.

	// TODO: More DSi header entries.
	// Reference: http://problemkaputt.de/gbatek.htm#dsicartridgeheader
	uint8_t reserved_more_dsi[3708];
};
#pragma pack()

/**
 * Read a Nintendo DS ROM image.
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
NintendoDS::NintendoDS(FILE *file)
	: RomData(file, md_fields, ARRAY_SIZE(md_fields))
{
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the ROM header.
	static_assert(sizeof(DS_ROMHeader) == 4096, "DS_ROMHeader is not 4,096 bytes.");
	DS_ROMHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	m_isValid = isRomSupported(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
}

/**
 * Detect if a disc image is supported by this class.
 * TODO: Actually detect the type; for now, just returns true if it's supported.
 * @param header Header data.
 * @param size Size of header.
 * @return True if the disc image is supported; false if it isn't.
 */
bool NintendoDS::isRomSupported(const uint8_t *header, size_t size)
{
	if (size >= sizeof(DS_ROMHeader)) {
		// Check the first 16 bytes of the Nintendo logo.
		static const uint8_t nintendo_gba_logo[16] = {
			0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21,
			0x3D, 0x84, 0x82, 0x0A, 0x84, 0xE4, 0x09, 0xAD
		};

		const DS_ROMHeader *ds_header = reinterpret_cast<const DS_ROMHeader*>(header);
		if (!memcmp(ds_header->nintendo_logo, nintendo_gba_logo, sizeof(nintendo_gba_logo))) {
			// Nintendo logo is present at the correct location.
			return true;
		}
	}

	// Not supported.
	return false;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NintendoDS::loadFieldData(void)
{
	if (m_fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	}
	if (!m_file) {
		// File isn't open.
		return -EBADF;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the ROM header.
	DS_ROMHeader header;
	size_t size = fread(&header, 1, sizeof(header), m_file);
	if (size != sizeof(header)) {
		// File isn't big enough for a Nintendo DS header...
		return -EIO;
	}

	// TODO
	/*
	{_RP("Title"), RomFields::RFT_STRING, {}},
	{_RP("Game ID"), RomFields::RFT_STRING, {}},
	{_RP("Publisher"), RomFields::RFT_STRING, {}},
	{_RP("Revision"), RomFields::RFT_STRING, {}},
	{_RP("Hardware"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(ds_hw_bitfield), 2, ds_hw_bitfield}},
	{_RP("DS Region"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(ds_region_bitfield), 3, ds_region_bitfield}},
	{_RP("DSi Region"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(dsi_region_bitfield), 3, dsi_region_bitfield}},
	*/

	// Game title.
	// TODO: Is Shift-JIS actually permissible here?
	m_fields->addData_string(cp1252_sjis_to_rp_string(header.title, sizeof(header.title)));

	// Game ID and publisher.
	m_fields->addData_string(ascii_to_rp_string(header.id6, sizeof(header.id6)));

	// Look up the publisher.
	const rp_char *publisher = NintendoPublishers::lookup(header.company);
	m_fields->addData_string(publisher ? publisher : _RP("Unknown"));

	// ROM version.
	m_fields->addData_string_numeric(header.rom_version, RomFields::FB_DEC, 2);

	// Hardware type.
	// NOTE: DS_HW_DS is inverted bit0; DS_HW_DSi is normal bit1.
	uint32_t hw_type = (header.unitcode & DS_HW_DS) ^ DS_HW_DS;
	hw_type |= (header.unitcode & DS_HW_DSi);
	if (hw_type == 0) {
		// 0x01 is invalid. Assume DS.
		hw_type = DS_HW_DS;
	}
	m_fields->addData_bitfield(hw_type);

	// TODO: Combine DS Region and DSi Region into one bitfield?

	// DS Region.
	uint32_t nds_region;
	if (header.nds_region == 0) {
		// Region-free.
		nds_region = DS_REGION_FREE;
	} else {
		nds_region = 0;
		if (header.nds_region & 0x80)
			nds_region |= DS_REGION_CHINA;
		if (header.nds_region & 0x40)
			nds_region |= DS_REGION_SKOREA;
	}
	printf("region: %02X\n", nds_region);
	m_fields->addData_bitfield(nds_region);

	// DSi Region.
	// Maps directly to the header field.
	if (hw_type & DS_HW_DSi) {
		m_fields->addData_bitfield(header.dsi_region);
	} else {
		// No DSi region.
		// TODO: addData_null()?
		m_fields->addData_string(nullptr);
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

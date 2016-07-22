/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.cpp: Sega Mega Drive ROM reader.                              *
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

#include "MegaDrive.hpp"
#include "MegaDrivePublishers.hpp"
#include "TextFuncs.hpp"
#include "byteswap.h"

// C includes. (C++ namespace)
#include <cstring>

// TODO: Move this elsewhere.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

namespace LibRomData {

// I/O support bitfield.
static const rp_char *md_io_bitfield[] = {
	_RP("Joypad"), _RP("6-button Joypad"), _RP("SMS Joypad"),
	_RP("Team Player"), _RP("Keyboard"), _RP("Serial I/O"),
	_RP("Printer"), _RP("Tablet"), _RP("Trackball"),
	_RP("Paddle"), _RP("Floppy Drive"), _RP("CD-ROM"),
	_RP("Activator"), _RP("Mega Mouse")
};

enum MD_IOSupport {
	MD_IO_JOYPAD_3		= (1 << 0),	// 3-button joypad
	MD_IO_JOYPAD_6		= (1 << 1),	// 6-button joypad
	MD_IO_JOYPAD_SMS	= (1 << 2),	// 2-button joypad (SMS)
	MD_IO_TEAM_PLAYER	= (1 << 3),	// Team Player
	MD_IO_KEYBOARD		= (1 << 4),	// Keyboard
	MD_IO_SERIAL		= (1 << 5),	// Serial (RS-232C)
	MD_IO_PRINTER		= (1 << 6),	// Printer
	MD_IO_TABLET		= (1 << 7),	// Tablet
	MD_IO_TRACKBALL		= (1 << 8),	// Trackball
	MD_IO_PADDLE		= (1 << 9),	// Paddle
	MD_IO_FDD		= (1 << 10),	// Floppy Drive
	MD_IO_CDROM		= (1 << 11),	// CD-ROM
	MD_IO_ACTIVATOR		= (1 << 12),	// Activator
	MD_IO_MEGA_MOUSE	= (1 << 13),	// Mega Mouse
};

// ROM fields.
// TODO: Private class?
static const struct RomFields::Desc md_fields[] = {
	{_RP("System"), RomFields::RFT_STRING, {}},
	{_RP("Copyright"), RomFields::RFT_STRING, {}},
	{_RP("Publisher"), RomFields::RFT_STRING, {}},
	{_RP("Domestic Title"), RomFields::RFT_STRING, {}},
	{_RP("Export Title"), RomFields::RFT_STRING, {}},
	{_RP("Serial Number"), RomFields::RFT_STRING, {}},
	{_RP("Checksum"), RomFields::RFT_STRING, {}},
	{_RP("I/O Support"), RomFields::RFT_BITFIELD, {ARRAY_SIZE(md_io_bitfield), 3, md_io_bitfield}},
	{_RP("ROM Range"), RomFields::RFT_STRING, {}},
	{_RP("RAM Range"), RomFields::RFT_STRING, {}},
	{_RP("SRAM Range"), RomFields::RFT_STRING, {}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {}},
	{_RP("Initial SP"), RomFields::RFT_STRING, {}}
};

/**
 * Mega Drive ROM header.
 * This matches the MD ROM header format exactly.
 * 
 * NOTE: Strings are NOT null-terminated!
 */
struct MD_RomHeader {
	char system[16];
	char copyright[16];
	char title_domestic[48];	// Japanese ROM name.
	char title_export[48];	// US/Europe ROM name.
	char serial[14];
	uint16_t checksum;
	char io_support[16];

	// ROM/RAM address information.
	uint32_t rom_start;
	uint32_t rom_end;
	uint32_t ram_start;
	uint32_t ram_end;

	// Save RAM information.
	// Info format: 'R', 'A', %1x1yz000, 0x20
	// x == 1 for backup (SRAM), 0 for not backup
	// yz == 10 for even addresses, 11 for odd addresses
	uint32_t sram_info;
	uint32_t sram_start;
	uint32_t sram_end;

	// Miscellaneous.
	char modem_info[12];
	char notes[40];
	char region_codes[16];
};

/**
 * Read a Sega Mega Drive ROM.
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
MegaDrive::MegaDrive(FILE *file)
	: RomData(file, md_fields, ARRAY_SIZE(md_fields))
{
	// TODO: Only validate that this is an MD ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	rewind(m_file);
	fflush(m_file);

	// Read the header. [0x200 bytes]
	uint8_t header[0x200];
	size_t size = fread(header, 1, sizeof(header), m_file);
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	m_isValid = isRomSupported(header, sizeof(header));
}

/**
 * Detect if a ROM is supported by this class.
 * TODO: Actually detect the type; for now, just returns true if it's supported.
 * @param header Header data.
 * @param size Size of header.
 * @return True if the ROM is supported; false if it isn't.
 */
bool MegaDrive::isRomSupported(const uint8_t *header, size_t size)
{
	// TODO: Handle SMD and other interleaved formats.
	// TODO: Handle Sega CD.
	// TODO: Store specific system type.

	// Check for certain strings at $100 and/or $101.
	// TODO: Better checks.
	static const char strchk[][17] = {
		"SEGA MEGA DRIVE ",
		"SEGA GENESIS    ",
		"SEGA PICO       ",
		"SEGA 32X        ",
	};

	if (size >= 0x200) {
		// Check the system name.
		const MD_RomHeader *romHeader = reinterpret_cast<const MD_RomHeader*>(&header[0x100]);
		for (int i = 0; i < 4; i++) {
			if (!strncmp(romHeader->system, strchk[i], 16) ||
			    !strncmp(&romHeader->system[1], strchk[i], 15))
			{
				// Found a Mega Drive ROM.
				return true;
			}
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
int MegaDrive::loadFieldData(void)
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

	// Read the header. [0x200 bytes]
	uint8_t header[0x200];
	size_t size = fread(header, 1, sizeof(header), m_file);
	if (size != sizeof(header)) {
		// File isn't big enough for an MD header...
		return -EIO;
	}

	// MD ROM header, excluding the vector table.
	const MD_RomHeader *romHeader = reinterpret_cast<const MD_RomHeader*>(&header[0x100]);

	// Read the strings from the header.
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->system, sizeof(romHeader->system)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->copyright, sizeof(romHeader->copyright)));

	// Determine the publisher.
	// Formats in the copyright line:
	// - "(C)SEGA"
	// - "(C)T-xx"
	// - "(C)T-xxx"
	// - "(C)Txxx"
	const rp_char *publisher = nullptr;
	unsigned int t_code = 0;
	if (!memcmp(romHeader->copyright, "(C)SEGA", 7)) {
		// Sega first-party game.
		publisher = _RP("Sega");
	} else if (!memcmp(romHeader->copyright, "(C)T", 4)) {
		// Third-party game.
		int start = 4;
		if (romHeader->copyright[4] == '-')
			start++;
		char *endptr;
		t_code = strtoul(&romHeader->copyright[start], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &romHeader->copyright[start] &&
		    endptr < &romHeader->copyright[start+3])
		{
			// Valid T-code. Look up the publisher.
			publisher = MegaDrivePublishers::lookup(t_code);
		}
	}

	if (publisher) {
		// Publisher identified.
		m_fields->addData_string(publisher);
	} else if (t_code > 0) {
		// Unknown publisher, but there is a valid T code.
		char buf[16];
		int len = snprintf(buf, sizeof(buf), "T-%u", t_code);
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));
	} else {
		// Unknown publisher.
		m_fields->addData_string(_RP("Unknown"));
	}

	// Titles, serial number, and checksum.
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_domestic, sizeof(romHeader->title_domestic)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_export, sizeof(romHeader->title_export)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->serial, sizeof(romHeader->serial)));
	m_fields->addData_string_numeric(be16_to_cpu(romHeader->checksum), RomFields::FB_HEX, 4);

	// Parse I/O support.
	uint32_t io_support = 0;
	for (int i = (int)sizeof(romHeader->io_support)-1; i >= 0; i--) {
		switch (romHeader->io_support[i]) {
			case 'J':
				io_support |= MD_IO_JOYPAD_3;
				break;
			case '6':
				io_support |= MD_IO_JOYPAD_6;
				break;
			case '0':
				io_support |= MD_IO_JOYPAD_SMS;
				break;
			case '4':
				io_support |= MD_IO_TEAM_PLAYER;
				break;
			case 'K':
				io_support |= MD_IO_KEYBOARD;
				break;
			case 'R':
				io_support |= MD_IO_SERIAL;
				break;
			case 'P':
				io_support |= MD_IO_PRINTER;
				break;
			case 'T':
				io_support |= MD_IO_TABLET;
				break;
			case 'B':
				io_support |= MD_IO_TRACKBALL;
				break;
			case 'V':
				io_support |= MD_IO_PADDLE;
				break;
			case 'F':
				io_support |= MD_IO_FDD;
				break;
			case 'C':
				io_support |= MD_IO_CDROM;
				break;
			case 'L':
				io_support |= MD_IO_ACTIVATOR;
				break;
			case 'M':
				io_support |= MD_IO_MEGA_MOUSE;
				break;
			default:
				break;
		}
	}

	// Add the I/O support field.
	m_fields->addData_bitfield(io_support);

	// ROM range.
	// TODO: Range helper? (Can't be used for SRAM, though...)
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "0x%08X - 0x%08X",
			be32_to_cpu(romHeader->rom_start),
			be32_to_cpu(romHeader->rom_end));
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	m_fields->addData_string(len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));

	// RAM range.
	len = snprintf(buf, sizeof(buf), "0x%08X - 0x%08X",
			be32_to_cpu(romHeader->ram_start),
			be32_to_cpu(romHeader->ram_end));
	if (len > (int)sizeof(buf))
		len = sizeof(buf);
	m_fields->addData_string(len > 0 ? ascii_to_rp_string(buf, len) : _RP(""));

	// SRAM range. (TODO)
	m_fields->addData_string(_RP(""));

	// Vectors.
	const uint32_t *vectors = reinterpret_cast<const uint32_t*>(header);
	m_fields->addData_string_numeric(be32_to_cpu(vectors[1]), RomFields::FB_HEX, 8);	// Entry point
	m_fields->addData_string_numeric(be32_to_cpu(vectors[0]), RomFields::FB_HEX, 8);	// Initial SP

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

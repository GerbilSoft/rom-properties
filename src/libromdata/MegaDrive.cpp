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
#include "common.h"

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class MegaDrivePrivate
{
	public:
		MegaDrivePrivate();

	private:
		MegaDrivePrivate(const MegaDrivePrivate &other);
		MegaDrivePrivate &operator=(const MegaDrivePrivate &other);

	public:
		/** RomFields **/

		// I/O support. (RFT_BITFIELD)
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
		static const rp_char *const md_io_bitfield_names[];
		static const RomFields::BitfieldDesc md_io_bitfield;

		// Region code. (RFT_BITFIELD)
		enum MD_RegionCode {
			MD_REGION_JAPAN		= (1 << 0),
			MD_REGION_ASIA		= (1 << 1),
			MD_REGION_USA		= (1 << 2),
			MD_REGION_EUROPE	= (1 << 3),
		};
		static const rp_char *const md_region_code_bitfield_names[];
		static const RomFields::BitfieldDesc md_region_code_bitfield;

		// Monospace string formatting.
		static const RomFields::StringDesc md_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc md_fields[];

		/** Internal ROM data. **/

		/**
		 * Mega Drive ROM header.
		 * This matches the MD ROM header format exactly.
		 *
		 * NOTE: Strings are NOT null-terminated!
		 */
		#pragma pack(1)
		struct PACKED MD_RomHeader {
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
		#pragma pack()

		/**
		 * Parse the I/O support field.
		 * @param io_support I/O support field.
		 * @param size Size of io_support.
		 * @return io_support bitfield.
		 */
		uint32_t parseIOSupport(const char *io_support, int size);

		/**
		 * Parse the region codes field.
		 * @param region_codes Region codes field.
		 * @param size Size of region_codes.
		 * @return region_codes bitfield.
		 */
		uint32_t parseRegionCodes(const char *region_codes, int size);

	public:
		// ROM header.
		uint32_t vectors[64];	// Interrupt vectors. (BE32)
		MD_RomHeader romHeader;	// ROM header.
};

// I/O support bitfield.
const rp_char *const MegaDrivePrivate::md_io_bitfield_names[] = {
	_RP("Joypad"), _RP("6-button"), _RP("SMS Joypad"),
	_RP("Team Player"), _RP("Keyboard"), _RP("Serial I/O"),
	_RP("Printer"), _RP("Tablet"), _RP("Trackball"),
	_RP("Paddle"), _RP("Floppy Drive"), _RP("CD-ROM"),
	_RP("Activator"), _RP("Mega Mouse")
};

const RomFields::BitfieldDesc MegaDrivePrivate::md_io_bitfield = {
	ARRAY_SIZE(md_io_bitfield_names), 3, md_io_bitfield_names
};

// Region code.
const rp_char *const MegaDrivePrivate::md_region_code_bitfield_names[] = {
	_RP("Japan"), _RP("Asia"),
	_RP("USA"), _RP("Europe")
};

const RomFields::BitfieldDesc MegaDrivePrivate::md_region_code_bitfield = {
	ARRAY_SIZE(md_region_code_bitfield_names), 0, md_region_code_bitfield_names
};

// Monospace string formatting.
const RomFields::StringDesc MegaDrivePrivate::md_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// ROM fields.
const struct RomFields::Desc MegaDrivePrivate::md_fields[] = {
	{_RP("System"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Copyright"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Publisher"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Domestic Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Export Title"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Serial Number"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Checksum"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("I/O Support"), RomFields::RFT_BITFIELD, {&md_io_bitfield}},
	{_RP("ROM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("RAM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("SRAM Range"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("Region Code"), RomFields::RFT_BITFIELD, {&md_region_code_bitfield}},
	{_RP("Entry Point"), RomFields::RFT_STRING, {&md_string_monospace}},
	{_RP("Initial SP"), RomFields::RFT_STRING, {&md_string_monospace}}
};

/**
 * Parse the I/O support field.
 * @param io_support I/O support field.
 * @param size Size of io_support.
 * @return io_support bitfield.
 */
uint32_t MegaDrivePrivate::parseIOSupport(const char *io_support, int size)
{
	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		switch (io_support[i]) {
			case 'J':
				ret |= MD_IO_JOYPAD_3;
				break;
			case '6':
				ret |= MD_IO_JOYPAD_6;
				break;
			case '0':
				ret |= MD_IO_JOYPAD_SMS;
				break;
			case '4':
				ret |= MD_IO_TEAM_PLAYER;
				break;
			case 'K':
				ret |= MD_IO_KEYBOARD;
				break;
			case 'R':
				ret |= MD_IO_SERIAL;
				break;
			case 'P':
				ret |= MD_IO_PRINTER;
				break;
			case 'T':
				ret |= MD_IO_TABLET;
				break;
			case 'B':
				ret |= MD_IO_TRACKBALL;
				break;
			case 'V':
				ret |= MD_IO_PADDLE;
				break;
			case 'F':
				ret |= MD_IO_FDD;
				break;
			case 'C':
				ret |= MD_IO_CDROM;
				break;
			case 'L':
				ret |= MD_IO_ACTIVATOR;
				break;
			case 'M':
				ret |= MD_IO_MEGA_MOUSE;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Parse the region codes field.
 * @param region_codes Region codes field.
 * @param size Size of region_codes.
 * @return region_codes bitfield.
 */
uint32_t MegaDrivePrivate::parseRegionCodes(const char *region_codes, int size)
{
	// Make sure the region codes field is valid.
	assert(region_codes != nullptr);	// NOT checking this in release builds.
	assert(size > 0);
	if (size <= 0)
		return 0;

	uint32_t ret = 0;

	// Check for a hex code.
	if (isalnum(region_codes[0]) &
	    (region_codes[1] == 0 || isspace(region_codes[1])))
	{
		// Single character region code.
		// Assume it's a hex code, *unless* it's 'E'.
		char code = toupper(region_codes[0]);
		if (code >= '0' && code <= '9') {
			// Numeric code from '0' to '9'.
			ret = code - '0';
		} else if (code == 'E') {
			// 'E'. This is probably Europe.
			// If interpreted as a hex code, this would be
			// Asia, USA, and Europe, with Japan excluded.
			ret = MD_REGION_EUROPE;
		} else if (code >= 'A' && code <= 'F') {
			// Letter code from 'A' to 'F'.
			ret = (code - 'A') + 10;
		}
	} else if (region_codes[0] < 16) {
		// Hex code not mapped to ASCII.
		ret = region_codes[0];
	}

	if (ret == 0) {
		// Not a hex code, or the hex code was 0.
		// Hex code being 0 shouldn't happen...

		// Check for string region codes.
		// Some games incorrectly use these.
		if (!strncasecmp(region_codes, "EUR", 3)) {
			ret = MD_REGION_EUROPE;
		} else if (!strncasecmp(region_codes, "USA", 3)) {
			ret = MD_REGION_USA;
		} else if (!strncasecmp(region_codes, "JPN", 3) ||
			   !strncasecmp(region_codes, "JAP", 3))
		{
			ret = MD_REGION_JAPAN | MD_REGION_ASIA;
		} else {
			// Check for old-style JUE region codes.
			// (J counts as both Japan and Asia.)
			for (int i = 0; i < size; i++) {
				if (region_codes[i] == 0 || isspace(region_codes[i]))
					break;
				switch (region_codes[i]) {
					case 'J':
						ret |= MD_REGION_JAPAN | MD_REGION_ASIA;
						break;
					case 'U':
						ret |= MD_REGION_USA;
						break;
					case 'E':
						ret |= MD_REGION_EUROPE;
						break;
					default:
						break;
				}
			}
		}
	}

	return ret;
}

/** MegaDrivePrivate **/

MegaDrivePrivate::MegaDrivePrivate()
{ }

/** MegaDrive **/

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
MegaDrive::MegaDrive(IRpFile *file)
	: RomData(file, MegaDrivePrivate::md_fields, ARRAY_SIZE(MegaDrivePrivate::md_fields))
	, d(new MegaDrivePrivate())
{
	// TODO: Only validate that this is an MD ROM here.
	// Load fields elsewhere.
	if (!m_file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	m_file->rewind();

	// Read the header. [0x200 bytes]
	uint8_t header[0x200];
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for MD.
	info.szFile = 0;	// Not needed for MD.
	m_isValid = (isRomSupported(&info) >= 0);

	if (m_isValid) {
		// Save the header for later.
		// TODO: Adjust for SMD, Sega CD, etc.
		static_assert(sizeof(d->romHeader) == 0x100, "MD_RomHeader is the wrong size. (should be 256 bytes)");
		memcpy(&d->vectors,    header,        sizeof(d->vectors));
		memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
	}
}

MegaDrive::~MegaDrive()
{
	delete d;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MegaDrive::isRomSupported_static(const DetectInfo *info)
{
	if (!info)
		return -1;

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

	if (info->szHeader >= 0x200) {
		// Check the system name.
		const MegaDrivePrivate::MD_RomHeader *romHeader =
			reinterpret_cast<const MegaDrivePrivate::MD_RomHeader*>(&info->pHeader[0x100]);
		for (int i = 0; i < 4; i++) {
			if (!strncmp(romHeader->system, strchk[i], 16) ||
			    !strncmp(&romHeader->system[1], strchk[i], 15))
			{
				// Found a Mega Drive ROM.
				// TODO: Identify the specific type.
				return 0;
			}
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
int MegaDrive::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @return System name, or nullptr if not supported.
 */
const rp_char *MegaDrive::systemName(void) const
{
	// TODO: Store system ID.
	return _RP("Mega Drive");
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
vector<const rp_char*> MegaDrive::supportedFileExtensions(void) const
{
	// NOTE: Not including ".md" due to conflicts with Markdown.
	// TODO: Add ".bin" later? (Too generic, though...)
	vector<const rp_char*> ret;
	ret.reserve(2);
	ret.push_back(_RP(".gen"));
	ret.push_back(_RP(".smd"));
	return ret;
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
	} else if (!m_file || !m_file->isOpen()) {
		// File isn't open.
		// NOTE: We already loaded the header,
		// so *maybe* this is okay?
		return -EBADF;
	} else if (!m_isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// MD ROM header, excluding the vector table.
	const MegaDrivePrivate::MD_RomHeader *romHeader = &d->romHeader;

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
	uint32_t io_support = d->parseIOSupport(romHeader->io_support, sizeof(romHeader->io_support));
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

	// Region codes.
	uint32_t region_code = d->parseRegionCodes(romHeader->region_codes, sizeof(romHeader->region_codes));
	m_fields->addData_bitfield(region_code);

	// Vectors.
	m_fields->addData_string_numeric(be32_to_cpu(d->vectors[1]), RomFields::FB_HEX, 8);	// Entry point
	m_fields->addData_string_numeric(be32_to_cpu(d->vectors[0]), RomFields::FB_HEX, 8);	// Initial SP

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

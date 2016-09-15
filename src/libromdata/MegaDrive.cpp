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
#include "CopierFormats.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

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
		MegaDrivePrivate() { }

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
		#define MD_RomHeader_SIZE 256
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
		enum MD_RomType {
			ROM_UNKNOWN = -1,		// Unknown ROM type.

			// Low byte: System ID.
			// (TODO: MCD Boot ROMs, other specialized types?)
			ROM_SYSTEM_MD = 0,		// Mega Drive
			ROM_SYSTEM_MCD = 1,		// Mega CD
			ROM_SYSTEM_32X = 2,		// Sega 32X
			ROM_SYSTEM_MCD32X = 3,		// Sega CD 32X
			ROM_SYSTEM_PICO = 4,		// Sega Pico
			ROM_SYSTEM_UNKNOWN = 0xFF,
			ROM_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			ROM_FORMAT_CART_BIN = (0 << 8),		// Cartridge: Binary format.
			ROM_FORMAT_CART_SMD = (1 << 8),		// Cartridge: SMD format.
			ROM_FORMAT_DISC_2048 = (2 << 8),	// Disc: 2048-byte sectors. (ISO)
			ROM_FORMAT_DISC_2352 = (3 << 8),	// Disc: 2352-byte sectors. (BIN)
			ROM_FORMAT_UNKNOWN = (0xFF << 8),
			ROM_FORMAT_MASK = (0xFF << 8),
		};

		// ROM type.
		int romType;

		// SMD bank size.
		static const uint32_t SMD_BLOCK_SIZE = 16384;

		/**
		 * Is this a disc?
		 * Discs don't have a vector table.
		 * @return True if this is a disc; false if not.
		 */
		inline bool isDisc(void)
		{
			int rfmt = romType & ROM_FORMAT_MASK;
			return (rfmt == ROM_FORMAT_DISC_2048 ||
				rfmt == ROM_FORMAT_DISC_2352);
		}

		/**
		 * Decode a Super Magic Drive interleaved block.
		 * @param dest	[out] Destination block. (Must be 16 KB.)
		 * @param src	[in] Source block. (Must be 16 KB.)
		 */
		static void decodeSMDBlock(uint8_t dest[SMD_BLOCK_SIZE], const uint8_t src[SMD_BLOCK_SIZE]);

	public:
		// ROM header.
		uint32_t vectors[64];	// Interrupt vectors. (BE32)
		MD_RomHeader romHeader;	// ROM header.
		SMD_Header smdHeader;	// SMD header.
};

/** MegaDrivePrivate **/

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

/** Internal ROM data. **/

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

/**
 * Decode a Super Magic Drive interleaved block.
 * @param dest	[out] Destination block. (Must be 16 KB.)
 * @param src	[in] Source block. (Must be 16 KB.)
 */
void MegaDrivePrivate::decodeSMDBlock(uint8_t dest[SMD_BLOCK_SIZE], const uint8_t src[SMD_BLOCK_SIZE])
{
	// TODO: Add the "restrict" keyword to both parameters?

	// First 8 KB of the source block is ODD bytes.
	const uint8_t *end_block = src + 8192;
	for (uint8_t *odd = dest + 1; src < end_block; odd += 16, src += 8) {
		odd[ 0] = src[0];
		odd[ 2] = src[1];
		odd[ 4] = src[2];
		odd[ 6] = src[3];
		odd[ 8] = src[4];
		odd[10] = src[5];
		odd[12] = src[6];
		odd[14] = src[7];
	}

	// Second 8 KB of the source block is EVEN bytes.
	end_block = src + 8192;
	for (uint8_t *even = dest; src < end_block; even += 16, src += 8) {
		even[ 0] = src[ 0];
		even[ 2] = src[ 1];
		even[ 4] = src[ 2];
		even[ 6] = src[ 3];
		even[ 8] = src[ 4];
		even[10] = src[ 5];
		even[12] = src[ 6];
		even[14] = src[ 7];
	}
}

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

	// Read the ROM header. [0x400 bytes]
	static_assert(sizeof(MegaDrivePrivate::MD_RomHeader) == MD_RomHeader_SIZE,
		"MD_RomHeader_SIZE is the wrong size. (Should be 256 bytes.)");
	uint8_t header[0x400];
	size_t size = m_file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.pHeader = header;
	info.szHeader = sizeof(header);
	info.ext = nullptr;	// Not needed for MD.
	info.szFile = 0;	// Not needed for MD.
	d->romType = isRomSupported(&info);

	if (d->romType >= 0) {
		// Save the header for later.
		// TODO (remove before committing): Does gcc/msvc optimize this into a jump table?
		switch (d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) {
			case MegaDrivePrivate::ROM_FORMAT_CART_BIN:
				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    header,        sizeof(d->vectors));
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_CART_SMD: {
				// Save the SMD header.
				memcpy(&d->smdHeader, header, sizeof(d->smdHeader));

				// First bank needs to be deinterleaved.
				uint8_t smd_data[MegaDrivePrivate::SMD_BLOCK_SIZE];
				uint8_t bin_data[MegaDrivePrivate::SMD_BLOCK_SIZE];
				m_file->seek(512);
				size = m_file->read(smd_data, sizeof(smd_data));
				if (size != sizeof(smd_data)) {
					// Short read. ROM is invalid.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					break;
				}

				// Decode the SMD block.
				d->decodeSMDBlock(bin_data, smd_data);

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    bin_data,        sizeof(d->vectors));
				memcpy(&d->romHeader, &bin_data[0x100], sizeof(d->romHeader));
				break;
			}

			case MegaDrivePrivate::ROM_FORMAT_DISC_2048:
				// MCD-specific header is at 0. [TODO]
				// MD-style header is at 0x100.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_DISC_2352:
				// MCD-specific header is at 0x10. [TODO]
				// MD-style header is at 0x110.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x110], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_UNKNOWN:
			default:
				d->romType = MegaDrivePrivate::ROM_UNKNOWN;
				break;
		}
	}

	m_isValid = (d->romType >= 0);
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

	// ROM header.
	const uint8_t *const pHeader = info->pHeader;

	// Magic strings.
	static const char sega_magic[4] = {'S','E','G','A'};
	static const char segacd_magic[16] =
		{'S','E','G','A','D','I','S','C','S','Y','S','T','E','M',' ',' '};;

	static const struct {
		const char system_name[16];
		uint32_t system_id;
	} cart_magic[] = {
		{{'S','E','G','A',' ','P','I','C','O',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_PICO},
		{{'S','E','G','A',' ','3','2','X',' ',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_32X},
		{{'S','E','G','A',' ','M','E','G','A',' ','D','R','I','V','E',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
		{{'S','E','G','A',' ','G','E','N','E','S','I','S',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
	};

	if (info->szHeader >= 0x200) {
		// Check for Sega CD.
		// TODO: Gens/GS II lists "ISO/2048", "ISO/2352",
		// "BIN/2048", and "BIN/2352". I don't think that's
		// right; there should only be 2048 and 2352.
		// TODO: Detect Sega CD 32X.
		if (!memcmp(&pHeader[0x0010], segacd_magic, sizeof(segacd_magic))) {
			// Found a Sega CD disc image. (2352-byte sectors)
			return MegaDrivePrivate::ROM_SYSTEM_MCD |
			       MegaDrivePrivate::ROM_FORMAT_DISC_2352;
		} else if (!memcmp(&pHeader[0x0000], segacd_magic, sizeof(segacd_magic))) {
			// Found a Sega CD disc image. (2048-byte sectors)
			return MegaDrivePrivate::ROM_SYSTEM_MCD |
			       MegaDrivePrivate::ROM_FORMAT_DISC_2048;
		}

		// Check for SMD format. (Mega Drive only)
		if (info->szHeader >= 0x300) {
			// Check if "SEGA" is in the header in the correct place
			// for a plain binary ROM.
			if (memcmp(&pHeader[0x100], sega_magic, sizeof(sega_magic)) != 0 &&
			    memcmp(&pHeader[0x101], sega_magic, sizeof(sega_magic)) != 0)
			{
				// "SEGA" is not in the header. This might be SMD.
				const SMD_Header *smdHeader = reinterpret_cast<const SMD_Header*>(pHeader);
				if (smdHeader->id[0] == 0xAA && smdHeader->id[1] == 0xBB &&
				    smdHeader->smd.file_data_type == SMD_FDT_68K_PROGRAM &&
				    smdHeader->file_type == SMD_FT_SMD_GAME_FILE)
				{
					// This is an SMD-format ROM.
					// TODO: Show extended information from the SMD header,
					// including "split" and other stuff?
					return MegaDrivePrivate::ROM_SYSTEM_MD |
					       MegaDrivePrivate::ROM_FORMAT_CART_SMD;
				}
			}
		}

		// Check for other MD-based cartridge formats.
		for (int i = 0; i < ARRAY_SIZE(cart_magic); i++) {
			if (!memcmp(&pHeader[0x100], cart_magic[i].system_name, 16) ||
			    !memcmp(&pHeader[0x101], cart_magic[i].system_name, 15))
			{
				// Found a matching system name.
				return MegaDrivePrivate::ROM_FORMAT_CART_BIN | cart_magic[i].system_id;
			}
		}
	}

	// Not supported.
	return MegaDrivePrivate::ROM_UNKNOWN;
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
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *MegaDrive::systemName(uint32_t type) const
{
	if (!m_isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// FIXME: Lots of system names and regions to check.
	// Also, games can be region-free, so we need to check
	// against the host system's locale.
	// For now, just use the generic "Mega Drive".

	static_assert(SYSNAME_TYPE_MASK == 3,
		"MegaDrive::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (short, long, abbreviation)
	static const rp_char *const sysNames[4] = {
		_RP("Sega Mega Drive"), _RP("Mega Drive"), _RP("MD"), nullptr
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
vector<const rp_char*> MegaDrive::supportedFileExtensions_static(void)
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
	return supportedFileExtensions_static();
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
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
	} else {
		// Unknown publisher.
		m_fields->addData_string(_RP("Unknown"));
	}

	// Titles, serial number, and checksum.
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_domestic, sizeof(romHeader->title_domestic)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->title_export, sizeof(romHeader->title_export)));
	m_fields->addData_string(cp1252_sjis_to_rp_string(romHeader->serial, sizeof(romHeader->serial)));
	if (!d->isDisc()) {
		m_fields->addData_string_numeric(be16_to_cpu(romHeader->checksum), RomFields::FB_HEX, 4);
	} else {
		// Checksum is not valid in Mega CD headers.
		m_fields->addData_invalid();
	}

	// Parse I/O support.
	uint32_t io_support = d->parseIOSupport(romHeader->io_support, sizeof(romHeader->io_support));
	m_fields->addData_bitfield(io_support);

	if (!d->isDisc()) {
		// ROM range.
		// TODO: Range helper? (Can't be used for SRAM, though...)
		char buf[32];
		int len = snprintf(buf, sizeof(buf), "0x%08X - 0x%08X",
				be32_to_cpu(romHeader->rom_start),
				be32_to_cpu(romHeader->rom_end));
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

		// RAM range.
		len = snprintf(buf, sizeof(buf), "0x%08X - 0x%08X",
				be32_to_cpu(romHeader->ram_start),
				be32_to_cpu(romHeader->ram_end));
		if (len > (int)sizeof(buf))
			len = sizeof(buf);
		m_fields->addData_string(len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));

		// SRAM range. (TODO)
		m_fields->addData_string(_RP(""));
	} else {
		// ROM, RAM, and SRAM ranges are not valid in Mega CD headers.
		m_fields->addData_invalid();
		m_fields->addData_invalid();
		m_fields->addData_invalid();
	}

	// Region codes.
	// TODO: Validate the Mega CD security program?
	uint32_t region_code = d->parseRegionCodes(romHeader->region_codes, sizeof(romHeader->region_codes));
	m_fields->addData_bitfield(region_code);

	// Vectors.
	if (!d->isDisc()) {
		m_fields->addData_string_numeric(be32_to_cpu(d->vectors[1]), RomFields::FB_HEX, 8);	// Entry point
		m_fields->addData_string_numeric(be32_to_cpu(d->vectors[0]), RomFields::FB_HEX, 8);	// Initial SP
	} else {
		// Discs don't have vector tables.
		// Add dummy entries for the vectors.
		m_fields->addData_invalid();
		m_fields->addData_invalid();
	}

	// Finished reading the field data.
	return (int)m_fields->count();
}

}

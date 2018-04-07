/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.cpp: Sega Mega Drive ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "MegaDrive.hpp"
#include "librpbase/RomData_p.hpp"

#include "md_structs.h"
#include "data/SegaPublishers.hpp"
#include "MegaDriveRegions.hpp"
#include "CopierFormats.h"
#include "utils/SuperMagicDrive.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/aligned_malloc.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes.
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <memory>
#include <string>
#include <vector>
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(MegaDrive)

class MegaDrivePrivate : public RomDataPrivate
{
	public:
		MegaDrivePrivate(MegaDrive *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(MegaDrivePrivate)

	public:
		/** RomFields **/

		// I/O support. (RFT_BITFIELD)
		enum MD_IO_Support_Bitfield {
			MD_IOBF_JOYPAD_3	= (1 << 0),	// 3-button joypad
			MD_IOBF_JOYPAD_6	= (1 << 1),	// 6-button joypad
			MD_IOBF_JOYPAD_SMS	= (1 << 2),	// 2-button joypad (SMS)
			MD_IOBF_TEAM_PLAYER	= (1 << 3),	// Team Player
			MD_IOBF_KEYBOARD	= (1 << 4),	// Keyboard
			MD_IOBF_SERIAL		= (1 << 5),	// Serial (RS-232C)
			MD_IOBF_PRINTER		= (1 << 6),	// Printer
			MD_IOBF_TABLET		= (1 << 7),	// Tablet
			MD_IOBF_TRACKBALL	= (1 << 8),	// Trackball
			MD_IOBF_PADDLE		= (1 << 9),	// Paddle
			MD_IOBF_FDD		= (1 << 10),	// Floppy Drive
			MD_IOBF_CDROM		= (1 << 11),	// CD-ROM
			MD_IOBF_ACTIVATOR	= (1 << 12),	// Activator
			MD_IOBF_MEGA_MOUSE	= (1 << 13),	// Mega Mouse
		};

		/** Internal ROM data. **/

		/**
		 * Parse the I/O support field.
		 * @param io_support I/O support field.
		 * @param size Size of io_support.
		 * @return io_support bitfield.
		 */
		static uint32_t parseIOSupport(const char *io_support, int size);

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
			ROM_SYSTEM_MAX = ROM_SYSTEM_PICO,
			ROM_SYSTEM_UNKNOWN = 0xFF,
			ROM_SYSTEM_MASK = 0xFF,

			// High byte: Image format.
			ROM_FORMAT_CART_BIN = (0 << 8),		// Cartridge: Binary format.
			ROM_FORMAT_CART_SMD = (1 << 8),		// Cartridge: SMD format.
			ROM_FORMAT_DISC_2048 = (2 << 8),	// Disc: 2048-byte sectors. (ISO)
			ROM_FORMAT_DISC_2352 = (3 << 8),	// Disc: 2352-byte sectors. (BIN)
			ROM_FORMAT_MAX = ROM_FORMAT_DISC_2352,
			ROM_FORMAT_UNKNOWN = (0xFF << 8),
			ROM_FORMAT_MASK = (0xFF << 8),
		};

		int romType;		// ROM type.
		unsigned int md_region;	// MD hexadecimal region code.

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

	public:
		/**
		 * Add fields for the ROM header.
		 *
		 * This function will not create a new tab.
		 * If one is desired, it should be created
		 * before calling this function.
		 *
		 * @param pRomHeader ROM header.
		 */
		void addFields_romHeader(const MD_RomHeader *pRomHeader);

		/**
		 * Add fields for the vector table.
		 *
		 * This function will not create a new tab.
		 * If one is desired, it should be created
		 * before calling this function.
		 *
		 * @param pVectors Vector table.
		 */
		void addFields_vectorTable(const M68K_VectorTable *pVectors);

	public:
		// ROM header.
		// NOTE: Must be byteswapped on access.
		M68K_VectorTable vectors;	// Interrupt vectors.
		MD_RomHeader romHeader;		// ROM header.
		SMD_Header smdHeader;		// SMD header.
};

/** MegaDrivePrivate **/

MegaDrivePrivate::MegaDrivePrivate(MegaDrive *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
	, md_region(0)
{
	// Clear the various structs.
	memset(&vectors, 0, sizeof(vectors));
	memset(&romHeader, 0, sizeof(romHeader));
	memset(&smdHeader, 0, sizeof(smdHeader));
}

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
			case MD_IO_JOYPAD_3:
				ret |= MD_IOBF_JOYPAD_3;
				break;
			case MD_IO_JOYPAD_6:
				ret |= MD_IOBF_JOYPAD_6;
				break;
			case MD_IO_JOYPAD_SMS:
				ret |= MD_IOBF_JOYPAD_SMS;
				break;
			case MD_IO_TEAM_PLAYER:
				ret |= MD_IOBF_TEAM_PLAYER;
				break;
			case MD_IO_KEYBOARD:
				ret |= MD_IOBF_KEYBOARD;
				break;
			case MD_IO_SERIAL:
				ret |= MD_IOBF_SERIAL;
				break;
			case MD_IO_PRINTER:
				ret |= MD_IOBF_PRINTER;
				break;
			case MD_IO_TABLET:
				ret |= MD_IOBF_TABLET;
				break;
			case MD_IO_TRACKBALL:
				ret |= MD_IOBF_TRACKBALL;
				break;
			case MD_IO_PADDLE:
				ret |= MD_IOBF_PADDLE;
				break;
			case MD_IO_FDD:
				ret |= MD_IOBF_FDD;
				break;
			case MD_IO_CDROM:
				ret |= MD_IOBF_CDROM;
				break;
			case MD_IO_ACTIVATOR:
				ret |= MD_IOBF_ACTIVATOR;
				break;
			case MD_IO_MEGA_MOUSE:
				ret |= MD_IOBF_MEGA_MOUSE;
				break;
			default:
				break;
		}
	}

	return ret;
}

/**
 * Add fields for the ROM header.
 *
 * This function will not create a new tab.
 * If one is desired, it should be created
 * before calling this function.
 *
 * @param pRomHeader ROM header.
 */
void MegaDrivePrivate::addFields_romHeader(const MD_RomHeader *pRomHeader)
{
	// Read the strings from the header.
	fields->addField_string(C_("MegaDrive", "System"),
		cp1252_sjis_to_utf8(pRomHeader->system, sizeof(pRomHeader->system)),
			RomFields::STRF_TRIM_END);
	fields->addField_string(C_("MegaDrive", "Copyright"),
		cp1252_sjis_to_utf8(pRomHeader->copyright, sizeof(pRomHeader->copyright)),
			RomFields::STRF_TRIM_END);

	// Determine the publisher.
	// Formats in the copyright line:
	// - "(C)SEGA"
	// - "(C)T-xx"
	// - "(C)T-xxx"
	// - "(C)Txxx"
	const char *publisher = nullptr;
	unsigned int t_code = 0;
	if (!memcmp(pRomHeader->copyright, "(C)SEGA", 7)) {
		// Sega first-party game.
		publisher = "Sega";
	} else if (!memcmp(pRomHeader->copyright, "(C)T", 4)) {
		// Third-party game.
		int start = 4;
		if (pRomHeader->copyright[4] == '-')
			start++;
		char *endptr;
		t_code = strtoul(&pRomHeader->copyright[start], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &pRomHeader->copyright[start] &&
		    endptr < &pRomHeader->copyright[start+3])
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		// Publisher identified.
		fields->addField_string(C_("MegaDrive", "Publisher"), publisher);
	} else if (t_code > 0) {
		// Unknown publisher, but there is a valid T code.
		fields->addField_string(C_("MegaDrive", "Publisher"), rp_sprintf("T-%u", t_code));
	} else {
		// Unknown publisher.
		fields->addField_string(C_("MegaDrive", "Publisher"),
			C_("MegaDrive", "Unknown"));
	}

	// Titles, serial number, and checksum.
	fields->addField_string(C_("MegaDrive", "Domestic Title"),
		cp1252_sjis_to_utf8(pRomHeader->title_domestic, sizeof(pRomHeader->title_domestic)),
			RomFields::STRF_TRIM_END);
	fields->addField_string(C_("MegaDrive", "Export Title"),
		cp1252_sjis_to_utf8(pRomHeader->title_export, sizeof(pRomHeader->title_export)),
			RomFields::STRF_TRIM_END);
	fields->addField_string(C_("MegaDrive", "Serial Number"),
		cp1252_sjis_to_utf8(pRomHeader->serial, sizeof(pRomHeader->serial)),
			RomFields::STRF_TRIM_END);
	if (!isDisc()) {
		// Checksum. (MD only; not valid for Mega CD.)
		fields->addField_string_numeric(C_("MegaDrive", "Checksum"),
			be16_to_cpu(pRomHeader->checksum), RomFields::FB_HEX, 4,
			RomFields::STRF_MONOSPACE);
	}

	// I/O support bitfield.
	static const char *const io_bitfield_names[] = {
		NOP_C_("MegaDrive|I/O", "Joypad"),
		NOP_C_("MegaDrive|I/O", "6-button"),
		NOP_C_("MegaDrive|I/O", "SMS Joypad"),
		NOP_C_("MegaDrive|I/O", "Team Player"),
		NOP_C_("MegaDrive|I/O", "Keyboard"),
		NOP_C_("MegaDrive|I/O", "Serial I/O"),
		NOP_C_("MegaDrive|I/O", "Printer"),
		NOP_C_("MegaDrive|I/O", "Tablet"),
		NOP_C_("MegaDrive|I/O", "Trackball"),
		NOP_C_("MegaDrive|I/O", "Paddle"),
		NOP_C_("MegaDrive|I/O", "Floppy Drive"),
		NOP_C_("MegaDrive|I/O", "CD-ROM"),
		NOP_C_("MegaDrive|I/O", "Activator"),
		NOP_C_("MegaDrive|I/O", "Mega Mouse"),
	};
	vector<string> *v_io_bitfield_names = RomFields::strArrayToVector_i18n(
		"MegaDrive|I/O", io_bitfield_names, ARRAY_SIZE(io_bitfield_names));
	// Parse I/O support.
	uint32_t io_support = parseIOSupport(pRomHeader->io_support, sizeof(pRomHeader->io_support));
	fields->addField_bitfield(C_("MegaDrive", "I/O Support"),
		v_io_bitfield_names, 3, io_support);

	if (!isDisc()) {
		// ROM range.
		fields->addField_string_address_range(C_("MegaDrive", "ROM Range"),
				be32_to_cpu(pRomHeader->rom_start),
				be32_to_cpu(pRomHeader->rom_end), 8,
				RomFields::STRF_MONOSPACE);

		// RAM range.
		fields->addField_string_address_range(C_("MegaDrive", "RAM Range"),
				be32_to_cpu(pRomHeader->ram_start),
				be32_to_cpu(pRomHeader->ram_end), 8,
				RomFields::STRF_MONOSPACE);

		// Check for external memory.
		const uint32_t sram_info = be32_to_cpu(pRomHeader->sram_info);
		if ((sram_info & 0xFFFFA7FF) == 0x5241A020) {
			// SRAM is present.
			// Format: 'R', 'A', %1x1yz000, 0x20
			// x == 1 for backup (SRAM), 0 for not backup
			// yz == 10 for even addresses, 11 for odd addresses
			// TODO: Print the 'x' bit.
			const char *suffix;
			switch ((sram_info >> (8+3)) & 0x03) {
				case 2:
					suffix = C_("MegaDrive", "(even only)");
					break;
				case 3:
					suffix = C_("MegaDrive", "(odd only)");
					break;
				default:
					// TODO: Are both alternates 16-bit?
					suffix = C_("MegaDrive", "(16-bit)");
					break;
			}

			fields->addField_string_address_range(C_("MegaDrive", "SRAM Range"),
				be32_to_cpu(pRomHeader->sram_start),
				be32_to_cpu(pRomHeader->sram_end),
				suffix, 8, RomFields::STRF_MONOSPACE);
		} else {
			fields->addField_string(C_("MegaDrive", "SRAM Range"), C_("MegaDrive", "None"));
		}

		// Check for an extra ROM chip.
		if (pRomHeader->extrom.info == cpu_to_be32(0x524F2020)) {
			// Extra ROM chip. (Sonic & Knuckles)
			// Format: 'R', 'O', 0x20, 0x20
			// Start and End locations are listed twice, in 24-bit format.
			// Not sure if there's any difference between the two...
			const uint32_t extrom_start = (pRomHeader->extrom.data[0] << 16) |
						      (pRomHeader->extrom.data[1] <<  8) |
						       pRomHeader->extrom.data[2];
			const uint32_t extrom_end   = (pRomHeader->extrom.data[3] << 16) |
						      (pRomHeader->extrom.data[4] <<  8) |
						       pRomHeader->extrom.data[5];
			fields->addField_string_address_range(C_("MegaDrive", "ExtROM Range"),
				extrom_start, extrom_end, nullptr, 8,
				RomFields::STRF_MONOSPACE);
		}
	}

	// Region code.
	// TODO: Validate the Mega CD security program?
	static const char *const region_code_bitfield_names[] = {
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "Asia"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	};
	vector<string> *v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	fields->addField_bitfield(C_("MegaDrive", "Region Code"),
		v_region_code_bitfield_names, 0, md_region);
}

/**
 * Add fields for the vector table.
 *
 * This function will not create a new tab.
 * If one is desired, it should be created
 * before calling this function.
 *
 * @param pVectors Vector table.
 */
void MegaDrivePrivate::addFields_vectorTable(const M68K_VectorTable *pVectors)
{
	// Use a LIST_DATA field in order to show all the vectors.
	// TODO:
	// - Make the "#" and "Address" columns monospace.
	// - Increase the height.
	// - Show on a separate line?

	static const char *const vectors_names[] = {
		// $00
		"Initial SP",
		"Entry Point",
		"Bus Error",
		"Address Error",
		// $10
		"Illegal Instruction",
		"Division by Zero",
		"CHK Exception",
		"TRAPV Exception",
		// $20
		"Privilege Violation",
		"TRACE Exception",
		"Line A Emulator",
		"Line F Emulator",
		// $60
		"Spurious Interrupt",
		"IRQ1",
		"IRQ2 (TH)",
		"IRQ3",
		// $70
		"IRQ4 (HBlank)",
		"IRQ5",
		"IRQ6 (VBlank)",
		"IRQ7 (NMI)",
	};

	// Map of displayed vectors to actual vectors.
	// This uses vector indees, *not* byte addresses.
	static const uint8_t vectors_map[] = {
		 0,  1,  2,  3,  4,  5,  6,  7,	// $00-$1C
		 8,  9, 10, 11,			// $20-$2C
		24, 25, 26, 27, 28, 29, 30, 31,	// $60-$7C
	};

	auto vectors_info = new std::vector<std::vector<string> >();
	vectors_info->resize(ARRAY_SIZE(vectors_names));

	for (unsigned int i = 0; i < ARRAY_SIZE(vectors_names); i++) {
		// No vectors are skipped yet.
		// TODO: Add a mapping table when skipping some.
		auto &data_row = vectors_info->at(i);

		// Actual vector number.
		const uint8_t vector_index = vectors_map[i];

		// #
		// NOTE: This is the byte address in the vector table.
		data_row.push_back(rp_sprintf("$%02X", vector_index*4));

		// Vector name
		data_row.push_back(vectors_names[i]);

		// Address
		data_row.push_back(rp_sprintf("$%08X", be32_to_cpu(pVectors->vectors[vector_index])));
	}

	static const char *const vectors_headers[] = {
		NOP_C_("MegaDrive|VectorTable", "#"),
		NOP_C_("MegaDrive|VectorTable", "Vector"),
		NOP_C_("MegaDrive|VectorTable", "Address"),
	};
	vector<string> *v_vectors_headers = RomFields::strArrayToVector_i18n(
		"MegaDrive|VectorTable", vectors_headers, ARRAY_SIZE(vectors_headers));
	fields->addField_listData(C_("MegaDrive", "Vector Table"),
		v_vectors_headers, vectors_info,
		8, RomFields::RFT_LISTDATA_SEPARATE_ROW);
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
	: super(new MegaDrivePrivate(this, file))
{
	RP_D(MegaDrive);
	d->className = "MegaDrive";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	d->file->rewind();

	// Read the ROM header. [0x400 bytes]
	uint8_t header[0x400];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for MD.
	info.szFile = 0;	// Not needed for MD.
	d->romType = isRomSupported_static(&info);

	if (d->romType >= 0) {
		// Save the header for later.
		// TODO (remove before committing): Does gcc/msvc optimize this into a jump table?
		switch (d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) {
			case MegaDrivePrivate::ROM_FORMAT_CART_BIN:
				d->fileType = FTYPE_ROM_IMAGE;

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    header,        sizeof(d->vectors));
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_CART_SMD: {
				d->fileType = FTYPE_ROM_IMAGE;

				// Save the SMD header.
				memcpy(&d->smdHeader, header, sizeof(d->smdHeader));

				// First bank needs to be deinterleaved.
				uint8_t *const block = static_cast<uint8_t*>(aligned_malloc(16, SuperMagicDrive::SMD_BLOCK_SIZE * 2));
				assert(block != nullptr);
				if (!block) {
					// aligned_malloc() failed.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					break;
				}
				uint8_t *const smd_data = block;
				uint8_t *const bin_data = block + SuperMagicDrive::SMD_BLOCK_SIZE;
				size = d->file->seekAndRead(512, smd_data, SuperMagicDrive::SMD_BLOCK_SIZE);
				if (size != SuperMagicDrive::SMD_BLOCK_SIZE) {
					// Short read. ROM is invalid.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					aligned_free(block);
					break;
				}

				// Decode the SMD block.
				SuperMagicDrive::decodeBlock(bin_data, smd_data);

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    bin_data,        sizeof(d->vectors));
				memcpy(&d->romHeader, &bin_data[0x100], sizeof(d->romHeader));
				aligned_free(block);
				break;
			}

			case MegaDrivePrivate::ROM_FORMAT_DISC_2048:
				d->fileType = FTYPE_DISC_IMAGE;

				// MCD-specific header is at 0. [TODO]
				// MD-style header is at 0x100.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_DISC_2352:
				d->fileType = FTYPE_DISC_IMAGE;

				// MCD-specific header is at 0x10. [TODO]
				// MD-style header is at 0x110.
				// No vector table is present on the disc.
				memcpy(&d->romHeader, &header[0x110], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_UNKNOWN:
			default:
				d->fileType = FTYPE_UNKNOWN;
				d->romType = MegaDrivePrivate::ROM_UNKNOWN;
				break;
		}
	}

	d->isValid = (d->romType >= 0);
	if (d->isValid) {
		// Parse the MD region code.
		d->md_region = MegaDriveRegions::parseRegionCodes(
			d->romHeader.region_codes, sizeof(d->romHeader.region_codes));
	}
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int MegaDrive::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 0x200)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// ROM header.
	const uint8_t *const pHeader = info->header.pData;

	// Magic strings.
	static const char sega_magic[4] = {'S','E','G','A'};
	static const char segacd_magic[16] =
		{'S','E','G','A','D','I','S','C','S','Y','S','T','E','M',' ',' '};

	static const struct {
		const char system_name[16];
		uint32_t system_id;
	} cart_magic[] = {
		{{'S','E','G','A',' ','P','I','C','O',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_PICO},
		{{'S','E','G','A',' ','3','2','X',' ',' ',' ',' ',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_32X},
		{{'S','E','G','A',' ','M','E','G','A',' ','D','R','I','V','E',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
		{{'S','E','G','A',' ','G','E','N','E','S','I','S',' ',' ',' ',' '}, MegaDrivePrivate::ROM_SYSTEM_MD},
	};

	if (info->header.size >= 0x200) {
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
		if (info->header.size >= 0x300) {
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
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *MegaDrive::systemName(unsigned int type) const
{
	RP_D(const MegaDrive);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// FIXME: Lots of system names and regions to check.
	// Also, games can be region-free, so we need to check
	// against the host system's locale.
	// For now, just use the generic "Mega Drive".

	static_assert(SYSNAME_TYPE_MASK == 3,
		"MegaDrive::systemName() array index optimization needs to be updated.");

	unsigned int romSys = (d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK);
	if (romSys > MegaDrivePrivate::ROM_SYSTEM_MAX) {
		// Invalid system type. Default to MD.
		romSys = MegaDrivePrivate::ROM_SYSTEM_MD;
	}

	// sysNames[] bitfield:
	// - Bits 0-1: Type. (short, long, abbreviation)
	// - Bits 2-4: System type.
	unsigned int idx = (romSys << 2) | (type & SYSNAME_TYPE_MASK);
	if (idx >= 20) {
		// Invalid index...
		idx &= SYSNAME_TYPE_MASK;
	}

	static_assert(SYSNAME_REGION_MASK == (1 << 2),
		"MegaDrive::systemName() region type optimization needs to be updated.");
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_GENERIC) {
		// Generic system name.
		static const char *const sysNames[20] = {
			"Sega Mega Drive", "Mega Drive", "MD", nullptr,
			"Sega Mega CD", "Mega CD", "MCD", nullptr,
			"Sega 32X", "Sega 32X", "32X", nullptr,
			"Sega Mega CD 32X", "Mega CD 32X", "MCD32X", nullptr,
			"Sega Pico", "Pico", "Pico", nullptr
		};
		return sysNames[idx];
	}

	// Get the system branding region.
	const MegaDriveRegions::MD_BrandingRegion md_bregion =
		MegaDriveRegions::getBrandingRegion(d->md_region);
	switch (md_bregion) {
		case MegaDriveRegions::MD_BREGION_JAPAN:
		default: {
			static const char *const sysNames_JP[20] = {
				"Sega Mega Drive", "Mega Drive", "MD", nullptr,
				"Sega Mega CD", "Mega CD", "MCD", nullptr,
				"Sega Super 32X", "Super 32X", "32X", nullptr,
				"Sega Mega CD 32X", "Mega CD 32X", "MCD32X", nullptr,
				"Sega Kids Computer Pico", "Kids Computer Pico", "Pico", nullptr
			};
			return sysNames_JP[idx];
		}

		case MegaDriveRegions::MD_BREGION_USA: {
			static const char *const sysNames_US[20] = {
				// TODO: "MD" or "Gen"?
				"Sega Genesis", "Genesis", "MD", nullptr,
				"Sega CD", "Sega CD", "MCD", nullptr,
				"Sega 32X", "Sega 32X", "32X", nullptr,
				"Sega CD 32X", "Sega CD 32X", "MCD32X", nullptr,
				"Sega Pico", "Pico", "Pico", nullptr
			};
			return sysNames_US[idx];
		}

		case MegaDriveRegions::MD_BREGION_EUROPE: {
			static const char *const sysNames_EU[20] = {
				"Sega Mega Drive", "Mega Drive", "MD", nullptr,
				"Sega Mega CD", "Mega CD", "MCD", nullptr,
				"Sega Mega Drive 32X", "Mega Drive 32X", "32X", nullptr,
				"Sega Mega CD 32X", "Sega Mega CD 32X", "MCD32X", nullptr,
				"Sega Pico", "Pico", "Pico", nullptr
			};
			return sysNames_EU[idx];
		}

		case MegaDriveRegions::MD_BREGION_SOUTH_KOREA: {
			static const char *const sysNames_KR[20] = {
				// TODO: "MD" or something else?
				"Samsung Super Aladdin Boy", "Super Aladdin Boy", "MD", nullptr,
				"Samsung CD Aladdin Boy", "CD Aladdin Boy", "MCD", nullptr,
				"Samsung Super 32X", "Super 32X", "32X", nullptr,
				"Sega Mega CD 32X", "Sega Mega CD 32X", "MCD32X", nullptr,
				"Sega Pico", "Pico", "Pico", nullptr
			};
			return sysNames_KR[idx];
		}

		case MegaDriveRegions::MD_BREGION_BRAZIL: {
			static const char *const sysNames_BR[20] = {
				"Sega Mega Drive", "Mega Drive", "MD", nullptr,
				"Sega CD", "Sega CD", "MCD", nullptr,
				"Sega Mega 32X", "Mega 32X", "32X", nullptr,
				"Sega CD 32X", "Sega CD 32X", "MCD32X", nullptr,
				"Sega Pico", "Pico", "Pico", nullptr
			};
			return sysNames_BR[idx];
		}
	}

	// Should not get here...
	return nullptr;
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
const char *const *MegaDrive::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".gen", ".smd",
		".32x", ".pco",
		".sgd",	// Official extension

		// NOTE: These extensions may cause conflicts on
		// Windows if fallback handling isn't working.
		".md",	// conflicts with Markdown
		".bin",	// too generic
		".iso",	// too generic

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int MegaDrive::loadFieldData(void)
{
	RP_D(MegaDrive);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		// NOTE: We already loaded the header,
		// so *maybe* this is okay?
		return -EBADF;
	} else if (!d->isValid || d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Maximum number of fields:
	// - ROM Header: 13
	// - Vector table: 1 (LIST_DATA)
	d->fields->reserve(14);

	// Reserve at least 2 tabs.
	d->fields->reserveTabs(2);

	// ROM Header.
	d->fields->setTabName(0, C_("MegaDrive", "ROM Header"));
	d->addFields_romHeader(&d->romHeader);

	if (!d->isDisc()) {
		// Vector table. (MD only; not valid for Mega CD.)
		d->fields->addTab(C_("MegaDrive", "Vector Table"));
		d->addFields_vectorTable(&d->vectors);
	}

	// Check for S&K.
	if (!memcmp(d->romHeader.serial, "GM MK-1563 -00", sizeof(d->romHeader.serial))) {
		// Check if a locked-on ROM is present.
		bool header_loaded = false;
		uint8_t header[0x200];

		if ((d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) == MegaDrivePrivate::ROM_FORMAT_CART_SMD) {
			// Load the 16K block and deinterleave it.
			if (d->file->size() >= (512 + (2*1024*1024) + 16384)) {
				uint8_t *const block = static_cast<uint8_t*>(aligned_malloc(16, SuperMagicDrive::SMD_BLOCK_SIZE * 2));
				assert(block != nullptr);
				if (block) {
					uint8_t *const smd_data = block;
					uint8_t *const bin_data = block + SuperMagicDrive::SMD_BLOCK_SIZE;
					size_t size = d->file->seekAndRead(512 + (2*1024*1024), smd_data, SuperMagicDrive::SMD_BLOCK_SIZE);
					if (size == SuperMagicDrive::SMD_BLOCK_SIZE) {
						// Deinterleave the block.
						SuperMagicDrive::decodeBlock(bin_data, smd_data);
						memcpy(header, bin_data, sizeof(header));
						header_loaded = true;
					}
					aligned_free(block);
				}
			}
		} else {
			// Load the header directly.
			size_t size = d->file->seekAndRead(2*1024*1024, header, sizeof(header));
			header_loaded = (size == sizeof(header));
		}

		if (header_loaded) {
			// Check the "SEGA" magic.
			static const char sega_magic[4] = {'S','E','G','A'};
			if (!memcmp(&header[0x100], sega_magic, sizeof(sega_magic)) ||
			    !memcmp(&header[0x101], sega_magic, sizeof(sega_magic)))
			{
				// Found the "SEGA" magic.
				// Reserve more fields for the second ROM header.
				d->fields->reserve(27);

				// Show the ROM header.
				const MD_RomHeader *const lockon_header =
					reinterpret_cast<const MD_RomHeader*>(&header[0x100]);
				d->fields->addTab(C_("MegaDrive", "Locked-On ROM Header"));
				d->addFields_romHeader(lockon_header);
			}
		}
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

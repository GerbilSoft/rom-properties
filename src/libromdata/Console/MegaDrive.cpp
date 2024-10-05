/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * MegaDrive.cpp: Sega Mega Drive ROM reader.                              *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MegaDrive.hpp"

#include "md_structs.h"
#include "data/SegaPublishers.hpp"
#include "MegaDriveRegions.hpp"
#include "CopierFormats.h"
#include "utils/SuperMagicDrive.hpp"

// Other rom-properties libraries
#include "librpbase/Achievements.hpp"
#include "librpbase/crypto/Hash.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// Other RomData subclasses
#include "Media/ISO.hpp"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class MegaDrivePrivate final : public RomDataPrivate
{
public:
	explicit MegaDrivePrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(MegaDrivePrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	/** RomFields **/

	// I/O support. (RFT_BITFIELD)
	// NOTE: These are bitshift values.
	enum MD_IO_Support_Bitshift {
		MD_IOSH_UNKNOWN		= -1,

		MD_IOSH_JOYPAD_3	=  0,	// 3-button joypad
		MD_IOSH_JOYPAD_6	=  1,	// 6-button joypad
		MD_IOSH_JOYPAD_SMS	=  2,	// 2-button joypad (SMS)
		MD_IOSH_ANALOG		=  3,	// Analog (XE-1 AP)
		MD_IOSH_TEAM_PLAYER	=  4,	// Team Player
		MD_IOSH_LIGHT_GUN	=  5,	// Light Gun (Menacer)
		MD_IOSH_KEYBOARD	=  6,	// Keyboard
		MD_IOSH_SERIAL		=  7,	// Serial (RS-232C)
		MD_IOSH_PRINTER		=  8,	// Printer
		MD_IOSH_TABLET		=  9,	// Tablet
		MD_IOSH_TRACKBALL	= 10,	// Trackball
		MD_IOSH_PADDLE		= 11,	// Paddle
		MD_IOSH_FDD		= 12,	// Floppy Drive
		MD_IOSH_CDROM		= 13,	// CD-ROM
		MD_IOSH_ACTIVATOR	= 14,	// Activator
		MD_IOSH_MEGA_MOUSE	= 15,	// Mega Mouse
	};

	/** Internal ROM data **/

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

		// Low byte: System ID
		// (TODO: MCD Boot ROMs, other specialized types?)
		ROM_SYSTEM_MD		= 0,	// Mega Drive
		ROM_SYSTEM_MCD		= 1,	// Mega CD
		ROM_SYSTEM_32X		= 2,	// Sega 32X
		ROM_SYSTEM_MCD32X	= 3,	// Sega CD 32X
		ROM_SYSTEM_PICO		= 4,	// Sega Pico
		ROM_SYSTEM_TERADRIVE	= 5,	// Sega Teradrive
		ROM_SYSTEM_MAX		= ROM_SYSTEM_TERADRIVE,
		ROM_SYSTEM_UNKNOWN	= 0xFF,
		ROM_SYSTEM_MASK		= 0xFF,

		// Mid byte: Image format
		ROM_FORMAT_CART_BIN	= (0 << 8),	// Cartridge: Binary format.
		ROM_FORMAT_CART_SMD	= (1 << 8),	// Cartridge: SMD format.
		ROM_FORMAT_DISC_2048	= (2 << 8),	// Disc: 2048-byte sectors. (ISO)
		ROM_FORMAT_DISC_2352	= (3 << 8),	// Disc: 2352-byte sectors. (BIN)
		ROM_FORMAT_MAX		= ROM_FORMAT_DISC_2352,
		ROM_FORMAT_UNKNOWN	= (0xFF << 8),
		ROM_FORMAT_MASK		= (0xFF << 8),

		// High byte: Special MD extensions
		ROM_EXT_SSF2		= (1 << 16),	// SSF2 mapper
		ROM_EXT_EVERDRIVE	= (2 << 16),	// Everdrive extensions
		ROM_EXT_MEGAWIFI	= (3 << 16),	// Mega Wifi extensions
		ROM_EXT_TERADRIVE_68k	= (4 << 16),	// Sega Teradrive: Boot from 68000
		ROM_EXT_TERADRIVE_x86	= (5 << 16),	// Sega Teradrive: Boot from x86
		ROM_EXT_MASK		= (0xFF << 16),
	};

	int romType;		// ROM type.
	unsigned int md_region;	// MD hexadecimal region code.

	/**
	 * Is this a disc?
	 * Discs don't have a vector table.
	 * @return True if this is a disc; false if not.
	 */
	inline bool isDisc(void) const
	{
		const int rfmt = (romType & ROM_FORMAT_MASK);
		return (rfmt == ROM_FORMAT_DISC_2048 ||
			rfmt == ROM_FORMAT_DISC_2352);
	}

	/**
	 * Parse region codes.
	 *
	 * Wrapper function to handle some games that don't have
	 * the region code in the correct location.
	 *
	 * @param pRomHeader ROM header.
	 */
	static uint32_t parseRegionCodes(const MD_RomHeader *pRomHeader);

public:
	/**
	 * Determine if a ROM header is using the 'early' format.
	 * @param pRomHeader ROM header.
	 * @return True if 'early'; false if standard.
	 */
	static bool checkIfEarlyRomHeader(const MD_RomHeader *pRomHeader);

	/**
	 * Add fields for the ROM header.
	 *
	 * This function will not create a new tab.
	 * If one is desired, it should be created
	 * before calling this function.
	 *
	 * @param pRomHeader ROM header.
	 * @param bRedetectRegion If true, re-detect the MD region. (i.e. don't use the one parsed in the constructor)
	 */
	void addFields_romHeader(const MD_RomHeader *pRomHeader, bool bRedetectRegion = false);

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
	// ROM header
	// NOTE: Must be byteswapped on access.
	union {
		M68K_VectorTable vectors;	// Interrupt vectors.
		uint8_t mcd_hdr[256];		// TODO: MCD-specific header struct.
	};
	MD_RomHeader romHeader;		// ROM header.
	uint32_t gt_crc;		// Game Toshokan: CRC32 of $20000-$200FF.

	// Extra headers
	//unique_ptr<SMD_Header> pSmdHeader;		// SMD header (NOTE: Not used anywhere right now.)
	unique_ptr<MD_RomHeader> pRomHeaderLockOn;	// Locked-on ROM header

public:
	/**
	 * Get the publisher.
	 * @param pRomHeader ROM header to check.
	 * @return Publisher, or "Unknown" if unknown.
	 */
	static string getPublisher(const MD_RomHeader *pRomHeader);
};

ROMDATA_IMPL(MegaDrive)
ROMDATA_IMPL_IMG(MegaDrive)

/** MegaDrivePrivate **/

/* RomDataInfo */
const char *const MegaDrivePrivate::exts[] = {
	".gen", ".smd",
	".32x", ".pco",
	".sgd",	".68k", // Official extensions

	// NOTE: These extensions may cause conflicts on
	// Windows if fallback handling isn't working.
	".md",	// conflicts with Markdown
	".bin",	// too generic
	".iso",	// too generic

	nullptr
};
const char *const MegaDrivePrivate::mimeTypes[] = {
	// NOTE: Ordering matches MD_RomType's system IDs.

	// Unofficial MIME types from FreeDesktop.org.
	"application/x-genesis-rom",
	"application/x-sega-cd-rom",
	"application/x-genesis-32x-rom",
	"application/x-sega-cd-32x-rom",	// NOTE: Not on fd.o!
	"application/x-sega-pico-rom",
	"application/x-sega-teradrive-rom",

	nullptr
};
const RomDataInfo MegaDrivePrivate::romDataInfo = {
	"MegaDrive", exts, mimeTypes
};

MegaDrivePrivate::MegaDrivePrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, romType(ROM_UNKNOWN)
	, md_region(0)
	, gt_crc(0)
{
	// Clear the various structs.
	memset(&vectors, 0, sizeof(vectors));
	memset(&romHeader, 0, sizeof(romHeader));
}

/** Internal ROM data **/

/**
 * Parse the I/O support field.
 * @param io_support I/O support field.
 * @param size Size of io_support.
 * @return io_support bitfield.
 */
uint32_t MegaDrivePrivate::parseIOSupport(const char *io_support, int size)
{
	// Array mapping MD device characters to MD_IO_Support_Bitfield values.
	// NOTE: Only 48 entries; starts at 0x30, ends at 0x5F.
	// Index: Character
	// Value: Bitfield value, or -1 if not applicable.
	static constexpr array<int8_t, 0x30> md_io_chr_map = {{
		// 0x30 ['0'-'9']
		MD_IOSH_JOYPAD_SMS, -1, -1, -1, MD_IOSH_TEAM_PLAYER, -1, MD_IOSH_JOYPAD_6, -1,
		-1, -1, -1, -1, -1, -1, -1 ,-1,

		// 0x40 ['@','A'-'O']
		-1, MD_IOSH_ANALOG, MD_IOSH_TRACKBALL, MD_IOSH_CDROM, -1, -1, MD_IOSH_FDD, MD_IOSH_LIGHT_GUN,
		-1, -1, MD_IOSH_JOYPAD_3, MD_IOSH_KEYBOARD, MD_IOSH_ACTIVATOR, MD_IOSH_MEGA_MOUSE, -1, -1,

		// 0x50 ['P'-'Z']
		MD_IOSH_PRINTER, -1, MD_IOSH_SERIAL, -1, MD_IOSH_TABLET, -1, MD_IOSH_PADDLE, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
	}};

	uint32_t ret = 0;
	for (int i = size-1; i >= 0; i--) {
		const uint8_t io_cur = static_cast<uint8_t>(io_support[i]);
		if (io_cur < 0x30 || io_cur > 0x5F)
			continue;

		const int8_t io_bit = md_io_chr_map[io_cur - 0x30];
		if (io_bit >= 0) {
			ret |= (1U << io_bit);
		}
	}

	return ret;
}

/**
 * Parse region codes.
 *
 * Wrapper function to handle some games that don't have
 * the region code in the correct location.
 *
 * @param pRomHeader ROM header.
 */
uint32_t MegaDrivePrivate::parseRegionCodes(const MD_RomHeader *pRomHeader)
{
	uint32_t md_region = MegaDriveRegions::parseRegionCodes(
		pRomHeader->region_codes, sizeof(pRomHeader->region_codes));
	if (md_region != 0)
		return md_region;

	// Some early games incorrectly have the MD region code at a different location.
	// - Golden Axe: 0x1C0
	if (pRomHeader->modem_info[4] != ' ' && pRomHeader->modem_info[7] == ' ') {
		md_region = MegaDriveRegions::parseRegionCodes(
			&pRomHeader->modem_info[4], 3);
	}

	return md_region;
}

/**
 * Determine if a ROM header is using the 'early' format.
 * @param pRomHeader ROM header.
 * @return True if 'early'; false if standard.
 */
bool MegaDrivePrivate::checkIfEarlyRomHeader(const MD_RomHeader *pRomHeader)
{
	// Check if the serial number is empty.
	switch (pRomHeader->serial_number[0]) {
		case ' ':
		case '\0':
			// Empty serial number.
			break;
		default:
			// Not an empty serial number.
			return false;
	}

	// Is a valid 'GM' serial number present at the 'early' header location?
	// If so, we'll assume this is an early ROM header.
	return !memcmp(pRomHeader->early.serial_number, "GM", 2);
}

/**
 * Add fields for the ROM header.
 *
 * This function will not create a new tab.
 * If one is desired, it should be created
 * before calling this function.
 *
 * @param pRomHeader ROM header.
 * @param bRedetectRegion If true, re-detect the MD region. (i.e. don't use the one parsed in the constructor)
 */
void MegaDrivePrivate::addFields_romHeader(const MD_RomHeader *pRomHeader, bool bRedetectRegion)
{
	const bool isEarlyRomHeader = checkIfEarlyRomHeader(pRomHeader);

	// Read the strings from the header.
	fields.addField_string(C_("MegaDrive", "System"),
		cp1252_sjis_to_utf8(pRomHeader->system, sizeof(pRomHeader->system)),
			RomFields::STRF_TRIM_END);
	fields.addField_string(C_("RomData", "Copyright"),
		cp1252_sjis_to_utf8(pRomHeader->copyright, sizeof(pRomHeader->copyright)),
			RomFields::STRF_TRIM_END);

	// Publisher
	fields.addField_string(C_("RomData", "Publisher"), getPublisher(pRomHeader));

	// Some fields vary depending on if this is a standard ROM header
	// or an 'early' ROM header.
	const char *s_title_domestic, *s_title_export;
	const char *s_serial_number, *s_io_support;
	const MD_RomRamInfo *pRomRam;
	int title_len;
	uint16_t checksum;
	if (!isEarlyRomHeader) {
		// Standard ROM header.
		s_title_domestic = pRomHeader->title_domestic;
		s_title_export = pRomHeader->title_export;
		s_serial_number = pRomHeader->serial_number;
		s_io_support = pRomHeader->io_support;
		pRomRam = &pRomHeader->rom_ram;
		title_len = sizeof(pRomHeader->title_domestic);
		checksum = be16_to_cpu(pRomHeader->checksum);
	} else {
		// 'Early' ROM header.
		s_title_domestic = pRomHeader->early.title_domestic;
		s_title_export = pRomHeader->early.title_export;
		s_serial_number = pRomHeader->early.serial_number;
		s_io_support = pRomHeader->early.io_support;
		pRomRam = &pRomHeader->early.rom_ram;
		title_len = sizeof(pRomHeader->early.title_domestic);
		checksum = be16_to_cpu(pRomHeader->early.checksum);
	}

	// Titles, serial number, and checksum.
	fields.addField_string(C_("MegaDrive", "Domestic Title"),
		cp1252_sjis_to_utf8(s_title_domestic, title_len), RomFields::STRF_TRIM_END);
	fields.addField_string(C_("MegaDrive", "Export Title"),
		cp1252_sjis_to_utf8(s_title_export, title_len), RomFields::STRF_TRIM_END);
	// NOTE: Serial number should be ASCII only.
	fields.addField_string(C_("MegaDrive", "Serial Number"),
		cp1252_to_utf8(s_serial_number, sizeof(pRomHeader->serial_number)),
			RomFields::STRF_TRIM_END);
	if (!isDisc()) {
		// Checksum. (MD only; not valid for Mega CD.)
		fields.addField_string_numeric(C_("RomData", "Checksum"),
			checksum, RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	}

	// I/O support bitfield.
	static const array<const char*, 16> io_bitfield_names = {{
		NOP_C_("MegaDrive|I/O", "Joypad"),
		NOP_C_("MegaDrive|I/O", "6-button"),
		// tr: Use a locale-specific abbreviation for Sega Master System, e.g. MK3 or similar.
		NOP_C_("MegaDrive|I/O", "SMS Joypad"),
		NOP_C_("MegaDrive|I/O", "Analog"),
		NOP_C_("MegaDrive|I/O", "Team Player"),
		NOP_C_("MegaDrive|I/O", "Light Gun"),
		NOP_C_("MegaDrive|I/O", "Keyboard"),
		NOP_C_("MegaDrive|I/O", "Serial I/O"),
		NOP_C_("MegaDrive|I/O", "Printer"),
		NOP_C_("MegaDrive|I/O", "Tablet"),
		NOP_C_("MegaDrive|I/O", "Trackball"),
		NOP_C_("MegaDrive|I/O", "Paddle"),
		NOP_C_("MegaDrive|I/O", "Floppy Drive"),
		NOP_C_("MegaDrive|I/O", "CD-ROM"),
		// tr: Brand name; only translate if the name was changed in your region.
		NOP_C_("MegaDrive|I/O", "Activator"),
		NOP_C_("MegaDrive|I/O", "Mega Mouse"),
	}};
	// NOTE: Using a plain text field because most games only support
	// one or two devices, so we don't need to list them all.
	const uint32_t io_support = parseIOSupport(s_io_support, sizeof(pRomHeader->io_support));
	string s_io_devices;
	s_io_devices.reserve(32);
	uint32_t bit = 1U;
	for (const char *name : io_bitfield_names) {
		if (io_support & bit) {
			if (!s_io_devices.empty()) {
				s_io_devices += ", ";
			}
			s_io_devices += pgettext_expr("MegaDrive|I/O", name);
		}

		// Next bit.
		bit <<= 1;
	}
	if (s_io_devices.empty()) {
		s_io_devices = C_("MegaDrive|I/O", "None");
	}
	fields.addField_string(C_("MegaDrive", "I/O Support"), s_io_devices);

	if (!isDisc()) {
		// ROM range.
		fields.addField_string_address_range(C_("MegaDrive", "ROM Range"),
				be32_to_cpu(pRomRam->rom_start),
				be32_to_cpu(pRomRam->rom_end), 8,
				RomFields::STRF_MONOSPACE);

		// RAM range.
		fields.addField_string_address_range(C_("MegaDrive", "RAM Range"),
				be32_to_cpu(pRomRam->ram_start),
				be32_to_cpu(pRomRam->ram_end), 8,
				RomFields::STRF_MONOSPACE);

		// Check for external memory.
		const uint32_t sram_info = be32_to_cpu(pRomHeader->sram_info);
		const char *const sram_range_title = C_("MegaDrive", "SRAM Range");
		if ((sram_info & 0xFFFFA7FF) == 0x5241A020) {
			// SRAM is present.
			// Format: 'R', 'A', %1x1yz000, 0x20
			// x == 1 for backup (SRAM), 0 for not backup
			// yz == 10 for even addresses, 11 for odd addresses
			// TODO: Print the 'x' bit.
			const char *suffix;
			switch ((sram_info >> (8+3)) & 0x03) {
				case 2:
					// tr: SRAM format: Even bytes only.
					suffix = C_("MegaDrive|SRAM", "(even only)");
					break;
				case 3:
					// tr: SRAM format: Odd bytes only.
					suffix = C_("MegaDrive|SRAM", "(odd only)");
					break;
				default:
					// TODO: Are both alternates 16-bit?
					// tr: SRAM format: 16-bit.
					suffix = C_("MegaDrive|SRAM", "(16-bit)");
					break;
			}

			fields.addField_string_address_range(sram_range_title,
				be32_to_cpu(pRomHeader->sram_start),
				be32_to_cpu(pRomHeader->sram_end),
				suffix, 8, RomFields::STRF_MONOSPACE);
		} else {
			fields.addField_string(sram_range_title, C_("MegaDrive", "None"));
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
			fields.addField_string_address_range(C_("MegaDrive", "ExtROM Range"),
				extrom_start, extrom_end, nullptr, 8,
				RomFields::STRF_MONOSPACE);
		}
	}

	// Region code
	// NOTE: bRedetectRegion is only used for S&K lock-on,
	// so we don't need to worry about the Mega CD security program.
	const uint32_t md_region_check = (unlikely(bRedetectRegion))
		? parseRegionCodes(pRomHeader)
		: this->md_region;

	static const array<const char*, 4> region_code_bitfield_names = {{
		NOP_C_("Region", "Japan"),
		NOP_C_("Region", "Asia"),
		NOP_C_("Region", "USA"),
		NOP_C_("Region", "Europe"),
	}};
	vector<string> *const v_region_code_bitfield_names = RomFields::strArrayToVector_i18n(
		"Region", region_code_bitfield_names);
	fields.addField_bitfield(C_("RomData", "Region Code"),
		v_region_code_bitfield_names, 0, md_region_check);

	// TODO: Extensions, e.g. SSF2, MegaWifi, and TeraDrive boot CPU.
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

	static constexpr char vectors_strtbl[] = {
		// $00
		"Initial SP\0"
		"Entry Point\0"
		"Bus Error\0"
		"Address Error\0"
		// $10
		"Illegal Instruction\0"
		"Division by Zero\0"
		"CHK Exception\0"
		"TRAPV Exception\0"
		// $20
		"Privilege Violation\0"
		"TRACE Exception\0"
		"Line A Emulator\0"
		"Line F Emulator\0"
		// $60
		"Spurious Interrupt\0"
		"IRQ1\0"
		"IRQ2 (TH)\0"
		"IRQ3\0"
		// $70
		"IRQ4 (HBlank)\0"
		"IRQ5\0"
		"IRQ6 (VBlank)\0"
		"IRQ7 (NMI)\0"
	};
	// Just under 255 (uint8_t max). Nice.
	static constexpr array<uint8_t, 20> vectors_offtbl = {{
		0, 11, 23, 33, 47, 67, 84, 98,	// $00-$1C
		114, 134, 150, 166,			// $20-$2C
		182, 201, 206, 216, 221, 235, 240, 254,	// $60-$7C
	}};

	// Map of displayed vectors to actual vectors.
	// This uses vector indees, *not* byte addresses.
	static constexpr array<uint8_t, 20> vectors_map = {{
		 0,  1,  2,  3,  4,  5,  6,  7,	// $00-$1C
		 8,  9, 10, 11,			// $20-$2C
		24, 25, 26, 27, 28, 29, 30, 31,	// $60-$7C
	}};

	static_assert(vectors_offtbl.size() == vectors_map.size(),
		"vectors_offtbl[] and vectors_map[] are out of sync.");

	auto vv_vectors = new RomFields::ListData_t(vectors_offtbl.size());
	auto iter = vv_vectors->begin();
	const auto vv_vectors_end = vv_vectors->end();
	for (size_t i = 0; i < vectors_offtbl.size() && iter != vv_vectors_end; ++i, ++iter) {
		auto &data_row = *iter;
		data_row.reserve(3);

		// Actual vector number.
		const unsigned int vector_index = vectors_map[i];

		// #
		// NOTE: This is the byte address in the vector table.
		data_row.emplace_back(rp_sprintf("$%02X", vector_index*4));

		// Vector name
		data_row.emplace_back(&vectors_strtbl[vectors_offtbl[i]]);

		// Address
		data_row.emplace_back(rp_sprintf("$%08X", be32_to_cpu(pVectors->vectors[vector_index])));
	}

	static const array<const char*, 3> vectors_headers = {{
		NOP_C_("RomData|VectorTable", "#"),
		NOP_C_("RomData|VectorTable", "Vector"),
		NOP_C_("RomData|VectorTable", "Address"),
	}};
	vector<string> *const v_vectors_headers = RomFields::strArrayToVector_i18n(
		"RomData|VectorTable", vectors_headers);

	RomFields::AFLD_PARAMS params(RomFields::RFT_LISTDATA_SEPARATE_ROW, 8);
	params.headers = v_vectors_headers;
	params.data.single = vv_vectors;
	fields.addField_listData(C_("RomData", "Vector Table"), &params);
}

/**
 * Get the publisher.
 * @param pRomHeader ROM header to check.
 * @return Publisher, or "Unknown" if unknown.
 */
string MegaDrivePrivate::getPublisher(const MD_RomHeader *pRomHeader)
{
	string s_publisher;

	// Determine the publisher.
	// Formats in the copyright line:
	// - "(C)SEGA"
	// - "(C)T-xx"
	// - "(C)T-xxx"
	// - "(C)Txxx"
	unsigned int t_code = 0;
	if (!memcmp(pRomHeader->copyright, "(C)SEGA", 7)) {
		// Sega first-party game.
		s_publisher = "Sega";
	} else if (!memcmp(pRomHeader->copyright, "(C)T", 4)) {
		// Third-party game.
		int start = 4;
		if (pRomHeader->copyright[4] == '-')
			start++;

		// Read up to three digits.
		char buf[4];
		memcpy(buf, &pRomHeader->copyright[start], 3);
		buf[3] = '\0';

		t_code = strtoul(buf, nullptr, 10);
		if (t_code != 0) {
			// Valid T-code. Look up the publisher.
			const char *const publisher = SegaPublishers::lookup(t_code);
			if (publisher) {
				s_publisher = publisher;
			}
		}
	}

	if (s_publisher.empty()) {
		// Publisher not identified.
		// Check for a T-code.
		if (t_code > 0) {
			// Found a T-code.
			char buf[16];
			snprintf(buf, sizeof(buf), "T-%u", t_code);
			s_publisher = buf;
		} else {
			// Unknown publisher.
			s_publisher = C_("RomData", "Unknown");
		}
	}

	return s_publisher;
}

/** MegaDrive **/

/**
 * Read a Sega Mega Drive ROM.
 *
 * A ROM file must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
 * data from the ROM.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM file.
 */
MegaDrive::MegaDrive(const IRpFilePtr &file)
	: super(new MegaDrivePrivate(file))
{
	RP_D(MegaDrive);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Seek to the beginning of the file.
	d->file->rewind();

	// Read the ROM header. [0x810 bytes; minimum 0x200]
	uint8_t header[0x810];
	size_t size = d->file->read(header, sizeof(header));
	if (size < 0x200) {
		d->file.reset();
		return;
	}

	// Check if this ROM is supported.
	const DetectInfo info = {
		{0, static_cast<uint32_t>(size), header},
		nullptr,	// ext (not needed for MegaDrive)
		0		// szFile (not needed for MegaDrive)
	};
	d->romType = isRomSupported_static(&info);

	// Mega CD security program CRC32
	uint32_t mcd_sp_crc = 0;

	if (d->romType >= 0) {
		// Save the header for later.
		switch (d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) {
			case MegaDrivePrivate::ROM_FORMAT_CART_BIN:
				d->fileType = FileType::ROM_Image;

				// MD header is at 0x100.
				// Vector table is at 0.
				memcpy(&d->vectors,    header,        sizeof(d->vectors));
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));
				break;

			case MegaDrivePrivate::ROM_FORMAT_CART_SMD: {
				// Save the SMD header.
				// NOTE: Not actually used anywhere; disabling for now.
				//d->pSmdHeader.reset(new SMD_Header);
				//memcpy(d->pSmdHeader.get(), header, sizeof(SMD_Header));

				// First bank needs to be deinterleaved.
				auto block = aligned_uptr<uint8_t>(16, SuperMagicDrive::SMD_BLOCK_SIZE * 2);
				uint8_t *const smd_data = block.get();
				uint8_t *const bin_data = smd_data + SuperMagicDrive::SMD_BLOCK_SIZE;
				size = d->file->seekAndRead(512, smd_data, SuperMagicDrive::SMD_BLOCK_SIZE);
				if (size != SuperMagicDrive::SMD_BLOCK_SIZE) {
					// Short read. ROM is invalid.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					break;
				}

				// Decode the SMD block.
				SuperMagicDrive::decodeBlock(bin_data, smd_data);

				// MD header is at 0x100.
				// Vector table is at 0.
				d->fileType = FileType::ROM_Image;
				memcpy(&d->vectors,    bin_data,        sizeof(d->vectors));
				memcpy(&d->romHeader, &bin_data[0x100], sizeof(d->romHeader));
				break;
			}

			case MegaDrivePrivate::ROM_FORMAT_DISC_2048:
				d->fileType = FileType::DiscImage;

				// MCD-specific header is at 0.
				// MD-style header is at 0x100.
				// No vector table is present on the disc.
				memcpy(&d->mcd_hdr, header, sizeof(d->mcd_hdr));
				memcpy(&d->romHeader, &header[0x100], sizeof(d->romHeader));

				// Calculate the security program CRC32.
				// NOTE: The JP security program fits into the first sector,
				// but US/EU takes up two sectors. This shouldn't be an issue,
				// since we don't need the full program; we just need to be
				// able to distinguish between them.
				// NOTE: Only the first 0x4C0 bytes seem to be identical
				// between USA and EUR, and 0x150 in JPN...
				if (size >= (0x200 + 0x150)) {
					Hash crc32Hash(Hash::Algorithm::CRC32);
					if (crc32Hash.isUsable()) {
						crc32Hash.process(&header[0x200], 0x150);
						mcd_sp_crc = crc32Hash.getHash32();
					}
				}
				break;

			case MegaDrivePrivate::ROM_FORMAT_DISC_2352:
				if (size < 0x210) {
					// Not enough data for a 2352-byte sector disc image.
					d->romType = MegaDrivePrivate::ROM_UNKNOWN;
					d->file.reset();
					return;
				}
				d->fileType = FileType::DiscImage;

				// MCD-specific header is at 0x10.
				// MD-style header is at 0x110.
				// No vector table is present on the disc.
				memcpy(&d->mcd_hdr, &header[0x10], sizeof(d->mcd_hdr));
				memcpy(&d->romHeader, &header[0x110], sizeof(d->romHeader));

				// Calculate the security program CRC32.
				// NOTE: The JP security program fits into the first sector,
				// but US/EU takes up two sectors. This shouldn't be an issue,
				// since we don't need the full program; we just need to be
				// able to distinguish between them.
				// NOTE: Only the first 0x4C0 bytes seem to be identical
				// between USA and EUR, and 0x150 in JPN...
				if (size >= (0x210 + 0x150)) {
					Hash crc32Hash(Hash::Algorithm::CRC32);
					if (crc32Hash.isUsable()) {
						crc32Hash.process(&header[0x210], 0x150);
						mcd_sp_crc = crc32Hash.getHash32();
					}
				}
				break;

			case MegaDrivePrivate::ROM_FORMAT_UNKNOWN:
			default:
				d->romType = MegaDrivePrivate::ROM_UNKNOWN;
				break;
		}
	}

	d->isValid = (d->romType >= 0);
	if (!d->isValid) {
		// Not valid. Close the file.
		d->file.reset();
		return;
	}

	// MD region code.
	// If this is a Mega CD title, check the security program,
	// since the code in the ROM header is sometimes inaccurate.
	switch (mcd_sp_crc) {
		case 0xD6BC66C7:
			d->md_region = MegaDriveRegions::MD_REGION_JAPAN | MegaDriveRegions::MD_REGION_ASIA;
			break;
		case 0xC5878BA4:
			d->md_region = MegaDriveRegions::MD_REGION_USA;
			break;
		case 0x233CFB15:
			d->md_region = MegaDriveRegions::MD_REGION_EUROPE;
			break;

		default:
			assert(!"Unrecognized Mega CD security sector.");
			// fall-through
		case 0:
			// Not a Mega CD title.
			// Parse the MD region code.
			d->md_region = d->parseRegionCodes(&d->romHeader);
			break;
	}

	// Determine the MIME type.
	const uint8_t sysID = (d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK);
	if (sysID < ARRAY_SIZE(d->mimeTypes)-1) {
		d->mimeType = d->mimeTypes[sysID];
	}

	// Special ROM checks. (MD only for now)
	if (sysID != MegaDrivePrivate::ROM_SYSTEM_MD)
		return;

	const bool isEarlyRomHeader = d->checkIfEarlyRomHeader(&d->romHeader);
	const off64_t fileSize = d->file->size();
	const char *const s_serial_number = (isEarlyRomHeader)
		? d->romHeader.early.serial_number
		: d->romHeader.serial_number;

	// Check for Game Toshokan with a downloaded game.
	if (fileSize == 256*1024 &&
	    !memcmp(s_serial_number, "GM 00054503-00", sizeof(d->romHeader.serial_number)) &&
	    (d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) == MegaDrivePrivate::ROM_FORMAT_CART_BIN)
	{
		Hash crc32Hash(Hash::Algorithm::CRC32);
		if (!crc32Hash.isUsable()) {
			// Can't initialize zlib to calculate the CRC32.
			return;
		}

		// Calculate the CRC32 of $20000-$200FF.
		// TODO: SMD deinterleaving.
		uint8_t buf[256];
		size_t size = d->file->seekAndRead(0x20000, buf, sizeof(buf));
		if (size == sizeof(buf)) {
			crc32Hash.process(buf, sizeof(buf));
			d->gt_crc = crc32Hash.getHash32();
		}
	}
	// If this is S&K, try reading the locked-on ROM header.
	else if (fileSize >= ((2*1024*1024)+512) &&
	         !memcmp(s_serial_number, "GM MK-1563 -00", sizeof(d->romHeader.serial_number)))
	{
		// Check if a locked-on ROM is present.
		if ((d->romType & MegaDrivePrivate::ROM_FORMAT_MASK) == MegaDrivePrivate::ROM_FORMAT_CART_SMD) {
			// Load the 16K block and deinterleave it.
			if (fileSize >= (2*1024*1024)+512+16384) {
				auto block = aligned_uptr<uint8_t>(16, SuperMagicDrive::SMD_BLOCK_SIZE * 2);
				uint8_t *const smd_data = block.get();
				uint8_t *const bin_data = smd_data + SuperMagicDrive::SMD_BLOCK_SIZE;
				size_t size = d->file->seekAndRead(512 + (2*1024*1024), smd_data, SuperMagicDrive::SMD_BLOCK_SIZE);
				if (size == SuperMagicDrive::SMD_BLOCK_SIZE) {
					// Deinterleave the block.
					SuperMagicDrive::decodeBlock(bin_data, smd_data);
					d->pRomHeaderLockOn.reset(new MD_RomHeader);
					memcpy(d->pRomHeaderLockOn.get(), &bin_data[0x100], sizeof(MD_RomHeader));
				}
			}
		} else {
			// Load the header directly.
			d->pRomHeaderLockOn.reset(new MD_RomHeader);
			size_t size = d->file->seekAndRead((2*1024*1024)+0x100, d->pRomHeaderLockOn.get(), sizeof(MD_RomHeader));
			if (size != sizeof(*d->pRomHeaderLockOn)) {
				// Error loading the ROM header.
				d->pRomHeaderLockOn.reset();
			}
		}

		if (d->pRomHeaderLockOn) {
			// Verify the "SEGA" magic.
			static constexpr array<char, 4> sega_magic = {{'S','E','G','A'}};
			if (!memcmp(&d->pRomHeaderLockOn->system[0], sega_magic.data(), sega_magic.size()) ||
			    !memcmp(&d->pRomHeaderLockOn->system[1], sega_magic.data(), sega_magic.size()))
			{
				// Found the "SEGA" magic.
			}
			else
			{
				// "SEGA" magic not found.
				// Assume this is invalid.
				d->pRomHeaderLockOn.reset();
			}
		}
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

	// Magic strings. (NOTE: **NOT** NULL-terminated!)
	static constexpr array<char,  4> sega_magic    = {{'S','E','G','A'}};
	static constexpr array<char, 16> segacd_magic  = {{'S','E','G','A','D','I','S','C','S','Y','S','T','E','M',' ',' '}};
	// NOTE: Only used for Sega CD 32X.
	static constexpr array<char, 16> sega32x_magic = {{'S','E','G','A',' ','3','2','X',' ',' ',' ',' ',' ',' ',' ',' '}};

	// Extra system types from:
	// - https://www.plutiedev.com/rom-header#system
	// NOTE: Doom 32X incorrectly has the region code at the end of the
	// system name field, so ignore the last two bytes for 32X.
	struct cart_magic_sega_t {
		char sys_name[12+1];
		uint8_t sys_name_len;	// Length to check at $100; for $101, subtract 1.
		uint32_t system_id;
	};
	static const array<cart_magic_sega_t, 10> cart_magic_sega = {{
		{" 32X      ",   10, MegaDrivePrivate::ROM_SYSTEM_32X},
		{" SSF        ", 12, MegaDrivePrivate::ROM_SYSTEM_MD |
		                     MegaDrivePrivate::ROM_EXT_SSF2},
		{" EVERDRIVE  ", 12, MegaDrivePrivate::ROM_SYSTEM_MD |
		                     MegaDrivePrivate::ROM_EXT_EVERDRIVE},
		{" MEGAWIFI   ", 12, MegaDrivePrivate::ROM_SYSTEM_MD |
		                     MegaDrivePrivate::ROM_EXT_MEGAWIFI},
		{" TERA68K    ", 12, MegaDrivePrivate::ROM_SYSTEM_TERADRIVE |
		                     MegaDrivePrivate::ROM_EXT_TERADRIVE_68k},
		{" TERA286    ", 12, MegaDrivePrivate::ROM_SYSTEM_TERADRIVE |
		                     MegaDrivePrivate::ROM_EXT_TERADRIVE_x86},
		{" PICO       ", 12, MegaDrivePrivate::ROM_SYSTEM_PICO},
		{"TOYS PICO   ", 12, MegaDrivePrivate::ROM_SYSTEM_PICO},	// Late 90s
		{" TOYS PICO  ", 12, MegaDrivePrivate::ROM_SYSTEM_PICO},	// Late 90s
		{" IAC        ", 12, MegaDrivePrivate::ROM_SYSTEM_PICO},	// Some JP ROMs
	}};

	// Ohter non-Sega system IDs. (Sega Pico, Sega Picture Magic)
	struct cart_magic_other_t {
		char sys_name[17];
		uint8_t system_id;
	};
	static const array<cart_magic_other_t, 4> cart_magic_other = {{
		{"SAMSUNG PICO    ", MegaDrivePrivate::ROM_SYSTEM_PICO},	// TODO: Indicate Korean.
		{"IMA IKUNOUJYUKU ", MegaDrivePrivate::ROM_SYSTEM_PICO},	// Some JP ROMs
		{"IMA IKUNOJYUKU  ", MegaDrivePrivate::ROM_SYSTEM_PICO},	// Some JP ROMs
		{"Picture Magic   ", MegaDrivePrivate::ROM_SYSTEM_32X},		// Picture Magic
	}};

	// Check for Sega CD.
	// TODO: Gens/GS II lists "ISO/2048", "ISO/2352",
	// "BIN/2048", and "BIN/2352". I don't think that's
	// right; there should only be 2048 and 2352.
	// TODO: Detect Sega CD 32X.
	// TODO: Use a struct instead of raw bytes?
	int mcd_offset = -1;
	if (!memcmp(&pHeader[0x10], segacd_magic.data(), segacd_magic.size())) {
		// Found a Sega CD disc image. (2352-byte sectors)
		mcd_offset = 0x10;
	} else if (!memcmp(&pHeader[0], segacd_magic.data(), segacd_magic.size())) {
		// Found a Sega CD disc image. (2048-byte sectors)
		mcd_offset = 0;
	}

	if (mcd_offset >= 0) {
		// Check for some Mega CD disc images that don't
		// properly indicate that they're 32X games.
		const uint8_t *const pMcdHeader = &pHeader[mcd_offset];
		const int discSectorSize = (mcd_offset != 0)
			? MegaDrivePrivate::ROM_FORMAT_DISC_2352
			: MegaDrivePrivate::ROM_FORMAT_DISC_2048;

		if (unlikely(!memcmp(&pMcdHeader[0x0100], sega32x_magic.data(), sega32x_magic.size()))) {
			// This is a Sega CD 32X disc image.
			// TODO: Check for 32X security code?
			return discSectorSize | MegaDrivePrivate::ROM_SYSTEM_MCD32X;
		} else if (unlikely(pMcdHeader[0x18A] == 'F')) {
			// Serial number has 'F'. This is probably 32X.
			return discSectorSize | MegaDrivePrivate::ROM_SYSTEM_MCD32X;
		} else if (!memcmp(&pMcdHeader[0x180], "GM MK-4438 -00", 14)) {
			// Fahrenheit (USA)
			// The 32X disc has the same serial number as the Mega CD
			// disc, but the build date in the MCD-specific header is
			// a month later.
			if (pMcdHeader[0x1F0] == 'U' && pMcdHeader[0x51] == '3') {
				return discSectorSize | MegaDrivePrivate::ROM_SYSTEM_MCD32X;
			}
		} else if (!memcmp(&pMcdHeader[0x180], "GM MK-4435 -00", 14)) {
			// Surgical Strike (USA/Brazil)
			// A 32X version was released in Brazil, which mostly has the
			// same header except for "32X" in the MCD-specific header and
			// region code 'U' instead of '4'.
			if (pMcdHeader[0x18] == '3') {
				return discSectorSize | MegaDrivePrivate::ROM_SYSTEM_MCD32X;
			}
		}

		return discSectorSize | MegaDrivePrivate::ROM_SYSTEM_MCD;
	}

	// Check for SMD format. (Mega Drive only)
	if (info->header.size >= 0x300) {
		// Check if "SEGA" is in the header in the correct place
		// for a plain binary ROM.
		if (memcmp(&pHeader[0x100], sega_magic.data(), sega_magic.size()) != 0 &&
		    memcmp(&pHeader[0x101], sega_magic.data(), sega_magic.size()) != 0)
		{
			// "SEGA" is not in the header. This might be SMD.
			const SMD_Header *const pSmdHeader = reinterpret_cast<const SMD_Header*>(pHeader);
			if (pSmdHeader->id[0] == 0xAA && pSmdHeader->id[1] == 0xBB &&
			    pSmdHeader->smd.file_data_type == SMD_FDT_68K_PROGRAM &&
			    pSmdHeader->file_type == SMD_FT_SMD_GAME_FILE)
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
	int sysId = MegaDrivePrivate::ROM_UNKNOWN;
	if (!memcmp(&pHeader[0x100], sega_magic.data(), sega_magic.size())) {
		// "SEGA" is at 0x100.
		for (const auto &p : cart_magic_sega) {
			if (!memcmp(&pHeader[0x104], p.sys_name, p.sys_name_len)) {
				// Found a matching system name.
				sysId = p.system_id;
				break;
			}
		}
		if (sysId == MegaDrivePrivate::ROM_UNKNOWN) {
			// Unknown "SEGA" header. Assume it's MD.
			// NOTE: "Virtua Racing Deluxe (USA).32x" has "SEGA 32X U".
			sysId = MegaDrivePrivate::ROM_SYSTEM_MD;
		}
	} else if (!memcmp(&pHeader[0x101], sega_magic.data(), sega_magic.size())) {
		// "SEGA" is at 0x101.
		for (const auto &p : cart_magic_sega) {
			if (!memcmp(&pHeader[0x105], p.sys_name, p.sys_name_len-1)) {
				// Found a matching system name.
				sysId = p.system_id;
				break;
			}
		}
		if (sysId == MegaDrivePrivate::ROM_UNKNOWN) {
			// Unknown "SEGA" header. Assume it's MD.
			// NOTE: "Virtua Racing Deluxe (USA).32x" has "SEGA 32X U".
			sysId = MegaDrivePrivate::ROM_SYSTEM_MD;
		}
	} else {
		// Not a "SEGA" cartridge header.
		// Check for less common ROMs without a "SEGA" system name.
		for (const auto &p : cart_magic_other) {
			if (!memcmp(&pHeader[0x100], p.sys_name, 16)) {
				// Found a matching system name.
				sysId = p.system_id;
				break;
			}
		}
	}

	if (sysId == MegaDrivePrivate::ROM_UNKNOWN) {
		// Still unknown...
		return sysId;
	}

	uint32_t sysIdOnly = (sysId & MegaDrivePrivate::ROM_SYSTEM_MASK);
	if (sysIdOnly == MegaDrivePrivate::ROM_SYSTEM_MD ||
	    sysIdOnly == MegaDrivePrivate::ROM_SYSTEM_32X)
	{
		// Verify the 32X security program if possible.
		static constexpr uint32_t secprgaddr = 0x512;
		static constexpr char secprgdesc[] = "MARS Initial & Security Program";
		if (info->header.size >= secprgaddr + sizeof(secprgdesc) - 1) {
			// TODO: Check other parts of the security program?
			if (!memcmp(&pHeader[secprgaddr], secprgdesc, sizeof(secprgdesc)-1)) {
				// Module name is correct.
				// TODO: Does the ROM header have to say "SEGA 32X"?
				sysIdOnly = MegaDrivePrivate::ROM_SYSTEM_32X;
			} else {
				// Module name is incorrect.
				// This ROM cannot activate 32X mode.
				sysIdOnly = MegaDrivePrivate::ROM_SYSTEM_MD;
			}
			sysId &= ~MegaDrivePrivate::ROM_SYSTEM_MASK;
			sysId |= sysIdOnly;
		}
	}

	return MegaDrivePrivate::ROM_FORMAT_CART_BIN | sysId;
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
	// - Bits 0-1: Type. (long, short, abbreviation)
	// - Bits 2-4: System type.

	static_assert(SYSNAME_REGION_MASK == (1U << 2),
		"MegaDrive::systemName() region type optimization needs to be updated.");
	const unsigned int idx = (type & SYSNAME_TYPE_MASK);
	if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_GENERIC) {
		// Generic system name.
		static const array<array<const char*, 4>, 6> sysNames = {{
			{{"Sega Mega Drive", "Mega Drive", "MD", nullptr}},
			{{"Sega Mega CD", "Mega CD", "MCD", nullptr}},
			{{"Sega 32X", "Sega 32X", "32X", nullptr}},
			{{"Sega Mega CD 32X", "Mega CD 32X", "MCD32X", nullptr}},
			{{"Sega Pico", "Pico", "Pico", nullptr}},
			{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}},
		}};
		return sysNames[romSys][idx];
	}

	// Get the system branding region.
	const MegaDriveRegions::MD_BrandingRegion md_bregion =
		MegaDriveRegions::getBrandingRegion(d->md_region);
	switch (md_bregion) {
		case MegaDriveRegions::MD_BrandingRegion::Japan:
		default: {
			static const array<array<const char*, 4>, 6> sysNames_JP = {{
				{{"Sega Mega Drive", "Mega Drive", "MD", nullptr}},
				{{"Sega Mega CD", "Mega CD", "MCD", nullptr}},
				{{"Sega Super 32X", "Super 32X", "32X", nullptr}},
				{{"Sega Mega CD 32X", "Mega CD 32X", "MCD32X", nullptr}},
				{{"Sega Kids Computer Pico", "Kids Computer Pico", "Pico", nullptr}},
				{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}},
			}};
			return sysNames_JP[romSys][idx];
		}

		case MegaDriveRegions::MD_BrandingRegion::USA: {
			static const array<array<const char*, 4>, 6> sysNames_US = {{
				// TODO: "MD" or "Gen"?
				{{"Sega Genesis", "Genesis", "MD", nullptr}},
				{{"Sega CD", "Sega CD", "MCD", nullptr}},
				{{"Sega 32X", "Sega 32X", "32X", nullptr}},
				{{"Sega CD 32X", "Sega CD 32X", "MCD32X", nullptr}},
				{{"Sega Pico", "Pico", "Pico", nullptr}},
				{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}},
			}};
			return sysNames_US[romSys][idx];
		}

		case MegaDriveRegions::MD_BrandingRegion::Europe: {
			static const array<array<const char*, 4>, 6> sysNames_EU = {{
				{{"Sega Mega Drive", "Mega Drive", "MD", nullptr}},
				{{"Sega Mega CD", "Mega CD", "MCD", nullptr}},
				{{"Sega Mega Drive 32X", "Mega Drive 32X", "32X", nullptr}},
				{{"Sega Mega CD 32X", "Sega Mega CD 32X", "MCD32X", nullptr}},
				{{"Sega Pico", "Pico", "Pico", nullptr}},
				{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}},
			}};
			return sysNames_EU[romSys][idx];
		}

		case MegaDriveRegions::MD_BrandingRegion::South_Korea: {
			static const array<array<const char*, 4>, 6> sysNames_KR = {{
				// TODO: "MD" or something else?
				{{"Samsung Super Aladdin Boy", "Super Aladdin Boy", "MD", nullptr}},
				{{"Samsung CD Aladdin Boy", "CD Aladdin Boy", "MCD", nullptr}},
				{{"Samsung Super 32X", "Super 32X", "32X", nullptr}},
				{{"Sega Mega CD 32X", "Sega Mega CD 32X", "MCD32X", nullptr}},
				{{"Samsung Pico", "Pico", "Pico", nullptr}},
				{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}},
			}};
			return sysNames_KR[romSys][idx];
		}

		case MegaDriveRegions::MD_BrandingRegion::Brazil: {
			static const array<array<const char*, 4>, 6> sysNames_BR = {{
				{{"Sega Mega Drive", "Mega Drive", "MD", nullptr}},
				{{"Sega CD", "Sega CD", "MCD", nullptr}},
				{{"Sega Mega 32X", "Mega 32X", "32X", nullptr}},
				{{"Sega CD 32X", "Sega CD 32X", "MCD32X", nullptr}},
				{{"Sega Pico", "Pico", "Pico", nullptr}},
				{{"Sega Teradrive", "Teradrive", "Teradrive", nullptr}}
			}};
			return sysNames_BR[romSys][idx];
		}
	}

	// Should not get here...
	return nullptr;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t MegaDrive::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> MegaDrive::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// FIXME: Assuming 320x224; some games might use 256x224,
			// which will need scaling.
			return {{nullptr, 320, 224, 0}};
		default:
			break;
	}

	// Unsupported image type.
	return {};
}

/**
 * Get image processing flags.
 *
 * These specify post-processing operations for images,
 * e.g. applying transparency masks.
 *
 * @param imageType Image type.
 * @return Bitfield of ImageProcessingBF operations to perform.
 */
uint32_t MegaDrive::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// Rescaling is required for the 8:7 pixel aspect ratio
			// if the image is 256px wide. Note that this flag only
			// takes effect for 256px or 512px images; 320px images
			// will not be rescaled.
			ret = IMGPF_RESCALE_ASPECT_8to7;
			break;

		default:
			break;
	}
	return ret;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int MegaDrive::loadFieldData(void)
{
	RP_D(MegaDrive);
	if (!d->fields.empty()) {
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
	d->fields.reserve(14);

	// Reserve at least 2 tabs.
	d->fields.reserveTabs(2);

	// ROM Header.
	d->fields.setTabName(0, C_("MegaDrive", "ROM Header"));
	d->addFields_romHeader(&d->romHeader);

	if (!d->isDisc()) {
		// Vector table. (MD only; not valid for Mega CD.)
		d->fields.addTab(C_("MegaDrive", "Vector Table"));
		d->addFields_vectorTable(&d->vectors);
	}

	// Check for a locked-on ROM image.
	if (d->pRomHeaderLockOn) {
		// Locked-on ROM is present.
		d->fields.addTab(C_("MegaDrive", "Locked-On ROM Header"));
		d->addFields_romHeader(d->pRomHeaderLockOn.get(), true);
	}

	// Try to open the ISO-9660 object.
	// NOTE: Only done here because the ISO-9660 fields
	// are used for field info only.
	if (d->isDisc()) {
		ISO *const isoData = new ISO(d->file);
		if (isoData->isOpen()) {
			// Add the fields.
			const RomFields *const isoFields = isoData->fields();
			assert(isoFields != nullptr);
			if (isoFields) {
				d->fields.addFields_romFields(isoFields,
					RomFields::TabOffset_AddTabs);
			}
		}
		delete isoData;
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int MegaDrive::loadMetaData(void)
{
	RP_D(MegaDrive);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || (int)d->romType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(2);	// Maximum of 2 metadata properties.

	// MD ROM header
	// TODO: Lock-on support?
	const MD_RomHeader *const romHeader = &d->romHeader;

	// Title
	// TODO: Domestic vs. export; space elimination?
	// Check domestic first. If empty, check overseas.
	string s_title = cp1252_sjis_to_utf8(romHeader->title_export, sizeof(romHeader->title_export));
	trimEnd(s_title);
	if (s_title.empty()) {
		s_title = cp1252_sjis_to_utf8(romHeader->title_domestic, sizeof(romHeader->title_domestic));
	}
	if (!s_title.empty()) {
		d->metaData->addMetaData_string(Property::Title, s_title);
	}

	// Publisher
	// TODO: Don't show if the publisher is unknown?
	d->metaData->addMetaData_string(Property::Publisher, d->getPublisher(romHeader));

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type.
 * @param pExtURLs	[out]    Output vector.
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int MegaDrive::extURLs(ImageType imageType, vector<ExtURL> *pExtURLs, int size) const
{
	ASSERT_extURLs(imageType, pExtURLs);
	pExtURLs->clear();

	RP_D(const MegaDrive);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// TODO: Some ROMs are 'worldwide' but may have a different
	// title screen depending on region. This may need to be
	// hard-coded...

	// System IDs.
	static constexpr char sys_tbl[][8] = {
		"md", "mcd", "32x", "mcd32x", "pico", "tera"
	};
	if ((d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK) >= ARRAY_SIZE(sys_tbl))
		return -ENOENT;
	const char *const sys = sys_tbl[d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK];

	// If this is S&K plus a locked-on ROM, use the
	// locked-on ROM's serial number with region "S&K".
	const MD_RomHeader *const pRomHeader = (d->pRomHeaderLockOn != nullptr
						? d->pRomHeaderLockOn.get()
						: &d->romHeader);

	const bool isEarlyRomHeader = d->checkIfEarlyRomHeader(pRomHeader);
	const char *const s_serial_number = (isEarlyRomHeader)
		? pRomHeader->early.serial_number
		: pRomHeader->serial_number;

	// Check for "generic" serial numbers used by some prototypes.
	// We can't easily look up these ROMs at the moment.
	struct md_serial_number_t {
		char serial_number[15];
		uint8_t len;
	};
	static const array<md_serial_number_t, 7> generic_serials = {{
		{"GM 00000000", 11},
		{"GM XXXXXXXX", 11},
		{"GM MK-0000 ", 11},
		{"GM T-000000", 11},
		{"GM T-00000 ", 11},
		{"GM T000000",  10},
		{"GM T-XXXXX ", 11}
	}};
	for (const auto &p : generic_serials) {
		if (!memcmp(s_serial_number, p.serial_number, p.len)) {
			// This ROM image has a generic serial number.
			return -ENOENT;
		}
	}

	// Make sure the ROM serial number is valid.
	// It should start with one of the following:
	// - "GM ": Game
	// - "G  ": Some games and non-game titles don't have the 'M'.
	// - "AI ": Educational
	// - "BR ": Mega CD Boot ROM
	// - "OS ": TMSS
	// - "SP ": Multitap I/O Sample Program
	// - "MK ": Incorrect "Game" designation used by some games.
	// - "mk ": Lowercase "MK", used by "Senjou no Ookami II ~ Mercs (World)".
	// - "T-":  Incorrect "third-party" designation. (Should be "GM T-")
	// - "G-":  Incorrect "first-party JP" designation. (Should be "GM G-")
	// - "TECTOY ": Used by TecToy titles.
	// - "SF-": Some unlicensed titles.
	// - "JN-": Some unlicensed titles.
	// - "LGM-": Some unlicensed titles.
	// - "HPC-": (Some) Sega Pico titles.
	// - "MPR-": (Some) Sega Pico titles. (BR)
	// - "837-": (Some) Samsung Pico titles. (KR)
	// - "610-": (Some) Sega Pico titles. (JP)
	// - TODO: Others?
	uint16_t rom_type;
	memcpy(&rom_type, s_serial_number, sizeof(rom_type));
	// Verify the separator.
	switch (s_serial_number[2]) {
		case ' ': case '_': case '-': case 'T':
			// ' ' is the normal version.
			// '_' and '-' are used incorrectly by some ROMs.
			// 'T' is used by a few that have a very long serial number.
			break;
		default:
			// Check for some exceptions:
			// - "T-": Incorrectly used by some games.
			// - "TECTOY": Used by TecToy titles.
			// - "HPC-": Used by some Pico titles.
			// - "MPR-": Used by some Pico titles.
			if (rom_type == cpu_to_be16('T-') ||
			    rom_type == cpu_to_be16('G-') ||
			    !memcmp(s_serial_number, "LGM-", 4) ||
			    !memcmp(s_serial_number, "HPC-", 4) ||
			    !memcmp(s_serial_number, "MPR-", 4) ||
			    !memcmp(s_serial_number, "837-", 4) ||
			    !memcmp(s_serial_number, "610-", 4) ||
			    !memcmp(s_serial_number, "TECTOY ", 7))
			{
				// Found an exception.
				break;
			}
			// Missing separator.
			return -ENOENT;
	}

	// Verify the ROM type.
	// TODO: Constant cpu_to_be16() macro for use in switch()?
	switch (be16_to_cpu(rom_type)) {
		case 'GM': case 'G ': case 'G-': case 'AI':
		case 'BR': case 'OS':
		case 'SP': case 'MK': case 'mk': case 'T-':
		case 'TE':	// "TECTOY "
		case 'SF': case 'JN': case 'LG':
		case 'HP': case 'MP': case '61': case '83':
			break;
		default:
			// Not a valid ROM type.
			return -ENOENT;
	}

	// If this is S&K plus a locked-on ROM, use the
	// locked-on ROM's serial number with region "S&K".
	char region_code[4];
	string gameID;
	if (d->pRomHeaderLockOn && rom_type == cpu_to_be16('GM')) {
		// Use region code "S&K" for locked-on ROMs.
		memcpy(region_code, "S&K", 4);

		// A unique title screen is only provided for:
		// - Sonic 1 (all Blue Spheres levels)
		// - Sonic 2 (Knuckles in Sonic 2)
		// - Sonic 3 (Sonic 3 & Knuckles)

		// If the serial number doesn't match, then use a
		// generic "NO WAY!" screen.
		static const array<md_serial_number_t, 6> lockon_valid_serials = {{
			{"00001009-00", 11},
			{"00004049-01", 11},
			{"00001051-00", 11},
			{"00001051-01", 11},
			{"00001051-02", 11},
			{"MK-1079 -00", 11}
		}};
		bool is_lockon_valid = false;
		for (const auto &p : lockon_valid_serials) {
			if (!memcmp(&s_serial_number[3], p.serial_number, p.len)) {
				is_lockon_valid = true;
				break;
			}
		}
		if (is_lockon_valid) {
			// Unique title screen is available.
			gameID = latin1_to_utf8(s_serial_number, sizeof(pRomHeader->serial_number));
		} else {
			// Generic title screen only.
			// TODO: If the locked-on ROM is >2 MB, show S&K?
			gameID = "NO-WAY";
		}
	} else {
		// Using the MD hex region code.
		static constexpr array<char, 16> dec_to_hex = {{
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
		}};
		region_code[0] = dec_to_hex[d->md_region & 0x0F];
		region_code[1] = '\0';

		// Get the serial number and trim it.
		gameID = latin1_to_utf8(s_serial_number, sizeof(pRomHeader->serial_number));
		while (!gameID.empty()) {
			const size_t sz = gameID.size();
			if (gameID[sz-1] != ' ')
				break;
			gameID.resize(sz-1);
		}
		if (gameID.empty()) {
			// No game ID. Image is not available.
			return -ENOENT;
		}
	}

	switch (d->romType & MegaDrivePrivate::ROM_SYSTEM_MASK) {
		default:
			break;

		case MegaDrivePrivate::ROM_SYSTEM_MD: {
			// Special handling for Wonder Mega 1.00 boot ROMs, since they
			// have the same serial number as the matching Mega CD.
			if (rom_type == cpu_to_be16('BR') &&
			    pRomHeader->title_domestic[0] == 'W' &&
			    pRomHeader->title_domestic[6] == '-')
			{
				gameID += "-WM";
				break;
			}

			// Special handling for Game Toshokan ROMs, which have a 128 KB Boot ROM
			// and 128 KB RAM for downloaded games.
			// NOTE: No-Intro has two sets:
			// - "Game no Kanzume Otokuyou": Boot ROM + game data (found on compilation discs)
			// - "SegaNet": Same, but most of these have had their checksums updated for the full 256 KB.
			if (d->gt_crc != 0) {
				// Use the CRC32 of $20000-$200FF as the filename with the GT "region".
				region_code[0] = 'G';
				region_code[1] = 'T';
				region_code[2] = '\0';
				char buf[16];
				snprintf(buf, sizeof(buf), "%08X", d->gt_crc);
				gameID = buf;
				break;
			}

			// Check for some ROMs that have different variants with the
			// same serial number. We'll append the ROM checksum to the
			// filename, similar to DMG.
			struct MDRomSerialData_t {
				uint8_t md_region;
				char serial[15];
			};
			static const array<MDRomSerialData_t, 26> md_rom_serial_data = {{
				{0x0F, "GM 00004039-00"},	// Arrow Flash
				{0x04, "GM T-24016 -00"},	// Atomic Robo-Kid
				{0x08, "GM T-120146-50"},	// Brian Lara Cricket 96 / Shane Warne Cricket (EUR)
				{0x04, "GM T-50016 -00"},	// Budokan / John Madden Football /
				                         	// F1 World Championship Edition (USA) (Proto 1)
				{0x03, "GM 64013-00   "},	// Dahna - Megami Tanjou (JPN)
				{0x08, "GM T-70246 -00"},	// Dune II - The Battle for Arrakis (EUR)
				{0x03, "GM T-68063 -00"},	// Dyna Brothers 2 [Special] (JPN)
				{0x04, "GM T-50236 -00"},	// EA Hockey (JPN) / NHL Hockey (USA)
				{0x07, "GM T-32063 -00"},	// El.Viento (JPN/USA)
				{0x0C, "GM T-081046-00"},	// Empire of Steel (EUR) / Steel Empire (USA)
				{0x04, "GM T-50256 -00"},	// Fatal Rewind / The Killing Game Show
				{0x04, "GM T-34016 -00"},	// Fire Shark (USA/EUR)
				{0x07, "GM T-50901 -00"},	// Frogger (USA)
				{0x04, "GM T-47026 -00"},	// Ka-Ge-Ki (JPN/USA)
				{0x08, "GM MK-1353 -00"},	// Landstalker (EUR)
				{0x03, "GM G-4078   00"},	// Mazin Saga (JPN)
				{0x0F, "GM T-81406 -00"},	// NBA Jam Tournament Edition /
				                         	// Blockbuster World Video Game Championship II
				{0x07, "GM T-13083 -00"},	// Side Pocket
				{0x03, "GM G-005536-01"},	// Soleil (JPN)
				{0x08, "GM MK-01182-00"},	// Soleil (EUR)
				{0x04, "GM T-30016 -00"},	// Super Volleyball (USA) / Sport Games (BRA)
				{0x04, "GM T-50376 -00"},	// Team USA Basketball (USA/EUR) / Dream Team USA (JPN)
				{0x03, "GM G-5543  -00"},	// The Story of Thor (JPN)
				{0x08, "GM MK-1354 -00"},	// The Story of Thor (EUR)
				{0x08, "GM MK-1043 -00"},	// ToeJam & Earl in Panic on Funkotron (EUR)
				{0x03, "GM T-22033 -00"},	// Caesar no Yabou / Warrior of Rome
				//{0x03, "GM T-22056 -00"},	// Caesar no Yabou II / Warrior of Rome II (FIXME: Checksums are identical...)
			}};

			for (const auto &p : md_rom_serial_data) {
				if (p.md_region != d->md_region)
					continue;
				if (memcmp(p.serial, pRomHeader->serial_number, 14) != 0)
					continue;

				// Found a match! Append the checksum.
				// NOTE: Not supporting "early" ROM headers here.
				char buf[16];
				snprintf(buf, sizeof(buf), ".%04X", be16_to_cpu(pRomHeader->checksum));
				gameID += buf;
				break;
			}
			break;
		}

		case MegaDrivePrivate::ROM_SYSTEM_MCD:
		case MegaDrivePrivate::ROM_SYSTEM_MCD32X:
			// Check for certain MCD titles for exceptions.

			if (!memcmp(s_serial_number, "GM T-162035-", 12) ||
			    !memcmp(s_serial_number, "GM T-16204F-", 12))
			{
				// Slam City with Scottie Pippen (MCD, MCD32X)
				// Different discs have a different number in the
				// MCD-specific header, but the serial number is
				// identical on all MCD discs, and all MCD32X discs.
				if (ISDIGIT(d->mcd_hdr[0x1A])) {
					// Append the disc number.
					gameID += ".disc";
					gameID += (char)d->mcd_hdr[0x1A];
				}
			} else {
				switch (d->md_region) {
					default:
						break;

					case MegaDriveRegions::MD_REGION_EUROPE:
						if (!memcmp(s_serial_number, "GM MK-4411 -00", 14)) {
							// Jurassic Park (EUR) - different language versions,
							// which have different titles but the same serial number.
							if (ISALPHA(pRomHeader->title_domestic[14])) {
								// Append the first character of the region.
								gameID += '.';
								gameID += pRomHeader->title_domestic[14];
							}
						} else if (!memcmp(s_serial_number, "GM T-93185-00 ", 14)) {
							// Sensible Soccer (EUR) - a demo version with a slightly
							// different title screen has the same serial number.
							// It can be identified by an earlier build date in the
							// MCD-specific header.
							if (d->mcd_hdr[0x51] == '5') {
								gameID += ".demo";
							}
						}
						break;

					case MegaDriveRegions::MD_REGION_JAPAN | MegaDriveRegions::MD_REGION_ASIA:
						if (!memcmp(s_serial_number, "GM G-6041     ", 14)) {
							// Ecco two-disc set (JPN) - both discs have the same
							// serial number, but Ecco II has a '2' in the
							// MCD-specific header.
							gameID += ".disc";
							gameID += (d->mcd_hdr[0x14] == '2' ? '2' : '1');
						} else if (!memcmp(s_serial_number, "GM T-32024 -01", 14)) {
							// Sega MultiMedia Studio Demo has the same
							// serial number as Sol-Feace (JPN).
							if (pRomHeader->title_domestic[0] == 'D') {
								// Sega MultiMedia Studio Demo
								gameID += ".mmstudio";
							}
						}
						break;

					case MegaDriveRegions::MD_REGION_USA:
						if (!memcmp(s_serial_number, "GM T-121015-00", 14)) {
							// Dragon's Lair (USA)
							// Demo version has the same serial number but says
							// "Demo" in the title field.
							// FIXME: Space Ace (USA) demo has the exact same serial number
							// and header data as Dragon's Lair (USA) for some reason.
							// Space Ace's data track is less than 10 MB, so we'll use that
							// to distinguish it.
							if (pRomHeader->title_domestic[13] == 'D') {
								// Dragon's Lair demo
								gameID += ".demo";
							} else if (d->file->size() < 10U*1024U*1024U) {
								// Space Ace demo
								gameID += ".sa-demo";
							}
						}
						break;
				}
			}
			break;
	}

	// NOTE: We only have one size for MegaDrive right now.
	// TODO: Determine the actual image size.
	RP_UNUSED(size);
	vector<ImageSizeDef> sizeDefs = supportedImageSizes(imageType);
	assert(sizeDefs.size() == 1);
	if (sizeDefs.empty()) {
		// No image sizes.
		return -ENOENT;
	}

	// NOTE: RPDB's title screen database only has one size.
	// There's no need to check image sizes, but we need to
	// get the image size for the extURLs struct.

	// Determine the image type name.
	const char *imageTypeName;
	const char *ext;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			imageTypeName = "title";
			ext = ".png";
			break;
		default:
			// Unsupported image type.
			return -ENOENT;
	}

	// Add the URLs.
	pExtURLs->resize(1);
	auto extURL_iter = pExtURLs->begin();
	extURL_iter->url = d->getURL_RPDB(sys, imageTypeName, region_code, gameID.c_str(), ext);
	extURL_iter->cache_key = d->getCacheKey_RPDB(sys, imageTypeName, region_code, gameID.c_str(), ext);
	extURL_iter->width = sizeDefs[0].width;
	extURL_iter->height = sizeDefs[0].height;
	extURL_iter->high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

/**
 * Check for "viewed" achievements.
 *
 * @return Number of achievements unlocked.
 */
int MegaDrive::checkViewedAchievements(void) const
{
	RP_D(const MegaDrive);
	if (!d->isValid) {
		// ROM is not valid.
		return 0;
	}

	Achievements *const pAch = Achievements::instance();
	int ret = 0;

	if (d->pRomHeaderLockOn) {
		// Is it S&K locked on to S&K?
		// NOTE: S&K always has a standard serial number,
		// so we don't have to call checkIfEarlyRomHeader().
		if (!memcmp(d->pRomHeaderLockOn->serial_number, "GM MK-1563 -00",
		            sizeof(d->pRomHeaderLockOn->serial_number)))
		{
			// It is!
			pAch->unlock(Achievements::ID::ViewedMegaDriveSKwithSK);
			ret++;
		}
	}

	return ret;
}

} // namespace LibRomData

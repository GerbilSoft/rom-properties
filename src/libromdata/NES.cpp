/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NES.cpp: Nintendo Entertainment System/Famicom ROM reader.              *
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

#include "NES.hpp"
#include "RomData_p.hpp"

#include "data/NintendoPublishers.hpp"
#include "nes_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

 // C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cctype>

 // C++ includes.
#include <vector>
#include <algorithm>
using std::vector;

namespace LibRomData {

class NESPrivate : public RomDataPrivate
{
	public:
		NESPrivate(NES *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		NESPrivate(const NESPrivate &other);
		NESPrivate &operator=(const NESPrivate &other);

	public:
		/** RomFields **/

		// Monospace string formatting.
		static const RomFields::StringDesc nes_string_monospace;

		// ROM fields.
		static const struct RomFields::Desc nes_fields[];

		// ROM image type.
		enum RomType {
			ROM_TYPE_UNKNOWN = -1,	// Unknown ROM type.

			ROM_TYPE_OLD_INES = 0,	// Archaic iNES format
			ROM_TYPE_INES = 1,	// iNES format
			ROM_TYPE_NES2 = 2,	// NES 2.0 format
			ROM_TYPE_TNES = 3,	// TNES (Nintendo 3DS Virtual Console)

			ROM_TYPE_FDS = 4,	// Famicom Disk System
			ROM_TYPE_FDS_TNES = 5,	// Famicom Disk System (TNES / TDS)

			// TODO: Other VC, FDS, maybe UNIF?
		};
		int romType;

	public:
		// ROM header.
		union {
			INES_RomHeader ines;
			TNES_RomHeader tnes;
		} header;

		/**
		 * Format PRG/CHR ROM bank sizes, in KB.
		 *
		 * This function expects the size to be a multiple of 1024,
		 * so it doesn't do any fractional rounding or printing.
		 *
		 * @param size File size.
		 * @return Formatted file size.
		 */
		static inline rp_string formatBankSizeKB(unsigned int size);
};

/** NESPrivate **/

// Monospace string formatting.
const RomFields::StringDesc NESPrivate::nes_string_monospace = {
	RomFields::StringDesc::STRF_MONOSPACE
};

// ROM fields.
const struct RomFields::Desc NESPrivate::nes_fields[] = {
	{_RP("Format"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Mapper"), RomFields::RFT_STRING, {nullptr}},
	{_RP("Submapper"), RomFields::RFT_STRING, {nullptr}},
	{_RP("TNES Mapper"), RomFields::RFT_STRING, {nullptr}},
	{_RP("PRG ROM Size"), RomFields::RFT_STRING, {nullptr}},
	{_RP("CHR ROM Size"), RomFields::RFT_STRING, {nullptr}},
};

NESPrivate::NESPrivate(NES *q, IRpFile *file)
	: super(q, file, nes_fields, ARRAY_SIZE(nes_fields))
	, romType(ROM_TYPE_UNKNOWN)
{
	// Clear the ROM header structs.
	memset(&header, 0, sizeof(header));
}

/**
 * Format PRG/CHR ROM bank sizes, in KB.
 *
 * This function expects the size to be a multiple of 1024,
 * so it doesn't do any fractional rounding or printing.
 *
 * @param size File size.
 * @return Formatted file size.
 */
inline rp_string NESPrivate::formatBankSizeKB(unsigned int size)
{
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "%u KB", (size / 1024));
	if (len > (int)sizeof(buf))
		len = (int)sizeof(buf);
	return (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
}

/** NES **/

/**
 * Read an NES ROM.
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
NES::NES(IRpFile *file)
	: super(new NESPrivate(this, file))
{
	RP_D(NES);

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}
	
	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header. [16 bytes]
	uint8_t header[16];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for NES.
	info.szFile = d->file->fileSize();
	d->romType = isRomSupported_static(&info);

	switch (d->romType) {
		case NESPrivate::ROM_TYPE_OLD_INES:
		case NESPrivate::ROM_TYPE_INES:
		case NESPrivate::ROM_TYPE_NES2:
			// iNES-style ROM header.
			d->fileType = FTYPE_ROM_IMAGE;
			memcpy(&d->header.ines, header, sizeof(d->header.ines));
			break;

		case NESPrivate::ROM_TYPE_TNES:
			// TNES ROM header.
			d->fileType = FTYPE_ROM_IMAGE;
			memcpy(&d->header.tnes, header, sizeof(d->header.tnes));
			break;

		case NESPrivate::ROM_TYPE_FDS:
			// FDS disk image.
			// TODO: Add header.
			d->fileType = FTYPE_DISK_IMAGE;
			break;

		case NESPrivate::ROM_TYPE_FDS_TNES:
			// FDS disk image. (TNES/TDS format)
			// TODO: Add header.
			d->fileType = FTYPE_DISK_IMAGE;
			break;

		default:
			// Unknown ROM type.
			d->fileType = FTYPE_UNKNOWN;
			d->romType = NESPrivate::ROM_TYPE_UNKNOWN;
			return;
	}

	d->isValid = true;
}

/** ROM detection functions. **/

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NES::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 16)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for iNES.
	const INES_RomHeader *inesHeader =
		reinterpret_cast<const INES_RomHeader*>(info->header.pData);
	static const uint8_t ines_magic[4] = {'N', 'E', 'S', 0x1A};
	if (!memcmp(inesHeader->magic, ines_magic, sizeof(inesHeader->magic))) {
		// Found an iNES ROM header.
		// Check for NES 2.0.
		if ((inesHeader->mapper_hi & 0x0C) == 0x08) {
			// May be NES 2.0
			// Verify the ROM size.
			int64_t size = sizeof(INES_RomHeader) +
				(inesHeader->prg_banks * INES_PRG_BANK_SIZE) +
				(inesHeader->chr_banks * INES_CHR_BANK_SIZE) +
				((inesHeader->nes2.prg_banks_hi << 8) * INES_PRG_BANK_SIZE);
			if (size <= info->szFile) {
				// This is an NES 2.0 header.
				return NESPrivate::ROM_TYPE_NES2;
			}
		}

		// Not NES 2.0.
		if ((inesHeader->mapper_hi & 0x0C) == 0x00) {
			// May be iNES.
			// TODO: Optimize this check?
			if (info->header.pData[12] == 0 &&
			    info->header.pData[13] == 0 &&
			    info->header.pData[14] == 0 &&
			    info->header.pData[15] == 0)
			{
				// Definitely iNES.
				return NESPrivate::ROM_TYPE_INES;
			}
		}

		// Archaic iNES format.
		return NESPrivate::ROM_TYPE_OLD_INES;
	}

	// Check for TNES.
	const TNES_RomHeader *tnesHeader =
		reinterpret_cast<const TNES_RomHeader*>(info->header.pData);
	static const uint8_t tnes_magic[4] = {'T', 'N', 'E', 'S'};
	if (!memcmp(tnesHeader->magic, tnes_magic, sizeof(tnesHeader->magic))) {
		// Found a TNES ROM header.
		if (tnesHeader->mapper == TNES_MAPPER_FDS) {
			// This is an FDS game.
			return NESPrivate::ROM_TYPE_FDS_TNES;
		}

		// TODO: Verify ROM size?
		return NESPrivate::ROM_TYPE_TNES;
	}

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int NES::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *NES::systemName(uint32_t type) const
{
	RP_D(NES);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Regional variant, e.g. Famicom.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NES::systemName() array index optimization needs to be updated.");

	uint32_t idx = (type & SYSNAME_TYPE_MASK);
	switch (d->romType) {
		case NESPrivate::ROM_TYPE_OLD_INES:
		case NESPrivate::ROM_TYPE_INES:
		case NESPrivate::ROM_TYPE_NES2:
		case NESPrivate::ROM_TYPE_TNES:
		default: {
			// TODO: PC10, VS System.
			static const rp_char *const sysNames_NES[] = {
				_RP("Nintendo Entertainment System"),
				_RP("Nintendo Entertainment System"),
				_RP("NES"), nullptr
			};
			return sysNames_NES[idx];
		}

		case NESPrivate::ROM_TYPE_FDS:
		case NESPrivate::ROM_TYPE_FDS_TNES: {
			static const rp_char *const sysNames_FDS[] = {
				_RP("Nintendo Famicom Disk System"),
				_RP("Famicom Disk System"),
				_RP("FDS"), nullptr
			};
			return sysNames_FDS[idx];
		}
	};

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
 * NOTE 2: The strings in the std::vector should *not*
 * be freed by the caller.
 *
 * @return List of all supported file extensions.
 */
vector<const rp_char*> NES::supportedFileExtensions_static(void)
{
	vector<const rp_char*> ret;
	ret.reserve(3);
	ret.push_back(_RP(".nes"));
	ret.push_back(_RP(".fds"));
	ret.push_back(_RP(".tds"));
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
vector<const rp_char*> NES::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NES::loadFieldData(void)
{
	RP_D(NES);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NES ROM header

	// Determine stuff based on the ROM format.
	const rp_char *rom_format;
	bool romOK = true;
	int mapper = -1;
	int submapper = -1;
	int tnes_mapper = -1;
	unsigned int prg_rom_size = 0;
	unsigned int chr_rom_size = 0;
	switch (d->romType) {
		case NESPrivate::ROM_TYPE_OLD_INES:
			rom_format = _RP("Archaic iNES");
			mapper = (d->header.ines.mapper_lo >> 4);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_TYPE_INES:
			rom_format = _RP("iNES");
			mapper = (d->header.ines.mapper_lo >> 4) |
				 (d->header.ines.mapper_hi & 0xF0);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_TYPE_NES2:
			rom_format = _RP("NES 2.0");
			mapper = (d->header.ines.mapper_lo >> 4) |
				 (d->header.ines.mapper_hi & 0xF0) |
				 ((d->header.ines.nes2.mapper_hi2 & 0x0F) << 8);
			submapper = (d->header.ines.nes2.mapper_hi2 >> 4);
			prg_rom_size = ((d->header.ines.prg_banks +
					(d->header.ines.nes2.prg_banks_hi << 8))
					* INES_PRG_BANK_SIZE);
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_TYPE_TNES:
			rom_format = _RP("TNES (Nintendo 3DS Virtual Console)");
			tnes_mapper = d->header.tnes.mapper;
			prg_rom_size = d->header.tnes.prg_banks * TNES_PRG_BANK_SIZE;
			chr_rom_size = d->header.tnes.chr_banks * TNES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_TYPE_FDS:
			rom_format = _RP("FDS disk image");
			// TODO: More fields.
			break;

		case NESPrivate::ROM_TYPE_FDS_TNES:
			rom_format = _RP("TDS (Nintendo 3DS Virtual Console)");
			// TODO: More fields.
			break;

		default:
			rom_format = _RP("Unknown");
			romOK = false;
			break;
	}

	// Display the fields.
	d->fields->addData_string(rom_format);
	if (!romOK) {
		// Invalid mapper.
		d->fields->addData_invalid();	// Mapper
		d->fields->addData_invalid();	// Submapper
		d->fields->addData_invalid();	// TNES Mapper
		d->fields->addData_invalid();	// PRG ROM Size
		d->fields->addData_invalid();	// CHR ROM Size
		return (int)d->fields->count();
	}

	if (mapper >= 0) {
		// Mapper.
		// TODO: Look up the mapper description.
		d->fields->addData_string_numeric(mapper, RomFields::FB_DEC);
	} else {
		// No mapper.
		if (tnes_mapper >= 0) {
			// This has a TNES mapper.
			// It *should* map to an iNES mapper...
			d->fields->addData_string(_RP("MISSING TNES MAPPING"));
		} else {
			// No mapper. Probably an FDS image.
			d->fields->addData_invalid();
		}
	}

	if (submapper >= 0) {
		// Submapper. (NES 2.0 only)
		d->fields->addData_string_numeric(submapper, RomFields::FB_DEC);
	} else {
		// No submapper.
		d->fields->addData_invalid();
	}

	if (tnes_mapper >= 0) {
		// TNES mapper.
		// TODO: Look up the name.
		d->fields->addData_string_numeric(tnes_mapper, RomFields::FB_DEC);
	} else {
		// Not TNES.
		d->fields->addData_invalid();
	}

	// ROM sizes.
	if (prg_rom_size > 0) {
		d->fields->addData_string(d->formatBankSizeKB(prg_rom_size));
	} else {
		d->fields->addData_invalid();
	}
	if (chr_rom_size > 0) {
		d->fields->addData_string(d->formatBankSizeKB(chr_rom_size));
	} else {
		d->fields->addData_invalid();
	}

	// TODO: More fields.
	return (int)d->fields->count();
}

}

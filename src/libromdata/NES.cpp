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
#include "data/NESMappers.hpp"
#include "SystemRegion.hpp"
#include "nes_structs.h"

#include "common.h"
#include "byteswap.h"
#include "TextFuncs.hpp"
#include "file/IRpFile.hpp"

 // C includes. (C++ namespace)
#include <cassert>
#include <cstring>
#include <cctype>
#include <ctime>

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
		RP_DISABLE_COPY(NESPrivate)

	public:
		/** RomFields **/

		// ROM image type.
		enum RomType {
			ROM_UNKNOWN = -1,		// Unknown ROM type.

			ROM_FORMAT_OLD_INES = 0,	// Archaic iNES format
			ROM_FORMAT_INES = 1,		// iNES format
			ROM_FORMAT_NES2 = 2,		// NES 2.0 format
			ROM_FORMAT_TNES = 3,		// TNES (Nintendo 3DS Virtual Console)
			ROM_FORMAT_FDS = 4,		// Famicom Disk System
			ROM_FORMAT_FDS_FWNES = 5,	// Famicom Disk System (with fwNES header)
			ROM_FORMAT_FDS_TNES = 6,	// Famicom Disk System (TNES / TDS)
			ROM_FORMAT_UNKNOWN = 0xFF,
			ROM_FORMAT_MASK = 0xFF,

			ROM_SYSTEM_NES = (0 << 8),	// NES / Famicom
			ROM_SYSTEM_FDS = (1 << 8),	// Famicom Disk System
			ROM_SYSTEM_VS = (2 << 8),	// VS. System
			ROM_SYSTEM_PC10 = (3 << 8),	// PlayChoice-10
			ROM_SYSTEM_UNKNOWN = (0xFF << 8),
			ROM_SYSTEM_MASK = (0xFF << 8),

			// Special flags. (bitfield)
			ROM_SPECIAL_WIIU_VC = (1 << 16),	// Wii U VC (modified iNES)
			// TODO: Other VC formats, maybe UNIF?
		};
		int romType;

	public:
		// ROM header.
		struct {
			// iNES and FDS are mutually exclusive.
			// TNES + FDS is possible, though.
			union {
				INES_RomHeader ines;
				struct {
					FDS_DiskHeader_fwNES fds_fwNES;
					FDS_DiskHeader fds;
				};
			};
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

		/**
		 * Convert an FDS BCD datestamp to Unix time.
		 * @param fds_bcd_ds FDS BCD datestamp.
		 * @return Unix time, or -1 if an error occurred.
		 *
		 * NOTE: -1 is a valid Unix timestamp (1970/01/01), but is
		 * not likely to be valid for NES/Famicom, since the Famicom
		 * was released in 1983.
		 */
		static int64_t fds_bcd_datestamp_to_unix(const FDS_BCD_DateStamp *fds_bcd_ds);
};

/** NESPrivate **/

NESPrivate::NESPrivate(NES *q, IRpFile *file)
	: super(q, file)
	, romType(ROM_UNKNOWN)
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

/**
 * Convert an FDS BCD datestamp to Unix time.
 * @param fds_bcd_ds FDS BCD datestamp.
 * @return Unix time, or -1 if an error occurred.
 *
 * NOTE: -1 is a valid Unix timestamp (1970/01/01), but is
 * not likely to be valid for NES/Famicom, since the Famicom
 * was released in 1983.
 */
int64_t NESPrivate::fds_bcd_datestamp_to_unix(const FDS_BCD_DateStamp *fds_bcd_ds)
{
	// Convert the VMI time to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	if ((fds_bcd_ds->year == 0 &&
	     fds_bcd_ds->mon == 0 &&
	     fds_bcd_ds->mday == 0) ||
	    (fds_bcd_ds->year == 0xFF &&
	     fds_bcd_ds->mon == 0xFF &&
	     fds_bcd_ds->mday == 0xFF))
	{
		// Invalid date.
		return -1;
	}

	struct tm fdstime = { };

	// TODO: Check for invalid BCD values.
	fdstime.tm_year = ((fds_bcd_ds->year >> 4) * 10) +
			   (fds_bcd_ds->year & 0x0F) + 1925 - 1900;
	fdstime.tm_mon  = ((fds_bcd_ds->mon >> 4) * 10) +
			   (fds_bcd_ds->mon & 0x0F) - 1;
	fdstime.tm_mday = ((fds_bcd_ds->mday >> 4) * 10) +
			   (fds_bcd_ds->mday & 0x0F);

	// If conversion fails, d->ctime will be set to -1.
#ifdef _WIN32
	// MSVCRT-specific version.
	return _mkgmtime(&fdstime);
#else /* !_WIN32 */
	// FIXME: Might not be available on some systems.
	return timegm(&fdstime);
#endif
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

	// Read the ROM header. [128 bytes]
	uint8_t header[128];
	size_t size = d->file->read(header, sizeof(header));
	if (size != sizeof(header))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(header);
	info.header.pData = header;
	info.ext = nullptr;	// Not needed for NES.
	info.szFile = d->file->size();
	d->romType = isRomSupported_static(&info);

	switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
		case NESPrivate::ROM_FORMAT_INES:
		case NESPrivate::ROM_FORMAT_NES2:
			// iNES-style ROM header.
			d->fileType = FTYPE_ROM_IMAGE;
			memcpy(&d->header.ines, header, sizeof(d->header.ines));
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			// TNES ROM header.
			d->fileType = FTYPE_ROM_IMAGE;
			memcpy(&d->header.tnes, header, sizeof(d->header.tnes));
			break;

		case NESPrivate::ROM_FORMAT_FDS:
			// FDS disk image.
			d->fileType = FTYPE_DISK_IMAGE;
			memcpy(&d->header.fds, header, sizeof(d->header.fds));
			break;

		case NESPrivate::ROM_FORMAT_FDS_FWNES:
			// FDS disk image, with fwNES header.
			d->fileType = FTYPE_DISK_IMAGE;
			memcpy(&d->header.fds_fwNES, header, sizeof(d->header.fds_fwNES));
			memcpy(&d->header.fds, &header[16], sizeof(d->header.fds));
			break;

		case NESPrivate::ROM_FORMAT_FDS_TNES: {
			// FDS disk image. (TNES/TDS format)
			int sret = d->file->seek(0x2010);
			if (sret != 0) {
				// Seek error.
				d->fileType = FTYPE_UNKNOWN;
				d->romType = NESPrivate::ROM_FORMAT_UNKNOWN;
				return;
			}

			size_t szret = d->file->read(&d->header.fds, sizeof(d->header.fds));
			if (szret != sizeof(d->header.fds)) {
				// Error reading the FDS header.
				d->fileType = FTYPE_UNKNOWN;
				d->romType = NESPrivate::ROM_FORMAT_UNKNOWN;
				return;
			}

			d->fileType = FTYPE_DISK_IMAGE;
			break;
		}

		default:
			// Unknown ROM type.
			d->fileType = FTYPE_UNKNOWN;
			d->romType = NESPrivate::ROM_FORMAT_UNKNOWN;
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
	static const uint8_t ines_magic[4] = {'N','E','S',0x1A};
	if (!memcmp(inesHeader->magic, ines_magic, 3) &&
	    (inesHeader->magic[3] == 0x1A || inesHeader->magic[3] == 0x00))
	{
		// Found an iNES ROM header.
		// If the fourth byte is 0x00, it's Wii U VC.
		int romType = (inesHeader->magic[3] == 0x00)
				? NESPrivate::ROM_SPECIAL_WIIU_VC
				: 0;

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
				switch (inesHeader->mapper_hi & INES_F7_SYSTEM_MASK) {
					case INES_F7_SYSTEM_VS:
						romType |= NESPrivate::ROM_FORMAT_NES2 |
							   NESPrivate::ROM_SYSTEM_VS;
						break;
					case INES_F7_SYSTEM_PC10:
						romType |= NESPrivate::ROM_FORMAT_NES2 |
							   NESPrivate::ROM_SYSTEM_PC10;
						break;
					default:
						// TODO: What if both are set?
						romType |= NESPrivate::ROM_FORMAT_NES2 |
							   NESPrivate::ROM_SYSTEM_NES;
						break;
				}
				return romType;
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
				switch (inesHeader->mapper_hi & INES_F7_SYSTEM_MASK) {
					case INES_F7_SYSTEM_VS:
						romType |= NESPrivate::ROM_FORMAT_INES |
							   NESPrivate::ROM_SYSTEM_VS;
						break;
					case INES_F7_SYSTEM_PC10:
						romType |= NESPrivate::ROM_FORMAT_INES |
							   NESPrivate::ROM_SYSTEM_PC10;
						break;
					default:
						// TODO: What if both are set?
						romType |= NESPrivate::ROM_FORMAT_INES |
							   NESPrivate::ROM_SYSTEM_NES;
						break;
				}
				return romType;
			}
		}

		// Archaic iNES format.
		romType |= NESPrivate::ROM_FORMAT_OLD_INES |
			   NESPrivate::ROM_SYSTEM_NES;
		return romType;
	}

	// Check for TNES.
	const TNES_RomHeader *tnesHeader =
		reinterpret_cast<const TNES_RomHeader*>(info->header.pData);
	static const uint8_t tnes_magic[4] = {'T','N','E','S'};
	if (!memcmp(tnesHeader->magic, tnes_magic, sizeof(tnesHeader->magic))) {
		// Found a TNES ROM header.
		if (tnesHeader->mapper == TNES_MAPPER_FDS) {
			// This is an FDS game.
			// TODO: Validate the FDS header?
			return NESPrivate::ROM_FORMAT_FDS_TNES |
			       NESPrivate::ROM_SYSTEM_FDS;
		}

		// TODO: Verify ROM size?
		return NESPrivate::ROM_FORMAT_TNES |
		       NESPrivate::ROM_SYSTEM_NES;
	}

	// Check for FDS.
	static const uint8_t fwNES_magic[4] = {'F','D','S',0x1A};
	static const uint8_t fds_magic[] = {'*','N','I','N','T','E','N','D','O','-','H','V','C','*'};

	// Check for headered FDS.
	const FDS_DiskHeader_fwNES *fwNESHeader =
		reinterpret_cast<const FDS_DiskHeader_fwNES*>(info->header.pData);
	if (!memcmp(fwNESHeader->magic, fwNES_magic, sizeof(fwNESHeader->magic))) {
		// fwNES header is present.
		// TODO: Check required NULL bytes.
		// For now, assume this is correct.
		const FDS_DiskHeader *fdsHeader =
			reinterpret_cast<const FDS_DiskHeader*>(&info->header.pData[16]);
		if (fdsHeader->block_code == 0x01 &&
		    !memcmp(fdsHeader->magic, fds_magic, sizeof(fdsHeader->magic)))
		{
			// This is an FDS disk image.
			return NESPrivate::ROM_FORMAT_FDS_FWNES |
			       NESPrivate::ROM_SYSTEM_FDS;
		}
	} else {
		// fwNES header is not present.
		const FDS_DiskHeader *fdsHeader =
			reinterpret_cast<const FDS_DiskHeader*>(info->header.pData);
		if (fdsHeader->block_code == 0x01 &&
		    !memcmp(fdsHeader->magic, fds_magic, sizeof(fdsHeader->magic)))
		{
			// This is an FDS disk image.
			return NESPrivate::ROM_FORMAT_FDS |
			       NESPrivate::ROM_SYSTEM_FDS;
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
	switch (d->romType & NESPrivate::ROM_SYSTEM_MASK) {
		case NESPrivate::ROM_SYSTEM_NES:
		default: {
			static const rp_char *const sysNames_NES[] = {
				// NES (International)
				_RP("Nintendo Entertainment System"),
				_RP("Nintendo Entertainment System"),
				_RP("NES"), nullptr,

				// Famicom (Japan)
				_RP("Nintendo Famicom"),
				_RP("Famicom"),
				_RP("FC"), nullptr,

				// Hyundai Comboy (South Korea)
				_RP("Hyundai Comboy"),
				_RP("Comboy"),
				_RP("CB"), nullptr,
			};

			if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_GENERIC) {
				// Use the international name.
				return sysNames_NES[idx];
			}

			// Get the system region.
			switch (SystemRegion::getCountryCode()) {
				case 'JP':
					return sysNames_NES[idx + 4];
				case 'KR':
					return sysNames_NES[idx + 8];
				default:
					return sysNames_NES[idx];
			}
		}

		case NESPrivate::ROM_SYSTEM_FDS: {
			static const rp_char *const sysNames_FDS[] = {
				_RP("Nintendo Famicom Disk System"),
				_RP("Famicom Disk System"),
				_RP("FDS"), nullptr
			};
			return sysNames_FDS[idx];
		}

		case NESPrivate::ROM_SYSTEM_VS: {
			static const rp_char *const sysNames_VS[] = {
				_RP("Nintendo VS. System"),
				_RP("VS. System"),
				_RP("VS"), nullptr
			};
			return sysNames_VS[idx];
		}

		case NESPrivate::ROM_SYSTEM_PC10: {
			static const rp_char *const sysNames_PC10[] = {
				_RP("Nintendo PlayChoice-10"),
				_RP("PlayChoice-10"),
				_RP("PC10"), nullptr
			};
			return sysNames_PC10[idx];
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
	// NOTE: .fds is missing block checksums.
	// .qd has block checksums, as does .tds (which is basically
	// a 16-byte header, FDS BIOS, and a .qd file).

	// This isn't too important right now because we're only
	// reading the header, but we'll need to take it into
	// account if file access is added.

	static const rp_char *const exts[] = {
		_RP(".nes"),	// iNES
		_RP(".fds"),	// Famicom Disk System
		_RP(".qd"),	// FDS (Animal Crossing)
		_RP(".tds"),	// FDS (3DS Virtual Console)
	};
	return vector<const rp_char*>(exts, exts + ARRAY_SIZE(exts));
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

	// NES ROM header.
	// TODO: Reduce the maximum by removing fields that
	// aren't usable on NES and aren't usable on FDS?
	d->fields->reserve(12);	// Maximum of 12 fields.

	// Determine stuff based on the ROM format.
	const rp_char *rom_format;
	bool romOK = true;
	int mapper = -1;
	int submapper = -1;
	int tnes_mapper = -1;
	unsigned int prg_rom_size = 0;
	unsigned int chr_rom_size = 0;
	switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
			rom_format = _RP("Archaic iNES");
			mapper = (d->header.ines.mapper_lo >> 4);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_FORMAT_INES:
			rom_format = _RP("iNES");
			mapper = (d->header.ines.mapper_lo >> 4) |
				 (d->header.ines.mapper_hi & 0xF0);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_FORMAT_NES2:
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

		case NESPrivate::ROM_FORMAT_TNES:
			rom_format = _RP("TNES (Nintendo 3DS Virtual Console)");
			tnes_mapper = d->header.tnes.mapper;
			mapper = NESMappers::tnesMapperToInesMapper(tnes_mapper);
			prg_rom_size = d->header.tnes.prg_banks * TNES_PRG_BANK_SIZE;
			chr_rom_size = d->header.tnes.chr_banks * TNES_CHR_BANK_SIZE;
			break;

		// NOTE: FDS fields are handled later.
		// We're just obtaining the ROM format name here.
		case NESPrivate::ROM_FORMAT_FDS:
			rom_format = _RP("FDS disk image");
			break;
		case NESPrivate::ROM_FORMAT_FDS_FWNES:
			rom_format = _RP("FDS disk image (with fwNES header)");
			break;
		case NESPrivate::ROM_FORMAT_FDS_TNES:
			rom_format = _RP("TDS (Nintendo 3DS Virtual Console)");
			break;

		default:
			rom_format = _RP("Unknown");
			romOK = false;
			break;
	}

	if (d->romType & NESPrivate::ROM_SPECIAL_WIIU_VC) {
		// Wii U Virtual Console.
		const int romFormat = (d->romType & NESPrivate::ROM_FORMAT_MASK);
		assert(romFormat >= NESPrivate::ROM_FORMAT_OLD_INES);
		assert(romFormat <= NESPrivate::ROM_FORMAT_NES2);
		if (romFormat >= NESPrivate::ROM_FORMAT_OLD_INES &&
		    romFormat <= NESPrivate::ROM_FORMAT_NES2)
		{
			rp_string str = rom_format;
			str += _RP(" (Wii U Virtual Console)");
			d->fields->addField_string(_RP("Format"), str);
		} else {
			d->fields->addField_string(_RP("Format"), rom_format);
		}
	} else {
		d->fields->addField_string(_RP("Format"), rom_format);
	}

	// Display the fields.
	if (!romOK) {
		// Invalid mapper.
		return (int)d->fields->count();
	}

	if (mapper >= 0) {
		char buf[16];
		int len = snprintf(buf, sizeof(buf), "%d", mapper);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		rp_string s_mapper = (len > 0 ? latin1_to_rp_string(buf, len) : _RP(""));
		s_mapper.reserve(64);

		// Look up the mapper name.
		const rp_char *mapper_name = NESMappers::lookup_ines(mapper);
		if (mapper_name) {
			s_mapper += _RP(" - ");
			s_mapper += mapper_name;
		}
		d->fields->addField_string(_RP("Mapper"), s_mapper);
	} else {
		// No mapper. If this isn't TNES,
		// then it's probably an FDS image.
		if (tnes_mapper >= 0) {
			// This has a TNES mapper.
			// It *should* map to an iNES mapper...
			d->fields->addField_string(_RP("Mapper"), _RP("MISSING TNES MAPPING"));
		}
	}

	if (submapper >= 0) {
		// Submapper. (NES 2.0 only)
		// TODO: String version?
		d->fields->addField_string_numeric(_RP("Submapper"),
			submapper, RomFields::FB_DEC);
	}

	if (tnes_mapper >= 0) {
		// TNES mapper.
		// TODO: Look up the name.
		d->fields->addField_string_numeric(_RP("TNES Mapper"),
			tnes_mapper, RomFields::FB_DEC);
	}

	// ROM sizes.
	if (prg_rom_size > 0) {
		d->fields->addField_string(_RP("PRG ROM Size"),
			d->formatBankSizeKB(prg_rom_size));
	}
	if (chr_rom_size > 0) {
		d->fields->addField_string(_RP("CHR ROM Size"),
			d->formatBankSizeKB(chr_rom_size));
	}

	// Check for FDS fields.
	if ((d->romType & NESPrivate::ROM_SYSTEM_MASK) == NESPrivate::ROM_SYSTEM_FDS) {
		char buf[64];
		int len;

		// Game ID.
		// TODO: Check for invalid characters?
		len = snprintf(buf, sizeof(buf), "%s-%.3s",
			(d->header.fds.disk_type == FDS_DTYPE_FSC ? "FSC" : "FMC"),
			d->header.fds.game_id);
		if (len > (int)sizeof(buf))
			len = (int)sizeof(buf);
		d->fields->addField_string(_RP("Game ID"),
			len > 0 ? latin1_to_rp_string(buf, len) : _RP("Unknown"));

		// Publisher.
		// NOTE: Verify that the FDS list matches NintendoPublishers.
		// https://wiki.nesdev.com/w/index.php/Family_Computer_Disk_System#Manufacturer_codes
		const rp_char* publisher =
			NintendoPublishers::lookup_old(d->header.fds.publisher_code);
		d->fields->addField_string(_RP("Publisher"),
			publisher ? publisher : _RP("Unknown"));

		// Revision.
		d->fields->addField_string_numeric(_RP("Revision"),
			d->header.fds.revision, RomFields::FB_DEC, 2);

		// Manufacturing Date.
		int64_t mfr_date = d->fds_bcd_datestamp_to_unix(&d->header.fds.mfr_date);
		d->fields->addField_dateTime(_RP("Manufacturing Date"), mfr_date,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC  // Date only.
		);

		// TODO: Disk Writer fields.
	} else {
		// Add non-FDS fields.
		const rp_char *mirroring = nullptr;
		const rp_char *vs_ppu = nullptr;
		switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
			case NESPrivate::ROM_FORMAT_OLD_INES:
			case NESPrivate::ROM_FORMAT_INES:
			case NESPrivate::ROM_FORMAT_NES2:
				// Mirroring.
				// TODO: Detect mappers that have programmable mirroring.
				// TODO: Also One Screen, e.g. AxROM.
				if (d->header.ines.mapper_lo & INES_F6_MIRROR_FOUR) {
					// Four screens using extra VRAM.
					mirroring = _RP("Four Screens");
				} else {
					// TODO: There should be a "one screen" option...
					if (d->header.ines.mapper_lo & INES_F6_MIRROR_VERT) {
						mirroring = _RP("Vertical");
					} else {
						mirroring = _RP("Horizontal");
					}
				}

				if ((d->romType & (NESPrivate::ROM_FORMAT_MASK | NESPrivate::ROM_SYSTEM_MASK)) ==
				    (NESPrivate::ROM_FORMAT_NES2 | NESPrivate::ROM_SYSTEM_VS))
				{
					// Check the VS. PPU type.
					static const rp_char *vs_ppu_types[16] = {
						_RP("RP2C03B"), _RP("RP2C03G"),
						_RP("RP2C04-0001"), _RP("RP2C04-0002"),
						_RP("RP2C04-0003"), _RP("RP2C04-0004"),
						_RP("RP2C03B"), _RP("RP2C03C"),
						_RP("RP2C05-01"), _RP("RP2C05-02"),
						_RP("RP2C05-03"), _RP("RP2C05-04"),
						_RP("RP2C05-05"), nullptr,
						nullptr, nullptr
					};
					vs_ppu = vs_ppu_types[d->header.ines.nes2.vs_hw & 0x0F];

					// TODO: VS. copy protection hardware?
				}
				break;

			case NESPrivate::ROM_FORMAT_TNES:
				// Mirroring.
				switch (d->header.tnes.mirroring) {
					case TNES_MIRRORING_PROGRAMMABLE:
						// For all mappers except AxROM, this is programmable.
						// For AxROM, this is One Screen.
						if (tnes_mapper == TNES_MAPPER_AxROM) {
							mirroring = _RP("One Screen");
						} else {
							mirroring = _RP("Programmable");
						}
						break;
					case TNES_MIRRORING_HORIZONTAL:
						mirroring = _RP("Horizontal");
						break;
					case TNES_MIRRORING_VERTICAL:
						mirroring = _RP("Vertical");
						break;
					default:
						mirroring = _RP("Unknown");
						break;
				}
				break;

			default:
				break;
		}

		if (mirroring) {
			d->fields->addField_string(_RP("Mirroring"), mirroring);
		}
		if (vs_ppu) {
			d->fields->addField_string(_RP("VS. PPU"), vs_ppu);
		}
	}

	// TODO: More fields.
	return (int)d->fields->count();
}

}

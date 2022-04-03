/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NES.cpp: Nintendo Entertainment System/Famicom ROM reader.              *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NES.hpp"
#include "data/NintendoPublishers.hpp"
#include "data/NESMappers.hpp"
#include "nes_structs.h"

// librpbase, librpfile
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using LibRpFile::IRpFile;

// C++ STL classes.
using std::string;
using std::vector;

namespace LibRomData {

class NESPrivate final : public RomDataPrivate
{
	public:
		NESPrivate(NES *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(NESPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

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

			ROM_SYSTEM_NES = (0U << 8),	// NES / Famicom
			ROM_SYSTEM_FDS = (1U << 8),	// Famicom Disk System
			ROM_SYSTEM_VS = (2U << 8),	// VS. System
			ROM_SYSTEM_PC10 = (3U << 8),	// PlayChoice-10
			ROM_SYSTEM_UNKNOWN = (0xFFU << 8),
			ROM_SYSTEM_MASK = (0xFFU << 8),

			// Special flags. (bitfield)
			ROM_SPECIAL_WIIU_VC = (1U << 16),	// Wii U VC (modified iNES)
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

		// ROM footer. (optional)
		NES_IntFooter footer;
		bool hasCheckedIntFooter;	// True if we already checked.
		uint8_t intFooterErrno;		// If checked: 0 if valid, positive errno on error.
		string s_footerName;		// Name from the footer.

		/**
		 * Convert an FDS BCD datestamp to Unix time.
		 * @param fds_bcd_ds FDS BCD datestamp.
		 * @return Unix time, or -1 if an error occurred.
		 *
		 * NOTE: -1 is a valid Unix timestamp (1970/01/01), but is
		 * not likely to be valid for NES/Famicom, since the Famicom
		 * was released in 1983.
		 */
		static time_t fds_bcd_datestamp_to_unix_time(const FDS_BCD_DateStamp *fds_bcd_ds);

		/**
		 * Load the internal footer.
		 * This is present at the end of the last PRG bank in some ROMs.
		 *
		 * NOTE: The NES header must have already been loaded, since we
		 * need to know how many PRG ROM banks are present.
		 *
		 * @return 0 if found; negative POSIX error code on error.
		 */
		int loadInternalFooter(void);
};

ROMDATA_IMPL(NES)

/** NESPrivate **/

/* RomDataInfo */
const char *const NESPrivate::exts[] = {
	// NOTE: .fds is missing block checksums.
	// .qd has block checksums, as does .tds (which is basically
	// a 16-byte header, FDS BIOS, and a .qd file).

	// This isn't too important right now because we're only
	// reading the header, but we'll need to take it into
	// account if file access is added.

	".nes",	// iNES
	".nez",	// Compressed iNES?
	".fds",	// Famicom Disk System
	".qd",	// FDS (Animal Crossing)
	".tds",	// FDS (3DS Virtual Console)

	nullptr
};
const char *const NESPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-nes-rom",
	"application/x-fds-disk",

	nullptr
};
const RomDataInfo NESPrivate::romDataInfo = {
	"NES", exts, mimeTypes
};

NESPrivate::NESPrivate(NES *q, IRpFile *file)
	: super(q, file, &romDataInfo)
	, romType(ROM_UNKNOWN)
	, hasCheckedIntFooter(false)
	, intFooterErrno(255)
{
	// Clear the ROM header structs.
	memset(&header, 0, sizeof(header));
	memset(&footer, 0, sizeof(footer));
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
time_t NESPrivate::fds_bcd_datestamp_to_unix_time(const FDS_BCD_DateStamp *fds_bcd_ds)
{
	// Convert the FDS time to Unix time.
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

	struct tm fdstime;

	// TODO: Check for invalid BCD values.
	fdstime.tm_year = ((fds_bcd_ds->year >> 4) * 10) +
			   (fds_bcd_ds->year & 0x0F) + 1925 - 1900;
	fdstime.tm_mon  = ((fds_bcd_ds->mon >> 4) * 10) +
			   (fds_bcd_ds->mon & 0x0F) - 1;
	fdstime.tm_mday = ((fds_bcd_ds->mday >> 4) * 10) +
			   (fds_bcd_ds->mday & 0x0F);

	// Time portion is empty.
	fdstime.tm_hour = 0;
	fdstime.tm_min = 0;
	fdstime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	fdstime.tm_wday = 0;
	fdstime.tm_yday = 0;
	fdstime.tm_isdst = 0;

	// If conversion fails, d->ctime will be set to -1.
	return timegm(&fdstime);
}

/**
 * Load the internal NES footer.
 * This is present at the end of the last PRG bank in some ROMs.
 *
 * NOTE: The NES header must have already been loaded, since we
 * need to know how many PRG ROM banks are present.
 *
 * @return 0 if found; negative POSIX error code on error.
 */
int NESPrivate::loadInternalFooter(void)
{
	if (hasCheckedIntFooter) {
		return intFooterErrno;
	}
	hasCheckedIntFooter = true;
	intFooterErrno = EIO;

	// Get the PRG ROM size.
	unsigned int prg_rom_size;
	switch (romType & NESPrivate::ROM_FORMAT_MASK) {
		case ROM_FORMAT_OLD_INES:
		case ROM_FORMAT_INES:
			prg_rom_size = header.ines.prg_banks * INES_PRG_BANK_SIZE;
			break;
		case ROM_FORMAT_NES2:
			prg_rom_size = ((header.ines.prg_banks +
					(header.ines.nes2.prg_banks_hi << 8))
					* INES_PRG_BANK_SIZE);
			break;
		case ROM_FORMAT_TNES:
			prg_rom_size = header.tnes.prg_banks * TNES_PRG_BANK_SIZE;
			break;
		default:
			// FDS is not supported here.
			// TODO: ENOTSUP instead?
			intFooterErrno = ENOENT;
			return -((int)intFooterErrno);
	}

	// Check for the internal NES footer.
	// This is located at the last 32 bytes of the last PRG bank in some ROMs.
	// We'll verify that it's valid by looking for an
	// all-ASCII title, possibly right-aligned with
	// 0xFF filler bytes.
	// NOTE: +16 to skip the iNES header.
	const unsigned int addr = prg_rom_size - sizeof(footer) + 16;

	size_t size = file->seekAndRead(addr, &footer, sizeof(footer));
	if (size != sizeof(footer)) {
		// Seek and/or read error.
		// Assume we don't have an internal footer.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}

	// Check the board information value.
	// TODO: Determine valid values.
	// - Bit 7: can be set, indicates ???
	// - Bits 0, 1, 2 control mirroring; mutually-exclusive.
	// FIXME: These are no longer detected:
	// - 0x03: J.League Fighting Soccer - The King of Ace Strikers (Japan)
	// FIXME: These *are* being detected but shouldn't be:
	// - 0x84: Mario Bros. (Europe) (PAL-MA-0)
	// - 0x84: Mario Bros. (World) (GameCube Edition)
	// - 0x84: Mario Bros. (World)

	// Check for special cases.
	switch (footer.board_info) {
		case 0x14:
			// Special case for: Pinball Quest (Australia)
			goto skip_board_info_check;
		case 0x84:
			// Mario Bros. - header is NOT valid.
			// NOTE: Bomberman II (Japan) also has 0x84,
			// so also check for an invalid CHR checksum.
			if (footer.chr_checksum == le16_to_cpu(0x8484)) {
				// Invalid CHR checksum.
				intFooterErrno = ENOENT;
				return intFooterErrno;
			}
			break;
		default:
			break;
	}

	if (footer.board_info & 0x78) {
		// Invalid bits set.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	} else {
		switch (footer.board_info & 0x07) {
			case 0: case 1:
			case 2: case 4:
				// Valid mirroring bits.
				break;
			case 5: {
				// These titles have "00 01 02 03 04 05 06 07":
				// - Higemaru - Makai-jima - Nanatsu no Shima Daibouken (Japan)
				// - Makai Island (USA) (Proto)
				static const uint8_t check_05[] = {0,1,2,3,4,5,6,7};
				const uint8_t *const u8 = reinterpret_cast<const uint8_t*>(&footer);
				if (memcmp(&u8[0x10], check_05, sizeof(check_05)) != 0) {
					// Not valid.
					intFooterErrno = ENOENT;
					return intFooterErrno;
				}
				break;
			}
			default:
				// Not valid mirroring bits.
				intFooterErrno = ENOENT;
				return intFooterErrno;
		}
	}

skip_board_info_check:
	// Check if the name looks right.
	unsigned int firstNonFF = static_cast<unsigned int>(sizeof(footer.name));
	unsigned int lastValidChar = static_cast<unsigned int>(sizeof(footer.name)) - 1;
	bool foundNonFF = false;
	bool foundInvalid = false;
	for (unsigned int i = 0; i < static_cast<unsigned int>(sizeof(footer.name)); i++) {
		uint8_t chr = static_cast<uint8_t>(footer.name[i]);

		// Skip leading NULL and 0xFF.
		// End at trailing NULL and 0xFF.
		// TODO:
		// - Adventures of Gilligan's Island, The (USA): Valid header; name is all 0x20.
		// - Akagawa Jirou no Yuurei Ressha (Japan): Name may have Shift-JIS.
		if (chr == 0U || chr == 0xFFU) {
			if (!foundNonFF) {
				// Leading 0xFF. Skip it.
				continue;
			} else {
				// Trailing 0xFF. End of string.
				if (i == 0) {
					// Shouldn't happen...
					foundInvalid = true;
					break;
				}
				lastValidChar = i - 1;
				break;
			}
		}
		// Also skip leading spaces.
		if (!foundNonFF && chr == ' ') {
			continue;
		}

		if (!foundNonFF) {
			foundNonFF = true;
			firstNonFF = i;
		}
		if (chr < 32 || chr >= 127) {
			// Invalid character.
			foundInvalid = true;
			break;
		}
	}

	if (!foundInvalid &&
	    firstNonFF < static_cast<unsigned int>(sizeof(footer.name)) &&
	    lastValidChar > firstNonFF && lastValidChar < static_cast<unsigned int>(sizeof(footer.name)))
	{
		// Name looks valid.
		s_footerName = latin1_to_utf8(&footer.name[firstNonFF], lastValidChar - firstNonFF + 1);
		intFooterErrno = 0;
	} else {
		// Does not appear to be a valid footer.
		intFooterErrno = ENOENT;
	}
	return -((int)intFooterErrno);
}

/** NES **/

/**
 * Read an NES ROM.
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
NES::NES(IRpFile *file)
	: super(new NESPrivate(this, file))
{
	RP_D(NES);

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}
	
	// Seek to the beginning of the header.
	d->file->rewind();

	// Read the ROM header. [128 bytes]
	// NOTE: Allowing smaller headers for certain types.
	uint8_t header[128];
	size_t size = d->file->read(header, sizeof(header));
	if (size < 16) {
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

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
			d->fileType = FileType::ROM_Image;
			d->mimeType = "application/x-nes-rom";	// unofficial
			memcpy(&d->header.ines, header, sizeof(d->header.ines));
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			// TNES ROM header.
			d->fileType = FileType::ROM_Image;
			d->mimeType = "application/x-nes-rom";	// unofficial
			memcpy(&d->header.tnes, header, sizeof(d->header.tnes));
			break;

		case NESPrivate::ROM_FORMAT_FDS:
			// FDS disk image.
			if (size < sizeof(FDS_DiskHeader)) {
				// Not enough data for an FDS image.
				UNREF_AND_NULL_NOCHK(d->file);
				return;
			}
			d->fileType = FileType::DiskImage;
			d->mimeType = "application/x-fds-disk";	// unofficial
			memcpy(&d->header.fds, header, sizeof(d->header.fds));
			break;

		case NESPrivate::ROM_FORMAT_FDS_FWNES:
			// FDS disk image, with fwNES header.
			if (size < (sizeof(FDS_DiskHeader) + sizeof(FDS_DiskHeader_fwNES))) {
				// Not enough data for an FDS image with fwNES header.
				UNREF_AND_NULL_NOCHK(d->file);
				return;
			}
			d->fileType = FileType::DiskImage;
			d->mimeType = "application/x-fds-disk";	// unofficial
			memcpy(&d->header.fds_fwNES, header, sizeof(d->header.fds_fwNES));
			memcpy(&d->header.fds, &header[16], sizeof(d->header.fds));
			break;

		case NESPrivate::ROM_FORMAT_FDS_TNES: {
			// FDS disk image. (TNES/TDS format)
			size_t szret = d->file->seekAndRead(0x2010, &d->header.fds, sizeof(d->header.fds));
			if (szret != sizeof(d->header.fds)) {
				// Seek and/or read error.
				UNREF_AND_NULL_NOCHK(d->file);
				d->fileType = FileType::Unknown;
				d->romType = NESPrivate::ROM_FORMAT_UNKNOWN;
				return;
			}
			d->fileType = FileType::DiskImage;
			d->mimeType = "application/x-fds-disk";	// unofficial
			break;
		}

		default:
			// Unknown ROM type.
			UNREF_AND_NULL_NOCHK(d->file);
			d->fileType = FileType::Unknown;
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
	if (inesHeader->magic == cpu_to_be32(INES_MAGIC) ||
	    inesHeader->magic == cpu_to_be32(INES_MAGIC_WIIU_VC))
	{
		// Found an iNES ROM header.
		// If the fourth byte is 0x00, it's Wii U VC.
		int romType = (inesHeader->magic == cpu_to_be32(INES_MAGIC_WIIU_VC))
				? NESPrivate::ROM_SPECIAL_WIIU_VC
				: 0;

		// Check for NES 2.0.
		if ((inesHeader->mapper_hi & 0x0C) == 0x08) {
			// May be NES 2.0
			// Verify the ROM size.
			const off64_t size = sizeof(INES_RomHeader) +
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
						// TODO: Handle Extended Console Type?
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
	if (tnesHeader->magic == cpu_to_be32(TNES_MAGIC)) {
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
	// TODO: Check that the block code is 0x01?
	static const uint8_t fds_magic[] = "*NINTENDO-HVC*";

	// Check for headered FDS.
	const FDS_DiskHeader_fwNES *fwNESHeader =
		reinterpret_cast<const FDS_DiskHeader_fwNES*>(info->header.pData);
	if (fwNESHeader->magic == cpu_to_be32(fwNES_MAGIC)) {
		// fwNES header is present.
		// TODO: Check required NULL bytes.
		// For now, assume this is correct.
		const FDS_DiskHeader *fdsHeader =
			reinterpret_cast<const FDS_DiskHeader*>(&info->header.pData[16]);
		if (fdsHeader->block_code == 0x01 &&
		    !memcmp(fdsHeader->magic, fds_magic, sizeof(fdsHeader->magic)-1))
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
		    !memcmp(fdsHeader->magic, fds_magic, sizeof(fdsHeader->magic)-1))
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
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *NES::systemName(unsigned int type) const
{
	RP_D(const NES);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Regional variant, e.g. Famicom.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"NES::systemName() array index optimization needs to be updated.");

	const unsigned int idx = (type & SYSNAME_TYPE_MASK);
	switch (d->romType & NESPrivate::ROM_SYSTEM_MASK) {
		case NESPrivate::ROM_SYSTEM_NES:
		default: {
			static const char *const sysNames_NES[3][4] = {
				// NES (International)
				{"Nintendo Entertainment System",
				 "Nintendo Entertainment System",
				 "NES", nullptr},

				// Famicom (Japan)
				{"Nintendo Famicom",
				 "Famicom",
				 "FC", nullptr},

				// Hyundai Comboy (South Korea)
				{"Hyundai Comboy",
				 "Comboy",
				 "CB", nullptr},
			};

			if ((type & SYSNAME_REGION_MASK) == SYSNAME_REGION_GENERIC) {
				// Use the international name.
				return sysNames_NES[0][idx];
			}

			// Get the system region.
			switch (SystemRegion::getCountryCode()) {
				case 'JP':
					return sysNames_NES[1][idx];
				case 'KR':
					return sysNames_NES[2][idx];
				default:
					return sysNames_NES[0][idx];
			}
		}

		case NESPrivate::ROM_SYSTEM_FDS: {
			static const char *const sysNames_FDS[] = {
				"Nintendo Famicom Disk System",
				"Famicom Disk System",
				"FDS", nullptr
			};
			return sysNames_FDS[idx];
		}

		case NESPrivate::ROM_SYSTEM_VS: {
			static const char *const sysNames_VS[] = {
				"Nintendo VS. System",
				"VS. System",
				"VS", nullptr
			};
			return sysNames_VS[idx];
		}

		case NESPrivate::ROM_SYSTEM_PC10: {
			static const char *const sysNames_PC10[] = {
				"Nintendo PlayChoice-10",
				"PlayChoice-10",
				"PC10", nullptr
			};
			return sysNames_PC10[idx];
		}
	};

	// Should not get here...
	return nullptr;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int NES::loadFieldData(void)
{
	RP_D(NES);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file || !d->file->isOpen()) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// ROM image isn't valid.
		return -EIO;
	}

	// NES ROM header:
	// - 15 regular fields. (iNES, NES 2.0, FDS)
	// - 4 more fields for NES 2.0.
	// - 8 fields for the internal NES header.
	d->fields->reserve(15+4+8);

	// Reserve at least 2 tabs:
	// iNES, Internal Footer
	d->fields->reserveTabs(2);

	// Determine stuff based on the ROM format.
	// Default tab name and ROM format name is "iNES", since it's the most common.
	const char *tabName = "iNES";
	const char *rom_format = "iNES";
	bool romOK = true;
	int mapper = -1;
	int submapper = -1;
	int tnes_mapper = -1;
	bool has_trainer = false;
	uint8_t tv_mode = 0xFF;			// NES2_TV_Mode (0xFF if unknown)
	unsigned int prg_rom_size = 0;
	unsigned int chr_rom_size = 0;
	unsigned int chr_ram_size = 0;		// CHR RAM
	unsigned int chr_ram_battery_size = 0;	// CHR RAM with battery (rare but it exists)
	unsigned int prg_ram_size = 0;		// PRG RAM ($6000-$7FFF)
	unsigned int prg_ram_battery_size = 0;	// PRG RAM with battery (save RAM)

	switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
			rom_format = C_("NES|Format", "Archaic iNES");
			mapper = (d->header.ines.mapper_lo >> 4);
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			if (chr_rom_size == 0) {
				chr_ram_size = 8192;
			}
			if (d->header.ines.mapper_lo & INES_F6_BATTERY) {
				prg_ram_battery_size = 8192;
			}
			break;

		case NESPrivate::ROM_FORMAT_INES:
			mapper = (d->header.ines.mapper_lo >> 4) |
				 (d->header.ines.mapper_hi & 0xF0);
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			// NOTE: Very few iNES ROMs have this set correctly,
			// so we're ignoring it for now.
			//tv_mode = (d->header.ines.ines.tv_mode & 1);
			prg_rom_size = d->header.ines.prg_banks * INES_PRG_BANK_SIZE;
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			if (chr_rom_size == 0) {
				chr_ram_size = 8192;
			}
			if (d->header.ines.mapper_lo & INES_F6_BATTERY) {
				if (d->header.ines.ines.prg_ram_size == 0) {
					prg_ram_battery_size = 8192;
				} else {
					prg_ram_battery_size = d->header.ines.ines.prg_ram_size * INES_PRG_RAM_BANK_SIZE;
				}
			} else {
				// FIXME: Is this correct?
				if (d->header.ines.ines.prg_ram_size > 0) {
					prg_ram_size = d->header.ines.ines.prg_ram_size * INES_PRG_RAM_BANK_SIZE;
				}
			}
			break;

		case NESPrivate::ROM_FORMAT_NES2:
			tabName = "NES 2.0";	// NOT translatable!
			rom_format = "NES 2.0";	// NOT translatable!
			mapper = (d->header.ines.mapper_lo >> 4) |
				 (d->header.ines.mapper_hi & 0xF0) |
				 ((d->header.ines.nes2.mapper_hi2 & 0x0F) << 8);
			submapper = (d->header.ines.nes2.mapper_hi2 >> 4);
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			tv_mode = (d->header.ines.nes2.tv_mode & 3);
			prg_rom_size = ((d->header.ines.prg_banks +
					(d->header.ines.nes2.prg_banks_hi << 8))
					* INES_PRG_BANK_SIZE);
			chr_rom_size = d->header.ines.chr_banks * INES_CHR_BANK_SIZE;
			// CHR RAM size. (TODO: Needs testing.)
			if (d->header.ines.nes2.vram_size & 0x0F) {
				chr_ram_size = 128 << ((d->header.ines.nes2.vram_size & 0x0F) - 1);
			}
			if ((d->header.ines.mapper_lo & INES_F6_BATTERY) &&
			    (d->header.ines.nes2.vram_size & 0xF0))
			{
				chr_ram_battery_size = 128 << ((d->header.ines.nes2.vram_size >> 4) - 1);
			}
			// PRG RAM size. (TODO: Needs testing.)
			if (d->header.ines.nes2.prg_ram_size & 0x0F) {
				prg_ram_size = 128 << ((d->header.ines.nes2.prg_ram_size >> 4) - 1);
			}
			// TODO: Require INES_F6_BATTERY?
			if (d->header.ines.nes2.prg_ram_size & 0xF0) {
				prg_ram_battery_size = 128 << ((d->header.ines.nes2.prg_ram_size >> 4) - 1);
			}
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			tabName = "TNES";	// NOT translatable!
			rom_format = C_("NES|Format", "TNES (Nintendo 3DS Virtual Console)");
			tnes_mapper = d->header.tnes.mapper;
			mapper = NESMappers::tnesMapperToInesMapper(tnes_mapper);
			prg_rom_size = d->header.tnes.prg_banks * TNES_PRG_BANK_SIZE;
			chr_rom_size = d->header.tnes.chr_banks * TNES_CHR_BANK_SIZE;
			// FIXME: Check Zelda TNES to see where 8K CHR RAM is.
			break;

		// NOTE: FDS fields are handled later.
		// We're just obtaining the ROM format name here.
		case NESPrivate::ROM_FORMAT_FDS:
			tabName = "FDS";	// NOT translatable!
			rom_format = C_("NES|Format", "FDS disk image");
			break;
		case NESPrivate::ROM_FORMAT_FDS_FWNES:
			tabName = "FDS";	// NOT translatable!
			rom_format = C_("NES|Format", "FDS disk image (with fwNES header)");
			break;
		case NESPrivate::ROM_FORMAT_FDS_TNES:
			tabName = "FDS";	// NOT translatable!
			rom_format = C_("NES|Format", "TDS (Nintendo 3DS Virtual Console)");
			break;

		default:
			tabName = "NES";	// NOT translatable!
			rom_format = C_("RomData", "Unknown");
			romOK = false;
			break;
	}
	d->fields->setTabName(0, tabName);

	const char *const format_title = C_("NES", "Format");
	if (d->romType & NESPrivate::ROM_SPECIAL_WIIU_VC) {
		// Wii U Virtual Console.
		const int romFormat = (d->romType & NESPrivate::ROM_FORMAT_MASK);
		assert(romFormat >= NESPrivate::ROM_FORMAT_OLD_INES);
		assert(romFormat <= NESPrivate::ROM_FORMAT_NES2);
		if (romFormat >= NESPrivate::ROM_FORMAT_OLD_INES &&
		    romFormat <= NESPrivate::ROM_FORMAT_NES2)
		{
			d->fields->addField_string(format_title,
				// tr: ROM format, e.g. iNES or FDS disk image.
				rp_sprintf(C_("NES|Format", "%s (Wii U Virtual Console)"), rom_format));
		} else {
			d->fields->addField_string(format_title, rom_format);
		}
	} else {
		d->fields->addField_string(format_title, rom_format);
	}

	// Display the fields.
	if (!romOK) {
		// Invalid mapper.
		return static_cast<int>(d->fields->count());
	}

	const char *const mapper_title = C_("NES", "Mapper");
	if (mapper >= 0) {
		// Look up the mapper name.
		string s_mapper;

		const char *const mapper_name = NESMappers::lookup_ines(mapper);
		if (mapper_name) {
			// tr: Print the mapper ID followed by the mapper name.
			s_mapper = rp_sprintf_p(C_("NES|Mapper", "%1$u - %2$s"),
				static_cast<unsigned int>(mapper), mapper_name);
		} else {
			// tr: Print only the mapper ID.
			s_mapper = rp_sprintf("%u", static_cast<unsigned int>(mapper));
		}
		d->fields->addField_string(mapper_title, s_mapper);
	} else {
		// No mapper. If this isn't TNES,
		// then it's probably an FDS image.
		if (tnes_mapper >= 0) {
			// This has a TNES mapper.
			// It *should* map to an iNES mapper...
			d->fields->addField_string(mapper_title,
				C_("NES", "[Missing TNES mapping!]"));
		}
	}

	if (submapper >= 0) {
		// Submapper. (NES 2.0 only)
		string s_submapper;

		// Look up the submapper name.
		// TODO: Needs testing.
		const char *const submapper_name = NESMappers::lookup_nes2_submapper(mapper, submapper);
		if (submapper_name) {
			// tr: Print the submapper ID followed by the submapper name.
			s_submapper = rp_sprintf_p(C_("NES|Mapper", "%1$u - %2$s"),
				static_cast<unsigned int>(submapper), submapper_name);
		} else {
			// tr: Print only the submapper ID.
			s_submapper = rp_sprintf("%u", static_cast<unsigned int>(submapper));
		}
		d->fields->addField_string(C_("NES", "Submapper"), s_submapper);
	}

	if (tnes_mapper >= 0) {
		// TNES mapper.
		// TODO: Look up the name.
		d->fields->addField_string_numeric(C_("NES", "TNES Mapper"),
			tnes_mapper, RomFields::Base::Dec);
	}

	// TV mode
	static const char *const tv_mode_tbl[] = {
		"NTSC (RP2C02)",
		"PAL (RP2C07)",
		NOP_C_("NES|TVMode", "Dual (NTSC/PAL)"),
		"Dendy (UMC 6527P)",
	};
	if (tv_mode < ARRAY_SIZE(tv_mode_tbl)) {
		d->fields->addField_string(C_("NES", "TV Mode"),
			dpgettext_expr(RP_I18N_DOMAIN, "NES|TVMode", tv_mode_tbl[tv_mode]));
	}

	// ROM features
	const char *rom_features = nullptr;
	if (prg_ram_battery_size > 0 && has_trainer) {
		rom_features = C_("NES|Features", "Save RAM, Trainer");
	} else if (prg_ram_battery_size > 0 && !has_trainer) {
		rom_features = C_("NES|Features", "Save RAM");
	} else if (prg_ram_battery_size == 0 && has_trainer) {
		rom_features = C_("NES|Features", "Trainer");
	}
	if (rom_features) {
		d->fields->addField_string(C_("NES", "Features"), rom_features);
	}

	// ROM sizes
	if (prg_rom_size > 0) {
		d->fields->addField_string(C_("NES", "PRG ROM Size"),
			formatFileSizeKiB(prg_rom_size));
	}
	if (chr_rom_size > 0) {
		d->fields->addField_string(C_("NES", "CHR ROM Size"),
			formatFileSizeKiB(chr_rom_size));
	}

	// RAM sizes
	if (chr_ram_size > 0) {
		d->fields->addField_string(C_("NES", "CHR RAM Size"),
			formatFileSizeKiB(chr_ram_size));
	}
	if (chr_ram_battery_size > 0) {
		// tr: CHR RAM with a battery backup.
		d->fields->addField_string(C_("NES", "CHR RAM (backed up)"),
			formatFileSizeKiB(chr_ram_battery_size));
	}
	if (prg_ram_size > 0) {
		d->fields->addField_string(C_("NES", "PRG RAM Size"),
			formatFileSizeKiB(prg_ram_size));
	}
	if (prg_ram_battery_size > 0) {
		// tr: Save RAM with a battery backup.
		d->fields->addField_string(C_("NES", "Save RAM (backed up)"),
			formatFileSizeKiB(prg_ram_battery_size));
	}

	// Check for FDS fields.
	if ((d->romType & NESPrivate::ROM_SYSTEM_MASK) == NESPrivate::ROM_SYSTEM_FDS) {
		// Game ID
		// TODO: Check for invalid characters?
		d->fields->addField_string(C_("RomData", "Game ID"),
			rp_sprintf("%s-%.3s",
				(d->header.fds.disk_type == FDS_DTYPE_FSC ? "FSC" : "FMC"),
				d->header.fds.game_id));

		// Publisher
		const char *const publisher_title = C_("RomData", "Publisher");
		const char *const publisher =
			NintendoPublishers::lookup_fds(d->header.fds.publisher_code);
		if (publisher) {
			d->fields->addField_string(publisher_title, publisher);
		} else {
			d->fields->addField_string(publisher_title,
				rp_sprintf(C_("RomData", "Unknown (0x%02X)"), d->header.fds.publisher_code));
		}

		// Revision
		d->fields->addField_string_numeric(C_("RomData", "Revision"),
			d->header.fds.revision, RomFields::Base::Dec, 2);

		// Manufacturing Date.
		time_t mfr_date = d->fds_bcd_datestamp_to_unix_time(&d->header.fds.mfr_date);
		d->fields->addField_dateTime(C_("NES", "Manufacturing Date"), mfr_date,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC  // Date only.
		);

		// TODO: Disk Writer fields.
	} else {
		// Add non-FDS fields.
		const char *s_mirroring = nullptr;
		const char *s_vs_ppu = nullptr;
		const char *s_vs_hw = nullptr;
		const char *s_extd_ct = nullptr;
		const char *s_exp_hw = nullptr;
		unsigned int misc_roms = 0;
		switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
			case NESPrivate::ROM_FORMAT_OLD_INES:
			case NESPrivate::ROM_FORMAT_INES:
			case NESPrivate::ROM_FORMAT_NES2:
				// Mirroring
				s_mirroring = NESMappers::lookup_ines_mirroring(mapper, submapper == -1 ? 0 : submapper,
					d->header.ines.mapper_lo & INES_F6_MIRROR_VERT,
					d->header.ines.mapper_lo & INES_F6_MIRROR_FOUR);

				// Check for NES 2.0 extended console types, including VS hardware.
				if ((d->romType & (NESPrivate::ROM_SYSTEM_MASK | NESPrivate::ROM_FORMAT_MASK)) ==
				    (NESPrivate::ROM_FORMAT_NES2 | NESPrivate::ROM_SYSTEM_VS))
				{
					// Check the Vs. PPU type.
					// NOTE: Not translatable!
					static const char vs_ppu_types[][12] = {
						"RP2C03B",     "RP2C03G",
						"RP2C04-0001", "RP2C04-0002",
						"RP2C04-0003", "RP2C04-0004",
						"RP2C03B",     "RP2C03C",
						"RP2C05-01",   "RP2C05-02",
						"RP2C05-03",   "RP2C05-04",
						"RP2C05-05"
					};
					const unsigned int vs_ppu = (d->header.ines.nes2.vs_hw & 0x0F);
					if (vs_ppu < ARRAY_SIZE(vs_ppu_types)) {
						s_vs_ppu = vs_ppu_types[vs_ppu];
					}

					// Check the Vs. hardware type.
					// NOTE: Not translatable!
					static const char *const vs_hw_types[] = {
						"Vs. Unisystem",
						"Vs. Unisystem (RBI Baseball)",
						"Vs. Unisystem (TKO Boxing)",
						"Vs. Unisystem (Super Xevious)",
						"Vs. Unisystem (Vs. Ice Climber Japan)",
						"Vs. Dualsystem",
						"Vs. Dualsystem (Raid on Bungeling Bay)",
					};
					const unsigned int vs_hw = (d->header.ines.nes2.vs_hw >> 4);
					if (vs_hw < ARRAY_SIZE(vs_hw_types)) {
						s_vs_hw = vs_hw_types[vs_hw];
					}
				}

				// Other NES 2.0 fields.
				if ((d->romType & NESPrivate::ROM_FORMAT_MASK) == NESPrivate::ROM_FORMAT_NES2) {
					if ((d->header.ines.mapper_hi & INES_F7_SYSTEM_MASK) == INES_F7_SYSTEM_EXTD) {
						// NES 2.0 Extended Console Type
						static const char *const ext_hw_types[] = {
							"NES/Famicom/Dendy",	// Not normally used.
							"Nintendo Vs. System",	// Not normally used.
							"PlayChoice-10",	// Not normally used.
							"Famiclone with BCD support",
							"V.R. Technology VT01 with monochrome palette",
							"V.R. Technology VT01 with red/cyan STN palette",
							"V.R. Technology VT02",
							"V.R. Technology VT03",
							"V.R. Technology VT09",
							"V.R. Technology VT32",
							"V.R. Technology VT369",
							"UMC UM6578",
						};

						const unsigned int extd_ct = (d->header.ines.nes2.vs_hw & 0x0F);
						if (extd_ct < ARRAY_SIZE(ext_hw_types)) {
							s_extd_ct = ext_hw_types[extd_ct];
						}
					}

					// Miscellaneous ROM count.
					misc_roms = d->header.ines.nes2.misc_roms & 3;

					// Default expansion hardware.
					static const char *const exp_hw_tbl[] = {
						// 0x00
						NOP_C_("NES|Expansion", "Unspecified"),
						NOP_C_("NES|Expansion", "NES/Famicom Controllers"),
						NOP_C_("NES|Expansion", "NES Four Score / Satellite"),
						NOP_C_("NES|Expansion", "Famicom Four Players Adapter"),
						NOP_C_("NES|Expansion", "Vs. System"),
						NOP_C_("NES|Expansion", "Vs. System (reversed inputs)"),
						NOP_C_("NES|Expansion", "Vs. Pinball (Japan)"),
						NOP_C_("NES|Expansion", "Vs. Zapper"),
						NOP_C_("NES|Expansion", "Zapper"),
						NOP_C_("NES|Expansion", "Two Zappers"),
						NOP_C_("NES|Expansion", "Bandai Hyper Shot"),
						NOP_C_("NES|Expansion", "Power Pad (Side A)"),
						NOP_C_("NES|Expansion", "Power Pad (Side B)"),
						NOP_C_("NES|Expansion", "Family Trainer (Side A)"),
						NOP_C_("NES|Expansion", "Family Trainer (Side B)"),
						NOP_C_("NES|Expansion", "Arkanoid Vaus Controller (NES)"),

						// 0x10
						NOP_C_("NES|Expansion", "Arkanoid Vaus Controller (Famicom)"),
						NOP_C_("NES|Expansion", "Two Vaus Controllers plus Famicom Data Recorder"),
						NOP_C_("NES|Expansion", "Konami Hyper Shot"),
						NOP_C_("NES|Expansion", "Coconuts Pachinko Controller"),
						NOP_C_("NES|Expansion", "Exciting Boxing Punching Bag"),
						NOP_C_("NES|Expansion", "Jissen Mahjong Controller"),
						NOP_C_("NES|Expansion", "Party Tap"),
						NOP_C_("NES|Expansion", "Oeka Kids Tablet"),
						NOP_C_("NES|Expansion", "Sunsoft Barcode Battler"),
						NOP_C_("NES|Expansion", "Miracle Piano Keyboard"),
						NOP_C_("NES|Expansion", "Pokkun Moguraa"),
						NOP_C_("NES|Expansion", "Top Rider"),
						NOP_C_("NES|Expansion", "Double-Fisted"),
						NOP_C_("NES|Expansion", "Famicom 3D System"),
						NOP_C_("NES|Expansion", "Doremikko Keyboard"),
						NOP_C_("NES|Expansion", "R.O.B. Gyro Set"),

						// 0x20
						NOP_C_("NES|Expansion", "Famicom Data Recorder (no keyboard)"),
						NOP_C_("NES|Expansion", "ASCII Turbo File"),
						NOP_C_("NES|Expansion", "IGS Storage Battle Box"),
						NOP_C_("NES|Expansion", "Family BASIC Keyboard plus Famicom Data Recorder"),
						NOP_C_("NES|Expansion", "Dongda PEC-586 Keyboard"),
						NOP_C_("NES|Expansion", "Bit Corp. Bit-79 Keyboard"),
						NOP_C_("NES|Expansion", "Subor Keyboard"),
						NOP_C_("NES|Expansion", "Subor Keyboard plus mouse (3x8-bit protocol)"),
						NOP_C_("NES|Expansion", "Subor Keyboard plus mouse (24-bit protocol)"),
						NOP_C_("NES|Expansion", "SNES Mouse"),
						NOP_C_("NES|Expansion", "Multicart"),
						NOP_C_("NES|Expansion", "SNES Controllers"),
						NOP_C_("NES|Expansion", "RacerMate Bicycle"),
						NOP_C_("NES|Expansion", "U-Force"),
						NOP_C_("NES|Expansion", "R.O.B. Stack-Up"),
						NOP_C_("NES|Expansion", "City Patrolman Lightgun"),
					};

					const unsigned int exp_hw = (d->header.ines.nes2.expansion & 0x3F);
					if (exp_hw < ARRAY_SIZE(exp_hw_tbl)) {
						s_exp_hw = dpgettext_expr(RP_I18N_DOMAIN, "NES|Expansion", exp_hw_tbl[exp_hw]);
					}
				}
				break;

			case NESPrivate::ROM_FORMAT_TNES:
				// Mirroring
				switch (d->header.tnes.mirroring) {
					case TNES_MIRRORING_PROGRAMMABLE:
						// For all mappers except AxROM, this is programmable.
						// For AxROM, this is One Screen.
						if (tnes_mapper == TNES_MAPPER_AxROM) {
							s_mirroring = C_("NES|Mirroring", "One Screen");
						} else {
							s_mirroring = C_("NES|Mirroring", "Programmable");
						}
						break;
					case TNES_MIRRORING_HORIZONTAL:
						s_mirroring = C_("NES|Mirroring", "Horizontal");
						break;
					case TNES_MIRRORING_VERTICAL:
						s_mirroring = C_("NES|Mirroring", "Vertical");
						break;
					default:
						s_mirroring = C_("RomData", "Unknown");
						break;
				}
				break;

			default:
				break;
		}

		const char *const mirroring_title = C_("NES", "Mirroring");
		if (s_mirroring) {
			d->fields->addField_string(mirroring_title, s_mirroring);
		}
		if (s_vs_ppu) {
			d->fields->addField_string(C_("NES", "Vs. PPU"), s_vs_ppu);
		}
		if (s_vs_hw) {
			// TODO: Increase the reserved field count?
			d->fields->addField_string(C_("NES", "Vs. Hardware"), s_vs_hw);
		}
		if (s_extd_ct) {
			// TODO: Increase the reserved field count?
			d->fields->addField_string(C_("NES", "Console Type"), s_extd_ct);
		}
		if (misc_roms > 0) {
			// TODO: Increase the reserved field count?
			d->fields->addField_string_numeric(C_("NES", "Misc. ROM Count"), misc_roms);
		}
		if (s_exp_hw) {
			// TODO: Increase the reserved field count?
			d->fields->addField_string(C_("NES", "Default Expansion"), s_exp_hw);
		}

		// Check for the internal footer.
		if (d->loadInternalFooter() == 0) {
			// Found the internal footer.
			d->fields->addTab("Internal Footer");
			const NES_IntFooter &footer = d->footer;

			// Internal name (Assuming ASCII)
			if (!d->s_footerName.empty()) {
				d->fields->addField_string(C_("NES", "Internal Name"),
					d->s_footerName, RomFields::STRF_TRIM_END);
			}

			// PRG checksum
			d->fields->addField_string_numeric(C_("NES", "PRG Checksum"),
				le16_to_cpu(footer.prg_checksum),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// CHR checksum
			d->fields->addField_string_numeric(C_("NES", "CHR Checksum"),
				le16_to_cpu(footer.chr_checksum),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// ROM sizes (as powers of two)
			static const uint8_t sz_shift_lookup[] = {
				0,	// 0
				14,	// 1 (16 KB)
				15,	// 2 (32 KB)
				17,	// 3 (128 KB)
				18,	// 4 (256 KB)
				19,	// 5 (512 KB)
			};

			const uint8_t prg_sz_idx = footer.rom_size >> 4;
			const uint8_t chr_sz_idx = footer.rom_size & 0x0F;
			unsigned int prg_size = 0, chr_size = 0;
			if (prg_sz_idx < ARRAY_SIZE(sz_shift_lookup)) {
				prg_size = (1 << sz_shift_lookup[prg_sz_idx]);
			}
			if (chr_sz_idx < ARRAY_SIZE(sz_shift_lookup)) {
				chr_size = (1 << sz_shift_lookup[chr_sz_idx]);
			}

			// PRG ROM size
			string s_prg_size;
			if (prg_sz_idx == 0) {
				s_prg_size = C_("NES", "Not set");
			} else if (prg_size > 1) {
				s_prg_size = formatFileSizeKiB(prg_size);
			} else {
				s_prg_size = rp_sprintf(C_("RomData", "Unknown (0x%02X)"), prg_sz_idx);
			}
			d->fields->addField_string(C_("NES", "PRG ROM Size"), s_prg_size);

			// CHR ROM size
			string s_chr_size;
			bool b_chr_ram = false;
			if (chr_sz_idx == 0) {
				s_chr_size = C_("NES", "Not set");
			} else if (chr_size > 1) {
				s_chr_size = formatFileSizeKiB(chr_size);
			} else if (chr_sz_idx == 8) {
				// CHR RAM: 8 KiB
				b_chr_ram = true;
				s_chr_size = formatFileSizeKiB(8192);
			} else {
				s_chr_size = rp_sprintf(C_("RomData", "Unknown (0x%02X)"), chr_sz_idx);
			}
			if (likely(!b_chr_ram)) {
				d->fields->addField_string(C_("NES", "CHR ROM Size"), s_chr_size);
			} else {
				// NOTE: Some ROMs that have CHR ROM indicate CHR RAM in the footer,
				// e.g. Castlevania II - Simon's Quest (E/U) and Contra (J).
				// Other regional versions of these games actually *do* have CHR RAM.
				d->fields->addField_string(C_("NES", "CHR RAM Size"), s_chr_size);
			}

			// Mirroring
			// TODO: This is incorrect; need to figure out the correct values...
			switch (footer.board_info >> 4) {
				case 0:
					s_mirroring = C_("NES|Mirroring", "Horizontal");
					break;
				case 1:
					s_mirroring = C_("NES|Mirroring", "Vertical");
					break;
				default:
					s_mirroring = nullptr;
					break;
			}
			if (s_mirroring) {
				d->fields->addField_string(mirroring_title, s_mirroring);
			} else {
				d->fields->addField_string(mirroring_title,
					rp_sprintf(C_("RomData", "Unknown (0x%02X)"), footer.board_info >> 4));
			}

			// Board type
			// TODO: Lookup table.
			d->fields->addField_string_numeric(C_("NES", "Board Type"),
				footer.board_info & 0x0F);

			// Publisher
			const char *const publisher_title = C_("RomData", "Publisher");
			const char *const publisher =
				NintendoPublishers::lookup_old(footer.publisher_code);
			if (publisher) {
				d->fields->addField_string(publisher_title, publisher);
			} else {
				d->fields->addField_string(publisher_title,
					rp_sprintf(C_("RomData", "Unknown (0x%02X)"), footer.publisher_code));
			}
		}
	}

	// TODO: More fields.
	return static_cast<int>(d->fields->count());
}

}

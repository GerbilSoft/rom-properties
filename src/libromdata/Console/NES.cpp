/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NES.cpp: Nintendo Entertainment System/Famicom ROM reader.              *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
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

// C++ STL classes
using std::string;
using std::u8string;
using std::unique_ptr;
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
		static const char8_t *const exts[];
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
		u8string s_footerName;		// Name from the footer.

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
		 * Get the PRG ROM size.
		 * @return PRG ROM size.
		 */
		unsigned int get_prg_rom_size(void) const;

		/**
		 * Get the CHR ROM size.
		 * @return CHR ROM size.
		 */
		unsigned int get_chr_rom_size(void) const;

		/**
		 * Get the iNES mapper number.
		 * @return iNES mapper number.
		 */
		int get_iNES_mapper_number(void) const;

		// Internal footer: PRG ROM sizes (as powers of two)
		static const uint8_t footer_prg_rom_size_shift_lkup[];

		// Internal footer: CHR ROM/RAM sizes (as powers of two)
		static const uint8_t footer_chr_rom_size_shift_lkup[];

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
const char8_t *const NESPrivate::exts[] = {
	// NOTE: .fds is missing block checksums.
	// .qd has block checksums, as does .tds (which is basically
	// a 16-byte header, FDS BIOS, and a .qd file).

	// This isn't too important right now because we're only
	// reading the header, but we'll need to take it into
	// account if file access is added.

	U8(".nes"),	// iNES
	U8(".nez"),	// Compressed iNES?
	U8(".fds"),	// Famicom Disk System
	U8(".qd"),	// FDS (Animal Crossing)
	U8(".tds"),	// FDS (3DS Virtual Console)

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

// Internal footer: PRG ROM sizes (as powers of two)
const uint8_t NESPrivate::footer_prg_rom_size_shift_lkup[] = {
	16,	// 0 (64 KB)
	14,	// 1 (16 KB)
	15,	// 2 (32 KB)
	17,	// 3 (128 KB)
	18,	// 4 (256 KB)
	19,	// 5 (512 KB)
};

// Internal footer: CHR ROM sizes (as powers of two)
const uint8_t NESPrivate::footer_chr_rom_size_shift_lkup[] = {
	13,	// 0 (8 KB)
	14,	// 1 (16 KB)
	15,	// 2 (32 KB)
	17,	// 3 (128 KB)	// FIXME: May be 64 KB.
	18,	// 4 (256 KB)
	19,	// 5 (512 KB)
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
 * Get the PRG ROM size.
 * @return PRG ROM size.
 */
unsigned int NESPrivate::get_prg_rom_size(void) const
{
	unsigned int prg_rom_size;

	switch (romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
		case NESPrivate::ROM_FORMAT_INES:
			// Special case: If a single PRG bank is present, CHR ROM is
			// present, and the total file size matches a PRG bank,
			// this is likely Galaxian with 8 KB PRG ROM.
			// TODO: Cache the file size in case the file is closed?
			if (header.ines.prg_banks == 1 && header.ines.chr_banks == 1) {
				if (file && file->size() == 16400) {
					return 8192;
				}
			}

			prg_rom_size = header.ines.prg_banks * INES_PRG_BANK_SIZE;
			break;

		case NESPrivate::ROM_FORMAT_NES2: {
			// NES 2.0 has an alternate method for indicating PRG ROM size,
			// so we need to check for that.
			if (likely((header.ines.nes2.banks_hi & 0x0F) != 0x0F)) {
				// Standard method.
				prg_rom_size = ((header.ines.prg_banks |
						((header.ines.nes2.banks_hi & 0x0F) << 8))
						* INES_PRG_BANK_SIZE);
			} else {
				// Alternate method: [EEEE EEMM] -> 2^E * (MM*2 + 1)
				// TODO: Verify that this works.
				prg_rom_size = (1U << (header.ines.prg_banks >> 2)) *
					       ((header.ines.prg_banks & 0x03) + 1);
			}
			break;
		}

		case NESPrivate::ROM_FORMAT_TNES:
			prg_rom_size = header.tnes.prg_banks * TNES_PRG_BANK_SIZE;
			break;

		default:
			// Not supported.
			prg_rom_size = 0;
			break;
	}

	return prg_rom_size;
}

/**
 * Get the PRG ROM size.
 * @return PRG ROM size.
 */
unsigned int NESPrivate::get_chr_rom_size(void) const
{
	unsigned int chr_rom_size;

	switch (romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
		case NESPrivate::ROM_FORMAT_INES:
			chr_rom_size = header.ines.chr_banks * INES_CHR_BANK_SIZE;
			break;

		case NESPrivate::ROM_FORMAT_NES2:
			chr_rom_size = ((header.ines.chr_banks |
					((header.ines.nes2.banks_hi & 0xF0) << 4))
					* INES_CHR_BANK_SIZE);
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			chr_rom_size = header.tnes.chr_banks * TNES_CHR_BANK_SIZE;
			break;

		default:
			// Not supported.
			chr_rom_size = 0;
			break;
	}

	return chr_rom_size;
}

/**
 * Get the iNES mapper number.
 * @return iNES mapper number.
 */
int NESPrivate::get_iNES_mapper_number(void) const
{
	int mapper;

	switch (romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
			mapper = (header.ines.mapper_lo >> 4);
			break;

		case NESPrivate::ROM_FORMAT_INES:
			mapper = (header.ines.mapper_lo >> 4) |
				 (header.ines.mapper_hi & 0xF0);
			break;

		case NESPrivate::ROM_FORMAT_NES2:
			mapper = (header.ines.mapper_lo >> 4) |
				 (header.ines.mapper_hi & 0xF0) |
				 ((header.ines.nes2.mapper_hi2 & 0x0F) << 8);
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			mapper = NESMappers::tnesMapperToInesMapper(header.tnes.mapper);
			break;

		default:
			// Mappers aren't supported.
			mapper = -1;
			break;
	}

	return mapper;
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

	const unsigned int prg_rom_size = get_prg_rom_size();
	if (prg_rom_size == 0) {
		// Unable to get the PRG ROM size.
		// This might be FDS, which isn't supported here.
		// TODO: ENOTSUP instead?
		intFooterErrno = ENOENT;
		return -((int)intFooterErrno);
	}

	// Check for the internal NES footer.
	// This is located at the last 32 bytes of the last PRG bank in some ROMs.
	// We'll verify that it's valid by looking for an
	// all-ASCII title, possibly right-aligned with
	// 0xFF filler bytes.
	// NOTE: +16 to skip the iNES/TNES header.
	const unsigned int addr = prg_rom_size - sizeof(footer) + 16;

	size_t size = file->seekAndRead(addr, &footer, sizeof(footer));
	if (size != sizeof(footer)) {
		// Seek and/or read error.
		// Assume we don't have an internal footer.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}

	// FIXME: This prevents a lot of otherwise valid footers from being displayed...
#if 0
	// Validate the checksum: sum of [FFF2-FFF9] == 0
	uint8_t checksum = std::accumulate(&footer.u8[0x12], &footer.u8[0x1A], 0);
	if (checksum != 0) {
		// Incorrect checksum.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}
#endif

	// We'll allow certain errors if the name is valid.
	bool onlyIfValidName = false;

	// Check for an invalid publisher.
	// NOTE: Some footers have partially valid information even with
	// an invalid publisher, but the info isn't very useful.
	if (footer.publisher_code == 0x00 || footer.publisher_code == 0xFF) {
		// Invalid publisher.
		onlyIfValidName = true;
	}

	// PRG and CHR checksums sometimes aren't set.
	// The ROM size field *must* be valid and match the iNES header.
	const uint8_t prg_sz_idx = footer.rom_size >> 4;
	const uint8_t chr_sz_idx = footer.rom_size & 0x07;
	if (prg_sz_idx >= ARRAY_SIZE(footer_prg_rom_size_shift_lkup) ||
	    chr_sz_idx >= ARRAY_SIZE(footer_chr_rom_size_shift_lkup))
	{
		// Invalid ROM size.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}

	// NOTE: Some ROMs have a footer that indicates half or double
	// the ROM size that's present in the iNES header.
	// (Dragon Quest, some others)
	// For these, we'll only allow the DIV2 version if a valid name is present.
	const unsigned int prg_rom_size_lkup = (1U << footer_prg_rom_size_shift_lkup[prg_sz_idx]);
	if (prg_rom_size == prg_rom_size_lkup) {
		// PRG ROM size is correct.
	} else if ((prg_rom_size / 2) == prg_rom_size_lkup ||
		   (prg_rom_size * 2) == prg_rom_size_lkup) {
		// PRG ROM size is half the actual size.
		onlyIfValidName = true;
	} else {
		// PRG ROM size is incorrect.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}
	// FIXME: Addams Family has an incorrect CHR ROM size. (8 KB; should be 128 KB)
#if 0
	if (!(footer.rom_size & 0x08)) {
		const unsigned int chr_rom_size = get_chr_rom_size();
		if (chr_rom_size != 0 &&
		    chr_rom_size != (1U << footer_chr_rom_size_shift_lkup[chr_sz_idx])) {
			// CHR ROM size is incorrect.
			intFooterErrno = ENOENT;
			return intFooterErrno;
		}
	}
#endif

	// Title encoding value must be valid.
	// NOTE: '100 Man Dollar Kid' has 4 here.
	if (footer.title_encoding > 4) {
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}

	// Get the title.
	// NOTE: Some ROMs have an encoding value of 4.
	// Handling anything that isn't SJIS as if it's ASCII.
	// FIXME: Some have a "none" title encoding even though they have a title?
	if (footer.title_encoding == NES_INTFOOTER_ENCODING_ASCII ||
	    footer.title_encoding == NES_INTFOOTER_ENCODING_SJIS ||
	    footer.title_encoding == 4)
	{
		// Check the title length.
		// TODO: Adjust for off-by-ones?
		if (unlikely(footer.title_length == 0)) {
			// No title.
			s_footerName.clear();
		} else if (footer.title_length <= 16) {
			// Add 1 to get the actual length.
			// (Unless it's 16, in which case, leave it as-is.)
			unsigned int len = footer.title_length;
			if (len < 16)
				len++;
			// NOTE: The spec says the title should be right-aligned,
			// but some games incorrectly left-align it.
			// Also, some titles use '\00' or '\x20' for padding instead of '\xFF'.
			unsigned int start;
			const uint8_t last_chr = (uint8_t)footer.title[0x0F];
			if (likely(last_chr != 0xFF &&
			           last_chr != 0x00 &&
				   last_chr != 0x20))
			{
				// Right-aligned
				start = 16 - len;
			} else {
				// Left-aligned
				start = 0;
			}

			// Trim leading spaces.
			while (len > 0) {
				const uint8_t chr = (uint8_t)footer.title[start];
				if (chr == 0x00 || chr == 0x20 || chr == 0xFF) {
					start++;
					len--;
				} else {
					break;
				}
			}

			// Trim to the last valid character.
			// NOTE: NESDEV says $20-3F and $41-$5A are allowed,
			// but some games have lowercase characters.
			// NOTE: Allowing all printable characters that aren't 0xFF.
			for (unsigned int i = 0; i < len; i++) {
				const uint8_t chr = (uint8_t)footer.title[start+i];
				if (chr >= 0x20 && chr != 0xFF)
					continue;
				len = i;
				break;
			}

			if (footer.title_encoding != NES_INTFOOTER_ENCODING_SJIS) {
				// Sometimes the length is too short:
				// - Imagineer: Addams Family titles
				// - Alfred Chicken
				// - Daikaijuu Deburas [right-aligned]
				if (start+len < 16) {
					// Increase to the right.
					for (; start+len < 16; len++) {
						// NOTE: Skipping spaces.
						const uint8_t chr = (uint8_t)footer.title[start+len];
						if (chr >= 0x21 && chr <= 0x7E) {
							continue;
						}
						break;
					}
				}

				// Check if we need to increase to the left.
				for (; start > 0; start--, len++) {
					// NOTE: Skipping spaces.
					const uint8_t chr = (uint8_t)footer.title[start-1];
					if (chr >= 0x21 && chr <= 0x7E) {
						continue;
					}
					break;
				}

				// TODO: ascii_to_utf8()?
				s_footerName = cp1252_to_utf8(&footer.title[start], len);
			} else {
				// Trim 0xFF characters.
				for (; len > 0; len--) {
					const uint8_t chr = (uint8_t)footer.title[start+len-1];
					if (chr != 0xFF && chr != 0x00 && chr != 0x20)
						break;
				}
				s_footerName = cp1252_sjis_to_utf8(&footer.title[start], len);
			}
		} else {
			// Invalid title length.
			s_footerName.clear();
		}
	} else {
		// No title.
		s_footerName.clear();
	}

	if (onlyIfValidName && s_footerName.empty()) {
		// Only allowing certain errors if the name is valid,
		// and there's no name. Not valid.
		intFooterErrno = ENOENT;
		return intFooterErrno;
	}

	// Internal footer is valid.
	intFooterErrno = 0;
	return 0;
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
	const DetectInfo info = {
		{0, sizeof(header), header},
		nullptr,	// ext (not needed for NES)
		d->file->size()	// szFile
	};
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

			// NES 2.0 has an alternate method for indicating PRG ROM size,
			// so we need to check for that.
			unsigned int prg_rom_size, chr_rom_size;
			if (likely((inesHeader->nes2.banks_hi & 0x0F) != 0x0F)) {
				// Standard method.
				prg_rom_size = ((inesHeader->prg_banks |
						((inesHeader->nes2.banks_hi & 0x0F) << 8))
						* INES_PRG_BANK_SIZE);
			} else {
				// Alternate method: [EEEE EEMM] -> 2^E * (MM*2 + 1)
				// TODO: Verify that this works.
				prg_rom_size = (1U << (inesHeader->prg_banks >> 2)) *
					       ((inesHeader->prg_banks & 0x03) + 1);
			}
			chr_rom_size = ((inesHeader->chr_banks |
					((inesHeader->nes2.banks_hi & 0xF0) << 4))
					* INES_CHR_BANK_SIZE);

			const off64_t size = sizeof(INES_RomHeader) +
				prg_rom_size + chr_rom_size;
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
 * @param type System name type (See the SystemName enum)
 * @return System name, or nullptr if type is invalid.
 */
const char8_t *NES::systemName(unsigned int type) const
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
			static const char8_t *const sysNames_NES[3][4] = {
				// NES (International)
				{U8("Nintendo Entertainment System"),
				 U8("Nintendo Entertainment System"),
				 U8("NES"), nullptr},

				// Famicom (Japan)
				{U8("Nintendo Famicom"),
				 U8("Famicom"),
				 U8("FC"), nullptr},

				// Hyundai Comboy (South Korea)
				{U8("Hyundai Comboy"),
				 U8("Comboy"),
				 U8("CB"), nullptr},
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
			static const char8_t *const sysNames_FDS[] = {
				U8("Nintendo Famicom Disk System"),
				U8("Famicom Disk System"),
				U8("FDS"), nullptr
			};
			return sysNames_FDS[idx];
		}

		case NESPrivate::ROM_SYSTEM_VS: {
			static const char8_t *const sysNames_VS[] = {
				U8("Nintendo VS. System"),
				U8("VS. System"),
				U8("VS"), nullptr
			};
			return sysNames_VS[idx];
		}

		case NESPrivate::ROM_SYSTEM_PC10: {
			static const char8_t *const sysNames_PC10[] = {
				U8("Nintendo PlayChoice-10"),
				U8("PlayChoice-10"),
				U8("PC10"), nullptr
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
	const char8_t *tabName = U8("iNES");
	const char8_t *rom_format = U8("iNES");
	bool romOK = true;
	int submapper = -1;
	int tnes_mapper = -1;
	bool has_trainer = false;
	uint8_t tv_mode = 0xFF;			// NES2_TV_Mode (0xFF if unknown)
	unsigned int chr_ram_size = 0;		// CHR RAM
	unsigned int chr_ram_battery_size = 0;	// CHR RAM with battery (rare but it exists)
	unsigned int prg_ram_size = 0;		// PRG RAM ($6000-$7FFF)
	unsigned int prg_ram_battery_size = 0;	// PRG RAM with battery (save RAM)

	const int mapper = d->get_iNES_mapper_number();
	const unsigned int prg_rom_size = d->get_prg_rom_size();
	const unsigned int chr_rom_size = d->get_chr_rom_size();

	switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
			rom_format = C_("NES|Format", "Archaic iNES");
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			if (chr_rom_size == 0) {
				chr_ram_size = 8192;
			}
			if (d->header.ines.mapper_lo & INES_F6_BATTERY) {
				prg_ram_battery_size = 8192;
			}
			break;

		case NESPrivate::ROM_FORMAT_INES:
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			// NOTE: Very few iNES ROMs have this set correctly,
			// so we're ignoring it for now.
			//tv_mode = (d->header.ines.ines.tv_mode & 1);
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
			tabName = U8("NES 2.0");	// NOT translatable!
			rom_format = U8("NES 2.0");	// NOT translatable!
			submapper = (d->header.ines.nes2.mapper_hi2 >> 4);
			has_trainer = !!(d->header.ines.mapper_lo & INES_F6_TRAINER);
			tv_mode = (d->header.ines.nes2.tv_mode & 3);
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
			tabName = U8("TNES");	// NOT translatable!
			rom_format = C_("NES|Format", "TNES (Nintendo 3DS Virtual Console)");
			tnes_mapper = d->header.tnes.mapper;
			// FIXME: Check Zelda TNES to see where 8K CHR RAM is.
			break;

		// NOTE: FDS fields are handled later.
		// We're just obtaining the ROM format name here.
		case NESPrivate::ROM_FORMAT_FDS:
			tabName = U8("FDS");	// NOT translatable!
			rom_format = C_("NES|Format", "FDS disk image");
			break;
		case NESPrivate::ROM_FORMAT_FDS_FWNES:
			tabName = U8("FDS");	// NOT translatable!
			rom_format = C_("NES|Format", "FDS disk image (with fwNES header)");
			break;
		case NESPrivate::ROM_FORMAT_FDS_TNES:
			tabName = U8("FDS");	// NOT translatable!
			rom_format = C_("NES|Format", "TDS (Nintendo 3DS Virtual Console)");
			break;

		default:
			tabName = U8("NES");	// NOT translatable!
			rom_format = C_("RomData", "Unknown");
			romOK = false;
			break;
	}
	d->fields->setTabName(0, tabName);

	const char8_t *const format_title = C_("NES", "Format");
	if (d->romType & NESPrivate::ROM_SPECIAL_WIIU_VC) {
		// Wii U Virtual Console.
		const int romFormat = (d->romType & NESPrivate::ROM_FORMAT_MASK);
		assert(romFormat >= NESPrivate::ROM_FORMAT_OLD_INES);
		assert(romFormat <= NESPrivate::ROM_FORMAT_NES2);
		if (romFormat >= NESPrivate::ROM_FORMAT_OLD_INES &&
		    romFormat <= NESPrivate::ROM_FORMAT_NES2)
		{
			// FIXME: U8STRFIX - rp_sprintf()
			d->fields->addField_string(format_title,
				// tr: ROM format, e.g. iNES or FDS disk image.
				rp_sprintf(reinterpret_cast<const char*>(C_("NES|Format", "%s (Wii U Virtual Console)")), rom_format));
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

	// Mapper
	const char8_t *const mapper_title = C_("NES", "Mapper");
	if (mapper >= 0) {
		// Look up the mapper name.
		// FIXME: U8STRFIX
		string s_mapper;

		const char8_t *const mapper_name = NESMappers::lookup_ines(mapper);
		if (mapper_name) {
			// FIXME: U8STRFIX - rp_sprintf_p()
			// tr: Print the mapper ID followed by the mapper name.
			s_mapper = rp_sprintf_p(reinterpret_cast<const char*>(C_("NES|Mapper", "%1$u - %2$s")),
				static_cast<unsigned int>(mapper),
				reinterpret_cast<const char*>(mapper_name));
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
		// Submapper (NES 2.0 only)
		// FIXME: U8STRFIX
		string s_submapper;

		// Look up the submapper name.
		// TODO: Needs testing.
		const char8_t *const submapper_name = NESMappers::lookup_nes2_submapper(mapper, submapper);
		if (submapper_name) {
			// FIXME: U8STRFIX - rp_sprintf_p()
			// tr: Print the submapper ID followed by the submapper name.
			s_submapper = rp_sprintf_p(reinterpret_cast<const char*>(C_("NES|Mapper", "%1$u - %2$s")),
				static_cast<unsigned int>(submapper),
				reinterpret_cast<const char*>(submapper_name));
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
	static const char8_t *const tv_mode_tbl[] = {
		U8("NTSC (RP2C02)"),
		U8("PAL (RP2C07)"),
		NOP_C_("NES|TVMode", "Dual (NTSC/PAL)"),
		U8("Dendy (UMC 6527P)"),
	};
	if (tv_mode < ARRAY_SIZE(tv_mode_tbl)) {
		// FIXME: U8STRFIX - dpgettext_expr()
		d->fields->addField_string(C_("NES", "TV Mode"),
			dpgettext_expr(RP_I18N_DOMAIN, "NES|TVMode",
				reinterpret_cast<const char*>(tv_mode_tbl[tv_mode])));
	}

	// ROM features
	const char8_t *rom_features = nullptr;
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
		const char8_t *const publisher_title = C_("RomData", "Publisher");
		const char8_t *const publisher =
			NintendoPublishers::lookup_fds(d->header.fds.publisher_code);
		if (publisher) {
			d->fields->addField_string(publisher_title, publisher);
		} else {
			// FIXME: U8STRFIX - rp_sprintf()
			d->fields->addField_string(publisher_title,
				rp_sprintf(reinterpret_cast<const char*>(C_("RomData", "Unknown (0x%02X)")),
					d->header.fds.publisher_code));
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
		const char8_t *s_mirroring = nullptr;
		const char8_t *s_vs_ppu = nullptr;
		const char8_t *s_vs_hw = nullptr;
		const char8_t *s_extd_ct = nullptr;
		const char8_t *s_exp_hw = nullptr;
		unsigned int misc_roms = 0;
		switch (d->romType & NESPrivate::ROM_FORMAT_MASK) {
			case NESPrivate::ROM_FORMAT_OLD_INES:
			case NESPrivate::ROM_FORMAT_INES:
			case NESPrivate::ROM_FORMAT_NES2:
				// Mirroring
				s_mirroring = NESMappers::lookup_ines_mirroring(mapper,
					(submapper == -1) ? 0 : submapper,
					d->header.ines.mapper_lo & INES_F6_MIRROR_VERT,
					d->header.ines.mapper_lo & INES_F6_MIRROR_FOUR);

				// Check for NES 2.0 extended console types, including VS hardware.
				if ((d->romType & (NESPrivate::ROM_SYSTEM_MASK | NESPrivate::ROM_FORMAT_MASK)) ==
				    (NESPrivate::ROM_FORMAT_NES2 | NESPrivate::ROM_SYSTEM_VS))
				{
					// Check the Vs. PPU type.
					// NOTE: Not translatable!
					static const char8_t vs_ppu_types[][12] = {
						U8("RP2C03B"),     U8("RP2C03G"),
						U8("RP2C04-0001"), U8("RP2C04-0002"),
						U8("RP2C04-0003"), U8("RP2C04-0004"),
						U8("RP2C03B"),     U8("RP2C03C"),
						U8("RP2C05-01"),   U8("RP2C05-02"),
						U8("RP2C05-03"),   U8("RP2C05-04"),
						U8("RP2C05-05")
					};
					const unsigned int vs_ppu = (d->header.ines.nes2.vs_hw & 0x0F);
					if (vs_ppu < ARRAY_SIZE(vs_ppu_types)) {
						s_vs_ppu = vs_ppu_types[vs_ppu];
					}

					// Check the Vs. hardware type.
					// NOTE: Not translatable!
					static const char8_t *const vs_hw_types[] = {
						U8("Vs. Unisystem"),
						U8("Vs. Unisystem (RBI Baseball)"),
						U8("Vs. Unisystem (TKO Boxing)"),
						U8("Vs. Unisystem (Super Xevious)"),
						U8("Vs. Unisystem (Vs. Ice Climber Japan)"),
						U8("Vs. Dualsystem"),
						U8("Vs. Dualsystem (Raid on Bungeling Bay)"),
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
						// TODO: Localization?
						static const char8_t *const ext_hw_types[] = {
							U8("NES/Famicom/Dendy"),	// Not normally used.
							U8("Nintendo Vs. System"),	// Not normally used.
							U8("PlayChoice-10"),		// Not normally used.
							U8("Famiclone with BCD support"),
							U8("V.R. Technology VT01 with monochrome palette"),
							U8("V.R. Technology VT01 with red/cyan STN palette"),
							U8("V.R. Technology VT02"),
							U8("V.R. Technology VT03"),
							U8("V.R. Technology VT09"),
							U8("V.R. Technology VT32"),
							U8("V.R. Technology VT369"),
							U8("UMC UM6578"),
						};

						const unsigned int extd_ct = (d->header.ines.nes2.vs_hw & 0x0F);
						if (extd_ct < ARRAY_SIZE(ext_hw_types)) {
							s_extd_ct = ext_hw_types[extd_ct];
						}
					}

					// Miscellaneous ROM count.
					misc_roms = d->header.ines.nes2.misc_roms & 3;

					// Default expansion hardware.
					static const char8_t *const exp_hw_tbl[] = {
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
						// FIXME: U8STRFIX - dpgettext_expr()
						s_exp_hw = reinterpret_cast<const char8_t*>(
							dpgettext_expr(RP_I18N_DOMAIN, "NES|Expansion",
								reinterpret_cast<const char*>(exp_hw_tbl[exp_hw])));
					}
				}
				break;

			case NESPrivate::ROM_FORMAT_TNES:
				// Mirroring
				// FIXME: U8STRFIX
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

		const char8_t *const mirroring_title = C_("NES", "Mirroring");
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

			const uint8_t prg_sz_idx = footer.rom_size >> 4;
			const uint8_t chr_sz_idx = footer.rom_size & 0x07;
			unsigned int prg_size = 0, chr_size = 0;
			if (prg_sz_idx < ARRAY_SIZE(d->footer_prg_rom_size_shift_lkup)) {
				prg_size = (1U << d->footer_prg_rom_size_shift_lkup[prg_sz_idx]);
			}
			if (chr_sz_idx < ARRAY_SIZE(d->footer_chr_rom_size_shift_lkup)) {
				chr_size = (1U << d->footer_chr_rom_size_shift_lkup[chr_sz_idx]);
			}

			// PRG ROM size
			const char8_t *const prg_rom_size_title = C_("NES", "PRG ROM Size");
			if (prg_size > 0) {
				d->fields->addField_string(prg_rom_size_title, formatFileSizeKiB(prg_size));
			} else {
				// FIXME: U8STRFIX - rp_sprintf()
				d->fields->addField_string(prg_rom_size_title,
					rp_sprintf(reinterpret_cast<const char*>(C_("RomData", "Unknown (0x%02X)")), prg_sz_idx));
			}

			// CHR ROM/RAM size
			u8string s_chr_size;
			bool b_chr_ram = !!(footer.rom_size & 0x08);
			if (chr_size > 0) {
				s_chr_size = formatFileSizeKiB(chr_size);
			} else {
				// FIXME: U8STRFIX - rp_sprintf()
				s_chr_size = reinterpret_cast<const char8_t*>(
					rp_sprintf(reinterpret_cast<const char*>(C_("RomData", "Unknown (0x%02X)")), chr_sz_idx).c_str());
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
			if (footer.board_info & 0x80) {
				d->fields->addField_string(mirroring_title,
					C_("NES|Mirroring", "Horizontal"));
			} else {
				d->fields->addField_string(mirroring_title,
					C_("NES|Mirroring", "Vertical"));
			}

			// Board type (Mapper)
			static const char footer_mapper_tbl[][8] = {
				"NROM", "CNROM", "UNROM", "GNROM", "MMCx"
			};
			const unsigned int footer_mapper = (footer.board_info & 0x7F);
			// FIXME: U8STRFIX - rp_sprintf_p()
			string s_footer_mapper;
			if (footer_mapper < ARRAY_SIZE(footer_mapper_tbl)) {
				// tr: Print the mapper ID followed by the mapper name.
				s_footer_mapper = rp_sprintf_p(reinterpret_cast<const char*>(C_("NES|Mapper", "%1$u - %2$s")),
						footer_mapper, footer_mapper_tbl[footer_mapper]);
			} else {
				// tr: Print only the mapper ID.
				s_footer_mapper = rp_sprintf("%u", footer_mapper);
			}
			d->fields->addField_string(C_("NES", "Board Type"), s_footer_mapper);

			// Publisher
			const char8_t *const publisher_title = C_("RomData", "Publisher");
			const char8_t *const publisher =
				NintendoPublishers::lookup_old(footer.publisher_code);
			if (publisher) {
				d->fields->addField_string(publisher_title, publisher);
			} else {
				// FIXME: U8STRFIX - rp_sprintf()
				d->fields->addField_string(publisher_title,
					rp_sprintf(reinterpret_cast<const char*>(C_("RomData", "Unknown (0x%02X)")),
						footer.publisher_code));
			}
		}
	}

	// TODO: More fields.
	return static_cast<int>(d->fields->count());
}

}

/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * NES.cpp: Nintendo Entertainment System/Famicom ROM reader.              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2022 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "NES.hpp"
#include "data/NintendoPublishers.hpp"
#include "data/NESMappers.hpp"
#include "nes_structs.h"

// for testMode
#include "RomDataFactory.hpp"

// Other rom-properties libraries
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C includes
#include "ctypex.h"

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

// zlib for crc32()
#include <zlib.h>
#ifdef _MSC_VER
// MSVC: Exception handling for /DELAYLOAD.
#  include "libwin32common/DelayLoadHelper.h"
#endif /* _MSC_VER */

namespace LibRomData {

#ifdef _MSC_VER
// DelayLoad test implementation.
DELAYLOAD_TEST_FUNCTION_IMPL0(get_crc_table);
#endif /* _MSC_VER */

class NESPrivate final : public RomDataPrivate
{
public:
	explicit NESPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(NESPrivate)

public:
	/** RomDataInfo **/
	static const array<const char*, 5+1> exts;
	static const array<const char*, 2+1> mimeTypes;
	static const RomDataInfo romDataInfo;

public:
	/** RomFields **/

	// ROM image type
	enum RomType {
		ROM_UNKNOWN = -1,		// Unknown ROM type

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
	// ROM header
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

	// ROM footer (optional)
	NES_IntFooter footer;
	string s_footerName;		// Name from the footer.
	bool hasCheckedIntFooter;	// True if we already checked.
	uint8_t intFooterErrno;		// If checked: 0 if valid, positive errno on error.

	// CRC32s for external images
	// 0 == PRG ROM; 1 == CHR ROM (if present)
	uint32_t rom_8k_crc32[2];

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
	static const array<uint8_t, 6> footer_prg_rom_size_shift_lkup;

	// Internal footer: CHR ROM sizes (as powers of two)
	static const array<uint8_t, 6> footer_chr_rom_size_shift_lkup;

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

public:
	/**
	 * Initialize zlib.
	 * @return 0 on success; non-zero on error.
	 */
	static int zlibInit(void);

	/**
	 * Calculate the CRC32s of the first 8 KB of PRG ROM and CHR ROM.
	 * This is used for external image URLs.
	 *
	 * The CRC32s will be stored in rom_8k_crc32[].
	 *
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int calcRomCRC32s(void);
};

ROMDATA_IMPL(NES)
ROMDATA_IMPL_IMG(NES)

/** NESPrivate **/

/* RomDataInfo */
const array<const char*, 5+1> NESPrivate::exts = {{
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
}};
const array<const char*, 2+1> NESPrivate::mimeTypes = {{
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-nes-rom",
	"application/x-fds-disk",

	nullptr
}};
const RomDataInfo NESPrivate::romDataInfo = {
	"NES", exts.data(), mimeTypes.data()
};

// Internal footer: PRG ROM sizes (as powers of two)
const array<uint8_t, 6> NESPrivate::footer_prg_rom_size_shift_lkup = {{
	16,	// 0 (64 KB)
	14,	// 1 (16 KB)
	15,	// 2 (32 KB)
	17,	// 3 (128 KB)
	18,	// 4 (256 KB)
	19,	// 5 (512 KB)
}};

// Internal footer: CHR ROM sizes (as powers of two)
const array<uint8_t, 6> NESPrivate::footer_chr_rom_size_shift_lkup = {{
	13,	// 0 (8 KB)
	14,	// 1 (16 KB)
	15,	// 2 (32 KB)
	17,	// 3 (128 KB)	// FIXME: May be 64 KB.
	18,	// 4 (256 KB)
	19,	// 5 (512 KB)
}};

NESPrivate::NESPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
	, romType(ROM_UNKNOWN)
	, hasCheckedIntFooter(false)
	, intFooterErrno(255)
{
	// Clear the ROM header structs.
	memset(&header, 0, sizeof(header));
	memset(&footer, 0, sizeof(footer));

	// Clear the CRC32s.
	memset(rom_8k_crc32, 0, sizeof(rom_8k_crc32));
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
	unsigned int bcd_year = ((fds_bcd_ds->year >> 4) * 10) +
	                         (fds_bcd_ds->year & 0x0F);
	if (bcd_year >= 58) {
		// >=58 (1983+): Shōwa era (1926-1989); add 1925
		fdstime.tm_year = bcd_year + 1925 - 1900;
	} else /*if (bcd_year <= 57)*/ {
		// <=57: Heisei era (1989-2019); add 1988
		fdstime.tm_year = bcd_year + 1988 - 1900;
	}
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
	if (prg_sz_idx >= footer_prg_rom_size_shift_lkup.size() ||
	    chr_sz_idx >= footer_chr_rom_size_shift_lkup.size())
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

/**
 * Initialize zlib.
 * @return 0 on success; non-zero on error.
 */
int NESPrivate::zlibInit(void)
{
#if defined(_MSC_VER) && defined(ZLIB_IS_DLL)
	// Delay load verification.
	// TODO: Only if linked with /DELAYLOAD?
	if (DelayLoad_test_get_crc_table() != 0) {
		// Delay load failed.
		// Can't calculate the CRC32.
		return -ENOENT;
	}
#else /* !defined(_MSC_VER) || !defined(ZLIB_IS_DLL) */
	// zlib isn't in a DLL, but we need to ensure that the
	// CRC table is initialized anyway.
	get_crc_table();
#endif /* defined(_MSC_VER) && defined(ZLIB_IS_DLL) */

	// zlib initialized.
	return 0;
}

int NESPrivate::calcRomCRC32s(void)
{
	if (rom_8k_crc32[0] != 0) {
		// We already have the CRC32s.
		return 0;
	}

	// CRC32s haven't been calculated yet.
	if (zlibInit() != 0) {
		// zlib could not be initialized.
		return -EIO;
	}

	if (!file || !file->isOpen()) {
		// File isn't open. Can't calculate the CRC32.
		return -EBADF;
	}

	// Determine the starting addresses of PRG and CHR.
	uint32_t prg_addr, chr_addr;
	switch (romType & NESPrivate::ROM_FORMAT_MASK) {
		case NESPrivate::ROM_FORMAT_OLD_INES:
		case NESPrivate::ROM_FORMAT_INES:
		case NESPrivate::ROM_FORMAT_NES2:
		case NESPrivate::ROM_FORMAT_TNES: {
			prg_addr = static_cast<uint32_t>(sizeof(header.ines));

			// If a trainer is present, add 512.
			if (header.ines.mapper_lo & INES_F6_TRAINER) {
				prg_addr += 512;
			}

			// PRG ROM is stored here.
			// CHR ROM is stored after all PRG ROM banks.
			if (likely(get_chr_rom_size() != 0)) {
				chr_addr = prg_addr + get_prg_rom_size();
			} else {
				// No CHR ROM in this image.
				chr_addr = 0;
			}
			break;
		}

		case NESPrivate::ROM_FORMAT_FDS:
		case NESPrivate::ROM_FORMAT_FDS_FWNES:
		case NESPrivate::ROM_FORMAT_FDS_TNES:
			// TODO
			return -ENOENT;

		default:
			// Not supported.
			return -ENOENT;
	}

	static constexpr size_t NES_ROM_BUF_SIZ = (8U * 1024U);
	unique_ptr<uint8_t[]> buf(new uint8_t[NES_ROM_BUF_SIZ]);

	// Read 8 KB of PRG ROM.
	size_t sz_rd = file->seekAndRead(prg_addr, buf.get(), NES_ROM_BUF_SIZ);
	if (sz_rd != NES_ROM_BUF_SIZ) {
		// Seek and/or read error.
		int err = -file->lastError();
		if (err == 0) {
			err = -EIO;
		}
		return err;
	}
	rom_8k_crc32[0] = crc32(0, buf.get(), NES_ROM_BUF_SIZ);

	// Read 8 KB of CHR ROM.
	if (likely(chr_addr != 0)) {
		size_t sz_rd = file->seekAndRead(chr_addr, buf.get(), NES_ROM_BUF_SIZ);
		if (sz_rd != NES_ROM_BUF_SIZ) {
			// Seek and/or read error.
			int err = -file->lastError();
			if (err == 0) {
				err = -EIO;
			}
			return err;
		}
		rom_8k_crc32[1] = crc32(0, buf.get(), NES_ROM_BUF_SIZ);
	} else {
		// No CHR ROM.
		rom_8k_crc32[1] = 0;
	}

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
NES::NES(const IRpFilePtr &file)
	: super(new NESPrivate(file))
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
		d->file.reset();
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
				d->file.reset();
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
				d->file.reset();
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
				d->file.reset();
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
			d->file.reset();
			d->fileType = FileType::Unknown;
			d->romType = NESPrivate::ROM_FORMAT_UNKNOWN;
			return;
	}

	d->isValid = true;

	// Is PAL?
	if (d->romType == NESPrivate::ROM_FORMAT_NES2 &&
	    (d->header.ines.nes2.tv_mode & 3) == NES2_F12_PAL)
	{
		d->isPAL = true;
	}
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
			bool isNES2 = false;

			if (unlikely(info->szFile == sizeof(INES_RomHeader))) {
				// 16-byte ROM file. This is a test file. [RomHeaderTest]
				// Assume it's NES2.
				isNES2 = true;
			} else {
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
				isNES2 = (size <= info->szFile);
			}

			if (isNES2) {
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
	static constexpr uint8_t fds_magic[] = "*NINTENDO-HVC*";

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
			static const array<array<const char*, 4>, 3> sysNames_NES = {{
				// NES (International)
				{{"Nintendo Entertainment System", "Nintendo Entertainment System", "NES", nullptr}},

				// Famicom (Japan)
				{{"Nintendo Famicom", "Famicom", "FC", nullptr}},

				// Hyundai Comboy (South Korea)
				{{"Hyundai Comboy", "Comboy", "CB", nullptr}},
			}};

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
			static const array<const char*, 4> sysNames_FDS = {{
				"Nintendo Famicom Disk System", "Famicom Disk System", "FDS", nullptr
			}};
			return sysNames_FDS[idx];
		}

		case NESPrivate::ROM_SYSTEM_VS: {
			static const array<const char*, 4> sysNames_VS = {{
				"Nintendo VS. System", "VS. System", "VS", nullptr
			}};
			return sysNames_VS[idx];
		}

		case NESPrivate::ROM_SYSTEM_PC10: {
			static const array<const char*, 4> sysNames_PC10 = {{
				"Nintendo PlayChoice-10", "PlayChoice-10", "PC10", nullptr
			}};
			return sysNames_PC10[idx];
		}
	};

	// Should not get here...
	return nullptr;
}

/**
 * Get a bitfield of image types this class can retrieve.
 * @return Bitfield of supported image types. (ImageTypesBF)
 */
uint32_t NES::supportedImageTypes_static(void)
{
	return IMGBF_EXT_TITLE_SCREEN;
}

/**
 * Get a list of all available image sizes for the specified image type.
 * @param imageType Image type.
 * @return Vector of available image sizes, or empty vector if no images are available.
 */
vector<RomData::ImageSizeDef> NES::supportedImageSizes_static(ImageType imageType)
{
	ASSERT_supportedImageSizes(imageType);

	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN:
			// NOTE: Always using 256x240.
			// TODO: Return x224 if NTSC and the user has selected to crop overscan?
			return {{nullptr, 256, 240, 0}};
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
uint32_t NES::imgpf(ImageType imageType) const
{
	ASSERT_imgpf(imageType);

	uint32_t ret = 0;
	switch (imageType) {
		case IMG_EXT_TITLE_SCREEN: {
			// Rescaling is required for the 8:7 pixel aspect ratio.
			// Also, crop overscan if this isn't confirmed to be a PAL ROM.
			RP_D(NES);
			if (unlikely(d->isPAL)) {
				ret = IMGPF_RESCALE_ASPECT_8to7;
			} else {
				ret = IMGPF_RESCALE_ASPECT_8to7 | IMGPF_NTSC_NES_OVERSCAN;
			}
			break;
		}

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
int NES::loadFieldData(void)
{
	RP_D(NES);
	if (!d->fields.empty()) {
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
	d->fields.reserve(15+4+8);

	// Reserve at least 2 tabs:
	// iNES, Internal Footer
	d->fields.reserveTabs(2);

	const auto *const header = &d->header;

	// Determine stuff based on the ROM format.
	// Default tab name and ROM format name is "iNES", since it's the most common.
	const char *tabName = "iNES";
	const char *rom_format = "iNES";
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
			has_trainer = !!(header->ines.mapper_lo & INES_F6_TRAINER);
			if (chr_rom_size == 0) {
				chr_ram_size = 8192;
			}
			if (header->ines.mapper_lo & INES_F6_BATTERY) {
				prg_ram_battery_size = 8192;
			}
			break;

		case NESPrivate::ROM_FORMAT_INES:
			has_trainer = !!(header->ines.mapper_lo & INES_F6_TRAINER);
			// NOTE: Very few iNES ROMs have this set correctly,
			// so we're ignoring it for now.
			//tv_mode = (header->ines.ines.tv_mode & 1);
			if (chr_rom_size == 0) {
				chr_ram_size = 8192;
			}
			if (header->ines.mapper_lo & INES_F6_BATTERY) {
				if (header->ines.ines.prg_ram_size == 0) {
					prg_ram_battery_size = 8192;
				} else {
					prg_ram_battery_size = header->ines.ines.prg_ram_size * INES_PRG_RAM_BANK_SIZE;
				}
			} else {
				// FIXME: Is this correct?
				if (header->ines.ines.prg_ram_size > 0) {
					prg_ram_size = header->ines.ines.prg_ram_size * INES_PRG_RAM_BANK_SIZE;
				}
			}
			break;

		case NESPrivate::ROM_FORMAT_NES2:
			tabName = "NES 2.0";	// NOT translatable!
			rom_format = "NES 2.0";	// NOT translatable!
			submapper = (header->ines.nes2.mapper_hi2 >> 4);
			has_trainer = !!(header->ines.mapper_lo & INES_F6_TRAINER);
			tv_mode = (header->ines.nes2.tv_mode & 3);
			// CHR RAM size. (TODO: Needs testing.)
			if (header->ines.nes2.vram_size & 0x0F) {
				chr_ram_size = 128 << ((header->ines.nes2.vram_size & 0x0F) - 1);
			}
			if ((header->ines.mapper_lo & INES_F6_BATTERY) &&
			    (header->ines.nes2.vram_size & 0xF0))
			{
				chr_ram_battery_size = 128 << ((header->ines.nes2.vram_size >> 4) - 1);
			}
			// PRG RAM size (TODO: Needs testing.)
			if (header->ines.nes2.prg_ram_size & 0x0F) {
				prg_ram_size = 128 << ((header->ines.nes2.prg_ram_size & 0x0F) - 1);
			}
			// TODO: Require INES_F6_BATTERY?
			if (header->ines.nes2.prg_ram_size & 0xF0) {
				prg_ram_battery_size = 128 << ((header->ines.nes2.prg_ram_size >> 4) - 1);
			}
			break;

		case NESPrivate::ROM_FORMAT_TNES:
			tabName = "TNES";	// NOT translatable!
			rom_format = C_("NES|Format", "TNES (Nintendo 3DS Virtual Console)");
			tnes_mapper = header->tnes.mapper;
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
	d->fields.setTabName(0, tabName);

	const char *const format_title = C_("NES", "Format");
	if (d->romType & NESPrivate::ROM_SPECIAL_WIIU_VC) {
		// Wii U Virtual Console.
		const int romFormat = (d->romType & NESPrivate::ROM_FORMAT_MASK);
		assert(romFormat >= NESPrivate::ROM_FORMAT_OLD_INES);
		assert(romFormat <= NESPrivate::ROM_FORMAT_NES2);
		if (romFormat >= NESPrivate::ROM_FORMAT_OLD_INES &&
		    romFormat <= NESPrivate::ROM_FORMAT_NES2)
		{
			d->fields.addField_string(format_title,
				// tr: ROM format, e.g. iNES or FDS disk image.
				fmt::format(FRUN(C_("NES|Format", "{:s} (Wii U Virtual Console)")), rom_format));
		} else {
			d->fields.addField_string(format_title, rom_format);
		}
	} else {
		d->fields.addField_string(format_title, rom_format);
	}

	// Display the fields.
	if (!romOK) {
		// Invalid mapper.
		return static_cast<int>(d->fields.count());
	}

	const char *const mapper_title = C_("NES", "Mapper");
	if (mapper >= 0) {
		// Look up the mapper name.
		string s_mapper;

		const char *const mapper_name = NESMappers::lookup_ines(mapper);
		if (mapper_name) {
			// tr: Print the mapper ID followed by the mapper name.
			s_mapper = fmt::format(FRUN(C_("NES|Mapper", "{0:d} - {1:s}")),
				static_cast<unsigned int>(mapper), mapper_name);
		} else {
			// tr: Print only the mapper ID.
			s_mapper = fmt::to_string(static_cast<unsigned int>(mapper));
		}
		d->fields.addField_string(mapper_title, s_mapper);
	} else {
		// No mapper. If this isn't TNES,
		// then it's probably an FDS image.
		if (tnes_mapper >= 0) {
			// This has a TNES mapper.
			// It *should* map to an iNES mapper...
			d->fields.addField_string(mapper_title,
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
			s_submapper = fmt::format(FRUN(C_("NES|Mapper", "{0:d} - {1:s}")),
				static_cast<unsigned int>(submapper), submapper_name);
		} else {
			// tr: Print only the submapper ID.
			s_submapper = fmt::to_string(static_cast<unsigned int>(submapper));
		}
		d->fields.addField_string(C_("NES", "Submapper"), s_submapper);
	}

	if (tnes_mapper >= 0) {
		// TNES mapper.
		// TODO: Look up the name.
		d->fields.addField_string_numeric(C_("NES", "TNES Mapper"),
			tnes_mapper, RomFields::Base::Dec);
	}

	// TV mode
	static const array<const char*, 4> tv_mode_tbl = {{
		"NTSC (RP2C02)",
		"PAL (RP2C07)",
		NOP_C_("NES|TVMode", "Dual (NTSC/PAL)"),
		"Dendy (UMC 6527P)",
	}};
	if (tv_mode < tv_mode_tbl.size()) {
		d->fields.addField_string(C_("NES", "TV Mode"),
			pgettext_expr("NES|TVMode", tv_mode_tbl[tv_mode]));
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
		d->fields.addField_string(C_("NES", "Features"), rom_features);
	}

	// ROM sizes
	if (prg_rom_size > 0) {
		d->fields.addField_string(C_("NES", "PRG ROM Size"),
			formatFileSizeKiB(prg_rom_size));
	}
	if (chr_rom_size > 0) {
		d->fields.addField_string(C_("NES", "CHR ROM Size"),
			formatFileSizeKiB(chr_rom_size));
	}

	// RAM sizes
	if (prg_ram_size > 0) {
		d->fields.addField_string(C_("NES", "PRG RAM Size"),
			formatFileSizeKiB(prg_ram_size));
	}
	if (prg_ram_battery_size > 0) {
		// tr: Save RAM with a battery backup.
		d->fields.addField_string(C_("NES", "Save RAM (backed up)"),
			formatFileSizeKiB(prg_ram_battery_size));
	}
	if (chr_ram_size > 0) {
		d->fields.addField_string(C_("NES", "CHR RAM Size"),
			formatFileSizeKiB(chr_ram_size));
	}
	if (chr_ram_battery_size > 0) {
		// tr: CHR RAM with a battery backup.
		d->fields.addField_string(C_("NES", "CHR RAM (backed up)"),
			formatFileSizeKiB(chr_ram_battery_size));
	}

	// Check for FDS fields.
	if ((d->romType & NESPrivate::ROM_SYSTEM_MASK) == NESPrivate::ROM_SYSTEM_FDS) {
		// Game ID
		// TODO: Check for invalid characters?
		char game_id[4];
		memcpy(game_id, header->fds.game_id, 3);
		game_id[3] = '\0';
		d->fields.addField_string(C_("RomData", "Game ID"),
			fmt::format(FSTR("{:s}-{:s}"),
				(header->fds.disk_type == FDS_DTYPE_FSC ? "FSC" : "FMC"),
				game_id));

		// Publisher
		const char *const publisher_title = C_("RomData", "Publisher");
		const char *const publisher =
			NintendoPublishers::lookup_fds(header->fds.publisher_code);
		if (publisher) {
			d->fields.addField_string(publisher_title, publisher);
		} else {
			d->fields.addField_string(publisher_title,
				fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), header->fds.publisher_code));
		}

		// Revision
		d->fields.addField_string_numeric(C_("RomData", "Revision"),
			header->fds.revision, RomFields::Base::Dec, 2);

		// Manufacturing Date.
		const time_t mfr_date = d->fds_bcd_datestamp_to_unix_time(&header->fds.mfr_date);
		d->fields.addField_dateTime(C_("NES", "Manufacturing Date"), mfr_date,
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
					header->ines.mapper_lo & INES_F6_MIRROR_VERT,
					header->ines.mapper_lo & INES_F6_MIRROR_FOUR);

				// Check for NES 2.0 extended console types, including VS hardware.
				if ((d->romType & (NESPrivate::ROM_SYSTEM_MASK | NESPrivate::ROM_FORMAT_MASK)) ==
				    (NESPrivate::ROM_FORMAT_NES2 | NESPrivate::ROM_SYSTEM_VS))
				{
					// Check the Vs. PPU type.
					// NOTE: Not translatable!
					static constexpr char vs_ppu_types[][12] = {
						"RP2C03B",     "RP2C03G",
						"RP2C04-0001", "RP2C04-0002",
						"RP2C04-0003", "RP2C04-0004",
						"RP2C03B",     "RP2C03C",
						"RP2C05-01",   "RP2C05-02",
						"RP2C05-03",   "RP2C05-04",
						"RP2C05-05"
					};
					const unsigned int vs_ppu = (header->ines.nes2.vs_hw & 0x0F);
					if (vs_ppu < ARRAY_SIZE(vs_ppu_types)) {
						s_vs_ppu = vs_ppu_types[vs_ppu];
					}

					// Check the Vs. hardware type.
					// NOTE: Not translatable!
					static const array<const char*, 7> vs_hw_types = {{
						"Vs. Unisystem",
						"Vs. Unisystem (RBI Baseball)",
						"Vs. Unisystem (TKO Boxing)",
						"Vs. Unisystem (Super Xevious)",
						"Vs. Unisystem (Vs. Ice Climber Japan)",
						"Vs. Dualsystem",
						"Vs. Dualsystem (Raid on Bungeling Bay)",
					}};
					const unsigned int vs_hw = (header->ines.nes2.vs_hw >> 4);
					if (vs_hw < vs_hw_types.size()) {
						s_vs_hw = vs_hw_types[vs_hw];
					}
				}

				// Other NES 2.0 fields.
				if ((d->romType & NESPrivate::ROM_FORMAT_MASK) == NESPrivate::ROM_FORMAT_NES2) {
					if ((header->ines.mapper_hi & INES_F7_SYSTEM_MASK) == INES_F7_SYSTEM_EXTD) {
						// NES 2.0 Extended Console Type
						static const array<const char*, 12> ext_hw_types = {{
							"NES/Famicom/Dendy",	// Not normally used.
							"Nintendo VS. System",	// Not normally used.
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
						}};

						const unsigned int extd_ct = (header->ines.nes2.vs_hw & 0x0F);
						if (extd_ct < ext_hw_types.size()) {
							s_extd_ct = ext_hw_types[extd_ct];
						}
					}

					// Miscellaneous ROM count.
					misc_roms = header->ines.nes2.misc_roms & 3;

					// Default expansion hardware.
					static const array<const char*, 48> exp_hw_tbl = {{
						// 0x00
						NOP_C_("NES|Expansion", "Unspecified"),
						NOP_C_("NES|Expansion", "NES/Famicom Controllers"),
						NOP_C_("NES|Expansion", "NES Four Score / Satellite"),
						NOP_C_("NES|Expansion", "Famicom Four Players Adapter"),
						NOP_C_("NES|Expansion", "VS. System"),
						NOP_C_("NES|Expansion", "VS. System (reversed inputs)"),
						NOP_C_("NES|Expansion", "VS. Pinball (Japan)"),
						NOP_C_("NES|Expansion", "VS. Zapper"),
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
						NOP_C_("NES|Expansion", "Pokkun Mogura"),
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
					}};

					const unsigned int exp_hw = (header->ines.nes2.expansion & 0x3F);
					if (exp_hw < exp_hw_tbl.size()) {
						s_exp_hw = pgettext_expr("NES|Expansion", exp_hw_tbl[exp_hw]);
					}
				}
				break;

			case NESPrivate::ROM_FORMAT_TNES:
				// Mirroring
				switch (header->tnes.mirroring) {
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
			d->fields.addField_string(mirroring_title, s_mirroring);
		}
		if (s_vs_ppu) {
			d->fields.addField_string(C_("NES", "Vs. PPU"), s_vs_ppu);
		}
		if (s_vs_hw) {
			// TODO: Increase the reserved field count?
			d->fields.addField_string(C_("NES", "Vs. Hardware"), s_vs_hw);
		}
		if (s_extd_ct) {
			// TODO: Increase the reserved field count?
			d->fields.addField_string(C_("NES", "Console Type"), s_extd_ct);
		}
		if (misc_roms > 0) {
			// TODO: Increase the reserved field count?
			d->fields.addField_string_numeric(C_("NES", "Misc. ROM Count"), misc_roms);
		}
		if (s_exp_hw) {
			// TODO: Increase the reserved field count?
			d->fields.addField_string(C_("NES", "Default Expansion"), s_exp_hw);
		}

		// Check for the internal footer.
		if (d->loadInternalFooter() == 0) {
			// Found the internal footer.
			d->fields.addTab("Internal Footer");
			const NES_IntFooter &footer = d->footer;

			// Internal name (Assuming ASCII)
			if (!d->s_footerName.empty()) {
				d->fields.addField_string(C_("NES", "Internal Name"),
					d->s_footerName, RomFields::STRF_TRIM_END);
			}

			// PRG checksum
			d->fields.addField_string_numeric(C_("NES", "PRG Checksum"),
				le16_to_cpu(footer.prg_checksum),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			// CHR checksum
			d->fields.addField_string_numeric(C_("NES", "CHR Checksum"),
				le16_to_cpu(footer.chr_checksum),
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

			const uint8_t prg_sz_idx = footer.rom_size >> 4;
			const uint8_t chr_sz_idx = footer.rom_size & 0x07;
			unsigned int prg_size = 0, chr_size = 0;
			if (prg_sz_idx < d->footer_prg_rom_size_shift_lkup.size()) {
				prg_size = (1U << d->footer_prg_rom_size_shift_lkup[prg_sz_idx]);
			}
			if (chr_sz_idx < d->footer_chr_rom_size_shift_lkup.size()) {
				chr_size = (1U << d->footer_chr_rom_size_shift_lkup[chr_sz_idx]);
			}

			// PRG ROM size
			string s_prg_size;
			if (prg_size > 0) {
				s_prg_size = formatFileSizeKiB(prg_size);
			} else {
				s_prg_size = fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), prg_sz_idx);
			}
			d->fields.addField_string(C_("NES", "PRG ROM Size"), s_prg_size);

			// CHR ROM/RAM size
			string s_chr_size;
			const bool b_chr_ram = !!(footer.rom_size & 0x08);
			if (chr_size > 0) {
				s_chr_size = formatFileSizeKiB(chr_size);
			} else {
				s_chr_size = fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), chr_sz_idx);
			}
			if (likely(!b_chr_ram)) {
				d->fields.addField_string(C_("NES", "CHR ROM Size"), s_chr_size);
			} else {
				// NOTE: Some ROMs that have CHR ROM indicate CHR RAM in the footer,
				// e.g. Castlevania II - Simon's Quest (E/U) and Contra (J).
				// Other regional versions of these games actually *do* have CHR RAM.
				d->fields.addField_string(C_("NES", "CHR RAM Size"), s_chr_size);
			}

			// Mirroring
			// TODO: This is incorrect; need to figure out the correct values...
			const char *s_mirroring_int;
			if (footer.board_info & 0x80) {
				s_mirroring_int = C_("NES|Mirroring", "Horizontal");
			} else {
				s_mirroring_int = C_("NES|Mirroring", "Vertical");
			}
			d->fields.addField_string(mirroring_title, s_mirroring_int);

			// Board type (Mapper)
			static constexpr char footer_mapper_tbl[][8] = {
				"NROM", "CNROM", "UNROM", "GNROM", "MMCx"
			};
			const unsigned int footer_mapper = (footer.board_info & 0x7F);
			string s_footer_mapper;
			if (footer_mapper < ARRAY_SIZE(footer_mapper_tbl)) {
				// tr: Print the mapper ID followed by the mapper name.
				s_footer_mapper = fmt::format(FRUN(C_("NES|Mapper", "{0:d} - {1:s}")),
					footer_mapper, footer_mapper_tbl[footer_mapper]);
			} else {
				// tr: Print only the mapper ID.
				s_footer_mapper = fmt::to_string(static_cast<unsigned int>(footer_mapper));
			}
			d->fields.addField_string(C_("NES", "Board Type"), s_footer_mapper);

			// Publisher
			const char *const publisher_title = C_("RomData", "Publisher");
			const char *const publisher =
				NintendoPublishers::lookup_old(footer.publisher_code);
			if (publisher) {
				d->fields.addField_string(publisher_title, publisher);
			} else {
				d->fields.addField_string(publisher_title,
					fmt::format(FRUN(C_("RomData", "Unknown (0x{:0>2X})")), footer.publisher_code));
			}
		}
	}

	// TODO: More fields.
	return static_cast<int>(d->fields.count());
}

/**
 * Get a list of URLs for an external image type.
 *
 * A thumbnail size may be requested from the shell.
 * If the subclass supports multiple sizes, it should
 * try to get the size that most closely matches the
 * requested size.
 *
 * @param imageType	[in]     Image type
 * @param extURLs	[out]    Output vector
 * @param size		[in,opt] Requested image size. This may be a requested
 *                               thumbnail size in pixels, or an ImageSizeType
 *                               enum value.
 * @return 0 on success; negative POSIX error code on error.
 */
int NES::extURLs(ImageType imageType, vector<ExtURL> &extURLs, int size) const
{
	extURLs.clear();
	ASSERT_extURLs(imageType);

	RP_D(NES);
	if (!d->isValid || (int)d->romType < 0) {
		// ROM image isn't valid.
		return -EIO;
	}

	// For NES ROMs, the filename consists of one or two CRC32s:
	// - First 8 KB of PRG ROM
	// - First 8 KB of CHR ROM
	// NOTE: If the title uses CHR RAM, then the second CRC32 is omitted.
	// NOTE 2: For FDS ROMs, the game ID from the disk header is used instead.
	string s_filename;
	bool isFDS = ((d->romType & NESPrivate::ROM_SYSTEM_MASK) == NESPrivate::ROM_SYSTEM_FDS);
	if (likely(!isFDS)) {
		if (unlikely(RomDataFactory::TestMode)) {
			// Cannot thumbnail NES ROM images in Test Mode.
			return -EFAULT;
		}

		int ret = d->calcRomCRC32s();
		if (ret != 0) {
			// Unable to get the ROM CRC32s.
			return ret;
		}

		// Lowercase hex CRC32s are used.
		if (likely(d->rom_8k_crc32[1] != 0)) {
			// CHR ROM is present.
			s_filename = fmt::format(FSTR("{:0>8X}-{:0>8X}"),
				d->rom_8k_crc32[0], d->rom_8k_crc32[1]);
		} else {
			// CHR ROM is not present.
			s_filename = fmt::format(FSTR("{:0>8X}"),
				d->rom_8k_crc32[0]);
		}
	} else {
		// Famicom Disk System: Use the game ID from the header.
		const auto *const header = &d->header;

		char game_id[4];
		memcpy(game_id, header->fds.game_id, 3);
		game_id[3] = '\0';

		// Verify that the game ID is valid. (uppercase letters, and numbers)
		for (unsigned int i = 0; i < 3; i++) {
			const char c = game_id[i];
			if (!isupper_ascii(c) && !isdigit_ascii(c)) {
				// Not a valid character.
				return -ENOENT;
			}
		}

		s_filename = fmt::format(FSTR("{:s}-{:s}"),
			(header->fds.disk_type == FDS_DTYPE_FSC ? "FSC" : "FMC"), game_id);
	}

	// NOTE: We only have one size for NES.
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

	// TODO: Table lookup?
	const char *sys;
	switch (d->romType & NESPrivate::ROM_SYSTEM_MASK) {
		case NESPrivate::ROM_SYSTEM_NES:
			sys = "nes";
			break;
		case NESPrivate::ROM_SYSTEM_FDS:
			sys = "fds";
			break;
		case NESPrivate::ROM_SYSTEM_VS:
			sys = "vs";
			break;
		case NESPrivate::ROM_SYSTEM_PC10:
			sys = "pc10";
			break;
		default:
			assert(!"Invalid system.");
			return -EIO;
	}

	// NOTE: If this system doesn't have mappers, '-1' will be used.
	char s_mapper[16];
	if (likely(!isFDS)) {
		snprintf(s_mapper, sizeof(s_mapper), "%d", d->get_iNES_mapper_number());
	}

	// Add the URLs.
	extURLs.resize(1);
	ExtURL &extURL = extURLs[0];
	extURL.url = d->getURL_RPDB(sys, imageTypeName, likely(!isFDS) ? s_mapper : nullptr, s_filename.c_str(), ext);
	extURL.cache_key = d->getCacheKey_RPDB(sys, imageTypeName, likely(!isFDS) ? s_mapper : nullptr, s_filename.c_str(), ext);
	extURL.width = sizeDefs[0].width;
	extURL.height = sizeDefs[0].height;
	extURL.high_res = (sizeDefs[0].index >= 2);

	// All URLs added.
	return 0;
}

} // namespace LibRomData

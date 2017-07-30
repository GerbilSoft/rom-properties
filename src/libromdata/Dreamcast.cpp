/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Dreamcast.hpp: Sega Dreamcast disc image reader.                        *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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

#include "Dreamcast.hpp"
#include "librpbase/RomData_p.hpp"

#include "data/SegaPublishers.hpp"
#include "dc_structs.h"
#include "cdrom_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <ctime>
#include <cstring>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

class DreamcastPrivate : public RomDataPrivate
{
	public:
		DreamcastPrivate(Dreamcast *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(DreamcastPrivate)

	public:
		enum DiscType {
			DISC_UNKNOWN		= -1,	// Unknown ROM type.
			DISC_ISO_2048		= 0,	// ISO-9660, 2048-byte sectors.
			DISC_ISO_2352		= 1,	// ISO-9660, 2352-byte sectors.
		};

		// Disc type.
		int discType;

		// Disc header.
		DC_IP0000_BIN_t discHeader;

		// Session start address.
		// ISO-9660 directories use physical offsets,
		// not offsets relative to the start of the track.
		unsigned int session_start_address;

		/**
		 * Calculate the Product CRC16.
		 * @param ip0000_bin IP0000.bin struct.
		 * @return Product CRC16.
		 */
		static uint16_t calcProductCRC16(const DC_IP0000_BIN_t *ip0000_bin);

		/**
		 * Convert an ASCII release date to Unix time.
		 * @param ascii_date ASCII date. (Must be at least 8 chars.)
		 * @return Unix time, or -1 if an error occurred.
		 */
		static time_t ascii_release_date_to_unix_time(const char *ascii_date);

		/**
		 * Trim spaces from the end of a string.
		 * @param str		[in] String.
		 * @param max_len	[in] Maximum length of `str`.
		 * @return String length, minus spaces.
		 */
		static inline int trim_spaces(const char *str, int max_len);
};

/** DreamcastPrivate **/

DreamcastPrivate::DreamcastPrivate(Dreamcast *q, IRpFile *file)
	: super(q, file)
	, discType(DISC_UNKNOWN)
	, session_start_address(0)
{
	// Clear the disc header struct.
	memset(&discHeader, 0, sizeof(discHeader));
}

/**
 * Calculate the Product CRC16.
 * @param ip0000_bin IP0000.bin struct.
 * @return Product CRC16.
 */
uint16_t DreamcastPrivate::calcProductCRC16(const DC_IP0000_BIN_t *ip0000_bin)
{
	unsigned int n = 0xFFFF;

	// CRC16 is for product number and version,
	// so we'll start at product number.
	const uint8_t *p = reinterpret_cast<const uint8_t*>(&ip0000_bin->product_number);
 
	for (unsigned int i = 16; i > 0; i--, p++) {
		n ^= (*p << 8);
		for (unsigned int c = 8; c > 0; c--) {
			if (n & 0x8000) {
				n = (n << 1) ^ 4129;
			} else {
				n = (n << 1);
			}
		}
	}

	return (n & 0xFFFF);
}

/**
 * Convert an ASCII release date to Unix time.
 * @param ascii_date ASCII date. (Must be at least 8 chars.)
 * @return Unix time, or -1 if an error occurred.
 */
time_t DreamcastPrivate::ascii_release_date_to_unix_time(const char *ascii_date)
{
	// Release date format: "YYYYMMDD"

	// Verify that all characters are digits.
	for (unsigned int i = 0; i < 8; i++) {
		if (!isdigit(ascii_date[i])) {
			// Invalid digit.
			return -1;
		}
	}
	// TODO: Verify that the remaining spaces are empty?

	// Convert from BCD to Unix time.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January
	struct tm dctime;

	dctime.tm_year = ((ascii_date[0] & 0xF) * 1000) +
			 ((ascii_date[1] & 0xF) * 100) +
			 ((ascii_date[2] & 0xF) * 10) +
			  (ascii_date[3] & 0xF) - 1900;
	dctime.tm_mon  = ((ascii_date[4] & 0xF) * 10) +
			  (ascii_date[5] & 0xF) - 1;
	dctime.tm_mday = ((ascii_date[6] & 0xF) * 10) +
			  (ascii_date[7] & 0xF);

	// Time portion is empty.
	dctime.tm_hour = 0;
	dctime.tm_min = 0;
	dctime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	dctime.tm_wday = 0;
	dctime.tm_yday = 0;
	dctime.tm_isdst = 0;

	// If conversion fails, this will return -1.
	return timegm(&dctime);
}

/**
 * Trim spaces from the end of a string.
 * @param str		[in] String.
 * @param max_len	[in] Maximum length of `str`.
 * @return String length, minus spaces.
 */
inline int DreamcastPrivate::trim_spaces(const char *str, int max_len)
{
	const char *p = &str[max_len-1];
	for (; max_len > 0; max_len--, p--) {
		if (*p != ' ')
			break;
	}
	return max_len;
}

/** Dreamcast **/

/**
 * Read a Sega Dreamcast disc image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be dup()'d and must be kept open in order to load
 * data from the ROM image.
 *
 * To close the file, either delete this object or call close().
 *
 * NOTE: Check isValid() to determine if this is a valid ROM.
 *
 * @param file Open ROM image.
 */
Dreamcast::Dreamcast(IRpFile *file)
	: super(new DreamcastPrivate(this, file))
{
	// This class handles disc images.
	RP_D(Dreamcast);
	d->className = "Dreamcast";
	d->fileType = FTYPE_DISC_IMAGE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the disc header.
	// NOTE: Reading 2352 bytes due to CD-ROM sector formats.
	CDROM_2352_Sector_t sector;
	d->file->rewind();
	size_t size = d->file->read(&sector, sizeof(sector));
	if (size != sizeof(sector))
		return;

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(sector);
	info.header.pData = reinterpret_cast<const uint8_t*>(&sector);
	info.ext = nullptr;	// Not needed for Dreamcast.
	info.szFile = 0;	// Not needed for Dreamcast.
	d->discType = isRomSupported_static(&info);

	if (d->discType < 0)
		return;

	switch (d->discType) {
		case DreamcastPrivate::DISC_ISO_2048:
			// 2048-byte sectors.
			// TODO: Determine session start address.
			memcpy(&d->discHeader, &sector, sizeof(d->discHeader));
			break;
		case DreamcastPrivate::DISC_ISO_2352:
			// 2352-byte sectors.
			// FIXME: Assuming Mode 1.
			d->session_start_address = cdrom_msf_to_lba(&sector.msf);
			memcpy(&d->discHeader, &sector.m1.data, sizeof(d->discHeader));
			break;
		default:
			// Unsupported.
			return;
	}
	d->isValid = true;
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Dreamcast::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(CDROM_2352_Sector_t))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for Dreamcast HW and Maker ID.

	// 0x0000: 2048-byte sectors.
	const DC_IP0000_BIN_t *ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(info->header.pData);
	if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
	    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
	{
		// Found HW and Maker IDs at 0x0000.
		// This is a 2048-byte sector image.
		return DreamcastPrivate::DISC_ISO_2048;
	}

	// 0x0010: 2352-byte sectors;
	ip0000_bin = reinterpret_cast<const DC_IP0000_BIN_t*>(&info->header.pData[0x10]);
	if (!memcmp(ip0000_bin->hw_id, DC_IP0000_BIN_HW_ID, sizeof(ip0000_bin->hw_id)) &&
	    !memcmp(ip0000_bin->maker_id, DC_IP0000_BIN_MAKER_ID, sizeof(ip0000_bin->maker_id)))
	{
		// Found HW and Maker IDs at 0x0010.
		// Verify the sync bytes.
		static const uint8_t cdrom_sync[12] =
			{0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00};
		if (!memcmp(info->header.pData, cdrom_sync, sizeof(cdrom_sync))) {
			// Found CD-ROM sync bytes.
			// This is a 2352-byte sector image.
			return DreamcastPrivate::DISC_ISO_2352;
		}
	}

	// TODO: Check for other formats, including CDI and NRG?

	// Not supported.
	return -1;
}

/**
 * Is a ROM image supported by this object?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Dreamcast::isRomSupported(const DetectInfo *info) const
{
	return isRomSupported_static(info);
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const rp_char *Dreamcast::systemName(unsigned int type) const
{
	RP_D(const Dreamcast);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// GBA has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Dreamcast::systemName() array index optimization needs to be updated.");

	static const rp_char *const sysNames[4] = {
		_RP("Sega Dreamcast"), _RP("Dreamcast"), _RP("DC"), nullptr
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
 * NOTE 2: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const rp_char *const *Dreamcast::supportedFileExtensions_static(void)
{
	static const rp_char *const exts[] = {
		_RP(".iso"),	// ISO-9660 (2048-byte)
		_RP(".bin"),	// Raw (2352-byte)

		// TODO: Add these formats?
		//_RP(".cdi"),	// DiscJuggler
		//_RP(".nrg"),	// Nero
		//_RP(".gdi"),	// GD-ROM cuesheet

		nullptr
	};
	return exts;
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
const rp_char *const *Dreamcast::supportedFileExtensions(void) const
{
	return supportedFileExtensions_static();
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Dreamcast::loadFieldData(void)
{
	RP_D(Dreamcast);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid || d->discType < 0) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Dreamcast disc header.
	const DC_IP0000_BIN_t *const discHeader = &d->discHeader;
	d->fields->reserve(8);	// Maximum of 8 fields.

	// FIXME: The CRC algorithm isn't working right...
#if 0
	// Product CRC16.
	unsigned int crc16_expected = 0;
	const char *p = discHeader->device_info;
	for (unsigned int i = 4; i > 0; i--, p++) {
		if (isxdigit(*p)) {
			crc16_expected <<= 4;
			if (isdigit(*p)) {
				crc16_expected |= (*p & 0xF);
			} else {
				crc16_expected |= ((*p & 0xF) + 10);
			}
		} else {
			// Invalid digit.
			crc16_expected = ~0;
			break;
		}
	}

	uint16_t crc16_actual = d->calcProductCRC16(discHeader);
	if (crc16_expected < 0x10000) {
		if (crc16_expected == crc16_actual) {
			// CRC16 is correct.
			d->fields->addField_string(_RP("Checksum"),
				rp_sprintf("0x%04X (valid)", crc16_expected));
		} else {
			// CRC16 is incorrect.
			d->fields->addField_string(_RP(" Checksum"),
				rp_sprintf("0x%04X (INVALID; should be 0x%04X)", crc16_expected, crc16_actual));
		}
	} else {
		// CRC16 in header is invalid.
		d->fields->addField_string(_RP("Product Checksum"),
			rp_sprintf("0x%04X (HEADER is INVALID: %.4s)", crc16_expected, discHeader->device_info));
	}
#endif

	// Disc number.
	uint8_t disc_num = 0;
	uint8_t disc_total = 0;
	if (!memcmp(&discHeader->device_info[4], " GD-ROM", 7) && discHeader->device_info[12] == '/') {
		// "GD-ROM" is present.
		if (isdigit(discHeader->device_info[11]) &&
		    isdigit(discHeader->device_info[13]))
		{
			// Disc digits are present.
			disc_num = discHeader->device_info[11] & 0x0F;
			disc_total = discHeader->device_info[13] & 0x0F;
		}
	}

	if (disc_num != 0) {
		d->fields->addField_string(_RP("Disc #"),
			rp_sprintf("%u of %u", disc_num, disc_total));
	} else {
		d->fields->addField_string(_RP("Disc #"), _RP("Unknown"));
	}

	// Region code.
	// Note that for Dreamcast, each character is assigned to
	// a specific position, so European games will be "  E",
	// not "E  ".
	unsigned int region_code = 0;
	region_code  = (discHeader->area_symbols[0] == 'J');
	region_code |= (discHeader->area_symbols[1] == 'U') << 1;
	region_code |= (discHeader->area_symbols[2] == 'E') << 2;

	static const rp_char *const region_code_bitfield_names[] = {
		_RP("Japan"), _RP("USA"), _RP("Europe")
	};
	vector<rp_string> *v_region_code_bitfield_names = RomFields::strArrayToVector(
		region_code_bitfield_names, ARRAY_SIZE(region_code_bitfield_names));
	d->fields->addField_bitfield(_RP("Region Code"),
		v_region_code_bitfield_names, 0, region_code);

	// Product number.
	int len = d->trim_spaces(discHeader->product_number, (int)sizeof(discHeader->product_number));
	d->fields->addField_string(_RP("Product #"),
		(len > 0 ? latin1_to_rp_string(discHeader->product_number, len) : _RP("Unknown")));

	// Product version.
	len = d->trim_spaces(discHeader->product_version, (int)sizeof(discHeader->product_version));
	d->fields->addField_string(_RP("Version"),
		(len > 0 ? latin1_to_rp_string(discHeader->product_version, len) : _RP("Unknown")));

	// Release date.
	time_t release_date = d->ascii_release_date_to_unix_time(discHeader->release_date);
	d->fields->addField_dateTime(_RP("Release Date"), release_date,
		RomFields::RFT_DATETIME_HAS_DATE |
		RomFields::RFT_DATETIME_IS_UTC  // Date only.
	);

	// Boot filename.
	len = d->trim_spaces(discHeader->boot_filename, (int)sizeof(discHeader->boot_filename));
	d->fields->addField_string(_RP("Boot Filename"),
		(len > 0 ? latin1_to_rp_string(discHeader->boot_filename, len) : _RP("Unknown")));

	// Publisher.
	const rp_char *publisher = nullptr;
	if (!memcmp(discHeader->publisher, DC_IP0000_BIN_MAKER_ID, sizeof(discHeader->publisher))) {
		// First-party Sega title.
		publisher = _RP("Sega");
	} else if (!memcmp(discHeader->publisher, "SEGA LC-T-", 10)) {
		// This may be a third-party T-code.
		char *endptr;
		unsigned int t_code = strtoul(&discHeader->publisher[10], &endptr, 10);
		if (t_code != 0 &&
		    endptr > &discHeader->publisher[10] &&
		    endptr <= &discHeader->publisher[15])
		{
			// Valid T-code. Look up the publisher.
			publisher = SegaPublishers::lookup(t_code);
		}
	}

	if (publisher) {
		d->fields->addField_string(_RP("Publisher"), publisher);
	} else {
		// Unknown publisher.
		// List the field as-is.
		len = d->trim_spaces(discHeader->publisher, (int)sizeof(discHeader->publisher));
		d->fields->addField_string(_RP("Publisher"),
			(len > 0 ? latin1_to_rp_string(discHeader->publisher, len) : _RP("Unknown")));
	}

	// Title. (TODO: Encoding?)
	len = d->trim_spaces(discHeader->title, (int)sizeof(discHeader->title));
	d->fields->addField_string(_RP("Title"),
		(len > 0 ? latin1_to_rp_string(discHeader->title, len) : _RP("Unknown")));

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

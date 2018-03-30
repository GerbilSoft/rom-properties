/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Sega8Bit.cpp: Sega 8-bit (SMS/GG) ROM reader.                           *
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

#include "Sega8Bit.hpp"
#include "librpbase/RomData_p.hpp"

#include "sega8_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ctime>

// C++ includes.
#include <vector>
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(Sega8Bit)

class Sega8BitPrivate : public RomDataPrivate
{
	public:
		Sega8BitPrivate(Sega8Bit *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Sega8BitPrivate)

	public:
		// ROM header. (0x7FE0-0x7FFF)
		struct {
			union {
				char m404_copyright[16];
				Sega8_Codemasters_RomHeader codemasters;
				Sega8_SDSC_RomHeader sdsc;
			};
			Sega8_RomHeader tmr;
		} romHeader;

		/**
		 * Add an SDSC string field.
		 * @param name Field name.
		 * @param ptr SDSC string pointer.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int addField_string_sdsc(const char *name, uint16_t ptr);
};

/** Sega8BitPrivate **/

Sega8BitPrivate::Sega8BitPrivate(Sega8Bit *q, IRpFile *file)
	: super(q, file)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Add an SDSC string field.
 * @param name Field name.
 * @param ptr SDSC string pointer.
 * @return 0 on success; negative POSIX error code on error.
 */
int Sega8BitPrivate::addField_string_sdsc(const char *name, uint16_t ptr)
{
	assert(file != nullptr);
	assert(file->isOpen());
	assert(isValid);
	if (!file || !file->isOpen() || !isValid) {
		// Can't add anything...
		return -EBADF;
	}

	if (ptr == 0x0000 || ptr == 0xFFFF) {
		// No string here...
		return 0;
	}

	char strbuf[256];
	size_t size = file->seekAndRead(ptr, strbuf, sizeof(strbuf));
	if (size > 0 && size <= sizeof(strbuf)) {
		// NOTE: SDSC documentation says these strings should be ASCII.
		// Since SDSC was introduced in 2001, I'll interpret them as cp1252.
		// Reference: http://www.smspower.org/Development/SDSCHeader#SDSC7fe04BytesASCII
		fields->addField_string(name,
			cp1252_to_utf8(strbuf, sizeof(strbuf)));
	}
	return 0;
}

/** Sega8Bit **/

/**
 * Read a Sega 8-bit (SMS/GG) ROM image.
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
Sega8Bit::Sega8Bit(IRpFile *file)
	: super(new Sega8BitPrivate(this, file))
{
	RP_D(Sega8Bit);
	d->className = "Sega8Bit";

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the ROM header.
	size_t size = d->file->seekAndRead(0x7FE0, &d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		// Seek and/or read error.
		return;
	}

	// Check if this ROM image is supported.
	DetectInfo info;
	info.header.addr = 0x7FE0;
	info.header.size = sizeof(d->romHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->romHeader);
	info.ext = nullptr;	// Not needed for Sega 8-bit.
	info.szFile = d->file->size();
	d->isValid = (isRomSupported_static(&info) >= 0);
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int Sega8Bit::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	if (!info || !info->header.pData)
		return -1;

	// Header data must contain 0x7FF0-0x7FFF.
	static const unsigned int header_addr_expected = 0x7FF0;
	if (info->szFile < header_addr_expected ||
	    info->header.addr > header_addr_expected ||
	    info->header.addr + info->header.size < header_addr_expected + 0x10)
	{
		// Header is out of range.
		return -1;
	}

	// Determine the offset.
	const unsigned int offset = header_addr_expected - info->header.addr;

	// Get the ROM header.
	const Sega8_RomHeader *const romHeader =
		reinterpret_cast<const Sega8_RomHeader*>(&info->header.pData[offset]);

	// Check "TMR SEGA".
	// TODO: Codemasters and SDSC checks.
	if (!memcmp(romHeader->magic, SEGA8_MAGIC, sizeof(romHeader->magic))) {
		// This is a Sega 8-bit ROM image.
		return 0;
	}

	// Not supported.
	return -1;
}

/**
 * Get the name of the system the loaded ROM is designed for.
 * @param type System name type. (See the SystemName enum.)
 * @return System name, or nullptr if type is invalid.
 */
const char *Sega8Bit::systemName(unsigned int type) const
{
	RP_D(const Sega8Bit);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// TODO: Region-specific variants.
	// Also SMS vs. GG.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"Sega8Bit::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Sega Master System",
		"Master System",
		"SMS",
		nullptr
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
const char *const *Sega8Bit::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".sms",	// Sega Master System
		".gg",	// Sega Game Gear
		// TODO: Other Sega 8-bit formats?

		nullptr
	};
	return exts;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Sega8Bit::loadFieldData(void)
{
	RP_D(Sega8Bit);
	if (d->fields->isDataLoaded()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	// Sega 8-bit ROM header. (TMR SEGA)
	const Sega8_RomHeader *const tmr = &d->romHeader.tmr;
	d->fields->reserve(11);	// Maximum of 11 fields.

	// BCD conversion buffer.
	char bcdbuf[16];

	// Product code. (little-endian BCD)
	char *p = bcdbuf;
	if (tmr->product_code[2] & 0xF0) {
		// Fifth digit is present.
		uint8_t digit = (tmr->product_code[2] >> 4) & 0xF;
		if (digit < 10) {
			*p++ = ('0' + digit);
		} else {
			p[0] = '1';
			p[1] = ('0' + digit - 10);
			p += 2;
		}
	}

	// Convert the product code to BCD.
	// NOTE: Little-endian BCD; first byte is the *second* set of digits.
	// TODO: Check for invalid BCD digits?
	p[0] = ('0' + ((tmr->product_code[1] >> 4) & 0xF));
	p[1] = ('0' +  (tmr->product_code[1] & 0xF));
	p[2] = ('0' + ((tmr->product_code[0] >> 4) & 0xF));
	p[3] = ('0' +  (tmr->product_code[0] & 0xF));
	p[4] = 0;
	d->fields->addField_string(C_("Sega8Bit", "Product Code"), bcdbuf);

	// Version.
	uint8_t digit = tmr->product_code[2] & 0xF;
	if (digit < 10) {
		bcdbuf[0] = ('0' + digit);
		bcdbuf[1] = 0;
	} else {
		bcdbuf[0] = '1';
		bcdbuf[1] = ('0' + digit - 10);
		bcdbuf[2] = 0;
	}
	d->fields->addField_string(C_("Sega8Bit", "Version"), bcdbuf);

	// Region code and system ID.
	const char *sysID;
	const char *region;
	switch ((tmr->region_and_size >> 4) & 0xF) {
		case Sega8_SMS_Japan:
			sysID = C_("Sega8Bit|SysID", "Sega Master System");
			region = C_("Sega8Bit|SysID", "Japan");
			break;
		case Sega8_SMS_Export:
			sysID = C_("Sega8Bit|SysID", "Sega Master System");
			region = C_("Sega8Bit|SysID", "Export");
			break;
		case Sega8_GG_Japan:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			region = C_("Sega8Bit|SysID", "Japan");
			break;
		case Sega8_GG_Export:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			region = C_("Sega8Bit|SysID", "Export");
			break;
		case Sega8_GG_International:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			region = C_("Sega8Bit|SysID", "International");
			break;
		default:
			sysID = nullptr;
			region = nullptr;
	}
	d->fields->addField_string(C_("Sega8Bit", "System"),
		(sysID ? sysID : C_("Sega8Bit", "Unknown")));
	d->fields->addField_string(C_("Sega8Bit", "Region Code"),
		(region ? region : C_("Sega8Bit", "Unknown")));

	// Checksum.
	d->fields->addField_string_numeric(C_("Sega8Bit", "Checksum"),
		le16_to_cpu(tmr->checksum), RomFields::FB_HEX, 4,
		RomFields::STRF_MONOSPACE);

	// TODO: ROM size?

	// Check for other headers.
	// TODO: SDSC header.
	if (0x10000 - (uint32_t)le16_to_cpu(d->romHeader.codemasters.checksum) ==
	    (uint32_t)le16_to_cpu(d->romHeader.codemasters.checksum_compl))
	{
		// Codemasters checksums match.
		const Sega8_Codemasters_RomHeader *const codemasters = &d->romHeader.codemasters;
		d->fields->addField_string(C_("Sega8Bit", "Extra Header"), "Codemasters");

		// Convert date/time from BCD.
		// NOTE: struct tm has some oddities:
		// - tm_year: year - 1900
		// - tm_mon: 0 == January

		// TODO: Check for invalid BCD values.
		struct tm cmtime;
		cmtime.tm_year = ((codemasters->timestamp.year >> 4) * 10) +
				  (codemasters->timestamp.year & 0x0F);
		if (cmtime.tm_year < 80) {
			// Assume date values lower than 80 are 2000+.
			cmtime.tm_year += 100;
		}
		cmtime.tm_mon  = ((codemasters->timestamp.month >> 4) * 10) +
				  (codemasters->timestamp.month & 0x0F) - 1;
		cmtime.tm_mday = ((codemasters->timestamp.day >> 4) * 10) +
				  (codemasters->timestamp.day & 0x0F);
		cmtime.tm_hour = ((codemasters->timestamp.hour >> 4) * 10) +
				  (codemasters->timestamp.hour & 0x0F);
		cmtime.tm_min  = ((codemasters->timestamp.minute >> 4) * 10) +
				  (codemasters->timestamp.minute & 0x0F);
		cmtime.tm_sec = 0;

		// tm_wday and tm_yday are output variables.
		cmtime.tm_wday = 0;
		cmtime.tm_yday = 0;
		cmtime.tm_isdst = 0;

		// If conversion fails, d->ctime will be set to -1.
		time_t ctime = timegm(&cmtime);

		// TODO: Interpret dateTime of -1 as "error"?
		d->fields->addField_dateTime(C_("Sega8Bit", "Build Time"), ctime,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME |
			RomFields::RFT_DATETIME_IS_UTC  // No timezone information here.
		);

		// Checksum.
		d->fields->addField_string_numeric(C_("Sega8Bit", "CM Checksum Banks"),
			codemasters->checksum_banks);
		d->fields->addField_string_numeric(C_("Sega8Bit", "CM Checksum 1"),
			le16_to_cpu(codemasters->checksum),
			RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);
		d->fields->addField_string_numeric(C_("Sega8Bit", "CM Checksum 2"),
			le16_to_cpu(codemasters->checksum_compl),
			RomFields::FB_HEX, 4, RomFields::STRF_MONOSPACE);
	} else if (!memcmp(d->romHeader.sdsc.magic, SDSC_MAGIC, 4)) {
		// SDSC header magic.
		const Sega8_SDSC_RomHeader *sdsc = &d->romHeader.sdsc;
		d->fields->addField_string(C_("Sega8Bit", "Extra Header"), "SDSC");

		// Version number. Stored as two BCD values, major.minor.
		// TODO: Verify BCD.
		p = bcdbuf;
		if (sdsc->version[0] > 9) {
			*p++ = ('0' + (sdsc->version[0] >> 4));
		}
		p[0] = ('0' + (sdsc->version[0] & 0x0F));
		p[1] = '.';
		p[2] = ('0' + (sdsc->version[1] >> 4));
		p[3] = ('0' + (sdsc->version[1] & 0x0F));
		p[4] = 0;
		d->fields->addField_string(C_("Sega8Bit", "SDSC Version"), bcdbuf);

		// Build date.

		// Convert date/time from BCD.
		// NOTE: struct tm has some oddities:
		// - tm_year: year - 1900
		// - tm_mon: 0 == January

		// TODO: Check for invalid BCD values.
		struct tm cmtime;
		cmtime.tm_year = ((sdsc->date.century >> 4) * 1000) +
				 ((sdsc->date.century & 0x0F) * 100) +
				 ((sdsc->date.year >> 4) * 10) +
				  (sdsc->date.year & 0x0F) - 1900;
		cmtime.tm_mon  = ((sdsc->date.month >> 4) * 10) +
				  (sdsc->date.month & 0x0F) - 1;
		cmtime.tm_mday = ((sdsc->date.day >> 4) * 10) +
				  (sdsc->date.day & 0x0F);
		cmtime.tm_hour = 0;
		cmtime.tm_min  = 0;
		cmtime.tm_sec = 0;

		// tm_wday and tm_yday are output variables.
		cmtime.tm_wday = 0;
		cmtime.tm_yday = 0;
		cmtime.tm_isdst = 0;

		// If conversion fails, d->ctime will be set to -1.
		time_t ctime = timegm(&cmtime);

		// TODO: Interpret dateTime of -1 as "error"?
		d->fields->addField_dateTime(C_("Sega8Bit", "Build Date"), ctime,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC  // No timezone information here.
		);

		// SDSC string fields.
		d->addField_string_sdsc(C_("Sega8Bit", "Author"), le16_to_cpu(sdsc->author_ptr));
		d->addField_string_sdsc(C_("Sega8Bit", "Name"), le16_to_cpu(sdsc->name_ptr));
		d->addField_string_sdsc(C_("Sega8Bit", "Description"), le16_to_cpu(sdsc->desc_ptr));

	} else if (!memcmp(d->romHeader.m404_copyright, "COPYRIGHT SEGA", 14) ||
		   !memcmp(d->romHeader.m404_copyright, "COPYRIGHTSEGA", 13))
	{
		// Sega Master System M404 prototype copyright.
		d->fields->addField_string(C_("Sega8Bit", "Extra Header"),
			C_("Sega8Bit", "M404 Copyright Header"));
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

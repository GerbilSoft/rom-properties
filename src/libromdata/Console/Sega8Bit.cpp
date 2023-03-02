/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Sega8Bit.cpp: Sega 8-bit (SMS/GG) ROM reader.                           *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Sega8Bit.hpp"
#include "sega8_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpText;
using LibRpFile::IRpFile;

// C++ STL classes
using std::string;
using std::vector;

namespace LibRomData {

class Sega8BitPrivate final : public RomDataPrivate
{
	public:
		Sega8BitPrivate(Sega8Bit *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(Sega8BitPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

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
		 * Get an SDSC string field.
		 * @param ptr SDSC string pointer.
		 * @return SDSC string on success; empty string on error.
		 */
		string getSdscString(uint16_t ptr);

		/**
		 * Convert a Codemasters timestamp to a Unix timestamp.
		 * @param timestamp Codemasters timestamp.
		 * @return Unix timestamp, or -1 on error.
		 */
		static time_t codemasters_timestamp_to_unix_time(const Sega8_Codemasters_Timestamp *timestamp);

		/**
		 * Convert an SDSC build date to a Unix timestamp.
		 * @param date SDSC build date.
		 * @return Unix timestamp, or -1 on error.
		 */
		static time_t sdsc_date_to_unix_time(const Sega8_SDSC_Date *date);
};

ROMDATA_IMPL(Sega8Bit)

/** Sega8BitPrivate **/

/* RomDataInfo */
const char *const Sega8BitPrivate::exts[] = {
	".sms",	// Sega Master System
	".gg",	// Sega Game Gear
	// TODO: Other Sega 8-bit formats?

	nullptr
};
const char *const Sega8BitPrivate::mimeTypes[] = {
	// Unofficial MIME types from FreeDesktop.org.
	"application/x-sms-rom",
	"application/x-gamegear-rom",

	nullptr
};
const RomDataInfo Sega8BitPrivate::romDataInfo = {
	"Sega8Bit", exts, mimeTypes
};

Sega8BitPrivate::Sega8BitPrivate(Sega8Bit *q, IRpFile *file)
	: super(q, file, &romDataInfo)
{
	// Clear the ROM header struct.
	memset(&romHeader, 0, sizeof(romHeader));
}

/**
 * Get an SDSC string field.
 * @param ptr SDSC string pointer.
 * @return SDSC string on success; empty string on error.
 */
string Sega8BitPrivate::getSdscString(uint16_t ptr)
{
	assert(file != nullptr);
	assert(file->isOpen());
	assert(isValid);
	if (!file || !file->isOpen() || !isValid) {
		// Can't add anything...
		return string();
	}

	if (ptr == 0x0000 || ptr == 0xFFFF) {
		// No string here...
		return string();
	}

	char strbuf[256];
	size_t size = file->seekAndRead(ptr, strbuf, sizeof(strbuf));
	if (size > 0 && size <= sizeof(strbuf)) {
		// NOTE: SDSC documentation says these strings should be ASCII.
		// Since SDSC was introduced in 2001, I'll interpret them as cp1252.
		// Reference: http://www.smspower.org/Development/SDSCHeader#SDSC7fe04BytesASCII
		return cp1252_to_utf8(strbuf, sizeof(strbuf));
	}

	// Unable to read the string...
	return string();
}

/**
 * Convert a Codemasters timestamp to a Unix timestamp.
 * @param timestamp Codemasters timestamp.
 * @return Unix timestamp, or -1 on error.
 */
time_t Sega8BitPrivate::codemasters_timestamp_to_unix_time(const Sega8_Codemasters_Timestamp *timestamp)
{
	// Convert date/time from BCD.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January

	// TODO: Check for invalid BCD values.
	struct tm cmtime;
	cmtime.tm_year = ((timestamp->year >> 4) * 10) +
			  (timestamp->year & 0x0F);
	if (cmtime.tm_year < 80) {
		// Assume date values lower than 80 are 2000+.
		cmtime.tm_year += 100;
	}
	cmtime.tm_mon  = ((timestamp->month >> 4) * 10) +
			  (timestamp->month & 0x0F) - 1;
	cmtime.tm_mday = ((timestamp->day >> 4) * 10) +
			  (timestamp->day & 0x0F);
	cmtime.tm_hour = ((timestamp->hour >> 4) * 10) +
			  (timestamp->hour & 0x0F);
	cmtime.tm_min  = ((timestamp->minute >> 4) * 10) +
			  (timestamp->minute & 0x0F);
	cmtime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	cmtime.tm_wday = 0;
	cmtime.tm_yday = 0;
	cmtime.tm_isdst = 0;

	// If conversion fails, d->ctime will be set to -1.
	return timegm(&cmtime);
}

/**
 * Convert an SDSC build date to a Unix timestamp.
 * @param date SDSC build date.
 * @return Unix timestamp, or -1 on error.
 */
time_t Sega8BitPrivate::sdsc_date_to_unix_time(const Sega8_SDSC_Date *date)
{
	// Convert date/time from BCD.
	// NOTE: struct tm has some oddities:
	// - tm_year: year - 1900
	// - tm_mon: 0 == January

	// NOTE: Some ROM images have the Century value set to 0x02 instead of 0x20:
	// - Interactive Sprite Test (PD).sms
	// - GG Hi-Res Graphics Demo by Charles McDonald (PD).gg
	unsigned int century = ((date->century >> 4) * 1000) +
			       ((date->century & 0x0F) * 100);
	if (century == 200)
		century = 2000;

	// TODO: Check for invalid BCD values.
	struct tm sdsctime;
	sdsctime.tm_year = century +
			   ((date->year >> 4) * 10) +
			    (date->year & 0x0F) - 1900;
	sdsctime.tm_mon  = ((date->month >> 4) * 10) +
			    (date->month & 0x0F) - 1;
	sdsctime.tm_mday = ((date->day >> 4) * 10) +
			    (date->day & 0x0F);
	sdsctime.tm_hour = 0;
	sdsctime.tm_min  = 0;
	sdsctime.tm_sec = 0;

	// tm_wday and tm_yday are output variables.
	sdsctime.tm_wday = 0;
	sdsctime.tm_yday = 0;
	sdsctime.tm_isdst = 0;

	// If conversion fails, d->ctime will be set to -1.
	return timegm(&sdsctime);
}

/** Sega8Bit **/

/**
 * Read a Sega 8-bit (SMS/GG) ROM image.
 *
 * A ROM image must be opened by the caller. The file handle
 * will be ref()'d and must be kept open in order to load
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
	d->mimeType = "application/x-sms-rom";	// unofficial (TODO: SMS vs. GG)

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the ROM header.
	size_t size = d->file->seekAndRead(0x7FE0, &d->romHeader, sizeof(d->romHeader));
	if (size != sizeof(d->romHeader)) {
		// Seek and/or read error.
		UNREF_AND_NULL_NOCHK(d->file);
		return;
	}

	// Check if this ROM image is supported.
	const DetectInfo info = {
		{0x7FE0, sizeof(d->romHeader), reinterpret_cast<const uint8_t*>(&d->romHeader)},
		nullptr,	// ext (not needed for Sega8Bit)
		d->file->size()	// szFile
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
	}
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
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int Sega8Bit::loadFieldData(void)
{
	RP_D(Sega8Bit);
	if (!d->fields.empty()) {
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
	d->fields.reserve(11);	// Maximum of 11 fields.

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
	d->fields.addField_string(C_("Sega8Bit", "Product Code"), bcdbuf);

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
	d->fields.addField_string(C_("RomData", "Version"), bcdbuf);

	// Region code and system ID.
	const char *sysID;
	const char *region;
	switch ((tmr->region_and_size >> 4) & 0xF) {
		case Sega8_SMS_Japan:
			sysID = C_("Sega8Bit|SysID", "Sega Master System");
			region = C_("Region", "Japan");
			break;
		case Sega8_SMS_Export:
			sysID = C_("Sega8Bit|SysID", "Sega Master System");
			// tr: Any region that isn't Japan. (used for Sega 8-bit)
			region = C_("Region", "Export");
			break;
		case Sega8_GG_Japan:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			region = C_("Region", "Japan");
			break;
		case Sega8_GG_Export:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			// tr: Any region that isn't Japan. (used for Sega 8-bit)
			region = C_("Region", "Export");
			break;
		case Sega8_GG_International:
			sysID = C_("Sega8Bit|SysID", "Game Gear");
			// tr: Effectively region-free.
			region = C_("Region", "Worldwide");
			break;
		default:
			sysID = nullptr;
			region = nullptr;
	}
	d->fields.addField_string(C_("Sega8Bit", "System"),
		(sysID ? sysID : C_("RomData", "Unknown")));
	d->fields.addField_string(C_("RomData", "Region Code"),
		(region ? region : C_("RomData", "Unknown")));

	// Checksum.
	d->fields.addField_string_numeric(C_("RomData", "Checksum"),
		le16_to_cpu(tmr->checksum), RomFields::Base::Hex, 4,
		RomFields::STRF_MONOSPACE);

	// TODO: ROM size?

	// Check for other headers.
	if (0x10000 - static_cast<uint32_t>(le16_to_cpu(d->romHeader.codemasters.checksum)) ==
	    static_cast<uint32_t>(le16_to_cpu(d->romHeader.codemasters.checksum_compl)))
	{
		// Codemasters checksums match.
		const Sega8_Codemasters_RomHeader *const codemasters = &d->romHeader.codemasters;
		d->fields.addField_string(C_("Sega8Bit", "Extra Header"), "Codemasters");

		// Build time.
		// NOTE: CreationDate is currently handled as QDate on KDE.
		time_t ctime = d->codemasters_timestamp_to_unix_time(&codemasters->timestamp);

		d->fields.addField_dateTime(C_("Sega8Bit", "Build Time"), ctime,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_HAS_TIME |
			RomFields::RFT_DATETIME_IS_UTC  // No timezone information here.
		);

		// Checksum.
		d->fields.addField_string_numeric(C_("Sega8Bit", "CM Checksum Banks"),
			codemasters->checksum_banks);
		d->fields.addField_string_numeric(C_("Sega8Bit", "CM Checksum 1"),
			le16_to_cpu(codemasters->checksum),
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
		d->fields.addField_string_numeric(C_("Sega8Bit", "CM Checksum 2"),
			le16_to_cpu(codemasters->checksum_compl),
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	} else if (d->romHeader.sdsc.magic == cpu_to_be32(SDSC_MAGIC)) {
		// SDSC header magic.
		const Sega8_SDSC_RomHeader *sdsc = &d->romHeader.sdsc;
		d->fields.addField_string(C_("Sega8Bit", "Extra Header"), "SDSC");

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
		d->fields.addField_string(C_("Sega8Bit", "SDSC Version"), bcdbuf);

		// Build date.
		time_t ctime = d->sdsc_date_to_unix_time(&sdsc->date);

		d->fields.addField_dateTime(C_("Sega8Bit", "Build Date"), ctime,
			RomFields::RFT_DATETIME_HAS_DATE |
			RomFields::RFT_DATETIME_IS_UTC  // No timezone information here.
		);

		// SDSC string fields.
		d->fields.addField_string(C_("RomData", "Author"),
			d->getSdscString(le16_to_cpu(sdsc->author_ptr)));
		d->fields.addField_string(C_("RomData", "Name"),
			d->getSdscString(le16_to_cpu(sdsc->name_ptr)));
		d->fields.addField_string(C_("RomData", "Description"),
			d->getSdscString(le16_to_cpu(sdsc->desc_ptr)));
	} else if (!memcmp(d->romHeader.m404_copyright, "COPYRIGHT SEGA", 14) ||
		   !memcmp(d->romHeader.m404_copyright, "COPYRIGHTSEGA", 13))
	{
		// Sega Master System M404 prototype copyright.
		d->fields.addField_string(C_("Sega8Bit", "Extra Header"),
			C_("Sega8Bit", "M404 Copyright Header"));
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int Sega8Bit::loadMetaData(void)
{
	RP_D(Sega8Bit);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown ROM image type.
		return -EIO;
	}

	if (0x10000 - static_cast<uint32_t>(le16_to_cpu(d->romHeader.codemasters.checksum)) ==
	    static_cast<uint32_t>(le16_to_cpu(d->romHeader.codemasters.checksum_compl)))
	{
		// Codemasters checksums match.
		d->metaData = new RomMetaData();
		d->metaData->reserve(1);	// Maximum of 1 metadata property.
		const Sega8_Codemasters_RomHeader *const codemasters = &d->romHeader.codemasters;

		// Build time.
		// NOTE: CreationDate is currently handled as QDate on KDE.
		time_t ctime = d->codemasters_timestamp_to_unix_time(&codemasters->timestamp);
		d->metaData->addMetaData_timestamp(Property::CreationDate, ctime);
	} else if (d->romHeader.sdsc.magic == cpu_to_be32(SDSC_MAGIC)) {
		// SDSC header is present.
		d->metaData = new RomMetaData();
		d->metaData->reserve(4);	// Maximum of 4 metadata properties.
		const Sega8_SDSC_RomHeader *const sdsc = &d->romHeader.sdsc;

		// Build date
		time_t ctime = d->sdsc_date_to_unix_time(&sdsc->date);
		d->metaData->addMetaData_timestamp(Property::CreationDate, ctime);

		// Author
		string str = d->getSdscString(le16_to_cpu(sdsc->author_ptr));
		if (!str.empty()) {
			d->metaData->addMetaData_string(Property::Author, str);
		}

		// Name (Title)
		str = d->getSdscString(le16_to_cpu(sdsc->name_ptr));
		if (!str.empty()) {
			d->metaData->addMetaData_string(Property::Title, str);
		}

		// Description
                str = d->getSdscString(le16_to_cpu(sdsc->desc_ptr));
		if (!str.empty()) {
			d->metaData->addMetaData_string(Property::Description, str);
		}
	}

	// Finished reading the metadata.
	return (d->metaData ? static_cast<int>(d->metaData->count()) : -ENOENT);
}

}

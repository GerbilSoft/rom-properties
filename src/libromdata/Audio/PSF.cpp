/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSF.hpp: PSF audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2019 by David Korth.                                 *
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

#include "PSF.hpp"
#include "librpbase/RomData_p.hpp"

#include "psf_structs.h"

// librpbase
#include "librpbase/common.h"
#include "librpbase/byteswap.h"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/IRpFile.hpp"
#include "libi18n/i18n.h"
using namespace LibRpBase;

// C includes. (C++ namespace)
#include "librpbase/ctypex.h"
#include <cassert>
#include <cerrno>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(PSF)

class PSFPrivate : public RomDataPrivate
{
	public:
		PSFPrivate(PSF *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(PSFPrivate)

	public:
		// PSF header.
		// NOTE: **NOT** byteswapped in memory.
		PSF_Header psfHeader;

		/**
		 * Parse the tag section.
		 * @param tag_addr Tag section starting address.
		 * @return Map containing key/value entries.
		 */
		unordered_map<string, string> parseTags(int64_t tag_addr);

		/**
		 * Get the "ripped by" tag name for the specified PSF version.
		 * @param version PSF version.
		 * @return "Ripped by" tag name.
		 */
		const char *getRippedByTagName(uint8_t version);

		/**
		 * Convert a PSF length string to milliseconds.
		 * @param str PSF length string.
		 * @return Milliseconds.
		 */
		static unsigned int lengthToMs(const char *str);
};

/** PSFPrivate **/

PSFPrivate::PSFPrivate(PSF *q, IRpFile *file)
	: super(q, file)
{
	// Clear the PSF header struct.
	memset(&psfHeader, 0, sizeof(psfHeader));
}

/**
 * Parse the tag section.
 * @param tag_addr Tag section starting address.
 * @return Map containing key/value entries.
 */
unordered_map<string, string> PSFPrivate::parseTags(int64_t tag_addr)
{
	unordered_map<string, string> kv;

	// Read the tag magic first.
	char tag_magic[sizeof(PSF_TAG_MAGIC)-1];
	size_t size = file->seekAndRead(tag_addr, tag_magic, sizeof(tag_magic));
	if (size != sizeof(tag_magic)) {
		// Seek and/or read error.
		return kv;
	}

	// Read the rest of the file.
	// NOTE: Maximum of 16 KB.
	int64_t data_len = file->size() - tag_addr - sizeof(tag_magic);
	if (data_len <= 0) {
		// Not enough data...
		return kv;
	}

	// NOTE: Values may be encoded as either cp1252/sjis or UTF-8.
	// Since we won't be able to determine this until we're finished
	// decoding variables, we'll have to do character conversion
	// *after* kv is populated.
	unique_ptr<char[]> tag_data(new char[data_len]);
	size = file->read(tag_data.get(), data_len);
	if (size != (size_t)data_len) {
		// Read error.
		return kv;
	}

	bool isUtf8 = false;
	const char *start = tag_data.get();
	const char *const endptr = start + data_len;
	for (const char *p = start; p < endptr; p++) {
		// Find the next newline.
		const char *nl = static_cast<const char*>(memchr(p, '\n', endptr-p));
		if (!nl) {
			// No newline. Assume this is the end of the tag section,
			// and read up to the end.
			nl = endptr;
		}
		if (p == nl) {
			// Empty line.
			continue;
		}

		// Find the equals sign.
		const char *eq = static_cast<const char*>(memchr(p, '=', nl-p));
		if (eq) {
			// Found the equals sign.
			int k_len = (int)(eq - p);
			int v_len = (int)(nl - eq - 1);
			if (k_len > 0 && v_len > 0) {
				// Key and value are valid.
				// NOTE: Key is case-insensitive, so convert to lowercase.
				// NOTE: Key *must* be ASCII.
				string key(p, k_len);
				std::transform(key.begin(), key.end(), key.begin(), ::tolower);
				kv.insert(std::make_pair(key, string(eq+1, v_len)));

				// Check for UTF-8.
				// NOTE: v_len check is redundant...
				if (key == "utf8" && v_len > 0) {
					// "utf8" key with non-empty value.
					// This is UTF-8.
					isUtf8 = true;
				}
			}
		}

		// Next line.
		p = nl;
	}

	// If we're not using UTF-8, convert the values.
	if (!isUtf8) {
		for (auto iter = kv.begin(); iter != kv.end(); ++iter) {
			iter->second = cp1252_sjis_to_utf8(iter->second);
		}
	}

	return kv;
}

/**
 * Get the "ripped by" tag name for the specified PSF version.
 * @param version PSF version.
 * @return "Ripped by" tag name.
 */
const char *PSFPrivate::getRippedByTagName(uint8_t version)
{
	switch (version) {
		default:
		case PSF_VERSION_PLAYSTATION:
		case PSF_VERSION_PLAYSTATION_2:
			return "psfby";
		case PSF_VERSION_SATURN:
			return "ssfby";
		case PSF_VERSION_DREAMCAST:
			return "dsfby";
		case PSF_VERSION_MEGA_DRIVE:
			return "msfby";	// FIXME: May be incorrect.
		case PSF_VERSION_N64:
			return "usfby";
		case PSF_VERSION_GBA:
			return "gsfby";
		case PSF_VERSION_SNES:
			return "snsfby";
		case PSF_VERSION_QSOUND:
			return "qsfby";
	}
}

/**
 * Convert a PSF length string to milliseconds.
 * @param str PSF length string.
 * @return Milliseconds.
 */
unsigned int PSFPrivate::lengthToMs(const char *str)
{
	/**
	 * Possible formats:
	 * - seconds.decimal
	 * - minutes:seconds.decimal
	 * - hours:minutes:seconds.decimal
	 *
	 * Decimal may be omitted.
	 * Commas are also accepted.
	 */

	// TODO: Verify 'frac' length.
	// All fractional portions observed thus far are
	// three digits (milliseconds).
	unsigned int hour, min, sec, frac;

	// Check the 'frac' length.
	unsigned int frac_adj = 0;
	const char *dp = strchr(str, '.');
	if (!dp) {
		dp = strchr(str, ',');
	}
	if (dp) {
		// Found the decimal point.
		// Count how many digits are after it.
		unsigned int digit_count = 0;
		dp++;
		for (; *dp != '\0'; dp++) {
			if (ISDIGIT(*dp)) {
				// Found a digit.
				digit_count++;
			} else {
				// Not a digit.
				break;
			}
		}
		switch (digit_count) {
			case 0:
				// No digits.
				frac_adj = 0;
				break;
			case 1:
				// One digit. (tenths)
				frac_adj = 100;
				break;
			case 2:
				// Two digits. (hundredths)
				frac_adj = 10;
				break;
			case 3:
				// Three digits. (thousandths)
				frac_adj = 1;
				break;
			default:
				// Too many digits...
				// TODO: Mask these digits somehow.
				frac_adj = 1;
				break;
		}
	}

	// hours:minutes:seconds.decimal
	int s = sscanf(str, "%u:%u:%u.%u", &hour, &min, &sec, &frac);
	if (s != 4) {
		s = sscanf(str, "%u:%u:%u,%u", &hour, &min, &sec, &frac);
	}
	if (s == 4) {
		// Format matched.
		return (hour * 60 * 60 * 1000) +
		       (min * 60 * 1000) +
		       (sec * 1000) +
		       (frac * frac_adj);
	}

	// hours:minutes:seconds
	s = sscanf(str, "%u:%u:%u", &hour, &min, &sec);
	if (s == 3) {
		// Format matched.
		return (hour * 60 * 60 * 1000) +
		       (min * 60 * 1000) +
		       (sec * 1000);
	}

	// minutes:seconds.decimal
	s = sscanf(str, "%u:%u.%u", &min, &sec, &frac);
	if (s != 3) {
		s = sscanf(str, "%u:%u,%u", &min, &sec, &frac);
	}
	if (s == 3) {
		// Format matched.
		return (min * 60 * 1000) +
		       (sec * 1000) +
		       (frac * frac_adj);
	}

	// minutes:seconds
	s = sscanf(str, "%u:%u", &min, &sec);
	if (s == 2) {
		// Format matched.
		return (min * 60 * 1000) +
		       (sec * 1000);
	}

	// seconds.decimal
	s = sscanf(str, "%u.%u", &sec, &frac);
	if (s != 2) {
		s = sscanf(str, "%u,%u", &sec, &frac);
	}
	if (s == 2) {
		// Format matched.
		return (min * 60 * 1000) +
		       (sec * 1000) +
		       (frac * frac_adj);
	}

	// seconds
	s = sscanf(str, "%u", &sec);
	if (s == 1) {
		// Format matched.
		return sec;
	}

	// No matches.
	return 0;
}

/** PSF **/

/**
 * Read an PSF audio file.
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
PSF::PSF(IRpFile *file)
	: super(new PSFPrivate(this, file))
{
	RP_D(PSF);
	d->className = "PSF";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the PSF header.
	d->file->rewind();
	size_t size = d->file->read(&d->psfHeader, sizeof(d->psfHeader));
	if (size != sizeof(d->psfHeader)) {
		d->file->unref();
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->psfHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->psfHeader);
	info.ext = nullptr;	// Not needed for PSF.
	info.szFile = 0;	// Not needed for PSF.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file->unref();
		d->file = nullptr;
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int PSF::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(PSF_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const PSF_Header *const psfHeader =
		reinterpret_cast<const PSF_Header*>(info->header.pData);

	// Check the PSF magic number.
	if (!memcmp(psfHeader->magic, PSF_MAGIC, sizeof(psfHeader->magic))) {
		// Found the PSF magic number.
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
const char *PSF::systemName(unsigned int type) const
{
	RP_D(const PSF);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// PSF has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"PSF::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Portable Sound Format", "PSF", "PSF", nullptr
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
const char *const *PSF::supportedFileExtensions_static(void)
{
	// NOTE: The .*lib files are not listed, since they
	// contain samples, not songs.
	static const char *const exts[] = {
		".psf", ".minipsf",
		".psf1", ".minipsf1",
		".psf2", ".minipsf2",

		".ssf", ".minissf",
		".dsf", ".minidsf",

		".usf", ".miniusf",
		".gsf", ".minigsf",
		".snsf", ".minisnsf",

		".qsf", ".miniqsf",

		nullptr
	};
	return exts;
}

/**
 * Get a list of all supported MIME types.
 * This is to be used for metadata extractors that
 * must indicate which MIME types they support.
 *
 * NOTE: The array and the strings in the array should
 * *not* be freed by the caller.
 *
 * @return NULL-terminated array of all supported file extensions, or nullptr on error.
 */
const char *const *PSF::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-psf",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int PSF::loadFieldData(void)
{
	RP_D(PSF);
	if (!d->fields->empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// PSF header.
	const PSF_Header *const psfHeader = &d->psfHeader;

	// PSF fields:
	// - 1 regular field.
	// - 11 fields in the "[TAG]" section.
	d->fields->reserve(1+11);

	// System.
	const char *sysname;
	switch (psfHeader->version) {
		default:
			sysname = nullptr;
			break;
		case PSF_VERSION_PLAYSTATION:
			sysname = C_("PSF", "Sony PlayStation");
			break;
		case PSF_VERSION_PLAYSTATION_2:
			sysname = C_("PSF", "Sony PlayStation 2");
			break;
		case PSF_VERSION_SATURN:
			sysname = C_("PSF", "Sega Saturn");
			break;
		case PSF_VERSION_DREAMCAST:
			sysname = C_("PSF", "Sega Dreamcast");
			break;
		case PSF_VERSION_MEGA_DRIVE:
			sysname = C_("PSF", "Sega Mega Drive");
			break;
		case PSF_VERSION_N64:
			sysname = C_("PSF", "Nintendo 64");
			break;
		case PSF_VERSION_GBA:
			sysname = C_("PSF", "Game Boy Advance");
			break;
		case PSF_VERSION_SNES:
			sysname = C_("PSF", "Super NES");
			break;
		case PSF_VERSION_QSOUND:
			sysname = C_("PSF", "Capcom QSound");
			break;
	}

	const char *const system_title = C_("PSF", "System");
	if (sysname) {
		d->fields->addField_string(system_title, sysname);
	} else {
		d->fields->addField_string(system_title,
			rp_sprintf(C_("RomData", "Unknown (0x%02X)"), psfHeader->version));
	}

	// Parse the tags.
	const int64_t tag_addr = (int64_t)sizeof(*psfHeader) +
		le32_to_cpu(psfHeader->reserved_size) +
		le32_to_cpu(psfHeader->compressed_prg_length);
	unordered_map<string, string> tags = d->parseTags(tag_addr);

	if (!tags.empty()) {
		// Title
		auto iter = tags.find("title");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Title"), iter->second);
		}

		// Artist
		iter = tags.find("artist");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Artist"), iter->second);
		}

		// Game
		iter = tags.find("game");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("PSF", "Game"), iter->second);
		}

		// Release Date
		// NOTE: The tag is "year", but it may be YYYY-MM-DD.
		iter = tags.find("year");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData", "Release Date"), iter->second);
		}

		// Genre
		iter = tags.find("genre");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Genre"), iter->second);
		}

		// Copyright
		iter = tags.find("copyright");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Copyright"), iter->second);
		}

		// Ripped By
		// NOTE: The tag varies based on PSF version.
		const char *const ripped_by_tag = d->getRippedByTagName(psfHeader->version);
		const char *const ripped_by_title = C_("PSF", "Ripped By");
		iter = tags.find(ripped_by_tag);
		if (iter != tags.end()) {
			d->fields->addField_string(ripped_by_title, iter->second);
		} else {
			// Try "psfby" if the system-specific one isn't there.
			iter = tags.find("psfby");
			if (iter != tags.end()) {
				d->fields->addField_string(ripped_by_title, iter->second);
			}
		}

		// Volume (floating-point number)
		iter = tags.find("volume");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("PSF", "Volume"), iter->second);
		}

		// Duration
		//
		// Possible formats:
		// - seconds.decimal
		// - minutes:seconds.decimal
		// - hours:minutes:seconds.decimal
		//
		// Decimal may be omitted.
		// Commas are also accepted.
		iter = tags.find("length");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Duration"), iter->second);
		}

		// Fadeout duration
		// Same format as duration.
		iter = tags.find("fade");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("PSF", "Fadeout Duration"), iter->second);
		}

		// Comment
		iter = tags.find("comment");
		if (iter != tags.end()) {
			d->fields->addField_string(C_("RomData|Audio", "Comment"), iter->second);
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields->count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int PSF::loadMetaData(void)
{
	RP_D(PSF);
	if (d->metaData != nullptr) {
		// Metadata *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// PSF header.
	const PSF_Header *const psfHeader = &d->psfHeader;

	// Attempt to parse the tags before doing anything else.
	const int64_t tag_addr = (int64_t)sizeof(*psfHeader) +
		le32_to_cpu(psfHeader->reserved_size) +
		le32_to_cpu(psfHeader->compressed_prg_length);
	unordered_map<string, string> tags = d->parseTags(tag_addr);

	if (tags.empty()) {
		// No tags.
		return -EIO;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(8);	// Maximum of 8 metadata properties.

	// Title
	auto iter = tags.find("title");
	if (iter != tags.end()) {
		d->metaData->addMetaData_string(Property::Title, iter->second);
	}

	// Artist
	iter = tags.find("artist");
	if (iter != tags.end()) {
		d->metaData->addMetaData_string(Property::Artist, iter->second);
	}

	// Game
	iter = tags.find("game");
	if (iter != tags.end()) {
		// NOTE: Not exactly "album"...
		d->metaData->addMetaData_string(Property::Album, iter->second);
	}

	// Release Date
	// NOTE: The tag is "year", but it may be YYYY-MM-DD.
	iter = tags.find("year");
	if (iter != tags.end()) {
		// Parse the release date.
		// NOTE: Only year is supported.
		int year;
		char chr;
		int s = sscanf(iter->second.c_str(), "%04d%c", &year, &chr);
		if (s == 1 || (s == 2 && (chr == '-' || chr == '/'))) {
			// Year seems to be valid.
			// Make sure the number is acceptable:
			// - No negatives.
			// - Four-digit only. (lol Y10K)
			if (year >= 0 && year < 10000) {
				d->metaData->addMetaData_uint(Property::ReleaseYear, (unsigned int)year);
			}
		}
	}

	// Genre
	iter = tags.find("genre");
	if (iter != tags.end()) {
		d->metaData->addMetaData_string(Property::Genre, iter->second);
	}

	// Copyright
	iter = tags.find("copyright");
	if (iter != tags.end()) {
		d->metaData->addMetaData_string(Property::Copyright, iter->second);
	}

#if 0
	// FIXME: No property for this...
	// Ripped By
	// NOTE: The tag varies based on PSF version.
	const char *const ripped_by_tag = d->getRippedByTagName(psfHeader->version);
	iter = tags.find(ripped_by_tag);
	if (iter != tags.end()) {
		// FIXME: No property for this...
		d->metaData->addMetaData_string(Property::RippedBy, iter->second);
	} else {
		// Try "psfby" if the system-specific one isn't there.
		iter = tags.find("psfby");
		if (iter != tags.end()) {
			// FIXME: No property for this...
			d->metaData->addMetaData_string(Property::RippedBy, iter->second);
		}
	}
#endif

	// Duration
	//
	// Possible formats:
	// - seconds.decimal
	// - minutes:seconds.decimal
	// - hours:minutes:seconds.decimal
	//
	// Decimal may be omitted.
	// Commas are also accepted.
	iter = tags.find("length");
	if (iter != tags.end()) {
		// Convert the length string to milliseconds.
		const unsigned int ms = d->lengthToMs(iter->second.c_str());
		d->metaData->addMetaData_integer(Property::Duration, ms);
	}

	// Comment
	iter = tags.find("comment");
	if (iter != tags.end()) {
		// TODO: Property::Comment is assumed to be user-added
		// on KDE Dolphin 18.08.1. Needs a description property.
		// Also needs verification on Windows.
		d->metaData->addMetaData_string(Property::Subject, iter->second);
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}

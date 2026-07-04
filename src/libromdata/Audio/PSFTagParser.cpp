/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * PSFTagParser.cpp: PSF-style tag parser.                                 *
 *                                                                         *
 * Copyright (c) 2018-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "PSFTagParser.hpp"

#include "psf_structs.h"
#include "s98_structs.h"

// Other rom-properties libraries
#include "libi18n/i18n.hpp"
#include "librptext/conversion.hpp"
using namespace LibRpBase;
using namespace LibRpText;

// C includes
#include "ctypex.h"

// C includes (C++ namespace)
#include <cstring>

// C++ STL classes
#include <algorithm>
using std::string;
using std::unordered_map;

namespace LibRomData { namespace PSFTagParser {

/**
 * Parse a PSF tag section.
 * @param pData Tag section
 * @param size Size of tag section
 * @param style Style of PSF tags
 */
unordered_map<string, string> parseTags(const char *pData, size_t size, PSFTagStyle style)
{
	const char *p = pData;
	const char *const pEnd = pData + size;

	// Verify the magic number first.
	const char *magicNumber;
	size_t magicSize;
	switch (style) {
		default:
		case PSFTagStyle::PSF:
			magicNumber = PSF_TAG_MAGIC;
			magicSize = sizeof(PSF_TAG_MAGIC) - 1;
			break;
		case PSFTagStyle::S98:
			magicNumber = S98_TAG_MAGIC;
			magicSize = sizeof(S98_TAG_MAGIC) - 1;
			break;
	}
	if (p + magicSize >= pEnd) {
		// Out of bounds?
		return {};
	}
	if (memcmp(p, magicNumber, magicSize) != 0) {
		// Invalid magic number.
		return {};
	}
	p += magicSize;

	// Older files may have tags encoded in cp1252 or Shift-JIS.
	bool isUtf8 = false;

	if (style == PSFTagStyle::S98) {
		// S98: Check for a UTF-8 BOM.
		if (p + 3 >= pEnd) {
			// Not enough data...
			return {};
		}

		const uint8_t *const up = reinterpret_cast<const uint8_t*>(p);
		if (up[0] == 0xEFU && up[1] == 0xBBU && up[2] == 0xBFU) {
			// Found a UTF-8 BOM.
			isUtf8 = true;
			p += 3;
		}
	}

	unordered_map<string, string> kv;
#ifdef HAVE_UNORDERED_MAP_RESERVE
	kv.reserve(12);
#endif /* HAVE_UNORDERED_MAP_RESERVE */

	for (; p < pEnd; p++) {
		if (*p == '\0') {
			// NULL byte. End of data for S98.
			// Not usually seen for PSF, but we'll handle it there too.
			break;
		}

		// Find the next newline.
		const char *nl = static_cast<const char*>(memchr(p, '\n', pEnd - p));
		if (!nl) {
			// No newline. Assume this is the end of the tag section,
			// and read up to the end.
			nl = pEnd;
		}
		if (p == nl) {
			// Empty line.
			continue;
		}

		// Find the equals sign.
		const char *eq = static_cast<const char*>(memchr(p, '=', nl - p));
		if (eq) {
			// Found the equals sign.
			const int k_len = static_cast<int>(eq - p);
			const int v_len = static_cast<int>(nl - eq - 1);
			if (k_len > 0 && v_len > 0) {
				// Key and value are valid.
				// NOTE: Key is case-insensitive, so convert to lowercase.
				// NOTE: Key *must* be ASCII.
				string s_key(p, k_len);
				std::transform(s_key.begin(), s_key.end(), s_key.begin(),
					[](unsigned char c) noexcept -> char { return std::tolower(c); });

				if (style == PSFTagStyle::PSF) {
					// Check for UTF-8.
					// NOTE: The v_len check is redundant...
					if (s_key == "utf8" && v_len > 0) {
						// "utf8" key with non-empty value.
						// This is UTF-8.
						isUtf8 = true;
					}
				}

				string s_value(eq+1, v_len);
				kv.emplace(std::move(s_key), std::move(s_value));
			}
		}

		// Next line.
		p = nl;
	}

	// If we're not using UTF-8, convert the values.
	if (!isUtf8) {
		for (auto &p : kv) {
			p.second = cp1252_sjis_to_utf8(p.second);
		}
	}

	return kv;
}

/**
 * Add PSF tags to RomFields.
 * @param fields RomFields
 * @param tags PSF tags [parsed using parseTags()]
 * @param psfby Key for "psfby" field
 * @return Number of fields added.
 */
int addTagsToRomFields(RomFields *fields, const unordered_map<string, string> &tags, const char *psfby)
{
	if (tags.empty()) {
		// No tags...
		return 0;
	}

	const int prev_count = fields->count();

	// Title
	auto iter = tags.find("title");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData|Audio", "Title"), iter->second);
	}

	// Artist
	iter = tags.find("artist");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData|Audio", "Artist"), iter->second);
	}

	// Game
	iter = tags.find("game");
	if (iter != tags.end()) {
		fields->addField_string(C_("PSF", "Game"), iter->second);
	}

	// Release Date
	// NOTE: The tag is "year", but it may be YYYY-MM-DD.
	iter = tags.find("year");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData", "Release Date"), iter->second);
	}

	// Genre
	iter = tags.find("genre");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData|Audio", "Genre"), iter->second);
	}

	// Copyright
	iter = tags.find("copyright");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData|Audio", "Copyright"), iter->second);
	}

	// Ripped By
	// NOTE: The tag varies based on PSF version.
	const char *const ripped_by_title = C_("PSF", "Ripped By");
	iter = tags.find(psfby);
	if (iter != tags.end()) {
		fields->addField_string(ripped_by_title, iter->second);
	} else {
		// Try "psfby" if the system-specific one isn't there.
		iter = tags.find("psfby");
		if (iter != tags.end()) {
			fields->addField_string(ripped_by_title, iter->second);
		}
	}

	// Volume (floating-point number)
	iter = tags.find("volume");
	if (iter != tags.end()) {
		fields->addField_string(C_("PSF", "Volume"), iter->second);
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
		fields->addField_string(C_("RomData|Audio", "Duration"), iter->second);
	}

	// Fadeout duration
	// Same format as duration.
	iter = tags.find("fade");
	if (iter != tags.end()) {
		fields->addField_string(C_("PSF", "Fadeout Duration"), iter->second);
	}

	// Comment
	iter = tags.find("comment");
	if (iter != tags.end()) {
		fields->addField_string(C_("RomData|Audio", "Comment"), iter->second);
	}

	// System (S98-only, but will add it if present in PSF anyway)
	iter = tags.find("system");
	if (iter != tags.end()) {
		fields->addField_string(C_("S98", "System"), iter->second);
	}

	// Done adding tags.
	return fields->count() - prev_count;
}

/**
 * Convert a PSF length string to milliseconds.
 * @param str PSF length string.
 * @return Milliseconds.
 */
static unsigned int lengthToMs(const char *str)
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
			if (isdigit_ascii(*dp)) {
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

/**
 * Add PSF tags to RomMetaData.
 * @param metaData RomMetaData
 * @param tags PSF tags [parsed using parseTags()]
 * @param psfby Key for "psfby" field
 * @return Number of metadata properties added.
 */
int addTagsToRomMetaData(LibRpBase::RomMetaData *metaData, const std::unordered_map<std::string, std::string> &tags, const char *psfby)
{
	if (tags.empty()) {
		// No tags...
		return 0;
	}

	const int prev_count = metaData->count();

	// Title
	auto iter = tags.find("title");
	if (iter != tags.end()) {
		metaData->addMetaData_string(Property::Title, iter->second);
	}

	// Artist
	iter = tags.find("artist");
	if (iter != tags.end()) {
		metaData->addMetaData_string(Property::Artist, iter->second);
	}

	// Game
	iter = tags.find("game");
	if (iter != tags.end()) {
		// NOTE: Not exactly "album"...
		metaData->addMetaData_string(Property::Album, iter->second);
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
				metaData->addMetaData_uint(Property::ReleaseYear, (unsigned int)year);
			}
		}
	}

	// Genre
	iter = tags.find("genre");
	if (iter != tags.end()) {
		metaData->addMetaData_string(Property::Genre, iter->second);
	}

	// Copyright
	iter = tags.find("copyright");
	if (iter != tags.end()) {
		metaData->addMetaData_string(Property::Copyright, iter->second);
	}

	// Ripped By
	// NOTE: The tag varies based on PSF version.
	// NOTE: This is a Custom Property, since KDE doesn't have "Ripped By".
	iter = tags.find(psfby);
	if (iter != tags.end()) {
		metaData->addMetaData_string(Property::RippedBy, iter->second);
	} else {
		// Try "psfby" if the system-specific one isn't there.
		iter = tags.find("psfby");
		if (iter != tags.end()) {
			metaData->addMetaData_string(Property::RippedBy, iter->second);
		}
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
		// Convert the length string to milliseconds.
		const unsigned int ms = lengthToMs(iter->second.c_str());
		metaData->addMetaData_integer(Property::Duration, ms);
	}

	// Comment
	iter = tags.find("comment");
	if (iter != tags.end()) {
		// NOTE: Property::Comment is assumed to be user-added
		// on KDE Dolphin 18.08.1. Use Property::Description.
		metaData->addMetaData_string(Property::Description, iter->second);
	}

	// Done adding tags.
	return metaData->count() - prev_count;
}

} } // namespace LibRomData::PSFTagParser

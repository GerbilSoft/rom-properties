/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SPC.hpp: SPC audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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

#include "SPC.hpp"
#include "librpbase/RomData_p.hpp"

#include "spc_structs.h"

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
#include <string>
#include <unordered_map>
#include <vector>
using std::string;
using std::unordered_map;
using std::vector;

namespace LibRomData {

ROMDATA_IMPL(SPC)

class SPCPrivate : public RomDataPrivate
{
	public:
		SPCPrivate(SPC *q, IRpFile *file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SPCPrivate)

	public:
		// SPC header.
		// NOTE: **NOT** byteswapped in memory.
		SPC_Header spcHeader;

		// Tag struct.
		struct TagData {
			// Vector of strings.
			// Contains all string data.
			vector<string> strs;

			// Value struct.
			// Contains an integer value, and a boolean
			// indicating if it's a numeric value or an
			// index into the strs vector.
			struct val_t {
				union {
					int ivalue;
					unsigned int uvalue;
					time_t timestamp;
				}; 
				bool isStrIdx;

				val_t() : timestamp(0), isStrIdx(false) { }
				val_t(int ivalue) : ivalue(ivalue), isStrIdx(false) { }
				val_t(unsigned int uvalue) : uvalue(uvalue), isStrIdx(false) { }
			};

			// Map of ID666 tags.
			// - Key: Extended ID666 tag index.
			// - Value: struct val_t.
			typedef unordered_map<SPC_xID6_Item_e, val_t> map_t;
			map_t map;

			/**
			 * Is the tag data empty?
			 * @return True if empty; false if not.
			 */
			inline bool empty(void) const
			{
				return map.empty();
			}

			// Wrapper find/begin/end functions for the map.
			inline map_t::iterator find(SPC_xID6_Item_e key)
			{
				return map.find(key);
			}
			inline map_t::iterator begin(void)
			{
				return map.begin();
			}
			inline map_t::iterator end(void)
			{
				return map.end();
			}
			inline map_t::const_iterator cbegin(void) const
			{
				return map.cbegin();
			}
			inline map_t::const_iterator cend(void) const
			{
				return map.cend();
			}

			/**
			 * Insert an integer value.
			 * @param key Extended ID666 tag index.
			 * @param ivalue Integer value.
			 */
			inline void insertInt(SPC_xID6_Item_e key, int ivalue)
			{
				map.insert(std::make_pair(key, val_t(ivalue)));
			}

			/**
			 * Insert an unsigned integer value.
			 * @param key Extended ID666 tag index.
			 * @param ivalue Unsigned integer value.
			 */
			inline void insertUInt(SPC_xID6_Item_e key, unsigned int uvalue)
			{
				map.insert(std::make_pair(key, val_t(uvalue)));
			}

			/**
			 * Insert a timestamp value.
			 * @param key Extended ID666 tag index.
			 * @param timestamp Timestamp value.
			 */
			inline void insertTimestamp(SPC_xID6_Item_e key, time_t timestamp)
			{
				val_t val;
				val.timestamp = timestamp;
				map.insert(std::make_pair(key, val));
			}

			/**
			 * Insert a string value.
			 * @param key Extended ID666 tag index.
			 * @param str String value.
			 */
			inline void insertStr(SPC_xID6_Item_e key, const string &str)
			{
				val_t val((unsigned int)strs.size());
				val.isStrIdx = true;
				strs.push_back(str);
				map.insert(std::make_pair(key, val));
			}

			/**
			 * Get a string.
			 * @param data val_t struct.
			 * @return String.
			 */
			inline const string &getStr(const val_t &val) const
			{
				// NOTE: This will throw an exception if out of range.
				assert(val.isStrIdx);
				return strs[val.uvalue];
			}
		};

		/**
		 * Parse the ID666 tags for the open SPC file.
		 * @return Map containing key/value entries.
		 */
		TagData parseTags(void);
};

/** SPCPrivate **/

SPCPrivate::SPCPrivate(SPC *q, IRpFile *file)
	: super(q, file)
{
	// Clear the SPC header struct.
	memset(&spcHeader, 0, sizeof(spcHeader));
}

/**
 * Parse the tag section.
 * @return Map containing key/value entries.
 */
SPCPrivate::TagData SPCPrivate::parseTags(void)
{
	TagData kv;
	// TODO: BEFORE REAL COMMIT, reserve space
	// also for other parseTags e.g. in PSF

	if (spcHeader.has_id666 != 26) {
		// No ID666 tags.
		// TODO: Check for Extended ID666?
		return kv;
	}

	// Read the ID666 tags first.
	const SPC_ID666 *const id666 = &spcHeader.id666;

	// NOTE: Text is assumed to be ASCII.
	// We'll use cp1252 just in case.
	// TODO: Check SPCs for Japanese text?

	// Reserve space for strings.
	kv.strs.reserve(5);

	// Fields that are the same regardless of binary vs. text.
	if (id666->song_title[0] != '\0') {
		kv.insertStr(SPC_xID6_ITEM_SONG_NAME,
			cp1252_to_utf8(id666->song_title, sizeof(id666->song_title)));
	}
	if (id666->game_title[0] != '\0') {
		kv.insertStr(SPC_xID6_ITEM_GAME_NAME,
			cp1252_to_utf8(id666->game_title, sizeof(id666->game_title)));
	}
	if (id666->dumper_name[0] != '\0') {
		kv.insertStr(SPC_xID6_ITEM_DUMPER_NAME,
			cp1252_to_utf8(id666->dumper_name, sizeof(id666->dumper_name)));
	}
	if (id666->comments[0] != '\0') {
		kv.insertStr(SPC_xID6_ITEM_COMMENTS,
			cp1252_to_utf8(id666->comments, sizeof(id666->comments)));
	}

	// Determine binary vs. text.
	// Based on bsnes-plus:
	// https://github.com/devinacker/bsnes-plus/blob/master/snesmusic/snesmusic.cpp#L90
	bool isBinary = false;
	for (size_t i = 0; i < sizeof(id666->test.length_fields); i++) {
		const uint8_t chr = id666->test.length_fields[i];
		if ((chr > 0 && chr < 0x20) || chr > 0x7E) {
			// Probably binary format.
			isBinary = true;
			break;
		}
	}
	if (!isBinary) {
		// Check the first byte of the binary artist field.
		if (id666->bin.artist[0] >= 'A') {
			// Probably binary format.
			isBinary = true;
		}
	}

	// Parse the remaining fields.
	if (isBinary) {
		// Binary version.

		// Dump date. (YYYYMMDD)
		// TODO: Untested.
		kv.insertTimestamp(SPC_xID6_ITEM_DUMP_DATE,
			bcd_to_unix_time(id666->bin.dump_date, sizeof(id666->bin.dump_date)));

		// Artist.
		if (id666->bin.artist[0] != '\0') {
			kv.insertStr(SPC_xID6_ITEM_ARTIST_NAME,
				cp1252_to_utf8(id666->bin.artist, sizeof(id666->bin.artist)));
		}

		// TODO: Duration.
		// Need to convert to ID666 format somehow...

		// TODO: Channel disables?

		// Emulator used.
		kv.insertUInt(SPC_xID6_ITEM_EMULATOR_USED, id666->bin.emulator_used);
	} else {
		// Text version.

		// Dump date. (MM/DD/YYYY; also allowing MM-DD-YYYY)
		// NOTE: Might not be NULL-terminated...
		// TODO: Untested.
		char dump_date[11];
		memcpy(dump_date, id666->text.dump_date, sizeof(dump_date));
		dump_date[sizeof(dump_date)-1] = '\0';

		unsigned int month, day, year;
		int s = sscanf("%02u/%02u/%04u", dump_date, &month, &day, &year);
		if (s != 3) {
			s = sscanf("%02u-%02u-%04u", dump_date, &month, &day, &year);
		}
		if (s == 3) {
			// Convert to Unix time.
			// NOTE: struct tm has some oddities:
			// - tm_year: year - 1900
			// - tm_mon: 0 == January
			struct tm ymdtime;

			ymdtime.tm_year = year - 1900;
			ymdtime.tm_mon  = month - 1;
			ymdtime.tm_mday = day;

			// Time portion is empty.
			ymdtime.tm_hour = 0;
			ymdtime.tm_min = 0;
			ymdtime.tm_sec = 0;

			// tm_wday and tm_yday are output variables.
			ymdtime.tm_wday = 0;
			ymdtime.tm_yday = 0;
			ymdtime.tm_isdst = 0;

			// If conversion fails, this will return -1.
			kv.insertTimestamp(SPC_xID6_ITEM_DUMP_DATE, timegm(&ymdtime));
		}

		// Artist.
		if (id666->text.artist[0] != '\0') {
			kv.insertStr(SPC_xID6_ITEM_ARTIST_NAME,
				cp1252_to_utf8(id666->text.artist, sizeof(id666->text.artist)));
		}

		// TODO: Duration.
		// Need to convert to ID666 format somehow...

		// TODO: Channel disables?

		// Emulator used.
		kv.insertUInt(SPC_xID6_ITEM_EMULATOR_USED, id666->text.emulator_used);
	}

	// TODO: Find Extended ID666 tags and parse them?
	return kv;
}

/** SPC **/

/**
 * Read an SPC audio file.
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
SPC::SPC(IRpFile *file)
	: super(new SPCPrivate(this, file))
{
	RP_D(SPC);
	d->className = "SPC";
	d->fileType = FTYPE_AUDIO_FILE;

	if (!d->file) {
		// Could not dup() the file handle.
		return;
	}

	// Read the SPC header.
	d->file->rewind();
	size_t size = d->file->read(&d->spcHeader, sizeof(d->spcHeader));
	if (size != sizeof(d->spcHeader)) {
		delete d->file;
		d->file = nullptr;
		return;
	}

	// Check if this file is supported.
	DetectInfo info;
	info.header.addr = 0;
	info.header.size = sizeof(d->spcHeader);
	info.header.pData = reinterpret_cast<const uint8_t*>(&d->spcHeader);
	info.ext = nullptr;	// Not needed for SPC.
	info.szFile = 0;	// Not needed for SPC.
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		delete d->file;
		d->file = nullptr;
		return;
	}
}

/**
 * Is a ROM image supported by this class?
 * @param info DetectInfo containing ROM detection information.
 * @return Class-specific system ID (>= 0) if supported; -1 if not.
 */
int SPC::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < sizeof(SPC_Header))
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	const SPC_Header *const spcHeader =
		reinterpret_cast<const SPC_Header*>(info->header.pData);

	// Check the SPC magic number.
	if (!memcmp(spcHeader->magic, SPC_MAGIC, sizeof(spcHeader->magic)-1)) {
		// Found the SPC magic number.
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
const char *SPC::systemName(unsigned int type) const
{
	RP_D(const SPC);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// SPC has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SPC::systemName() array index optimization needs to be updated.");

	static const char *const sysNames[4] = {
		"Super NES SPC Audio",
		"SPC",
		"SPC",
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
const char *const *SPC::supportedFileExtensions_static(void)
{
	static const char *const exts[] = {
		".spc",

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
const char *const *SPC::supportedMimeTypes_static(void)
{
	static const char *const mimeTypes[] = {
		// Unofficial MIME types.
		"audio/x-spc",

		nullptr
	};
	return mimeTypes;
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SPC::loadFieldData(void)
{
	RP_D(SPC);
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

	// SPC header.
	const SPC_Header *const spcHeader = &d->spcHeader;
	d->fields->reserve(7);	// Maximum of 7 fields.

	// Get the ID666 tags.
	auto kv = d->parseTags();
	if (!kv.empty()) {
		// TODO: Add more tags.

		// Song name.
		auto iter = kv.find(SPC_xID6_ITEM_SONG_NAME);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(data.isStrIdx);
			if (data.isStrIdx) {
				d->fields->addField_string(C_("SPC", "Song Name"), kv.getStr(data));
			}
		}

		// Game name.
		iter = kv.find(SPC_xID6_ITEM_GAME_NAME);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(data.isStrIdx);
			if (data.isStrIdx) {
				d->fields->addField_string(C_("SPC", "Game Name"), kv.getStr(data));
			}
		}

		// Artist.
		iter = kv.find(SPC_xID6_ITEM_ARTIST_NAME);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(data.isStrIdx);
			if (data.isStrIdx) {
				d->fields->addField_string(C_("SPC", "Artist"), kv.getStr(data));
			}
		}

		// Dumper.
		iter = kv.find(SPC_xID6_ITEM_DUMPER_NAME);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(data.isStrIdx);
			if (data.isStrIdx) {
				d->fields->addField_string(C_("SPC", "Dumper"), kv.getStr(data));
			}
		}

		// Dump date. (TODO)
		iter = kv.find(SPC_xID6_ITEM_DUMP_DATE);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(!data.isStrIdx);
			if (!data.isStrIdx) {
				d->fields->addField_dateTime(C_("SPC", "Dump Date"),
					data.timestamp,
					RomFields::RFT_DATETIME_HAS_DATE |
					RomFields::RFT_DATETIME_IS_UTC  // Date only.
				);
			}
		}

		// Comments.
		iter = kv.find(SPC_xID6_ITEM_COMMENTS);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(data.isStrIdx);
			if (data.isStrIdx) {
				d->fields->addField_string(C_("SPC", "Comments"), kv.getStr(data));
			}
		}

		// Emulator used.
		iter = kv.find(SPC_xID6_ITEM_EMULATOR_USED);
		if (iter != kv.end()) {
			const auto &data = iter->second;
			assert(!data.isStrIdx);
			if (!data.isStrIdx) {
				const char *emu;
				switch (data.uvalue) {
					case SPC_EMULATOR_UNKNOWN:
						emu = C_("SPC|Emulator", "Unknown");
						break;
					case SPC_EMULATOR_ZSNES:
						emu = "ZSNES";
						break;
					case SPC_EMULATOR_SNES9X:
						emu = "Snes9x";
						break;
					default:
						emu = nullptr;
						break;
				}

				if (emu) {
					d->fields->addField_string(C_("SPC", "Emulator Used"), emu);
				} else {
					d->fields->addField_string(C_("SPC", "Emulator Used"),
						rp_sprintf(C_("SPC", "Unknown (0x%02X)"), data.uvalue));
				}
			}
		}
	}

	// Finished reading the field data.
	return (int)d->fields->count();
}

}

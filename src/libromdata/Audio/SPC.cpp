/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SPC.hpp: SPC audio reader.                                              *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "SPC.hpp"
#include "spc_structs.h"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

namespace LibRomData {

class SPCPrivate final : public RomDataPrivate
{
	public:
		SPCPrivate(const IRpFilePtr &file);

	private:
		typedef RomDataPrivate super;
		RP_DISABLE_COPY(SPCPrivate)

	public:
		/** RomDataInfo **/
		static const char *const exts[];
		static const char *const mimeTypes[];
		static const RomDataInfo romDataInfo;

	public:
		// SPC header.
		// NOTE: **NOT** byteswapped in memory.
		SPC_Header spcHeader;

		// Tag struct.
		struct spc_tags_t {
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

				explicit val_t() : timestamp(0), isStrIdx(false) { }
				explicit val_t(int ivalue) : ivalue(ivalue), isStrIdx(false) { }
				explicit val_t(unsigned int uvalue) : uvalue(uvalue), isStrIdx(false) { }
			};

			// Map of ID666 tags.
			// - Key: Extended ID666 tag index.
			// - Value: struct val_t.
			// NOTE: gcc-6.1 added support for using enums as keys for unordered_map.
			// Older gcc requires uint8_t instead.
			// References:
			// - https://stackoverflow.com/questions/18837857/cant-use-enum-class-as-unordered-map-key
			// - https://github.com/dropbox/djinni/issues/213
			// - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60970
			typedef unordered_map<uint8_t, val_t> map_t;
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
			inline map_t::const_iterator find(SPC_xID6_Item_e key) const
			{
				return map.find(key);
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
				map.emplace(key, val_t(ivalue));
			}

			/**
			 * Insert an unsigned integer value.
			 * @param key Extended ID666 tag index.
			 * @param ivalue Unsigned integer value.
			 */
			inline void insertUInt(SPC_xID6_Item_e key, unsigned int uvalue)
			{
				map.emplace(key, val_t(uvalue));
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
				map.emplace(key, val);
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
				strs.emplace_back(str);
				map.emplace(key, val);
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
		spc_tags_t parseTags(void);

		/**
		 * Get the duration from the SPC tags.
		 * @param kv spc_tags_t
		 * @return Duration (in milliseconds), or 0 if not found.
		 */
		static unsigned int getDurationMs(const spc_tags_t &kv);
};

ROMDATA_IMPL(SPC)

/** SPCPrivate **/

/* RomDataInfo */
const char *const SPCPrivate::exts[] = {
	".spc",

	nullptr
};
const char *const SPCPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"audio/x-spc",

	nullptr
};
const RomDataInfo SPCPrivate::romDataInfo = {
	"SPC", exts, mimeTypes
};

SPCPrivate::SPCPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{
	// Clear the SPC header struct.
	memset(&spcHeader, 0, sizeof(spcHeader));
}

/**
 * Parse the tag section.
 * @return Map containing key/value entries.
 */
SPCPrivate::spc_tags_t SPCPrivate::parseTags(void)
{
	spc_tags_t kv;

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
	for (const uint8_t chr : id666->test.length_fields) {
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
		// Binary version

		// Dump date (YYYYMMDD)
		// TODO: Untested.
		kv.insertTimestamp(SPC_xID6_ITEM_DUMP_DATE,
			bcd_to_unix_time(id666->bin.dump_date, sizeof(id666->bin.dump_date)));

		// Artist
		if (id666->bin.artist[0] != '\0') {
			kv.insertStr(SPC_xID6_ITEM_ARTIST_NAME,
				cp1252_to_utf8(id666->bin.artist, sizeof(id666->bin.artist)));
		}

		// Duration
		// TODO: Find binary SPCs and verify this.

		// Main loop duration (in seconds)
		// NOTE: 3-byte value, little-endian
		const unsigned int duration = (id666->bin.seconds_before_fade[0]) |
		                              (id666->bin.seconds_before_fade[1] << 8) |
		                              (id666->bin.seconds_before_fade[2] << 16);

		// Fadeout duration (in milliseconds)
		const unsigned int fadeout_ms = le32_to_cpu(id666->bin.fadeout_length_ms);

		// Convert to xID6. (measured in 1/64000ths of a second)
		// FIXME: May overflow?
		const uint64_t duration_xid6 = static_cast<uint64_t>(duration) * 64000ULL;
		const uint64_t fadeout_xid6 = static_cast<uint64_t>(fadeout_ms) * 64ULL;
		kv.insertInt(SPC_xID6_ITEM_LOOP_COUNT, -1);	// special value for "from ID666"
		kv.insertUInt(SPC_xID6_ITEM_LOOP_LENGTH, static_cast<unsigned int>(duration_xid6));
		kv.insertUInt(SPC_xID6_ITEM_FADE_LENGTH, static_cast<unsigned int>(fadeout_xid6));

		// TODO: Channel disables?

		// Emulator used
		kv.insertUInt(SPC_xID6_ITEM_EMULATOR_USED, id666->bin.emulator_used);
	} else {
		// Text version

		// Dump date (MM/DD/YYYY; also allowing MM-DD-YYYY)
		// Convert to UNIX time.
		// NOTE: Might not be NULL-terminated...
		// TODO: Untested.
		char dump_date[11];
		memcpy(dump_date, id666->text.dump_date, sizeof(dump_date));
		dump_date[sizeof(dump_date)-1] = '\0';

		struct tm ymdtime;
		char chr;
		int s = sscanf("%02u/%02u/%04u%c", dump_date, &ymdtime.tm_mon, &ymdtime.tm_mday, &ymdtime.tm_year, &chr);
		if (s != 3) {
			s = sscanf("%02u-%02u-%04u%c", dump_date, &ymdtime.tm_mon, &ymdtime.tm_mday, &ymdtime.tm_year, &chr);
		}
		if (s == 3) {
			ymdtime.tm_year -= 1900;	// year - 1900
			ymdtime.tm_mon--;		// 0 == January

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

		// Artist
		if (id666->text.artist[0] != '\0') {
			kv.insertStr(SPC_xID6_ITEM_ARTIST_NAME,
				cp1252_to_utf8(id666->text.artist, sizeof(id666->text.artist)));
		}

		// Duration

		// Main loop duration (in seconds)
		// TODO: 2-byte + NULL or 3-byte? Allowing both.
		unsigned int duration = 0, fadeout_ms = 0;
		for (size_t i = 0; i < sizeof(id666->text.seconds_before_fade); i++) {
			const char chr = id666->text.seconds_before_fade[i];
			if (chr >= '0' && chr <= '9') {
				duration *= 10;
				duration += (chr - '0');
			} else {
				// Invalid character.
				break;
			}
		}
		for (size_t i = 0; i < sizeof(id666->text.fadeout_length_ms); i++) {
			const char chr = id666->text.fadeout_length_ms[i];
			if (chr >= '0' && chr <= '9') {
				fadeout_ms *= 10;
				fadeout_ms += (chr - '0');
			} else {
				// Invalid character.
				break;
			}
		}

		// Convert to xID6. (measured in 1/64000ths of a second)
		// FIXME: May overflow?
		duration *= 64000U;
		fadeout_ms *= 64U;
		kv.insertInt(SPC_xID6_ITEM_LOOP_COUNT, -1);	// special value for "from ID666"
		kv.insertUInt(SPC_xID6_ITEM_LOOP_LENGTH, duration);
		kv.insertUInt(SPC_xID6_ITEM_FADE_LENGTH, fadeout_ms);

		// TODO: Channel disables?

		// Emulator used.
		kv.insertUInt(SPC_xID6_ITEM_EMULATOR_USED, id666->text.emulator_used);
	}

	// Check for Extended ID666 tags.
	SPC_xID6_Header xID6;
	size_t size = file->seekAndRead(SPC_xID6_ADDRESS, &xID6, sizeof(xID6));
	if (size != sizeof(xID6) || xID6.magic != cpu_to_be32(SPC_xID6_MAGIC)) {
		// Seek and/or read error, or incorrect magic.
		return kv;
	}

	// NOTE: Maximum of 16 KB.
	const unsigned int len = le32_to_cpu(xID6.size);
	if (len < 4 || len > 16384) {
		// Invalid length.
		return kv;
	}

	// NOTE: Using indexes instead of pointer arithmetic
	// due to DWORD alignment requirements.
	unique_ptr<uint8_t[]> data(new uint8_t[len]);
	size = file->read(data.get(), len);
	if (size != (size_t)len) {
		// Read error.
		return kv;
	}

	for (unsigned int i = 0; i < len-3; i++) {
		switch (data[i]) {
			default:
				// Unsupported tag key.
				// Check the length and skip it.
				if (data[i+1] == 0) {
					// No extra data.
					i += 4-1;
				} else {
					// Extra data. Add the length.
					// NOTE: Must be DWORD-aligned.
					const unsigned int slen = data[i+2] | (data[i+3] << 8);
					i += 4 + slen;
					i = ALIGN_BYTES(4, i);
					i--;
				}
				break;

			// Strings.
			case SPC_xID6_ITEM_SONG_NAME:
			case SPC_xID6_ITEM_GAME_NAME:
			case SPC_xID6_ITEM_ARTIST_NAME:
			case SPC_xID6_ITEM_DUMPER_NAME:
			case SPC_xID6_ITEM_OST_TITLE:
			case SPC_xID6_ITEM_PUBLISHER: {
				// Must be stored after the header.
				assert(data[i+1] != 0);
				if (data[i+1] == 0) {
					i = len;
					break;
				}

				// Next two bytes: String length.
				// Should be 4-256, but we'll accept anything
				// as long as it doesn't go out of bounds.
				const unsigned int slen = data[i+2] | (data[i+3] << 8);
				assert(slen > 0);
				if (i + 4 + slen > len) {
					// Out of bounds.
					i = len;
					break;
				}

				// NOTE: Strings are encoded using cp1252.
				kv.insertStr(static_cast<SPC_xID6_Item_e>(data[i]),
					cp1252_to_utf8(reinterpret_cast<const char*>(&data[i+4]), (size_t)slen));

				// DWORD alignment.
				i += 4 + slen;
				i = ALIGN_BYTES(4, i);
				i--; // for loop iterator
				break;
			}

			// 16-bit lengths. (integer values stored within the header)
			case SPC_xID6_ITEM_OST_TRACK:
			case SPC_xID6_ITEM_COPYRIGHT_YEAR: {
				// Must be stored within the header.
				assert(data[i+1] == 0);
				if (data[i+1] != 0) {
					i = len;
					break;
				}

				// Next two bytes: 16-bit value.
				const unsigned int u16val = data[i+2] | (data[i+3] << 8);
				kv.insertUInt(static_cast<SPC_xID6_Item_e>(data[i]), u16val);

				// Next tag.
				i += 4-1;
				break;
			}

			// 8-bit lengths. (integer values stored within the header)
			case SPC_xID6_ITEM_EMULATOR_USED:
			case SPC_xID6_ITEM_OST_DISC:
			case SPC_xID6_ITEM_MUTED_CHANNELS:
			case SPC_xID6_ITEM_LOOP_COUNT: {
				// Must be stored within the header.
				assert(data[i+1] == 0);
				if (data[i+1] != 0) {
					i = len;
					break;
				}

				// Next byte: 8-bit value.
				const unsigned int u8val = data[i+2];
				kv.insertUInt(static_cast<SPC_xID6_Item_e>(data[i]), u8val);

				// Next tag.
				i += 4-1;
				break;
			}

			// Integers. (32-bit; stored after the header)
			case SPC_xID6_ITEM_DUMP_DATE:
			case SPC_xID6_ITEM_INTRO_LENGTH:
			case SPC_xID6_ITEM_LOOP_LENGTH:
			case SPC_xID6_ITEM_END_LENGTH:
			case SPC_xID6_ITEM_FADE_LENGTH:
			case SPC_xID6_ITEM_AMP_VALUE:
				// Must be stored after the header.
				assert(data[i+1] != 0);
				if (data[i+1] == 0) {
					i = len;
					break;
				}

				// Next two bytes: Size.
				// Should be 4 for uint32_t.
				const unsigned int slen = data[i+2] | (data[i+3] << 8);
				assert(slen == 4);
				if (slen != 4 || i + 4 + 4 > len) {
					// Incorrect length, or out of bounds.
					i = len;
					break;
				}

				// Get the integer value.
				if (data[i] == SPC_xID6_ITEM_DUMP_DATE) {
					// Convert from BCD to Unix time.
					kv.insertTimestamp(static_cast<SPC_xID6_Item_e>(data[i]),
						bcd_to_unix_time(&data[i+4], 4));
				} else {
					// Regular integer.
					// TODO: Use le32_to_cpu()?
					const unsigned int uival =  data[i+4] |
								   (data[i+5] <<  8) |
								   (data[i+6] << 16) |
								   (data[i+7] << 24);
					kv.insertUInt(static_cast<SPC_xID6_Item_e>(data[i]), uival);
				}

				// Next tag.
				i += 8-1;
				break;
		}
	}

	// Tags parsed.
	return kv;
}

/**
 * Get the duration from the SPC tags.
 * @param kv spc_tags_t
 * @return Duration (in milliseconds), or 0 if not found.
 */
unsigned int SPCPrivate::getDurationMs(const SPCPrivate::spc_tags_t &kv)
{
	const auto iter = kv.find(SPC_xID6_ITEM_LOOP_COUNT);
	if (iter == kv.cend())
		return 0;

	const auto &data = iter->second;
	assert(!data.isStrIdx);
	if (data.isStrIdx)
		return 0;

	// If loop count is < 0, this is ID666.
	// Otherwise, it's Extended ID666.
	// NOTE: Extended ID666 tags will usually not set loop duration unless
	// loop count is also set, in which case, the song length will be in
	// the intro field. Hence, we'll handle all of them the same way.
	const int loopCount = data.ivalue;

#define FIELD_DATA_GET_xID6_DURATION(var, tag) do { \
	const auto leniter = kv.find(tag); \
	if (leniter != kv.cend()) { \
		const auto &lendata = leniter->second; \
		assert(!lendata.isStrIdx); \
		if (!lendata.isStrIdx) { \
			var = lendata.uvalue; \
		} \
	} \
} while (0)

	// Get the durations.
	unsigned int intro = 0, loop = 0, end = 0, fadeout = 0;
	FIELD_DATA_GET_xID6_DURATION(intro, SPC_xID6_ITEM_INTRO_LENGTH);
	FIELD_DATA_GET_xID6_DURATION(loop, SPC_xID6_ITEM_LOOP_LENGTH);
	FIELD_DATA_GET_xID6_DURATION(end, SPC_xID6_ITEM_END_LENGTH);
	FIELD_DATA_GET_xID6_DURATION(fadeout, SPC_xID6_ITEM_FADE_LENGTH);

	uint64_t total_duration = static_cast<uint64_t>(intro) + static_cast<uint64_t>(end);
	if (loopCount < 0) {
		// Regular ID666: Add the loop duration as-is,
		// but only if the other values aren't present.
		if (total_duration == 0) {
			total_duration = static_cast<uint64_t>(loop);
		}
	} else if (loopCount > 0) {
		// Extended ID666: Multiply by the loop count.
		total_duration += (static_cast<uint64_t>(loop) * static_cast<uint64_t>(loopCount));
	}

	// Add fadeout after loop handling.
	total_duration += static_cast<uint64_t>(fadeout);

	// Divide by 64 to get milliseconds.
	total_duration /= 64;

	// We're done here.
	return static_cast<unsigned int>(total_duration);
}

/** SPC **/

/**
 * Read an SPC audio file.
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
SPC::SPC(const IRpFilePtr &file)
	: super(new SPCPrivate(file))
{
	RP_D(SPC);
	d->mimeType = "audio/x-spc";	// unofficial
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the SPC header.
	d->file->rewind();
	size_t size = d->file->read(&d->spcHeader, sizeof(d->spcHeader));
	if (size != sizeof(d->spcHeader)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(d->spcHeader), reinterpret_cast<const uint8_t*>(&d->spcHeader)},
		nullptr,	// ext (not needed for SPC)
		0		// szFile (not needed for SPC)
	};
	d->isValid = (isRomSupported_static(&info) >= 0);

	if (!d->isValid) {
		d->file.reset();
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

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Super NES SPC Audio", "SPC", "SPC", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SPC::loadFieldData(void)
{
	RP_D(SPC);
	if (!d->fields.empty()) {
		// Field data *has* been loaded...
		return 0;
	} else if (!d->file) {
		// File isn't open.
		return -EBADF;
	} else if (!d->isValid) {
		// Unknown file type.
		return -EIO;
	}

	// Get the ID666 tags.
	auto kv = d->parseTags();
	if (kv.empty()) {
		// No tags.
		return 0;
	}

	// SPC header
	d->fields.reserve(11);	// Maximum of 11 fields.

	// TODO: Add more tags.
	// TODO: Duration.

	// Song name
	auto iter = kv.find(SPC_xID6_ITEM_SONG_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("RomData|Audio", "Song Name"), kv.getStr(data));
		}
	}

	// Game name
	iter = kv.find(SPC_xID6_ITEM_GAME_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("SPC", "Game Name"), kv.getStr(data));
		}
	}

	// Artist
	iter = kv.find(SPC_xID6_ITEM_ARTIST_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("RomData|Audio", "Artist"), kv.getStr(data));
		}
	}

	// Copyright year
	iter = kv.find(SPC_xID6_ITEM_COPYRIGHT_YEAR);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			d->fields.addField_string_numeric(C_("SPC", "Copyright Year"), data.uvalue);
		}
	}

	// Duration
	unsigned int duration = d->getDurationMs(kv);
	if (duration > 0) {
		// Convert from milliseconds to centiseconds.
		duration /= 10;

		// Split minutes, seconds, and centiseconds for display purposes.
		const unsigned int cs = static_cast<unsigned int>(duration % 100);
		const unsigned int sec = static_cast<unsigned int>(duration / 100) % 60;
		const unsigned int min = static_cast<unsigned int>(duration / 100 / 60);
		d->fields.addField_string(C_("RomData|Audio", "Duration"),
			rp_sprintf("%u:%02u.%02u", min, sec, cs));
	}

	// Dumper
	iter = kv.find(SPC_xID6_ITEM_DUMPER_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("SPC", "Dumper"), kv.getStr(data));
		}
	}

	// Dump date
	iter = kv.find(SPC_xID6_ITEM_DUMP_DATE);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			d->fields.addField_dateTime(C_("SPC", "Dump Date"),
				data.timestamp,
				RomFields::RFT_DATETIME_HAS_DATE |
				RomFields::RFT_DATETIME_IS_UTC  // Date only.
			);
		}
	}

	// Comments
	iter = kv.find(SPC_xID6_ITEM_COMMENTS);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("RomData|Audio", "Comments"), kv.getStr(data));
		}
	}

	// Emulator used
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

			const char *const emulator_used_title = C_("SPC", "Emulator Used");
			if (emu) {
				d->fields.addField_string(emulator_used_title, emu);
			} else {
				d->fields.addField_string(emulator_used_title,
					rp_sprintf(C_("RomData", "Unknown (0x%02X)"), data.uvalue));
			}
		}
	}

	// OST title
	iter = kv.find(SPC_xID6_ITEM_OST_TITLE);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->fields.addField_string(C_("SPC", "OST Title"), kv.getStr(data));
		}
	}

	// OST disc number
	iter = kv.find(SPC_xID6_ITEM_OST_DISC);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			d->fields.addField_string_numeric(C_("SPC", "OST Disc #"), data.uvalue);
		}
	}

	// OST track number
	iter = kv.find(SPC_xID6_ITEM_OST_TRACK);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			// High byte: Track number. (0-99)
			// Low byte: Optional letter.
			// TODO: Restrict track number?
			char buf[32];
			const uint8_t track_num = data.uvalue >> 8;
			const char track_letter = data.uvalue & 0xFF;
			if (ISALNUM(track_letter)) {
				// Valid track letter.
				snprintf(buf, sizeof(buf), "%u%c", track_num, track_letter);
			} else {
				// Not a valid track letter.
				snprintf(buf, sizeof(buf), "%u", track_num);
			}

			d->fields.addField_string(C_("SPC", "OST Track #"), buf);
		}
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the field data hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int SPC::loadMetaData(void)
{
	RP_D(SPC);
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

	// Get the ID666 tags.
	auto kv = d->parseTags();
	if (kv.empty()) {
		// No tags.
		// TODO: Return 0 instead of -ENOENT?
		return -ENOENT;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(10);	// Maximum of 10 metadata properties.

	// TODO: Add more tags.
	// TODO: Duration.

	// Song name
	auto iter = kv.find(SPC_xID6_ITEM_SONG_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->metaData->addMetaData_string(Property::Title, kv.getStr(data));
		}
	}

	// Game name
	iter = kv.find(SPC_xID6_ITEM_GAME_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->metaData->addMetaData_string(Property::Album, kv.getStr(data));
		}
	}

	// Artist
	iter = kv.find(SPC_xID6_ITEM_ARTIST_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->metaData->addMetaData_string(Property::Artist, kv.getStr(data));
		}
	}

	// Copyright year (Release year)
	iter = kv.find(SPC_xID6_ITEM_COPYRIGHT_YEAR);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			d->metaData->addMetaData_uint(Property::ReleaseYear, data.uvalue);
		}
	}

	// Duration
	const unsigned int duration = d->getDurationMs(kv);
	if (duration > 0) {
		// NOTE: Property::Duration uses int, not unsigned int.
		d->metaData->addMetaData_integer(Property::Duration, static_cast<int>(duration));
	}

#if 0
	// Dumper
	// TODO: No "Dumper" property...
	iter = kv.find(SPC_xID6_ITEM_DUMPER_NAME);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->metaData->addMetaData_string(Property::Dumper, kv.getStr(data));
		}
	}
#endif

	// Dump date
	iter = kv.find(SPC_xID6_ITEM_DUMP_DATE);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			d->metaData->addMetaData_timestamp(Property::CreationDate, data.timestamp);
		}
	}

	// Comments
	iter = kv.find(SPC_xID6_ITEM_COMMENTS);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			// NOTE: Property::Comment is assumed to be user-added
			// on KDE Dolphin 18.08.1. Use Property::Description.
			d->metaData->addMetaData_string(Property::Description, kv.getStr(data));
		}
	}

#if 0
	// Emulator used
	// TODO: No property...
#endif

	// OST title
	// NOTE: Using "Compilation" as the property.
	iter = kv.find(SPC_xID6_ITEM_OST_TITLE);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(data.isStrIdx);
		if (data.isStrIdx) {
			d->metaData->addMetaData_string(Property::Compilation, kv.getStr(data));
		}
	}

	// OST disc number
	iter = kv.find(SPC_xID6_ITEM_OST_DISC);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			// TODO: Int or UInt on KDE?
			d->metaData->addMetaData_uint(Property::DiscNumber, data.uvalue);
		}
	}

	// OST track number
	iter = kv.find(SPC_xID6_ITEM_OST_TRACK);
	if (iter != kv.end()) {
		const auto &data = iter->second;
		assert(!data.isStrIdx);
		if (!data.isStrIdx) {
			// High byte: Track number. (0-99)
			// Low byte: Optional letter.
			// TODO: Restrict track number?
			// TODO: How to represent the letter here?
			// TODO: Int or UInt on KDE?
			const uint8_t track_num = data.uvalue >> 8;
			d->metaData->addMetaData_uint(Property::TrackNumber, track_num);
		}
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}

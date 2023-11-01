/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * SAP.cpp: Atari 8-bit SAP audio reader.                                  *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://asap.sourceforge.net/sap-format.html
// NOTE: The header format is plaintext, so we don't have a structs file.

#include "stdafx.h"
#include "SAP.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// C++ STL classes
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

class SAPPrivate final : public RomDataPrivate
{
public:
	SAPPrivate(const IRpFilePtr &file);

private:
	typedef RomDataPrivate super;
	RP_DISABLE_COPY(SAPPrivate)

public:
	/** RomDataInfo **/
	static const char *const exts[];
	static const char *const mimeTypes[];
	static const RomDataInfo romDataInfo;

public:
	// Parsed tags.
	struct sap_tags_t {
		bool tags_read;		// True if tags were read successfully.

		string author;		// Author
		string name;		// Song title
		string date;		// Date (TODO: Disambiguate year vs. date.)

		uint16_t songs;		// Number of songs in the file (Default is 1)
		uint16_t def_song;	// Default song (zero-based; default is 0)

		// TODO: Use a bitfield for flags?
		bool ntsc;		// True if NTSC tag is present.
		bool stereo;		// True if STEREO tag is present. (dual POKEY)

		char type;		// B, C, D, S
		uint16_t fastplay;	// Number of scanlines between calls of the player routine.
					// Default is one frame: 312 lines for PAL, 262 lines for NTSC.
		uint16_t init_addr;	// Init address (Required for Types B, D, and S; invalid for others)
		uint16_t music_addr;	// Music data address (Required for Type C; invalid for others)
		uint16_t player_addr;	// Player address
		uint16_t covox_addr;	// COVOX hardware address (If not specified, set to 0)

		// TIME tags.
		// - first: Duration, in milliseconds.
		// - second: Loop flag.
		vector<pair<uint32_t, bool> > durations;

		sap_tags_t() : tags_read(false), songs(1), def_song(0), ntsc(false), stereo(false)
				, type('\0'), fastplay(0), init_addr(0), music_addr(0), player_addr(0)
				, covox_addr(0) { }
	};

	/**
	 * Convert a duration to milliseconds + loop flag.
	 * @param str	[in] Duration string.
	 * @param pMs	[out] Milliseconds.
	 * @param pLoop	[out] Loop flag.
	 * @return 0 on success; non-zero on error.
	 */
	static int durationToMsLoop(const char *str, uint32_t *pMs, bool *pLoop);

	/**
	 * Parse the tags from the open SAP file.
	 * @return TagData object.
	 */
	sap_tags_t parseTags(void);
};

ROMDATA_IMPL(SAP)

/** SAPPrivate **/

/* RomDataInfo */
const char *const SAPPrivate::exts[] = {
	".sap",

	nullptr
};
const char *const SAPPrivate::mimeTypes[] = {
	// Unofficial MIME types.
	"audio/x-sap",

	nullptr
};
const RomDataInfo SAPPrivate::romDataInfo = {
	"SAP", exts, mimeTypes
};

SAPPrivate::SAPPrivate(const IRpFilePtr &file)
	: super(file, &romDataInfo)
{ }

/**
 * Convert a duration to milliseconds + loop flag.
 * @param str	[in] Duration string.
 * @param pMs	[out] Milliseconds.
 * @param pLoop	[out] Loop flag.
 * @return 0 on success; non-zero on error.
 */
int SAPPrivate::durationToMsLoop(const char *str, uint32_t *pMs, bool *pLoop)
{
	// Time format:
	// - One or two digits specifying minutes
	// - Colon
	// - Two digits specifying seconds
	// - Optional: Decimal point followed by one to three digits
	// - Optional: One space followed by four uppercase letters "LOOP"

	// Examples:
	// - 0:12
	// - 01:23.4
	// - 12:34.56
	// - 12:34.567

	// TODO: Consolidate code with PSF?
	// NOTE: PSF allows ',' for decimal; SAP doesn't.
	unsigned int min, sec, frac;

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

	// minutes:seconds.decimal
	int s = sscanf(str, "%u:%u.%u", &min, &sec, &frac);
	if (s == 3) {
		// Format matched.
		*pMs = (min * 60 * 1000) +
		       (sec * 1000) +
		       (frac * frac_adj);
	} else {
		// minutes:seconds
		int s = sscanf(str, "%u:%u", &min, &sec);
		if (s == 3) {
			// Format matched.
			*pMs = (min * 60 * 1000) +
			       (sec * 1000);
		} else {
			// No match.
			return -EINVAL;
		}
	}

	// Check for "LOOP".
	*pLoop = false;
	const char *space = strchr(str, ' ');
	if (space) {
		space++;
		if (!strncasecmp(space, "LOOP", 4) && (space[4] == '\0' || ISSPACE(space[4]))) {
			// Found "LOOP".
			*pLoop = true;
		}
	}

	// Done.
	return 0;
}

/**
 * Parse the tags from the open SAP file.
 * @return TagData object.
 */
SAPPrivate::sap_tags_t SAPPrivate::parseTags(void)
{
	sap_tags_t tags;

	// Read up to 4 KB from the beginning of the file.
	// TODO: Support larger headers?
	unique_ptr<char[]> header(new char[4096+1]);
	size_t sz = file->seekAndRead(0, header.get(), 4096);
	if (sz < 6) {
		// Not enough data for "SAP\n" and 0xFFFF.
		return tags;
	}
	header[sz] = 0;	// ensure NULL-termination for strtok_r()

	// Verify the header.
	// NOTE: SAP is defined as using CRLF line endings,
	// but we'll allow LF line endings too.
	char *str;
	if (!memcmp(header.get(), "SAP\r\n", 5)) {
		// Standard SAP header.
		str = &header[5];
	} else if (!memcmp(header.get(), "SAP\n", 4)) {
		// SAP header with Unix line endings.
		str = &header[4];
	} else {
		// Invalid header.
		return tags;
	}

	// Parse each line.
	char *saveptr = nullptr;
	for (char *token = strtok_r(str, "\n", &saveptr);
	     token != nullptr; token = strtok_r(nullptr, "\n", &saveptr))
	{
		// Check if this is the end of the tags.
		if (token[0] == (char)0xFF) {
			// End of tags.
			break;
		}

		// Find the first space. This delimits the keyword.
		char *space = strchr(token, ' ');
		char *params;
		if (space) {
			// Found a space. Zero it out so we can check the keyword.
			params = space+1;
			*space = '\0';

			// Check for the first non-space character in the parameter.
			for (; *params != '\0' && ISSPACE(*params); params++) { }
			if (*params == '\0') {
				// No non-space characters.
				// Ignore the parameter.
				params = nullptr;
			}
		} else {
			// No space. This means no parameters are present.
			params = nullptr;

			// Remove '\r' if it's present.
			char *cr = strchr(token, '\r');
			if (cr) {
				*cr = '\0';
			}
		}

		// Check the keyword.
		// NOTE: Official format uses uppercase tags, but we'll allow mixed-case.
		// NOTE: String encoding is the common subset of ASCII and ATASCII.
		// TODO: ascii_to_utf8()?
		// TODO: Check for duplicate keywords?
		enum class KeywordType : uint8_t {
			Unknown = 0,

			Bool,		// bool: Keyword presence sets the value to true.
			UInt16_Dec,	// uint16 dec: Parameter is a decimal uint16_t.
			UInt16_Hex,	// uint16 hex: Parameter is a hexadecimal uint16_t.
			Char,		// char: Parameter is a single character.
			String,		// string: Parameter is a string, enclosed in double-quotes.
			TimeLoop,	// time+loop: Parameter is a duration, plus an optional "LOOP" setting.
		};
		struct KeywordDef {
			char keyword[15];	// Keyword, e.g. "AUTHOR".
			KeywordType type;	// Keyword type.
			void *ptr;		// Data pointer.
		};

		const std::array<KeywordDef, 14> kwds = {{
			{"AUTHOR",	KeywordType::String,		&tags.author},
			{"NAME",	KeywordType::String,		&tags.name},
			{"DATE",	KeywordType::String,		&tags.date},
			{"SONGS",	KeywordType::UInt16_Dec,	&tags.songs},
			{"DEFSONG",	KeywordType::UInt16_Dec,	&tags.def_song},
			{"STEREO",	KeywordType::Bool,		&tags.stereo},
			{"NTSC",	KeywordType::Bool,		&tags.ntsc},
			{"TYPE",	KeywordType::Char,		&tags.type},
			{"FASTPLAY",	KeywordType::UInt16_Dec,	&tags.fastplay},
			{"INIT",	KeywordType::UInt16_Hex,	&tags.init_addr},
			{"MUSIC",	KeywordType::UInt16_Hex,	&tags.music_addr},
			{"PLAYER",	KeywordType::UInt16_Hex,	&tags.player_addr},
			{"COVOX",	KeywordType::UInt16_Hex,	&tags.covox_addr},
			// TIME is handled separately.
			{"TIME",	KeywordType::TimeLoop,		nullptr},
		}};

		// TODO: Show errors for unsupported tags?
		// TODO: Print errors?
		for (const KeywordDef &kwd : kwds) {
			if (strcasecmp(kwd.keyword, token) != 0) {
				// Not a match. Keep going.
				continue;
			}

			switch (kwd.type) {
				default:
					assert(!"Unsupported keyword type.");
					break;

				case KeywordType::Bool:
					// Presence of this keyword sets the value to true.
					*(static_cast<bool*>(kwd.ptr)) = true;
					break;

				case KeywordType::UInt16_Dec: {
					// Decimal value.
					if (!params)
						break;

					char *endptr = nullptr;
					long val = strtol(params, &endptr, 10);
					if (endptr > params && (*endptr == '\0' || ISSPACE(*endptr))) {
						*(static_cast<uint16_t*>(kwd.ptr)) = static_cast<uint16_t>(val);
					}
					break;
				}

				case KeywordType::UInt16_Hex: {
					// Hexadecimal value.
					if (!params)
						break;

					char *endptr = nullptr;
					long val = strtol(params, &endptr, 16);
					if (endptr > params && (*endptr == '\0' || ISSPACE(*endptr))) {
						*(static_cast<uint16_t*>(kwd.ptr)) = static_cast<uint16_t>(val);
					}
					break;
				}

				case KeywordType::Char: {
					// Character.
					if (!params)
						break;

					if (!ISSPACE(params[0] && (params[1] == '\0' || ISSPACE(params[1])))) {
						// Single character.
						*(static_cast<char*>(kwd.ptr)) = params[0];
					}
					break;
				}

				case KeywordType::String: {
					// String value.
					if (!params)
						break;

					// String must be enclosed in double-quotes.
					// TODO: Date parsing?
					if (*params != '"') {
						// Invalid tag. (TODO: Print an error?)
						continue;
					}
					// Find the second '"'.
					char *dblq = strchr(params+1, '"');
					if (!dblq) {
						// Invalid tag. (TODO: Print an error?)
						continue;
					}
					// Zero it out and take the string.
					*dblq = '\0';
					*(static_cast<string*>(kwd.ptr)) = latin1_to_utf8(params+1, -1);
					break;
				}

				case KeywordType::TimeLoop: {
					// Duration, plus optional "LOOP" keyword.
					// TODO: Verify that we don't go over the song count?
					if (tags.durations.empty()) {
						// Reserve space.
						tags.durations.reserve(tags.songs);
					}

					uint32_t duration;
					bool loop_flag;
					if (!durationToMsLoop(params, &duration, &loop_flag)) {
						// Parsed successfully.
						tags.durations.emplace_back(duration, loop_flag);
					}
					break;
				}
			}

			// Keyword parsed.
			break;
		}
	}

	// Tags parsed.
	tags.tags_read = true;
	return tags;
}

/** SAP **/

/**
 * Read an SAP audio file.
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
SAP::SAP(const IRpFilePtr &file)
	: super(new SAPPrivate(file))
{
	RP_D(SAP);
	d->mimeType = "audio/x-sap";	// unofficial
	d->fileType = FileType::AudioFile;

	if (!d->file) {
		// Could not ref() the file handle.
		return;
	}

	// Read the SAP header.
	uint8_t buf[16];
	d->file->rewind();
	size_t size = d->file->read(buf, sizeof(buf));
	if (size != sizeof(buf)) {
		d->file.reset();
		return;
	}

	// Check if this file is supported.
	const DetectInfo info = {
		{0, sizeof(buf), buf},
		nullptr,	// ext (not needed for SAP)
		0		// szFile (not needed for SAP)
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
int SAP::isRomSupported_static(const DetectInfo *info)
{
	assert(info != nullptr);
	assert(info->header.pData != nullptr);
	assert(info->header.addr == 0);
	if (!info || !info->header.pData ||
	    info->header.addr != 0 ||
	    info->header.size < 6)
	{
		// Either no detection information was specified,
		// or the header is too small.
		return -1;
	}

	// Check for "SAP\r\n" and "SAP\n".
	if (info->header.size >= 7 && !memcmp(info->header.pData, "SAP\r\n", 5)) {
		// Found the SAP magic number. (CRLF line endings)
		return 0;
	} else if (!memcmp(info->header.pData, "SAP\n", 4)) {
		// Found the SAP magic number. (LF line endings)
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
const char *SAP::systemName(unsigned int type) const
{
	RP_D(const SAP);
	if (!d->isValid || !isSystemNameTypeValid(type))
		return nullptr;

	// SAP has the same name worldwide, so we can
	// ignore the region selection.
	static_assert(SYSNAME_TYPE_MASK == 3,
		"SAP::systemName() array index optimization needs to be updated.");

	// Bits 0-1: Type. (long, short, abbreviation)
	static const char *const sysNames[4] = {
		"Atari 8-bit SAP Audio", "SAP", "SAP", nullptr
	};

	return sysNames[type & SYSNAME_TYPE_MASK];
}

/**
 * Load field data.
 * Called by RomData::fields() if the field data hasn't been loaded yet.
 * @return Number of fields read on success; negative POSIX error code on error.
 */
int SAP::loadFieldData(void)
{
	RP_D(SAP);
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

	// Get the tags.
	const SAPPrivate::sap_tags_t tags = d->parseTags();
	if (!tags.tags_read) {
		// No tags.
		return 0;
	}

	// SAP header.
	d->fields.reserve(11);	// Maximum of 11 fields.

	// Author
	if (!tags.author.empty()) {
		d->fields.addField_string(C_("RomData|Audio", "Author"), tags.author);
	}

	// Song title
	if (!tags.name.empty()) {
		d->fields.addField_string(C_("RomData|Audio", "Song Title"), tags.name);
	}

	// Date (TODO: Parse?)
	if (!tags.date.empty()) {
		d->fields.addField_string(C_("SAP", "Date"), tags.date);
	}

	// Number of songs
	d->fields.addField_string_numeric(C_("RomData|Audio", "# of Songs"), tags.songs);

	// Default song number
	if (tags.songs > 1) {
		d->fields.addField_string_numeric(C_("RomData|Audio", "Default Song #"), tags.def_song);
	}

	// Flags: NTSC/PAL, Stereo
	static const char *const flags_names[] = {
		// tr: PAL is default; if set, the file is for NTSC.
		"NTSC",
		NOP_C_("SAP|Flags", "Stereo"),
	};
	vector<string> *const v_flags_names = RomFields::strArrayToVector_i18n(
		"SAP|Flags", flags_names, ARRAY_SIZE(flags_names));
	// TODO: Use a bitfield in tags?
	uint32_t flags = 0;
	if (tags.ntsc)   flags |= (1U << 0);
	if (tags.stereo) flags |= (1U << 1);
	d->fields.addField_bitfield(C_("SAP", "Flags"),
		v_flags_names, 0, flags);

	// Type
	// TODO: Verify that the type is valid?
	const char *const type_title = C_("SAP", "Type");
	if (ISALPHA(tags.type)) {
		const char s_tag_type[2] = {tags.type, '\0'};
		d->fields.addField_string(type_title, s_tag_type);
	} else {
		d->fields.addField_string(type_title,
			rp_sprintf("0x%02X", static_cast<unsigned int>(tags.type)),
			RomFields::STRF_MONOSPACE);
	}

	// Fastplay. (Number of scanlines)
	unsigned int scanlines = tags.fastplay;
	if (scanlines == 0) {
		// Use the default value for NTSC/PAL.
		scanlines = (tags.ntsc ? 262 : 312);
	}
	d->fields.addField_string_numeric(C_("SAP", "Fastplay"), scanlines);

	// Init address (Types B, D, S) / music address (Type C)
	switch (toupper(tags.type)) {
		default:
			// Skipping for invalid types.
			break;

		case 'B': case 'D': case 'S':
			d->fields.addField_string_numeric(C_("SAP", "Init Address"),
				tags.init_addr,
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
			break;

		case 'C':
			d->fields.addField_string_numeric(C_("SAP", "Music Address"),
				tags.music_addr,
				RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
			break;
	}

	// Player address.
	d->fields.addField_string_numeric(C_("SAP", "Player Address"),
		tags.player_addr,
		RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);

	// COVOX address. (if non-zero)
	if (tags.covox_addr != 0) {
		d->fields.addField_string_numeric(C_("SAP", "COVOX Address"),
			tags.covox_addr,
			RomFields::Base::Hex, 4, RomFields::STRF_MONOSPACE);
	}

	// Song list.
	if (!tags.durations.empty()) {
		const char *const s_yes = C_("RomData", "Yes");
		const char *const s_no = C_("RomData", "No");

		unsigned int song_num = 0;
		auto song_list = new RomFields::ListData_t(tags.durations.size());
		auto src_iter = tags.durations.cbegin();
		auto dest_iter = song_list->begin();
		const auto song_list_end = song_list->end();
		for ( ; dest_iter != song_list_end; ++src_iter, ++dest_iter, song_num++) {
			vector<string> &data_row = *dest_iter;
			data_row.reserve(3);	// 3 fields per row.

			// Format as m:ss.ddd.
			// TODO: Separate function?
			const uint32_t duration = src_iter->first;
			const uint32_t min = (duration / 1000) / 60;
			const uint32_t sec = (duration / 1000) % 60;
			const uint32_t ms =  (duration % 1000);
			data_row.emplace_back(rp_sprintf("%u", song_num));
			data_row.emplace_back(rp_sprintf("%u:%02u.%03u", min, sec, ms));
			data_row.emplace_back(src_iter->second ? s_yes : s_no);
		}

		static const char *const song_list_hdr[3] = {
			NOP_C_("SAP|SongList", "#"),
			NOP_C_("RomData|Audio", "Duration"),
			NOP_C_("SAP|SongList", "Looping"),
		};
		vector<string> *const v_song_list_hdr = RomFields::strArrayToVector_i18n(
			"SAP|SongList", song_list_hdr, ARRAY_SIZE(song_list_hdr));

		RomFields::AFLD_PARAMS params;
		params.headers = v_song_list_hdr;
		params.data.single = song_list;
		d->fields.addField_listData(C_("SAP", "Song List"), &params);
	}

	// Finished reading the field data.
	return static_cast<int>(d->fields.count());
}

/**
 * Load metadata properties.
 * Called by RomData::metaData() if the metadata hasn't been loaded yet.
 * @return Number of metadata properties read on success; negative POSIX error code on error.
 */
int SAP::loadMetaData(void)
{
	RP_D(SAP);
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

	// Get the tags.
	const SAPPrivate::sap_tags_t tags = d->parseTags();
	if (!tags.tags_read) {
		// No tags.
		return 0;
	}

	// Create the metadata object.
	d->metaData = new RomMetaData();
	d->metaData->reserve(4);	// Maximum of 4 metadata properties.

	// Composer
	if (!tags.author.empty()) {
		d->metaData->addMetaData_string(Property::Composer, tags.author);
	}

	// Song title
	if (!tags.name.empty()) {
		d->metaData->addMetaData_string(Property::Title, tags.name);
	}

	// TODO: Date

	// Number of channels
	d->metaData->addMetaData_integer(Property::Channels, (tags.stereo ? 2 : 1));

	// NOTE: Including all songs in the duration.
	const uint32_t duration = std::accumulate(tags.durations.cbegin(), tags.durations.cend(), 0U,
		[](uint32_t a, const pair<uint32_t, bool> &tag_duration) noexcept -> uint32_t {
			return a + tag_duration.first;
		}
	);
	if (duration > 0) {
		d->metaData->addMetaData_integer(Property::Duration, static_cast<int>(duration));
	}

	// Finished reading the metadata.
	return static_cast<int>(d->metaData->count());
}

}

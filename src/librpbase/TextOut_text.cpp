/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextOut.hpp: Text output for RomData. (User-readable text)              *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TextOut.hpp"

// C includes (C++ namespace)
#include <cassert>

// C++ STL classes
using std::array;
using std::max;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

// libi18n
#include "libi18n/i18n.h"

// librpbase
#include "RomData.hpp"
#include "RomFields.hpp"
#include "SystemRegion.hpp"

// librptext
#include "librptext/conversion.hpp"
#include "librptext/utf8_strlen.hpp"
using namespace LibRpText;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// TextOut_text isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern unsigned char RP_LibRpBase_TextOut_text_ForceLinkage;
	unsigned char RP_LibRpBase_TextOut_text_ForceLinkage;
}

namespace LibRpBase {

class StreamStateSaver {
	RP_DISABLE_COPY(StreamStateSaver)

	std::ios &stream;	// Stream being adjusted.
	std::ios state;		// Copy of original flags.
public:
	explicit StreamStateSaver(std::ios &stream)
		: stream(stream)
		, state(nullptr)
	{
		// Save the stream's state.
		state.copyfmt(stream);
	}

	~StreamStateSaver()
	{
		// Restore the stream's state.
		stream.copyfmt(state);
	}
};

class Pad {
	size_t width;
public:
	explicit Pad(size_t width) :width(width) {}
	friend ostream& operator<<(ostream& os, const Pad& pad) {
		StreamStateSaver state(os);
		for (size_t x = pad.width; x > 0; x--) {
			os << ' ';
		}
		return os;
	}

};
class ColonPad {
	size_t width;
	const char* str;
public:
	ColonPad(size_t width, const char* str) :width(width), str(str) {}
	friend ostream& operator<<(ostream& os, const ColonPad& cp) {
		StreamStateSaver state(os);
		const size_t str_sz = utf8_disp_strlen(cp.str);
		os << cp.str << ':';
		for (size_t x = str_sz; x < cp.width-1; x++) {
			os << ' ';
		}
		return os;
	}
};
class SafeString {
	const char *str;
	size_t len;
	size_t width;
	bool quotes;
public:
	explicit SafeString(const char *str, bool quotes = true, size_t width = 0)
		: str(str)
		, len(str ? strlen(str) : 0)
		, width(width)
		, quotes(quotes) { }

	explicit SafeString(const char *str, size_t len, bool quotes = true, size_t width = 0)
		: str(str)
		, len(len)
		, width(width)
		, quotes(quotes) { }

	explicit SafeString(const string *str, bool quotes = true, size_t width = 0)
		: str(str ? str->c_str() : nullptr)
		, len(str ? str->size() : 0)
		, width(width)
		, quotes(quotes) { }

	explicit SafeString(const string &str, bool quotes = true, size_t width = 0)
		: str(str.c_str())
		, len(str.size())
		, width(width)
		, quotes(quotes) { }

private:
	static string process(const SafeString& cp)
	{
		// NOTE: We have to use a temporary string here because
		// the caller might be using setw() for field padding.
		// TODO: Try optimizing it out while preserving setw().
		assert(cp.str != nullptr);
		if (!cp.str || cp.len == 0) {
			// nullptr or empty string.
			return "''";
		}

		string escaped;
		escaped.reserve(cp.len + (cp.quotes ? 2 : 0));
		if (cp.quotes) {
			escaped += '\'';
		}

		size_t n = cp.len;
		for (const char *p = cp.str; n > 0; p++, n--) {
			if (cp.width && *p == '\n') {
				escaped += '\n';
				escaped.append(cp.width + (cp.quotes ? 1 : 0), ' ');
			} else if (static_cast<unsigned char>(*p) < 0x20) {
				// Encode control characters using U+2400 through U+241F.
				escaped += "\xE2\x90";
				escaped += static_cast<char>(0x80 + static_cast<unsigned char>(*p));
			} else {
				escaped += *p;
			}
		}

		if (cp.quotes) {
			escaped += '\'';
		}

		return escaped;
	}

public:
	friend ostream& operator<<(ostream& os, const SafeString& cp) {
		if (!cp.str) {
			return os << "(null)";
		}

		return os << process(cp);
	}

	operator string() const {
		return process(*this);
	}
};

class StringField {
	size_t width;
	const RomFields::Field &romField;
public:
	StringField(size_t width, const RomFields::Field &romField)
		: width(width), romField(romField) { }
	friend ostream& operator<<(ostream& os, const StringField& field) {
		// NOTE: nullptr string is an empty string, not an error.
		auto romField = field.romField;
		os << ColonPad(field.width, romField.name);
		if (romField.data.str) {
			os << SafeString(romField.data.str, true, field.width);
		} else {
			// Empty string.
			os << "''";
		}
		return os;
	}
};

class BitfieldField {
	size_t width;
	const RomFields::Field &romField;
public:
	BitfieldField(size_t width, const RomFields::Field &romField)
		: width(width), romField(romField) { }
	friend ostream& operator<<(ostream& os, const BitfieldField& field) {
		auto romField = field.romField;
		const auto &bitfieldDesc = romField.desc.bitfield;
		assert(bitfieldDesc.names != nullptr);
		if (!bitfieldDesc.names) {
			return os << "[ERROR: No bitfield names.]";
		}
		const unsigned int perRow = static_cast<unsigned int>(
			(bitfieldDesc.elemsPerRow ? bitfieldDesc.elemsPerRow : 4));

		unique_ptr<size_t[]> colSize(new size_t[perRow]());
		assert(bitfieldDesc.names->size() <= 32);

		// Determine the column widths.
		unsigned int col = 0;
		for (const string &name : *(bitfieldDesc.names)) {
			if (name.empty()) {
				continue;
			}

			colSize[col] = max(utf8_disp_strlen(name), colSize[col]);
			col++;
			if (col == perRow) {
				col = 0;
			}
		}

		// Print the bits.
		// FIXME: Why do we need to subtract 1 here to correctly align
		// the first-row boxes? Maybe it should be somewhere else...
		os << ColonPad(field.width-1, romField.name);
		StreamStateSaver state(os);
		col = 0;
		uint32_t bitfield = romField.data.bitfield;
		for (const string &name : *(bitfieldDesc.names)) {
			if (name.empty()) {
				bitfield >>= 1;
				continue;
			}

			// Update the current column number before printing.
			// This prevents an empty row from being printed
			// if the number of valid elements is divisible by
			// the column count.
			if (col == perRow) {
				os << '\n' << Pad(field.width);
				col = 0;
			} else {
				os << ' ';
			}

			const size_t str_sz = utf8_disp_strlen(name);
			os << '[' << ((bitfield & 1) ? '*' : ' ') << "] " << name;
			for (size_t x = str_sz; x < colSize[col]; x++) {
				os << ' ';
			}

			col++;
			bitfield >>= 1;
		}
		return os;
	}
};

/**
 * Format an RFT_DATETIME field (or an `is_timestamp` column in RFT_LISTDATA).
 * @param buf		[out] Output buffer
 * @param size		[in] Size of `buf`
 * @param timestamp	[in] Timestamp to format
 * @param dtflags	[in] DateTimeFlags
 * @return 0 on success; non-zero on error.
 */
static int formatDateTime(char *buf, size_t size, time_t timestamp, RomFields::DateTimeFlags dtflags)
{
	struct tm tm_struct;
	struct tm *ret;
	if (dtflags & RomFields::RFT_DATETIME_IS_UTC) {
		ret = gmtime_r(&timestamp, &tm_struct);
	} else {
		tzset();
		ret = localtime_r(&timestamp, &tm_struct);
	}

	if (!ret) {
		// gmtime_r() or localtime_r() failed.
		return -1;
	}

	if (likely(SystemRegion::getLanguageCode() != 0)) {
		// Localized time format
		static constexpr char formats_strtbl[] =
			"\0"		// [0] No date or time
			"%x\0"		// [1] Date
			"%X\0"		// [4] Time
			"%x %X\0"	// [7] Date Time

			// TODO: Better localization here.
			"\0"		// [13] No date or time
			"%b %d\0"	// [14] Date (no year)
			"%X\0"		// [20] Time
			"%b %d %X\0";	// [23] Date Time (no year)
		static constexpr array<uint8_t, 8> formats_offtbl = {0, 1, 4, 7, 13, 14, 20, 23};
		static_assert(sizeof(formats_strtbl) == 33, "formats_offtbl[] needs to be recalculated");

		const unsigned int offset = (dtflags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK);
		const char *format = &formats_strtbl[formats_offtbl[offset]];
		assert(format[0] != '\0');
		if (likely(format[0] != '\0')) {
			strftime(buf, size, format, &tm_struct);
		} else {
			return -2;
		}
	} else {
		// LC_ALL=C
		// Always use the same format regardless of platform.
		// This is needed on Windows because LC_ALL doesn't affect
		// MSVCRT's strftime().

		// Month names for dates without years
		static constexpr char months[12][4] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};

		switch (dtflags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK) {
			case 0:
			case RomFields::RFT_DATETIME_NO_YEAR:
			default:
				// Nothing to do...
				return -3;

			case RomFields::RFT_DATETIME_HAS_DATE:
				// Date, with year
				snprintf(buf, size, "%04d/%02d/%02d",
					tm_struct.tm_year + 1900, tm_struct.tm_mon + 1, tm_struct.tm_mday);
				break;
			case RomFields::RFT_DATETIME_HAS_TIME:
			case RomFields::RFT_DATETIME_HAS_TIME | RomFields::RFT_DATETIME_NO_YEAR:
				// Time
				snprintf(buf, size, "%02d:%02d:%02d",
					 tm_struct.tm_hour, tm_struct.tm_min, tm_struct.tm_sec);
				break;
			case RomFields::RFT_DATETIME_HAS_DATE |
			     RomFields::RFT_DATETIME_HAS_TIME:
				// Date and time (with year)
				snprintf(buf, size, "%04d/%02d/%02d %02d:%02d:%02d",
					tm_struct.tm_year + 1900, tm_struct.tm_mon + 1, tm_struct.tm_mday,
					tm_struct.tm_hour, tm_struct.tm_min, tm_struct.tm_sec);
				break;

			case RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_NO_YEAR: {
				// Date, without year
				const char *const s_mon = (tm_struct.tm_mon >= 0 && tm_struct.tm_mon < 12)
					? months[tm_struct.tm_mon]
					: "Unk";

				snprintf(buf, size, "%s %02d", s_mon, tm_struct.tm_mday);
				break;
			}

			case RomFields::RFT_DATETIME_HAS_DATE |
			     RomFields::RFT_DATETIME_HAS_TIME | RomFields::RFT_DATETIME_NO_YEAR: {
				// Date and time (without year)
				const char *const s_mon = (tm_struct.tm_mon >= 0 && tm_struct.tm_mon < 12)
					? months[tm_struct.tm_mon]
					: "Unk";

				snprintf(buf, size, "%s %02d %02d:%02d:%02d",
					s_mon, tm_struct.tm_mday,
					tm_struct.tm_hour, tm_struct.tm_min, tm_struct.tm_sec);
				break;
			}
		}
	}

	return 0;
}

class ListDataField {
	size_t width;
	const RomFields::Field &romField;
	uint32_t def_lc;	// ROM-default language code.
	uint32_t user_lc;	// User-specified language code.
	unsigned int flags;
public:
	ListDataField(size_t width, const RomFields::Field &romField, uint32_t def_lc, uint32_t user_lc, unsigned int flags)
		: width(width), romField(romField), def_lc(def_lc), user_lc(user_lc), flags(flags) { }
	friend ostream& operator<<(ostream& os, const ListDataField& field) {
		auto romField = field.romField;
		os << ColonPad(field.width, romField.name);

		const auto &listDataDesc = romField.desc.list_data;
		// NOTE: listDataDesc.names can be nullptr,
		// which means we don't have any column headers.

		// Get the ListData_t container.
		const RomFields::ListData_t *pListData = nullptr;
		if (romField.flags & RomFields::RFT_LISTDATA_MULTI) {
			// ROM must have set a default language code.
			assert(field.def_lc != 0);

			// Determine the language to use.
			const auto *const pListData_multi = romField.data.list_data.data.multi;
			assert(pListData_multi != nullptr);
			assert(!pListData_multi->empty());
			if (pListData_multi && !pListData_multi->empty()) {
				// Get the ListData_t.
				pListData = RomFields::getFromListDataMulti(pListData_multi, field.def_lc, field.user_lc);
			}
		} else {
			// Single language.
			pListData = romField.data.list_data.data.single;
		}

		assert(pListData != nullptr);
		if (!pListData) {
			return os << C_("TextOut", "[ERROR: No list data.]");
		}

		if (field.flags & OF_SkipListDataMoreThan10) {
			if (pListData->size() > 10) {
				return os << C_("TextOut", "[More than 10 items; skipping...]");
			}
		}

		unsigned int col_count = 1;
		if (listDataDesc.names) {
			col_count = static_cast<unsigned int>(listDataDesc.names->size());
		} else {
			// No column headers.
			// Use the first row.
			if (pListData && !pListData->empty()) {
				col_count = static_cast<unsigned int>(pListData->at(0).size());
			}
		}
		assert(col_count > 0);
		if (col_count <= 0) {
			return os << C_("TextOut", "[ERROR: No list data.]");
		}

		/** Calculate the column widths. **/

		// Column names
		unique_ptr<size_t[]> colSize(new size_t[col_count]());
		size_t totalWidth = col_count + 1;
		if (listDataDesc.names) {
			unsigned int i = 0;
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			for (const string &name : *(listDataDesc.names)) {
				colSize[i] = utf8_disp_strlen(name);

				if (unlikely(is_timestamp & 1)) {
					// This is a timestamp column.
					// Use a dummy timestamp to figure out the width.
					char buf[128];
					int ret = formatDateTime(buf, sizeof(buf), 0,
						listDataDesc.col_attrs.dtflags);
					if (likely(ret == 0)) {
						// Got the column width.
						colSize[i] = std::max(colSize[i], utf8_disp_strlen(buf));
					}
				}

				// Next column
				is_timestamp >>= 1;
				i++;
			}
		} else if (listDataDesc.col_attrs.is_timestamp != 0) {
			// No column names, but at least one column has timestamps.
			unsigned int i = 0;
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			for (; is_timestamp != 0 && i < col_count; i++, is_timestamp >>= 1) {
				if (unlikely(is_timestamp & 1)) {
					// This is a timestamp column.
					// Use a dummy timestamp to figure out the width.
					char buf[128];
					int ret = formatDateTime(buf, sizeof(buf), 0,
						listDataDesc.col_attrs.dtflags);
					if (likely(ret == 0)) {
						// Got the column width.
						colSize[i] = utf8_disp_strlen(buf);
					}
				}
			}
		}

		// Row data
		// FIXME: Handle control characters (U+0000-U+001F) as fullwidth.
		unique_ptr<unsigned int[]> nl_count(new unsigned int[pListData->size()]());
		unsigned int row = 0;
		const auto pListData_cend = pListData->cend();
		for (auto it = pListData->cbegin(); it != pListData_cend; ++it, row++) {
			unsigned int col = 0;
			unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
			const auto it_cend = it->cend();
			for (auto jt = it->cbegin(); jt != it_cend; ++jt, col++, is_timestamp >>= 1) {
				if (unlikely(is_timestamp & 1)) {
					// Timestamp field. No newlines here.
					nl_count[row] = 0;
					continue;
				}

				// Check for newlines.
				unsigned int nl_row = 0;
				const size_t str_sz = jt->size();
				size_t prev_pos = 0;
				size_t cur_pos;
				do {
					size_t cur_sz;	// in bytes
					size_t col_sz;	// in UTF-8 characters
					cur_pos = jt->find('\n', prev_pos);
					if (cur_pos == string::npos) {
						// End of string.
						cur_sz = str_sz - prev_pos;
						col_sz = utf8_disp_strlen(&(*jt)[prev_pos], cur_sz);
					} else {
						// Found a newline.
						cur_sz = cur_pos - prev_pos;
						col_sz = utf8_disp_strlen(&(*jt)[prev_pos], cur_sz);
						prev_pos = cur_pos + 1;
						nl_row++;
					}
					colSize[col] = max(col_sz, colSize[col]);
				} while (cur_pos != string::npos && prev_pos < str_sz);

				// Update the newline count for this row.
				nl_count[row] = max(nl_count[row], nl_row);
			}
		}

		// Extra spacing for checkboxes
		// TODO: Use a separate column for the checkboxes?
		if (romField.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Prepend 4 spaces in column 0 for "[x] ".
			colSize[0] += 4;
		}

		/** Print the list data. **/
		StreamStateSaver state(os);

		// Print the list on a separate row from the field name?
		const bool separateRow = !!(romField.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW);
		if (separateRow) {
			os << '\n';
		}

		bool skipFirstNL = true;
		if (listDataDesc.names) {
			// Print the column names.
			unsigned int col = 0;
			unsigned int align = listDataDesc.col_attrs.align_headers;
			const auto names_cend = listDataDesc.names->cend();
			for (auto it = listDataDesc.names->cbegin(); it != names_cend; ++it, ++col, align >>= 2) {
				// FIXME: What was this used for?
				totalWidth += colSize[col]; // this could be in a separate loop, but whatever
				os << '|';
				const size_t str_sz = utf8_disp_strlen(*it);
				switch (align & 3) {
					case TXA_L:
						// Left alignment
						os << *it;
						for (size_t x = str_sz; x < colSize[col]; x++) {
							os << ' ';
						}
						break;
					default:
					case TXA_D:
					case TXA_C: {
						// Center alignment (default)
						// For odd sizes, the extra space
						// will be on the right.
						const size_t spc = colSize[col] - str_sz;
						for (size_t x = spc/2; x > 0; x--) {
							os << ' ';
						}
						os << *it;
						for (size_t x = (spc/2) + (spc%2); x > 0; x--) {
							os << ' ';
						}
						break;
					}
					case TXA_R:
						// Right alignment
						for (size_t x = str_sz; x < colSize[col]; x++) {
							os << ' ';
						}
						os << *it;
						break;
				}
			}
			os << '|' << '\n';

			// Separator between the headers and the data.
			if (!separateRow) {
				os << Pad(field.width);
			}
			for (col = 0; col < col_count; col++) {
				os << '|' << string(colSize[col], '-');
			}
			os << '|';

			// Don't skip the first newline, since we're
			// printing headers.
			skipFirstNL = false;
		}

		uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
		if (romField.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Remove the 4 spaces in column 0.
			// Those spaces will not be used in the text area.
			colSize[0] -= 4;
		}

		// Current line position.
		// NOTE: Special handling is needed for npos due to the use of unsigned int.
		unique_ptr<unsigned int[]> linePos(new unsigned int[col_count]);

		row = 0;
		for (auto it = pListData->cbegin(); it != pListData_cend; ++it, row++) {
			// Print one line at a time for multi-line entries.
			// TODO: Better formatting for multi-line?
			// Right now we're assuming that at least one column is a single line.
			// If all columns are multi-line, then everything will look like it's
			// all single-line entries.
			memset(linePos.get(), 0, col_count * sizeof(linePos[0]));
			// NOTE: nl_count[row] is 0 for single-line items.
			for (int line = nl_count[row]; line >= 0; line--) {
				if (!skipFirstNL) {
					os << '\n';
					if (!separateRow) {
						os << Pad(field.width);
					}
				} else {
					skipFirstNL = false;
				}
				os << '|';
				if (romField.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
					os << '[' << ((checkboxes & 1) ? 'x' : ' ') << "] ";
					checkboxes >>= 1;
				}
				unsigned int col = 0;
				unsigned int align = listDataDesc.col_attrs.align_data;
				unsigned int is_timestamp = listDataDesc.col_attrs.is_timestamp;
				const auto it_cend = it->cend();
				for (auto jt = it->cbegin(); jt != it_cend; ++jt, ++col, align >>= 2, is_timestamp >>= 1) {
					string str;
					if (nl_count[row] == 0) {
						// No newlines. Print the string directly.
						if (unlikely((is_timestamp & 1) && jt->size() == sizeof(int64_t))) {
							// Timestamp column. Format the timestamp.
							RomFields::TimeString_t time_string;
							memcpy(time_string.str, jt->data(), 8);

							char buf[128];
							int ret = formatDateTime(buf, sizeof(buf),
								static_cast<time_t>(time_string.time),
								listDataDesc.col_attrs.dtflags);
							if (likely(ret == 0)) {
								str.assign(buf);
							} else {
								str = C_("RomData", "Unknown");
							}
						} else {
							// Not a timestamp column. Use the string as-is.
							str = SafeString(*jt, false);
						}
					} else if (linePos[col] == (unsigned int)string::npos) {
						// End of string.
					} else {
						// Find the next newline.
						const size_t nl_pos = jt->find('\n', linePos[col]);
						if (nl_pos == string::npos) {
							// No more newlines.
							str = SafeString(jt->c_str() + linePos[col], false);
							linePos[col] = (unsigned int)string::npos;
						} else {
							// Found a newline.
							str = SafeString(jt->c_str() + linePos[col], nl_pos - linePos[col], false);
							linePos[col] = (unsigned int)(nl_pos + 1);
							if (linePos[col] > (unsigned int)jt->size()) {
								// End of string.
								linePos[col] = (unsigned int)string::npos;
							}
						}
					}

					// Align the data.
					size_t str_sz = utf8_disp_strlen(str);
					switch (align & 3) {
						default:
						case TXA_D:
						case TXA_L:
							// Left alignment (default)
							os << str;
							for (size_t x = str_sz; x < colSize[col]; x++) {
								os << ' ';
							}
							break;
						case TXA_C: {
							// Center alignment
							// For odd sizes, the extra space
							// will be on the right.
							const size_t spc = colSize[col] - str_sz;
							for (size_t x = spc/2; x > 0; x--) {
								os << ' ';
							}
							os << str;
							for (size_t x = (spc/2) + (spc%2); x > 0; x--) {
								os << ' ';
							}
							break;
						}
						case TXA_R:
							// Right alignment
							for (size_t x = str_sz; x < colSize[col]; x++) {
								os << ' ';
							}
							os << str;
							break;
					}
					os << '|';
				}
			}
		}
		return os;
	}
};

class DateTimeField {
	size_t width;
	const RomFields::Field &romField;
public:
	DateTimeField(size_t width, const RomFields::Field &romField)
		: width(width), romField(romField) { }
	friend ostream& operator<<(ostream& os, const DateTimeField& field) {
		auto romField = field.romField;

		os << ColonPad(field.width, romField.name);
		StreamStateSaver state(os);

		if (unlikely(romField.data.date_time == -1)) {
			// Invalid date/time.
			os << C_("RomData", "Unknown");
			return os;
		}

		char str[128];
		str[0] = '\0';
		int ret = formatDateTime(str, sizeof(str),
			romField.data.date_time,
			static_cast<RomFields::DateTimeFlags>(romField.flags));
		if (likely(ret == 0)) {
			os << str;
		} else {
			os << C_("RomData", "Unknown");
		}
		return os;
	}
};

class AgeRatingsField {
	size_t width;
	const RomFields::Field &romField;
public:
	AgeRatingsField(size_t width, const RomFields::Field &romField)
		: width(width), romField(romField) { }
	friend ostream& operator<<(ostream& os, const AgeRatingsField& field) {
		auto romField = field.romField;

		os << ColonPad(field.width, romField.name);
		StreamStateSaver state(os);

		// Convert the age ratings field to a string.
		const RomFields::age_ratings_t *age_ratings = romField.data.age_ratings;
		os << RomFields::ageRatingsDecode(age_ratings, false);
		return os;
	}
};

class DimensionsField {
	size_t width;
	const RomFields::Field &romField;
public:
	DimensionsField(size_t width, const RomFields::Field &romField)
		: width(width), romField(romField) { }
	friend ostream& operator<<(ostream& os, const DimensionsField& field) {
		auto romField = field.romField;

		os << ColonPad(field.width, romField.name);
		StreamStateSaver state(os);

		// Convert the dimensions field to a string.
		const int *const dimensions = romField.data.dimensions;
		os << dimensions[0];
		if (dimensions[1] > 0) {
			os << 'x' << dimensions[1];
			if (dimensions[2] > 0) {
				os << 'x' << dimensions[2];
			}
		}
		return os;
	}
};

class StringMultiField {
	size_t width;
	const RomFields::Field &romField;
	uint32_t def_lc;	// ROM-default language code.
	uint32_t user_lc;	// User-specified language code.
public:
	StringMultiField(size_t width, const RomFields::Field &romField, uint32_t def_lc, uint32_t user_lc)
		: width(width), romField(romField), def_lc(def_lc), user_lc(user_lc)
	{
		assert(this->def_lc != 0);
	}
	friend ostream& operator<<(ostream& os, const StringMultiField& field) {
		// NOTE: nullptr string is an empty string, not an error.
		auto romField = field.romField;
		os << ColonPad(field.width, romField.name);

		const auto *const pStr_multi = romField.data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (pStr_multi && !pStr_multi->empty()) {
			// Get the string and update the text.
			const string *const pStr = RomFields::getFromStringMulti(pStr_multi, field.def_lc, field.user_lc);
			assert(pStr != nullptr);
			os << SafeString((pStr ? *pStr : ""), true, field.width);
		} else {
			// Empty string.
			os << "''";
		}
		return os;
	}
};

class FieldsOutput {
	const RomFields& fields;
	uint32_t lc;
	unsigned int flags;
public:
	explicit FieldsOutput(const RomFields& fields, uint32_t lc = 0, unsigned int flags = 0)
		: fields(fields), lc(lc), flags(flags) { }
	friend std::ostream& operator<<(std::ostream& os, const FieldsOutput& fo) {
		// FIXME: Use std::max_element() [but that requires more strlen() calls...]
		size_t maxWidth = 0;
		std::for_each(fo.fields.cbegin(), fo.fields.cend(),
			[&maxWidth](const RomFields::Field &field) {
				maxWidth = max(maxWidth, strlen(field.name));
			}
		);
		maxWidth += 2;

		const int tabCount = fo.fields.tabCount();
		int tabIdx = -1;

		// Language codes.
		const uint32_t def_lc = fo.fields.defaultLanguageCode();
		const uint32_t user_lc = (fo.lc != 0 ? fo.lc : def_lc);

		bool printed_first = false;
		for (const RomFields::Field &romField : fo.fields) {
			assert(romField.isValid());
			if (!romField.isValid()) {
				continue;
			}

			if (printed_first) {
				os << '\n';
			}

			// New tab?
			if (tabCount > 1 && tabIdx != romField.tabIdx) {
				// Tab indexes must be consecutive.
				assert(tabIdx + 1 == romField.tabIdx);
				tabIdx = romField.tabIdx;

				// TODO: Better formatting?
				const char *name = fo.fields.tabName(tabIdx);
				assert(name != nullptr);
				os << "----- ";
				if (name) {
					os << name;
				} else {
					os << fmt::format(C_("TextOut", "(tab {:d})"), tabIdx);
				}
				os << " -----" << '\n';
			}

			switch (romField.type) {
				case RomFields::RFT_INVALID:
					// Should not happen due to the above check...
					assert(!"Field type is RFT_INVALID");
					break;
				case RomFields::RFT_STRING:
					os << StringField(maxWidth, romField);
					break;
				case RomFields::RFT_BITFIELD:
					os << BitfieldField(maxWidth, romField);
					break;
				case RomFields::RFT_LISTDATA:
					os << ListDataField(maxWidth, romField, def_lc, user_lc, fo.flags);
					break;
				case RomFields::RFT_DATETIME:
					os << DateTimeField(maxWidth, romField);
					break;
				case RomFields::RFT_AGE_RATINGS:
					os << AgeRatingsField(maxWidth, romField);
					break;
				case RomFields::RFT_DIMENSIONS:
					os << DimensionsField(maxWidth, romField);
					break;
				case RomFields::RFT_STRING_MULTI:
					os << StringMultiField(maxWidth, romField, def_lc, user_lc);
					break;
				default:
					assert(!"Unknown RomFieldType");
					os << ColonPad(maxWidth, romField.name) << "NYI";
					break;
			}

			printed_first = true;
		}
		return os;
	}
};


ROMOutput::ROMOutput(const RomData *romdata, uint32_t lc, unsigned int flags)
	: romdata(romdata)
	, lc(lc)
	, flags(flags) { }
RP_LIBROMDATA_PUBLIC
std::ostream& operator<<(std::ostream& os, const ROMOutput& fo) {
	const auto *const romdata = fo.romdata;
	const char *const systemName = romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *const fileType = romdata->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);

	// NOTE: RomDataView context is used for the "unknown" strings.
	{
		// tr: "[System] [FileType] detected."
		const string detectMsg = fmt::format(C_("TextOut", "{0:s} {1:s} detected"),
			(systemName ? systemName : C_("RomDataView", "(unknown system)")),
			(fileType ? fileType : C_("RomDataView", "(unknown filetype)")));

		os << "-- " << detectMsg << '\n';
	}

	// Fields
	const RomFields *const fields = romdata->fields();
	assert(fields != nullptr);
	if (fields) {
		os << FieldsOutput(*fields, fo.lc, fo.flags) << '\n';
	}

	const uint32_t imgbf = romdata->supportedImageTypes();
	if (imgbf != 0) {
		// Internal images
		if (!(fo.flags & OF_SkipInternalImages)) {
			for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
				if (!(imgbf & (1U << i))) {
					continue;
				}

				auto image = romdata->image(static_cast<RomData::ImageType>(i));
				if (image && image->isValid()) {
					// tr: Image Type name, followed by Image Type ID
					os << "-- " << fmt::format(C_("TextOut", "{0:s} is present (use -x{1:d} to extract)"),
						RomData::getImageTypeName(static_cast<RomData::ImageType>(i)), i) << '\n';
					// TODO: After localizing, add enough spaces for alignment.
					os << "   Format : " << rp_image::getFormatName(image->format()) << '\n';
					os << "   Size   : " << image->width() << " x " << image->height() << '\n';
					if (romdata->imgpf(static_cast<RomData::ImageType>(i))  & RomData::IMGPF_ICON_ANIMATED) {
						os << "   " << C_("TextOut", "Animated icon is present (use -a to extract)") << '\n';
					}
				}
			}
		}

		// External image URLs
		// NOTE: IMGPF_ICON_ANIMATED won't ever appear in external images.
		std::vector<RomData::ExtURL> extURLs;
		for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
			if (!(imgbf & (1U << i))) {
				continue;
			}

			// NOTE: extURLs may be empty even though the class supports it.
			// Check extURLs before doing anything else.

			extURLs.clear();	// NOTE: May not be needed...
			// TODO: Customize the image size parameter?
			// TODO: Option to retrieve supported image size?
			int ret = romdata->extURLs(static_cast<RomData::ImageType>(i), &extURLs, RomData::IMAGE_SIZE_DEFAULT);
			if (ret != 0 || extURLs.empty()) {
				continue;
			}

			for (const RomData::ExtURL &extURL : extURLs) {
				os << "-- " <<
					RomData::getImageTypeName(static_cast<RomData::ImageType>(i)) << ": " << urlPartialUnescape(extURL.url) <<
					" (cache_key: " << extURL.cache_key << ')' << '\n';
			}
		}
	}

	os.flush();
	return os;
}

} // namespace LibRpBase

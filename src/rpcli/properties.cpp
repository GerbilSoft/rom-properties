/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * properties.cpp: Properties output.                                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "properties.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
using std::endl;
using std::left;
using std::max;
using std::ostream;
using std::setw;
using std::string;
using std::unique_ptr;
using std::vector;

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/img/IconAnimData.hpp"
using namespace LibRpBase;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

class StreamStateSaver {
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
		os << setw(pad.width) << "";
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
		os << cp.str << left << setw(max(0, (signed)(cp.width - strlen(cp.str)))) << ':';
		return os;
	}
};
class SafeString {
	const char* str;
	bool quotes;
	size_t width;
public:
	SafeString(const char* str, bool quotes = true, size_t width=0) :str(str), quotes(quotes), width(width) {}
	SafeString(const string* str, bool quotes = true, size_t width=0) :quotes(quotes), width(width) {
		this->str = (str ? str->c_str() : nullptr);
	}

private:
	static string process(const SafeString& cp)
	{
		// NOTE: We have to use a temporary string here because
		// the caller might be using setw() for field padding.
		// TODO: Try optimizing it out while preserving setw().

		assert(cp.str != nullptr);
		if (!cp.str) {
			// nullptr string...
			return "''";
		}

		string escaped;
		escaped.reserve(strlen(cp.str) + (cp.quotes ? 2 : 0));
		if (cp.quotes) {
			escaped += '\'';
		}

		for (const char *p = cp.str; *p != 0; p++) {
			if (cp.width && *p == '\n') {
				escaped += '\n';
				escaped.append(cp.width + (cp.quotes ? 1 : 0), ' ');
			} else if ((unsigned char)*p < 0x20) {
				// Encode control characters using U+2400 through U+241F.
				escaped += "\xE2\x90";
				escaped += (char)(0x80 + (unsigned char)*p);
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

	operator string() {
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
		os << ColonPad(field.width, romField.name.c_str());
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
		unsigned int count = static_cast<unsigned int>(bitfieldDesc.names->size());
		assert(count <= 32);
		if (count > 32)
			count = 32;

		// Determine the column widths.
		unsigned int col = 0;
		const auto iter_end = bitfieldDesc.names->cend();
		for (auto iter = bitfieldDesc.names->cbegin(); iter != iter_end; ++iter) {
			const string &name = *iter;
			if (name.empty())
				continue;

			colSize[col] = max(name.size(), colSize[col]);
			col++;
			if (col == perRow) {
				col = 0;
			}
		}

		// Print the bits.
		os << ColonPad(field.width, romField.name.c_str());
		StreamStateSaver state(os);
		os << left;
		col = 0;
		uint32_t bitfield = romField.data.bitfield;
		for (auto iter = bitfieldDesc.names->cbegin(); iter != iter_end; ++iter, bitfield >>= 1) {
			const string &name = *iter;
			if (name.empty())
				continue;

			// Update the current column number before printing.
			// This prevents an empty row from being printed
			// if the number of valid elements is divisible by
			// the column count.
			if (col == perRow) {
				os << endl << Pad(field.width);
				col = 0;
			}

			os << " [" << ((bitfield & 1) ? '*' : ' ') << "] " <<
				setw(colSize[col]) << name;
			col++;
		}
		return os;
	}
};

class ListDataField {
	size_t width;
	const RomFields::Field &romField;
	uint32_t def_lc;	// ROM-default language code.
	uint32_t user_lc;	// User-specified language code.
public:
	ListDataField(size_t width, const RomFields::Field &romField, uint32_t def_lc, uint32_t user_lc)
		: width(width), romField(romField), def_lc(def_lc), user_lc(user_lc) { }
	friend ostream& operator<<(ostream& os, const ListDataField& field) {
		auto romField = field.romField;

		const auto &listDataDesc = romField.desc.list_data;
		// NOTE: listDataDesc.names can be nullptr,
		// which means we don't have any column headers.

		// Get the ListData_t container.
		const RomFields::ListData_t *pListData = nullptr;
		if (listDataDesc.flags & RomFields::RFT_LISTDATA_MULTI) {
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
			return os << "[ERROR: No list data.]";
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
			return os << "[ERROR: No list data.]";
		}

		/** Calculate the column widths. **/

		// Column names
		unique_ptr<size_t[]> colSize(new size_t[col_count]());
		size_t totalWidth = col_count + 1;
		if (listDataDesc.names) {
			int i = 0;
			for (auto it = listDataDesc.names->cbegin(); it != listDataDesc.names->cend(); ++it, ++i) {
				colSize[i] = it->size();
			}
		}

		// Row data
		unique_ptr<unsigned int[]> nl_count(new unsigned int[pListData->size()]());
		unsigned int row = 0;
		for (auto it = pListData->cbegin(); it != pListData->cend(); ++it, row++) {
			unsigned int col = 0;
			for (auto jt = it->cbegin(); jt != it->cend(); ++jt, col++) {
				// Check for newlines.
				unsigned int nl_row = 0;
				const size_t str_sz = jt->size();
				size_t prev_pos = 0;
				size_t cur_pos;
				do {
					size_t cur_sz;
					cur_pos = jt->find('\n', prev_pos);
					if (cur_pos == string::npos) {
						// End of string.
						cur_sz = str_sz - prev_pos;
					} else {
						// Found a newline.
						cur_sz = cur_pos - prev_pos;
						prev_pos = cur_pos + 1;
						nl_row++;
					}
					colSize[col] = max(cur_sz, colSize[col]);
				} while (cur_pos != string::npos && prev_pos < str_sz);

				// Update the newline count for this row.
				nl_count[row] = max(nl_count[row], nl_row);
			}
		}

		// Extra spacing for checkboxes
		// TODO: Use a separate column for the checkboxes?
		if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Prepend 4 spaces in column 0 for "[x] ".
			colSize[0] += 4;
		}

		/** Print the list data. **/

		os << ColonPad(field.width, romField.name.c_str());
		StreamStateSaver state(os);

		// Print the list on a separate row from the field name?
		const bool separateRow = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW);
		if (separateRow) {
			os << endl;
		}

		bool skipFirstNL = true;
		if (listDataDesc.names) {
			// Print the column names.
			unsigned int col = 0;
			uint32_t align = listDataDesc.alignment.headers;
			for (auto it = listDataDesc.names->cbegin(); it != listDataDesc.names->cend(); ++it, ++col, align >>= 2) {
				// FIXME: What was this used for?
				totalWidth += colSize[col]; // this could be in a separate loop, but whatever
				os << setw(0) << '|';
				switch (align & 3) {
					case TXA_L:
						// Left alignment
						os << setw(colSize[col]) << std::left;
						os << *it;
						break;
					default:
					case TXA_D:
					case TXA_C: {
						// Center alignment (default)
						// For odd sizes, the extra space
						// will be on the right.
						const size_t spc = colSize[col] - it->size();
						os << setw(spc/2);
						os << "";
						os << setw(0);
						os << *it;
						os << setw((spc/2) + (spc%2));
						os << "";
						break;
					}
					case TXA_R:
						// Right alignment
						os << setw(colSize[col]) << std::right;
						os << *it;
						break;
				}
			}
			os << setw(0) << std::right;
			os << '|' << endl;

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
		if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Remove the 4 spaces in column 0.
			// Those spaces will not be used in the text area.
			colSize[0] -= 4;
		}

		// Current line position.
		// NOTE: Special handling is needed for npos due to the use of unsigned int.
		unique_ptr<unsigned int[]> linePos(new unsigned int[col_count]);

		row = 0;
		for (auto it = pListData->cbegin(); it != pListData->cend(); ++it, row++) {
			// Print one line at a time for multi-line entries.
			// TODO: Better formatting for multi-line?
			// Right now we're assuming that at least one column is a single line.
			// If all columns are multi-line, then everything will look like it's
			// all single-line entries.
			memset(linePos.get(), 0, col_count * sizeof(linePos[0]));
			// NOTE: nl_count[row] is 0 for single-line items.
			for (int line = nl_count[row]; line >= 0; line--) {
				if (!skipFirstNL) {
					os << endl;
					if (!separateRow) {
						os << Pad(field.width);
					}
				} else {
					skipFirstNL = false;
				}
				os << '|';
				if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
					os << '[' << ((checkboxes & 1) ? 'x' : ' ') << "] ";
					checkboxes >>= 1;
				}
				unsigned int col = 0;
				uint32_t align = listDataDesc.alignment.data;
				for (auto jt = it->cbegin(); jt != it->cend(); ++jt, ++col, align >>= 2) {
					string str;
					if (nl_count[row] == 0) {
						// No newlines. Print the string directly.
						str = SafeString(jt->c_str(), false);
					} else if (linePos[col] == (unsigned int)string::npos) {
						// End of string.
					} else {
						// Find the next newline.
						size_t nl_pos = jt->find('\n', linePos[col]);
						if (nl_pos == string::npos) {
							// No more newlines.
							str = SafeString(jt->c_str() + linePos[col], false);
							linePos[col] = (unsigned int)string::npos;
						} else {
							// Found a newline.
							// TODO: Update SafeString to take a length parameter instead of creating a temporary string.
							str = SafeString(jt->substr(linePos[col], nl_pos - linePos[col]).c_str(), false);
							linePos[col] = (unsigned int)(nl_pos + 1);
							if (linePos[col] > (unsigned int)jt->size()) {
								// End of string.
								linePos[col] = (unsigned int)string::npos;
							}
						}
					}

					// Align the data.
					switch (align & 3) {
						default:
						case TXA_D:
						case TXA_L:
							// Left alignment (default)
							os << setw(colSize[col]) << std::left;
							os << str;
							break;
						case TXA_C: {
							// Center alignment
							// For odd sizes, the extra space
							// will be on the right.
							const size_t spc = colSize[col] - str.size();
							os << setw(spc/2);
							os << "";
							os << setw(0);
							os << str;
							os << setw((spc/2) + (spc%2));
							os << "";
							break;
						}
						case TXA_R:
							// Right alignment
							os << setw(colSize[col]) << std::right;
							os << str;
							break;
					}
					os << setw(0) << std::right << '|';
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
		auto flags = romField.desc.flags;

		os << ColonPad(field.width, romField.name.c_str());
		StreamStateSaver state(os);

		if (romField.data.date_time == -1) {
			// Invalid date/time.
			os << "Unknown";
			return os;
		}

		// FIXME: This may result in truncated times on 32-bit Linux.
		struct tm timestamp;
		struct tm *ret;
		time_t date_time = (time_t)romField.data.date_time;
		if (flags & RomFields::RFT_DATETIME_IS_UTC) {
			ret = gmtime_r(&date_time, &timestamp);
		}
		else {
			ret = localtime_r(&date_time, &timestamp);
		}

		if (!ret) {
			// gmtime_r() or localtime_r() failed.
			os << "Invalid DateTime";
			return os;
		}

		static const char *const formats[8] = {
			"Invalid DateTime",	// No date or time.
			"%x",			// Date
			"%X",			// Time
			"%x %X",		// Date Time

			// TODO: Better localization here.
			"Invalid DateTime",	// No date or time.
			"%b %d",		// Date (no year)
			"%X",			// Time
			"%b %d %X",		// Date Time (no year)
		};

		char str[128];
		strftime(str, 128, formats[flags & RomFields::RFT_DATETIME_HAS_DATETIME_NO_YEAR_MASK], &timestamp);
		os << str;
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

		os << ColonPad(field.width, romField.name.c_str());
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

		os << ColonPad(field.width, romField.name.c_str());
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
		os << ColonPad(field.width, romField.name.c_str());

		const auto *const pStr_multi = romField.data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (pStr_multi && !pStr_multi->empty()) {
			// Get the string and update the text.
			const string *const pStr = RomFields::getFromStringMulti(pStr_multi, field.def_lc, field.user_lc);
			assert(pStr != nullptr);
			os << SafeString((pStr ? pStr->c_str() : ""), true, field.width);
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
public:
	explicit FieldsOutput(const RomFields& fields, uint32_t lc = 0)
		: fields(fields), lc(lc) { }
	friend std::ostream& operator<<(std::ostream& os, const FieldsOutput& fo) {
		size_t maxWidth = 0;
		const auto iter_end = fo.fields.cend();
		for (auto iter = fo.fields.cbegin(); iter != iter_end; ++iter) {
			maxWidth = max(maxWidth, iter->name.size());
		}
		maxWidth += 2;

		const int tabCount = fo.fields.tabCount();
		int tabIdx = -1;

		// Language codes.
		const uint32_t def_lc = fo.fields.defaultLanguageCode();
		const uint32_t user_lc = (fo.lc != 0 ? fo.lc : def_lc);

		bool printed_first = false;
		for (auto iter = fo.fields.cbegin(); iter != iter_end; ++iter) {
			const auto &romField = *iter;
			if (!romField.isValid)
				continue;

			if (printed_first)
				os << endl;

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
					os << "(tab " << tabIdx << ')';
				}
				os << " -----" << endl;
			}

			switch (romField.type) {
			case RomFields::RFT_INVALID: {
				assert(!"INVALID field type");
				os << ColonPad(maxWidth, romField.name.c_str()) << "INVALID";
				break;
			}
			case RomFields::RFT_STRING: {
				os << StringField(maxWidth, romField);
				break;
			}
			case RomFields::RFT_BITFIELD: {
				os << BitfieldField(maxWidth, romField);
				break;
			}
			case RomFields::RFT_LISTDATA: {
				os << ListDataField(maxWidth, romField, def_lc, user_lc);
				break;
			}
			case RomFields::RFT_DATETIME: {
				os << DateTimeField(maxWidth, romField);
				break;
			}
			case RomFields::RFT_AGE_RATINGS: {
				os << AgeRatingsField(maxWidth, romField);
				break;
			}
			case RomFields::RFT_DIMENSIONS: {
				os << DimensionsField(maxWidth, romField);
				break;
			}
			case RomFields::RFT_STRING_MULTI: {
				os << StringMultiField(maxWidth, romField, def_lc, user_lc);
				break;
			}
			default: {
				assert(!"Unknown RomFieldType");
				os << ColonPad(maxWidth, romField.name.c_str()) << "NYI";
				break;
			}
			}

			printed_first = true;
		}
		return os;
	}
};

class JSONString {
	const char* str;
public:
	explicit JSONString(const char* str) :str(str) {}
	friend ostream& operator<<(ostream& os, const JSONString& js) {
		if (!js.str) {
			// NULL string.
			// Treat this like an empty string.
			return os << "\"\"";
		}

		// Certain characters need to be escaped.
		const char *str = js.str;
		os << '"';
		for (; *str != 0; str++) {
			const uint8_t chr = static_cast<uint8_t>(*str);
			if (chr < 0x20) { 
				// Control characters need to be escaped.
				static const char ctrl_escape_letters[0x20] = {
					  0,   0,   0,   0,   0,   0,   0,   0,	// 0x00-0x07
					'b', 't', 'n',   0, 'f', 'r',   0,   0,	// 0x08-0x0F
					  0,   0,   0,   0,   0,   0,   0,   0,	// 0x10-0x17
					  0,   0,   0,   0,   0,   0,   0,   0,	// 0x18-0x1F
				};
				const char letter = ctrl_escape_letters[chr];
				if (letter != 0) {
					// Escape character is available.
					os << '\\' << letter;
				} else {
					// No escape character. Use a Unicode escape.
					char buf[16];
					snprintf(buf, sizeof(buf), "\\u%04X", chr);
					os << buf;
				}
			} else {
				// Check for backslash and double-quotes.
				if (chr == '\\') {
					os << "\\\\";
				} else if (chr == '"') {
					os << "\\\"";
				} else {
					// Normal character.
					os << static_cast<char>(chr);
				}
			}
		}

		return os << '"';
	}
};

class JSONFieldsOutput {
	const RomFields& fields;
public:
	explicit JSONFieldsOutput(const RomFields& fields) :fields(fields) {}
	friend std::ostream& operator<<(std::ostream& os, const JSONFieldsOutput& fo) {
		os << "[\n";
		bool printed_first = false;
		const auto iter_end = fo.fields.cend();
		for (auto iter = fo.fields.cbegin(); iter != iter_end; ++iter) {
			const auto &romField = *iter;
			if (!romField.isValid)
				continue;

			if (printed_first)
				os << ',' << endl;

			switch (romField.type) {
			case RomFields::RFT_INVALID: {
				assert(!"INVALID field type");
				os << "{\"type\":\"INVALID\"}";
				break;
			}

			case RomFields::RFT_STRING: {
				os << "{\"type\":\"STRING\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << ",\"format\":" << romField.desc.flags
				   << "},\"data\":" << JSONString(romField.data.str->c_str()) << '}';
				break;
			}

			case RomFields::RFT_BITFIELD: {
				const auto &bitfieldDesc = romField.desc.bitfield;
				os << "{\"type\":\"BITFIELD\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << ",\"elementsPerRow\":" << bitfieldDesc.elemsPerRow
				   << ",\"names\":";
				assert(bitfieldDesc.names != nullptr);
				if (bitfieldDesc.names) {
					os << '[';
					unsigned int count = static_cast<unsigned int>(bitfieldDesc.names->size());
					assert(count <= 32);
					if (count > 32)
						count = 32;
					bool printedOne = false;
					const auto iter_end = bitfieldDesc.names->cend();
					for (auto iter = bitfieldDesc.names->cbegin(); iter != iter_end; ++iter) {
						const string &name = *iter;
						if (name.empty())
							continue;

						if (printedOne) os << ',';
						printedOne = true;
						os << JSONString(name.c_str());
					}
					os << ']';
				} else {
					os << "\"ERROR\"";
				}
				os << "},\"data\":" << romField.data.bitfield << '}';
				break;
			}

			case RomFields::RFT_LISTDATA: {
				const auto &listDataDesc = romField.desc.list_data;
				os << "{\"type\":\"LISTDATA\",\"desc\":{\"name\":" << JSONString(romField.name.c_str());

				if (listDataDesc.names) {
					os << ",\"names\":[";
					const unsigned int col_count = static_cast<unsigned int>(listDataDesc.names->size());
					if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
						// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
						os << "checked,";
					}
					for (unsigned int j = 0; j < col_count; j++) {
						if (j) os << ',';
						os << JSONString(listDataDesc.names->at(j).c_str());
					}
					os << ']';
				} else {
					os << ",\"names\":[]";
				}

				os << "},\"data\":";
				if (!(listDataDesc.flags & RomFields::RFT_LISTDATA_MULTI)) {
					// Single-language ListData.
					os << "[\n";
					const auto list_data = romField.data.list_data.data.single;
					assert(list_data != nullptr);
					if (list_data) {
						uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
						for (auto it = list_data->cbegin(); it != list_data->cend(); ++it) {
							if (it != list_data->cbegin()) os << ",\n";
							os << "\t[";
							if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
								// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
								os << ((checkboxes & 1) ? "true" : "false") << ',';
								checkboxes >>= 1;
							}

							bool did_one = false;
							for (auto jt = it->cbegin(); jt != it->cend(); ++jt) {
								if (did_one) os << ',';
								did_one = true;
								os << JSONString(jt->c_str());
							}
							os << ']';
						}
						if (!list_data->empty()) {
							os << '\n';
						}
					}
					os << "]";
				} else {
					// Multi-language ListData.
					os << "{\n";
					const auto *const list_data = romField.data.list_data.data.multi;
					assert(list_data != nullptr);
					if (list_data) {
						for (auto mapIter = list_data->cbegin(); mapIter != list_data->cend(); ++mapIter) {
							// Key: Language code
							// Value: Vector of string data
							if (mapIter != list_data->cbegin()) os << ",\n";
							os << "\t\"";
							for (uint32_t lc = mapIter->first; lc != 0; lc <<= 8) {
								char chr = (char)(lc >> 24);
								if (chr != 0) {
									os << chr;
								}
							}
							os << "\":[";
							// TODO: Consolidate single/multi here?
							const auto &lc_data = mapIter->second;
							if (!lc_data.empty()) {
								os << '\n';
								uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
								for (auto lcIter = lc_data.cbegin(); lcIter != lc_data.cend(); ++lcIter) {
									if (lcIter != lc_data.cbegin()) os << ",\n";
									os << "\t\t[";
									if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
										// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
										os << ((checkboxes & 1) ? "true" : "false") << ',';
										checkboxes >>= 1;
									}

									bool did_one = false;
									for (auto jt = lcIter->cbegin(); jt != lcIter->cend(); ++jt) {
										if (did_one) os << ',';
										did_one = true;
										os << JSONString(jt->c_str());
									}
									os << ']';
								}
								if (!lc_data.empty()) {
									os << '\n';
								}
							}
							os << "\t]";
						}
						if (!list_data->empty()) {
							os << '\n';
						}
					}
					os << '}';
				}
				os << '}';
				break;
			}

			case RomFields::RFT_DATETIME: {
				os << "{\"type\":\"DATETIME\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << ",\"flags\":" << romField.desc.flags
				   << "},\"data\":" << romField.data.date_time
				   << '}';
				break;
			}

			case RomFields::RFT_AGE_RATINGS: {
				os << "{\"type\":\"AGE_RATINGS\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << "},\"data\":";

				const RomFields::age_ratings_t *age_ratings = romField.data.age_ratings;
				assert(age_ratings != nullptr);
				if (!age_ratings) {
					os << "\"ERROR\"}";
					break;
				}

				os << '[';
				bool printedOne = false;
				const unsigned int age_ratings_max = static_cast<unsigned int>(age_ratings->size());
				for (unsigned int j = 0; j < age_ratings_max; j++) {
					const uint16_t rating = age_ratings->at(j);
					if (!(rating & RomFields::AGEBF_ACTIVE))
						continue;

					if (printedOne) {
						// Append a comma.
						os << ',';
					}
					printedOne = true;

					os << "{\"name\":";
					const char *const abbrev = RomFields::ageRatingAbbrev(j);
					if (abbrev) {
						os << '"' << abbrev << '"';
					} else {
						// Invalid age rating.
						// Use the numeric index.
						os << j;
					}
					os << ",\"rating\":\""
					   << RomFields::ageRatingDecode(j, rating)
					   << "\"}";
				}
				os << "]}";
				break;
			}

			case RomFields::RFT_DIMENSIONS: {
				os << "{\"type\":\"DIMENSIONS\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << "},\"data\":";

				const int *const dimensions = romField.data.dimensions;
				os << "{\"w\":" << dimensions[0];
				if (dimensions[1] > 0) {
					os << ",\"h\":" << dimensions[1];
					if (dimensions[2] > 0) {
						os << ",\"d\":" << dimensions[2];
					}
				}
				os << "}}";
				break;
			}

			case RomFields::RFT_STRING_MULTI: {
				// TODO: Act like RFT_STRING if there's only one language?
				os << "{\"type\":\"STRING_MULTI\",\"desc\":{\"name\":" << JSONString(romField.name.c_str())
				   << ",\"format\":" << romField.desc.flags
				   << "},\"data\":{\n";
				const auto *const pStr_multi = romField.data.str_multi;
				bool didFirst = false;
				for (auto iter = pStr_multi->cbegin(); iter != pStr_multi->cend(); ++iter) {
					// Convert the language code to ASCII.
					if (didFirst) {
						os << ",\n";
					}
					didFirst = true;
					os << "\t\"";
					for (uint32_t lc = iter->first; lc != 0; lc <<= 8) {
						char chr = (char)(lc >> 24);
						if (chr != 0) {
							os << chr;
						}
					}
					os << "\":" << JSONString(iter->second.c_str());
				}
				os << "\n}}";
				break;
			}

			default: {
				assert(!"Unknown RomFieldType");
				os << "{\"type\":\"NYI\",\"desc\":{\"name\":" << JSONString(romField.name.c_str()) << "}}";
				break;
			}
			}

			printed_first = true;
		}
		os << ']';
		return os;
	}
};



ROMOutput::ROMOutput(const RomData *romdata, uint32_t lc)
	: romdata(romdata)
	, lc(lc) { }
std::ostream& operator<<(std::ostream& os, const ROMOutput& fo) {
	auto romdata = fo.romdata;
	const char *const systemName = romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *const fileType = romdata->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);

	os << "-- " << (systemName ? systemName : "(unknown system)") <<
	      ' ' << (fileType ? fileType : "(unknown filetype)") <<
	      " detected" << endl;
	const RomFields *const fields = romdata->fields();
	assert(fields != nullptr);
	if (fields) {
		os << FieldsOutput(*fields, fo.lc) << endl;
	}

	const int supported = romdata->supportedImageTypes();

	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (!(supported & (1U << i)))
			continue;

		auto image = romdata->image((RomData::ImageType)i);
		if (image && image->isValid()) {
			os << "-- " << RomData::getImageTypeName((RomData::ImageType)i) << " is present (use -x" << i << " to extract)" << endl;
			os << "   Format : " << rp_image::getFormatName(image->format()) << endl;
			os << "   Size   : " << image->width() << " x " << image->height() << endl;
			if (romdata->imgpf((RomData::ImageType) i)  & RomData::IMGPF_ICON_ANIMATED) {
				os << "   Animated icon present (use -a to extract)" << endl;
			}
		}
	}

	std::vector<RomData::ExtURL> extURLs;
	for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
		if (!(supported & (1U << i)))
			continue;

		// NOTE: extURLs may be empty even though the class supports it.
		// Check extURLs before doing anything else.

		extURLs.clear();	// NOTE: May not be needed...
		// TODO: Customize the image size parameter?
		// TODO: Option to retrieve supported image size?
		int ret = romdata->extURLs((RomData::ImageType)i, &extURLs, RomData::IMAGE_SIZE_DEFAULT);
		if (ret != 0 || extURLs.empty())
			continue;

		std::for_each(extURLs.cbegin(), extURLs.cend(),
			[i, &os](const RomData::ExtURL &extURL) {
				os << "-- " <<
					RomData::getImageTypeName((RomData::ImageType)i) << ": " << extURL.url <<
					" (cache_key: " << extURL.cache_key << ')' << endl;
			}
		);
	}
	return os;
}

JSONROMOutput::JSONROMOutput(const RomData *romdata, uint32_t lc)
	: romdata(romdata)
	, lc(lc) { }
std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo) {
	auto romdata = fo.romdata;
	assert(romdata && romdata->isValid());

	const char *const systemName = romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *const fileType = romdata->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);

	os << "{\"system\":";
	if (systemName) {
		os << JSONString(systemName);
	} else {
		os << "\"unknown\"";
	}
	os << ",\"filetype\":";
	if (fileType) {
		os << JSONString(fileType);
	} else {
		os << "\"unknown\"";
	}
	const RomFields *const fields = romdata->fields();
	assert(fields != nullptr);
	if (fields) {
		os << ",\"fields\":" << JSONFieldsOutput(*fields);
	}

	const int supported = romdata->supportedImageTypes();

	// TODO: Tabs.
	bool first = true;
	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (!(supported & (1U << i)))
			continue;

		if (first) {
			os << ",\n\"imgint\":[";
			first = false;
		} else {
			os << ',';
		}

		os << "{\"type\":" << JSONString(RomData::getImageTypeName((RomData::ImageType)i));
		auto image = romdata->image((RomData::ImageType)i);
		if (image && image->isValid()) {
			os << ",\"format\":" << JSONString(rp_image::getFormatName(image->format()));
			os << ",\"size\":[" << image->width() << ',' << image->height() << ']';
			int ppf = romdata->imgpf((RomData::ImageType) i);
			if (ppf) {
				os << ",\"postprocessing\":" << ppf;
			}
			if (ppf & RomData::IMGPF_ICON_ANIMATED) {
				auto animdata = romdata->iconAnimData();
				if (animdata) {
					os << ",\"frames\":" << animdata->count;
					os << ",\"sequence\":[";
					for (int j = 0; j < animdata->seq_count; j++) {
						if (j) os << ',';
						os << (unsigned)animdata->seq_index[j];
					}
					os << "],\"delay\":[";
					for (int j = 0; j < animdata->seq_count; j++) {
						if (j) os << ',';
						os << animdata->delays[j].ms;
					}
					os << ']';
				}
			}
		}
		os << '}';
	}
	if (!first) {
		os << ']';
	}

	first = true;
	vector<RomData::ExtURL> extURLs;
	for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
		if (!(supported & (1U << i)))
			continue;

		// NOTE: extURLs may be empty even though the class supports it.
		// Check extURLs before doing anything else.

		extURLs.clear();	// NOTE: May not be needed...
		// TODO: Customize the image size parameter?
		// TODO: Option to retrieve supported image size?
		int ret = romdata->extURLs((RomData::ImageType)i, &extURLs, RomData::IMAGE_SIZE_DEFAULT);
		if (ret != 0 || extURLs.empty())
			continue;

		if (first) {
			os << ",\n\"imgext\":[";
			first = false;
		} else {
			os << ',';
		}

		os << "{\"type\":" << JSONString(RomData::getImageTypeName((RomData::ImageType)i));
		int ppf = romdata->imgpf((RomData::ImageType) i);
		if (ppf) {
			os << ",\"postprocessing\":" << ppf;
		}
		// NOTE: IMGPF_ICON_ANIMATED won't ever appear in external image
		os << ",\"exturls\":[";
		bool did_one = false;
		for (auto iter = extURLs.cbegin(); iter != extURLs.cend(); ++iter) {
			if (did_one) os << ',';
			did_one = true;

			os << "{\"url\":" << JSONString(iter->url.c_str());
			os << ",\"cache_key\":" << JSONString(iter->cache_key.c_str()) << '}';
		}
		os << "]}";
	}
	if (!first) {
		os << ']';
	}

	return os << '}';
}

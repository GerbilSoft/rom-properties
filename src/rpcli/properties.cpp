/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * properties.cpp: Properties output.                                      *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
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

#include "stdafx.h"
#include "librpbase/config.librpbase.h"
#include "config.rpcli.h"
#include "properties.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
using std::endl;
using std::left;
using std::max;
using std::ostream;
using std::setw;
using std::string;
using std::unique_ptr;

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/IconAnimData.hpp"
using namespace LibRpBase;

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
	friend ostream& operator<<(ostream& os, const SafeString& cp) {
		if (!cp.str) {
			//assert(0); // RomData should never return a null string // disregard that
			return os << "(null)";
		}
		
		string escaped;
		escaped.reserve(strlen(cp.str));
		for (const char* str = cp.str; *str != 0; str++) {
			if (cp.width && *str == '\n') {
				escaped += '\n';
				escaped.append(cp.width + (cp.quotes?1:0), ' ');
			} else if ((unsigned char)*str < 0x20) {
				// Encode control characters using U+2400 through U+241F.
				escaped += "\xE2\x90";
				escaped += (char)(0x80 + (unsigned char)*str);
			} else {
				escaped += *str;
			}
		}
		if (cp.quotes) {
			return os << '\'' << escaped << '\'';
		}
		else {
			return os << escaped;
		}
	}
};
class StringField {
	size_t width;
	const RomFields::Field *romField;
public:
	StringField(size_t width, const RomFields::Field *romField) :width(width), romField(romField) {}
	friend ostream& operator<<(ostream& os, const StringField& field) {
		auto romField = field.romField;
		return os << ColonPad(field.width, romField->name.c_str())
			  << SafeString(romField->data.str, true, field.width);
	}
};

class BitfieldField {
	size_t width;
	const RomFields::Field *romField;
public:
	BitfieldField(size_t width, const RomFields::Field *romField) :width(width), romField(romField) {}
	friend ostream& operator<<(ostream& os, const BitfieldField& field) {
		auto romField = field.romField;
		const auto &bitfieldDesc = romField->desc.bitfield;
		assert(romField->desc.bitfield.names != nullptr);
		if (!romField->desc.bitfield.names) {
			return os << "[ERROR: No bitfield names.]";
		}
		const int perRow = (bitfieldDesc.elemsPerRow ? bitfieldDesc.elemsPerRow : 4);

		// TODO: Remove bitfieldDesc.elements?
		unique_ptr<size_t[]> colSize(new size_t[perRow]());
		int count = bitfieldDesc.elements;
		assert(count <= (int)bitfieldDesc.names->size());
		if (count > (int)bitfieldDesc.names->size()) {
			count = (int)bitfieldDesc.names->size();
		}

		// Determine the column widths.
		int col = 0;
		for (int bit = 0; bit < count; bit++) {
			const string &name = bitfieldDesc.names->at(bit);
			if (name.empty())
				continue;

			colSize[col] = max(name.size(), colSize[col]);
			col++;
			if (col == perRow) {
				col = 0;
			}
		}

		// Print the bits.
		os << ColonPad(field.width, romField->name.c_str());
		StreamStateSaver state(os);
		os << left;
		col = 0;
		for (int bit = 0; bit < count; bit++) {
			const string &name = bitfieldDesc.names->at(bit);
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

			os << " [" << ((romField->data.bitfield & (1 << bit)) ? '*' : ' ') << "] " <<
				setw(colSize[col]) << name;
			col++;
		}
		return os;
	}
};

class ListDataField {
	size_t width;
	const RomFields::Field *romField;
public:
	ListDataField(size_t width, const RomFields::Field *romField) :width(width), romField(romField) {}
	friend ostream& operator<<(ostream& os, const ListDataField& field) {
		auto romField = field.romField;

		const auto &listDataDesc = romField->desc.list_data;
		// NOTE: listDataDesc.names can be nullptr,
		// which means we don't have any column headers.

		const auto list_data = romField->data.list_data;
		assert(list_data != nullptr);
		if (!list_data) {
			return os << "[ERROR: No list data.]";
		}

		int col_count = 1;
		if (listDataDesc.names) {
			col_count = (int)listDataDesc.names->size();
		} else {
			// No column headers.
			// Use the first row.
			if (list_data && !list_data->empty()) {
				col_count = (int)list_data->at(0).size();
			}
		}
		assert(col_count > 0);
		if (col_count < 0) {
			return os << "[ERROR: No list data.]";
		}

		unique_ptr<size_t[]> colSize(new size_t[col_count]());
		size_t totalWidth = col_count + 1;
		if (listDataDesc.names) {
			for (int i = (int)listDataDesc.names->size()-1; i >= 0; i--) {
				colSize[i] = listDataDesc.names->at(i).size();
			}
		}

		for (auto it = list_data->cbegin(); it != list_data->cend(); ++it) {
			int i = 0;
			for (auto jt = it->cbegin(); jt != it->cend(); ++jt) {
				colSize[i] = max(jt->length(), colSize[i]);
				i++;
			}
		}
		// TODO: Use a separate column for the checkboxes?
		if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Prepend 4 spaces in column 0 for "[x] ".
			colSize[0] += 4;
		}

		os << ColonPad(field.width, romField->name.c_str());
		StreamStateSaver state(os);

		bool skipFirstNL = true;
		if (listDataDesc.names) {
			// Print the column names.
			for (int i = 0; i < (int)listDataDesc.names->size(); i++) {
				totalWidth += colSize[i]; // this could be in a separate loop, but whatever
				os << '|' << setw(colSize[i]) << listDataDesc.names->at(i);
			}
			os << '|' << endl << Pad(field.width) << string(totalWidth, '-');
			// Don't skip the first newline, since we're
			// printing headers.
			skipFirstNL = false;
		}

		uint32_t checkboxes = romField->data.list_checkboxes;
		if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
			// Remove the 4 spaces in column 0.
			// Those spaces will not be used in the text area.
			colSize[0] -= 4;
		}
		for (auto it = list_data->cbegin(); it != list_data->cend(); ++it) {
			int i = 0;
			if (!skipFirstNL) {
				os << endl << Pad(field.width);
			} else {
				skipFirstNL = false;
			}
			os << '|';
			if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
				os << '[' << ((checkboxes & 1) ? 'x' : ' ') << "] ";
				checkboxes >>= 1;
			}
			for (auto jt = it->cbegin(); jt != it->cend(); ++jt) {
				os << setw(colSize[i++]) << SafeString(jt->c_str(), false) << '|';
			}
		}
		return os;
	}
};

class DateTimeField {
	size_t width;
	const RomFields::Field *romField;
public:
	DateTimeField(size_t width, const RomFields::Field *romField) :width(width), romField(romField) {}
	friend ostream& operator<<(ostream& os, const DateTimeField& field) {
		auto romField = field.romField;
		auto flags = romField->desc.flags;

		os << ColonPad(field.width, romField->name.c_str());
		StreamStateSaver state(os);

		if (romField->data.date_time == -1) {
			// Invalid date/time.
			os << "Unknown";
			return os;
		}

		// FIXME: This may result in truncated times on 32-bit Linux.
		struct tm timestamp;
		struct tm *ret;
		time_t date_time = (time_t)romField->data.date_time;
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
	const RomFields::Field *romField;
public:
	AgeRatingsField(size_t width, const RomFields::Field *romField) :width(width), romField(romField) {}
	friend ostream& operator<<(ostream& os, const AgeRatingsField& field) {
		auto romField = field.romField;

		os << ColonPad(field.width, romField->name.c_str());
		StreamStateSaver state(os);

		// Convert the age ratings field to a string.
		const RomFields::age_ratings_t *age_ratings = romField->data.age_ratings;
		os << RomFields::ageRatingsDecode(age_ratings, false);
		return os;
	}
};

class FieldsOutput {
	const RomFields& fields;
public:
	explicit FieldsOutput(const RomFields& fields) :fields(fields) {}
	friend std::ostream& operator<<(std::ostream& os, const FieldsOutput& fo) {
		size_t maxWidth = 0;
		for (int i = 0; i < fo.fields.count(); i++) {
			const RomFields::Field *const field = fo.fields.field(i);
			if (likely(field != nullptr)) {
				maxWidth = max(maxWidth, field->name.size());
			}
		}
		maxWidth += 2;
		bool printed_first = false;
		for (int i = 0; i < fo.fields.count(); i++) {
			auto romField = fo.fields.field(i);
			assert(romField != nullptr);
			if (!romField || !romField->isValid)
				continue;

			if (printed_first)
				os << endl;
			switch (romField->type) {
			case RomFields::RFT_INVALID: {
				assert(!"INVALID field type");
				os << ColonPad(maxWidth, romField->name.c_str()) << "INVALID";
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
				os << ListDataField(maxWidth, romField);
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
			default: {
				assert(!"Unknown RomFieldType");
				os << ColonPad(maxWidth, romField->name.c_str()) << "NYI";
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
		//assert(js.str); // not all strings can't be null, apparently
		if (!js.str) {
			// NULL string.
			// Print "0" to indicate this.
			return os << '0';
		}

		// Certain characters need to be escaped.
		const char *str = js.str;
		string escaped;
		escaped.reserve(strlen(str));
		for (; *str != 0; str++) {
			switch (*str) {
				case '\\':
					escaped += "\\\\";
					break;
				case '"':
					escaped += "\\";
					break;
				case '\b':
					escaped += "\\b";
					break;
				case '\f':
					escaped += "\\f";
					break;
				case '\t':
					escaped += "\\t";
					break;
				case '\n':
					escaped += "\\n";
					break;
				case '\r':
					escaped += "\\r";
					break;
				default:
					escaped += *str;
					break;
			}
		}

		return os << '"' << escaped << '"';
	}
};

class JSONFieldsOutput {
	const RomFields& fields;
public:
	explicit JSONFieldsOutput(const RomFields& fields) :fields(fields) {}
	friend std::ostream& operator<<(std::ostream& os, const JSONFieldsOutput& fo) {
		os << "[\n";
		bool printed_first = false;
		for (int i = 0; i < fo.fields.count(); i++) {
			auto romField = fo.fields.field(i);
			assert(romField != nullptr);
			if (!romField || !romField->isValid)
				continue;

			if (printed_first)
				os << ',' << endl;

			switch (romField->type) {
			case RomFields::RFT_INVALID: {
				assert(0); // INVALID field type
				os << "{\"type\":\"INVALID\"}";
				break;
			}

			case RomFields::RFT_STRING: {
				os << "{\"type\":\"STRING\",\"desc\":{\"name\":" << JSONString(romField->name.c_str())
				   << ",\"format\":" << romField->desc.flags
				   << "},\"data\":" << JSONString(romField->data.str->c_str()) << '}';
				break;
			}

			case RomFields::RFT_BITFIELD: {
				const auto &bitfieldDesc = romField->desc.bitfield;
				os << "{\"type\":\"BITFIELD\",\"desc\":{\"name\":" << JSONString(romField->name.c_str())
				   << ",\"elements\":" << bitfieldDesc.elements
				   << ",\"elementsPerRow\":" << bitfieldDesc.elemsPerRow
				   << ",\"names\":";
				assert(bitfieldDesc.names != nullptr);
				if (bitfieldDesc.names) {
					os << '[';
					// TODO: Remove bitfieldDesc.elements?
					int count = bitfieldDesc.elements;
					assert(count <= (int)bitfieldDesc.names->size());
					if (count > (int)bitfieldDesc.names->size()) {
						count = (int)bitfieldDesc.names->size();
					}
					bool printedOne = false;
					for (int bit = 0; bit < count; bit++) {
						const string &name = bitfieldDesc.names->at(bit);
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
				os << "},\"data\":" << romField->data.bitfield << '}';
				break;
			}

			case RomFields::RFT_LISTDATA: {
				const auto &listDataDesc = romField->desc.list_data;
				os << "{\"type\":\"LISTDATA\",\"desc\":{\"name\":" << JSONString(romField->name.c_str());
				if (listDataDesc.names) {
					os << ",\"names\":[";
					const int col_count = (int)listDataDesc.names->size();
					if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
						// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
						os << "checked,";
					}
					for (int j = 0; j < col_count; j++) {
						if (j) os << ',';
						os << JSONString(listDataDesc.names->at(j).c_str());
					}
					os << ']';
				} else {
					os << ",\"names\":[]";
				}
				os << "},\"data\":[";
				const auto list_data = romField->data.list_data;
				assert(list_data != nullptr);
				if (list_data) {
					uint32_t checkboxes = romField->data.list_checkboxes;
					for (auto it = list_data->cbegin(); it != list_data->cend(); ++it) {
						if (it != list_data->cbegin()) os << ',';
						os << '[';
						if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
							// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
							os << ((checkboxes & 1) ? "true" : "false") << ',';
							checkboxes >>= 1;
						}
						for (auto jt = it->cbegin(); jt != it->cend(); ++jt) {
							if (jt != it->cbegin()) os << ',';
							os << JSONString(jt->c_str());
						}
						os << ']';
					}
				}
				os << "]}";
				break;
			}

			case RomFields::RFT_DATETIME: {
				os << "{\"type\":\"DATETIME\",\"desc\":{\"name\":" << JSONString(romField->name.c_str())
				   << ",\"flags\":" << romField->desc.flags
				   << "},\"data\":" << romField->data.date_time
				   << '}';
				break;
			}

			case RomFields::RFT_AGE_RATINGS: {
				os << "{\"type\":\"AGE_RATINGS\",\"desc\":{\"name\":" << JSONString(romField->name.c_str())
				   << "},\"data\":";

				const RomFields::age_ratings_t *age_ratings = romField->data.age_ratings;
				assert(age_ratings != nullptr);
				if (!age_ratings) {
					os << "\"ERROR\"}";
					break;
				}

				os << '[';
				bool printedOne = false;
				for (int i = 0; i < (int)age_ratings->size(); i++) {
					const uint16_t rating = age_ratings->at(i);
					if (!(rating & RomFields::AGEBF_ACTIVE))
						continue;

					if (printedOne) {
						// Append a comma.
						os << ',';
					}
					printedOne = true;

					os << "{\"name\":";
					const char *abbrev = RomFields::ageRatingAbbrev(i);
					if (abbrev) {
						os << '"' << abbrev << '"';
					} else {
						// Invalid age rating.
						// Use the numeric index.
						os << i;
					}
					os << ",\"rating\":\""
					   << RomFields::ageRatingDecode(i, rating)
					   << "\"}";
				}
				os << "]}";
				break;
			}

			default: {
				assert(!"Unknown RomFieldType");
				os << "{\"type\":\"NYI\",\"desc\":{\"name\":" << JSONString(romField->name.c_str()) << "}}";
				break;
			}
			}

			printed_first = true;
		}
		os << ']';
		return os;
	}
};



ROMOutput::ROMOutput(const RomData *romdata) : romdata(romdata) { }
std::ostream& operator<<(std::ostream& os, const ROMOutput& fo) {
	auto romdata = fo.romdata;
	const char *sysName = romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_GENERIC);
	const char *fileType = romdata->fileType_string();

	os << "-- " << (sysName ? sysName : "(unknown system)") <<
	      ' ' << (fileType ? fileType : "(unknown filetype)") <<
	      " detected" << endl;
	os << FieldsOutput(*(romdata->fields())) << endl;

	int supported = romdata->supportedImageTypes();

	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (!(supported & (1 << i)))
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
		if (!(supported & (1 << i)))
			continue;

		// NOTE: extURLs may be empty even though the class supports it.
		// Check extURLs before doing anything else.

		extURLs.clear();	// NOTE: May not be needed...
		// TODO: Customize the image size parameter?
		// TODO: Option to retrieve supported image size?
		int ret = romdata->extURLs((RomData::ImageType)i, &extURLs, RomData::IMAGE_SIZE_DEFAULT);
		if (ret != 0 || extURLs.empty())
			continue;

		for (auto iter = extURLs.cbegin(); iter != extURLs.end(); ++iter) {
			os << "-- " <<
				RomData::getImageTypeName((RomData::ImageType)i) << ": " << iter->url <<
				" (cache_key: " << iter->cache_key << ')' << endl;
		}
	}
	return os;
}

JSONROMOutput::JSONROMOutput(const RomData *romdata) : romdata(romdata) {}
std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo) {
	auto romdata = fo.romdata;
	assert(romdata && romdata->isValid());

	const char *sysName = romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_GENERIC);
	const char *fileType = romdata->fileType_string();

	os << "{\"system\":";
	if (sysName) {
		os << JSONString(sysName);
	} else {
		os << "\"unknown\"";
	}
	os << ",\"filetype\":";
	if (fileType) {
		os << JSONString(fileType);
	} else {
		os << "\"unknown\"";
	}
	os << ",\"fields\":" << JSONFieldsOutput(*(romdata->fields()));

	int supported = romdata->supportedImageTypes();

	bool first = true;
	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (!(supported & (1 << i)))
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
					for (int i = 0; i < animdata->seq_count; i++) {
						if (i) os << ',';
						os << (unsigned)animdata->seq_index[i];
					}
					os << "],\"delay\":[";
					for (int i = 0; i < animdata->seq_count; i++) {
						if (i) os << ',';
						os << animdata->delays[i].ms;
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
	std::vector<RomData::ExtURL> extURLs;
	for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
		if (!(supported & (1 << i)))
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
		bool firsturl = true;

		for (auto iter = extURLs.cbegin(); iter != extURLs.end(); ++iter) {
			if (firsturl) firsturl = false;
			else os << ',';

			os << "{\"url\":" << JSONString(iter->url.c_str());
			os << ",\"cache_key\":" << JSONString(iter->cache_key.c_str()) << '}';
		}
	}
	if (!first) {
		os << ']';
	}

	return os << '}';
}

/***************************************************************************
 * ROM Properties Page shell extension. (rpcli)                            *
 * properties.cpp: Properties output.                                      *
 *                                                                         *
 * Copyright (c) 2016 by Egor.                                             *
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
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/
#include "properties.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <libromdata/RomData.hpp>
#include <libromdata/RomFields.hpp>
#include <libromdata/TextFuncs.hpp>
#include <libromdata/img/rp_image.hpp>
#include <libromdata/img/IconAnimData.hpp>
using std::setw;
using std::left;
using std::ostream;
using std::max;
using std::endl;
using namespace LibRomData;
class Pad {
	size_t width;
public:
	Pad(size_t width) :width(width) {}
	friend ostream& operator<<(ostream& os, const Pad& pad) {
		return os << setw(pad.width) << "";
	}

};
class ColonPad {
	size_t width;
	const rp_char* str;
public:
	ColonPad(size_t width, const rp_char* str) :width(width), str(str) {}
	friend ostream& operator<<(ostream& os, const ColonPad& cp) {
		return os << cp.str << left << setw(max(0, (signed)(cp.width - rp_strlen(cp.str)))) << ":";
	}
};
class SafeString {
	const rp_char* str;
	bool quotes;
public:
	SafeString(const rp_char* str, bool quotes = true) :str(str), quotes(quotes) {}
	friend ostream& operator<<(ostream& os, const SafeString& cp) {
		if (!cp.str) {
			//assert(0); // RomData should never return a null string // disregard that
			return os << "(null)";
		}
		if (cp.quotes) {
			return os << "'" << cp.str << "'";
		}
		else {
			return os << cp.str;
		}
	}
};
class StringField {
	size_t width;
	const RomFields::Desc* desc;
	const RomFields::Data* data;
public:
	StringField(size_t width, const RomFields::Desc* desc, const RomFields::Data* data) :width(width), desc(desc), data(data) {}
	friend ostream& operator<<(ostream& os, const StringField& field) {
		auto desc = field.desc;
		auto data = field.data;
		return os << ColonPad(field.width, desc->name) << SafeString(data->str, true);
	}
};

class BitfieldField {
	size_t width;
	const RomFields::Desc* desc;
	const RomFields::Data* data;
public:
	BitfieldField(size_t width, const RomFields::Desc* desc, const RomFields::Data* data) :width(width), desc(desc), data(data) {}
	friend ostream& operator<<(ostream& os, const BitfieldField& field) {
		auto desc = field.desc;
		auto data = field.data;

		int perRow = desc->bitfield->elemsPerRow ? desc->bitfield->elemsPerRow : 4;

		size_t *colSize = new size_t[perRow]();
		for (int i = 0; i < desc->bitfield->elements; i++) {
			colSize[i%perRow] = max(rp_strlen(desc->bitfield->names[i]), colSize[i%perRow]);
		}

		os << ColonPad(field.width, desc->name); // ColonPad sets std::left

		for (int i = 0; i < desc->bitfield->elements; i++) {
			if (i && i%perRow == 0) os << endl << Pad(field.width);
			os << " [" << (data->bitfield & (1 << i) ? '*' : ' ') << "] " <<
				setw(colSize[i%perRow]) << desc->bitfield->names[i];
		}
		delete[] colSize;
		return os;
	}
};

class ListDataField {
	size_t width;
	const RomFields::Desc* desc;
	const RomFields::Data* data;
public:
	ListDataField(size_t width, const RomFields::Desc* desc, const RomFields::Data* data) :width(width), desc(desc), data(data) {}
	friend ostream& operator<<(ostream& os, const ListDataField& field) {
		auto desc = field.desc;
		auto data = field.data;

		size_t *colSize = new size_t[desc->list_data->count]();
		size_t totalWidth = desc->list_data->count + 1;
		for (int i = 0; i < desc->list_data->count; i++) {
			colSize[i] = rp_strlen(desc->list_data->names[i]);
		}
		for (auto it = data->list_data->data.begin(); it != data->list_data->data.end(); ++it) {
			int i = 0;
			for (auto jt = it->begin(); jt != it->end(); ++jt) {
				colSize[i] = max(jt->length(), colSize[i]);
				i++;
			}
		}

		os << ColonPad(field.width, desc->name); // ColonPad sets std::left

		for (int i = 0; i < desc->list_data->count; i++) {
			totalWidth += colSize[i]; // this could be in a separate loop, but whatever
			os << "|" << setw(colSize[i]) << desc->list_data->names[i];
		}
		os << "|" << endl << Pad(field.width) << rp_string(totalWidth, '-');
		for (auto it = data->list_data->data.begin(); it != data->list_data->data.end(); ++it) {
			int i = 0;
			os << endl << Pad(field.width);
			for (auto jt = it->begin(); jt != it->end(); ++jt) {
				os << "|" << setw(colSize[i++]) << *jt;
			}
			os << "|";
		}
		delete[] colSize;
		return os;
	}
};

class DateTimeField {
	size_t width;
	const RomFields::Desc* desc;
	const RomFields::Data* data;
public:
	DateTimeField(size_t width, const RomFields::Desc* desc, const RomFields::Data* data) :width(width), desc(desc), data(data) {}
	friend ostream& operator<<(ostream& os, const DateTimeField& field) {
		auto desc = field.desc;
		auto data = field.data;

		assert(desc->date_time);
		auto flags = desc->date_time->flags;

		os << ColonPad(field.width, desc->name); // ColonPad sets std::left

		tm timestamp;
		if (flags & RomFields::RFT_DATETIME_IS_UTC) {
			timestamp = *gmtime((time_t*)&data->date_time);
		}
		else {
			timestamp = *localtime((time_t*)&data->date_time);
		}

		static const char *formats[4] = {
			"Invalid DateTime",
			"%x", // Date
			"%X", // Time
			"%x %X", // Date Time
		};

		char str[128];

		strftime(str, 128, formats[flags & RomFields::RFT_DATETIME_HAS_DATETIME_MASK], &timestamp);

		os << str;

		return os;
	}
};

class FieldsOutput {
	const RomFields& fields;
public:
	FieldsOutput(const RomFields& fields) :fields(fields) {}
	friend std::ostream& operator<<(std::ostream& os, const FieldsOutput& fo) {
		size_t maxWidth = 0;
		for (int i = 0; i < fo.fields.count(); i++) {
			maxWidth = max(maxWidth, rp_strlen(fo.fields.desc(i)->name));
		}
		maxWidth += 2;
		for (int i = 0; i < fo.fields.count(); i++) {
			auto desc = fo.fields.desc(i);
			auto data = fo.fields.data(i);
			assert(desc);
			assert(data);
			if (i) os << endl;
			switch (desc->type) {
			case RomFields::RFT_INVALID: {
				assert(!"INVALID field type");
				os << ColonPad(maxWidth, desc->name) << "INVALID";
				break;
			}
			case RomFields::RFT_STRING: {
				os << StringField(maxWidth, desc, data);
				break;
			}
			case RomFields::RomFields::RFT_BITFIELD: {
				os << BitfieldField(maxWidth, desc, data);
				break;
			}
			case RomFields::RomFields::RFT_LISTDATA: {
				os << ListDataField(maxWidth, desc, data);
				break;
			}
			case RomFields::RomFields::RFT_DATETIME: {
				os << DateTimeField(maxWidth, desc, data);
				break;
			}
			default: {
				assert(!"Unknown RomFieldType");
				os << ColonPad(maxWidth, desc->name) << "NYI";
			}
			}
		}
		return os;
	}
};

class JSONString {
	const rp_char* str;
	static rp_string Replace(rp_string str, const rp_string &a, const rp_string &b) {
		size_t pos = 0;
		while ((pos = str.find(a, pos)) != rp_string::npos) {
			str.replace(pos, a.length(), b);
			pos += b.length();
		}
		return str;
	}
public:
	JSONString(const rp_char* str) :str(str) {}
	friend ostream& operator<<(ostream& os, const JSONString& js) {
		//assert(js.str); // not all strings can't be null, apparently
		if (!js.str) return os << "0"; // clever way to distinguish nullptr
		rp_string escaped = rp_string(js.str);
		escaped = Replace(escaped, "\\", "\\\\");
		escaped = Replace(escaped, "\"", "\\\"");
		escaped = Replace(escaped, "\b", "\\b");
		escaped = Replace(escaped, "\f", "\\f");
		escaped = Replace(escaped, "\t", "\\t");
		escaped = Replace(escaped, "\n", "\\n");
		escaped = Replace(escaped, "\r", "\\r");
		return os << "\"" << escaped << "\"";
	}
};

class JSONFieldsOutput {
	const RomFields& fields;
public:
	JSONFieldsOutput(const RomFields& fields) :fields(fields) {}
	friend std::ostream& operator<<(std::ostream& os, const JSONFieldsOutput& fo) {
		// TODO: make DoFile output everything as json in json mode
		os << "[";
		for (int i = 0; i < fo.fields.count(); i++) {
			if (i) os << ",";
			auto desc = fo.fields.desc(i);
			auto data = fo.fields.data(i);
			assert(desc);
			assert(data);
			if (i) os << endl;
			switch (desc->type) {
			case RomFields::RFT_INVALID: {
				assert(0); // INVALID field type
				os << "{\"type\":\"INVALID\"}";
				break;
			}
			case RomFields::RFT_STRING: {
				os << "{\"type\":\"STRING\",\"desc\":{\"name\":" << JSONString(desc->name);
				if (desc->str_desc) { // nullptr check is required
					os << ",\"format\":" << desc->str_desc->formatting;
				}
				os << "},\"data\":" << JSONString(data->str) << "}";
				break;
			}
			case RomFields::RomFields::RFT_BITFIELD: {
				os << "{\"type\":\"BITFIELD\",\"desc\":{\"name\":" << JSONString(desc->name);
				assert(desc->bitfield);
				if (desc->bitfield) {
					os << ",\"elements\":" << desc->bitfield->elements;
					os << ",\"elementsPerRow\":" << desc->bitfield->elemsPerRow;
					os << ",\"names\":[";
					for (int j = 0; j < desc->bitfield->elements; j++) {
						if (j) os << ",";
						os << JSONString(desc->bitfield->names[j]);
					}
					os << "]";

				}
				os << "},\"data\":" << data->bitfield << "}";
				break;
			}
			case RomFields::RomFields::RFT_LISTDATA: {
				os << "{\"type\":\"LISTDATA\",\"desc\":{\"name\":" << JSONString(desc->name);
				assert(desc->list_data);
				if (desc->list_data) {
					os << ",\"count\":" << desc->list_data->count;
					os << ",\"names\":[";
					for (int j = 0; j < desc->list_data->count; j++) {
						if (j) os << ",";
						os << JSONString(desc->list_data->names[j]);
					}
					os << "]";
				}
				os << "},\"data\":[";
				assert(data->list_data);
				if (data->list_data) {
					for (auto it = data->list_data->data.begin(); it != data->list_data->data.end(); ++it) {
						if (it != data->list_data->data.begin()) os << ",";
						os << "[";
						for (auto jt = it->begin(); jt != it->end(); ++jt) {
							if (jt != it->begin()) os << ",";
							os << JSONString(jt->c_str());
						}
						os << "]";
					}
				}
				os << "]}";
				break;
			}
			case RomFields::RFT_DATETIME: {
				os << "{\"type\":\"DATETIME\",\"desc\":{\"name\":" << JSONString(desc->name);
				assert(desc->date_time);
				if (desc->date_time) {
					os << ",\"flags\":" << desc->date_time->flags;
				}
				os << "},\"data\":" << data->date_time << "}";
				break;
			}
			default: {
				assert(!"Unknown RomFieldType");
				os << "{\"type\":\"NYI\",\"desc\":{\"name\":" << JSONString(desc->name) << "}}";
			}
			}
		}
		os << "]";
		return os;
	}
};



ROMOutput::ROMOutput(const LibRomData::RomData* romdata) : romdata(romdata) { }
std::ostream& operator<<(std::ostream& os, const ROMOutput& fo) {
	auto romdata = fo.romdata;
	os << "-- " << romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_GENERIC) << " rom detected" << endl;
	os << FieldsOutput(*(romdata->fields())) << endl;

	int supported = romdata->supportedImageTypes();

	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (supported&(1 << i)) {
			os << "-- " << RomData::getImageTypeName((RomData::ImageType)i) << " is present (use -x" << i << " to extract)" << endl;
			auto image = romdata->image((RomData::ImageType)i);
			if (image && image->isValid()) {
				os << "   Format : " << rp_image::getFormatName(image->format()) << endl;
				os << "   Size   : " << image->width() << " x " << image->height() << endl;
				if (romdata->imgpf((RomData::ImageType) i)  & RomData::IMGPF_ICON_ANIMATED) {
					os << "   Animated icon present (use -a to extract)" << endl;
				}
			}
		}
	}

	for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
		if (supported&(1 << i)) {
			auto &urls = *romdata->extURLs((RomData::ImageType)i);
			for (auto s : urls)
				os << "-- " << RomData::getImageTypeName((RomData::ImageType)i) << ": " << s.url << " (cache_key: " << s.cache_key << ")" << endl;
		}
	}
	return os;
}

JSONROMOutput::JSONROMOutput(const LibRomData::RomData* romdata) : romdata(romdata) {}
std::ostream& operator<<(std::ostream& os, const JSONROMOutput& fo) {
	auto romdata = fo.romdata;
	assert(romdata && romdata->isValid());

	os << "{\"system\":" << JSONString(romdata->systemName(RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_GENERIC));
	os << ",\"filetype\":" << JSONString(romdata->fileType_string());
	os << ",\"fields\":" << JSONFieldsOutput(*(romdata->fields()));

	int supported = romdata->supportedImageTypes();

	os << ",\"imgint\":[";
	bool first = true;
	for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
		if (supported&(1 << i)) {
			if (first) first = false;
			else os << ",";

			os << "{\"type\":" << JSONString(RomData::getImageTypeName((RomData::ImageType)i));
			auto image = romdata->image((RomData::ImageType)i);
			if (image && image->isValid()) {
				os << ",\"format\":" << JSONString(rp_image::getFormatName(image->format()));
				os << ",\"size\":[" << image->width() << "," << image->height() << "]";
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
							if (i) os << ",";
							os << (unsigned)animdata->seq_index[i];
						}
						os << "],\"delay\":[";
						for (int i = 0; i < animdata->seq_count; i++) {
							if (i) os << ",";
							os << animdata->delays[i];
						}
						os << "]";
					}
				}
			}
			os << "}";
		}
	}

	os << "],\"imgext\":[";
	first = true;
	for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
		if (supported&(1 << i)) {
			if (first) first = false;
			else os << ",";

			os << "{\"type\":" << JSONString(RomData::getImageTypeName((RomData::ImageType)i));
			int ppf = romdata->imgpf((RomData::ImageType) i);
			if (ppf) {
				os << ",\"postprocessing\":" << ppf;
			}
			// NOTE: IMGPF_ICON_ANIMATED won't ever appear in external image
			os << ",\"exturls\":[";
			bool firsturl = true;
			auto &urls = *romdata->extURLs((RomData::ImageType)i);
			for (auto s : urls) {
				if (firsturl) firsturl = false;
				else os << ",";

				os << "{\"url\":" << JSONString(s.url.c_str());
				os << ",\"cache_key\":" << JSONString(s.cache_key.c_str()) << "}";
			}
			os << "]}";
		}
	}
	return os << "]}";
}
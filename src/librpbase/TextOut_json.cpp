/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextOut.hpp: Text output for RomData. (JSON output)                     *
 *                                                                         *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TextOut.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include "ctypex.h"

// C++ includes.
using std::endl;
using std::ostream;
using std::string;
using std::vector;

// librpbase
#include "RomData.hpp"
#include "RomFields.hpp"
#include "TextFuncs.hpp"
#include "img/IconAnimData.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

namespace LibRpBase {

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

			os << "{\"url\":" << JSONString(urlPartialUnescape(iter->url).c_str());
			os << ",\"cache_key\":" << JSONString(iter->cache_key.c_str()) << '}';
		}
		os << "]}";
	}
	if (!first) {
		os << ']';
	}

	return os << '}';
}

}

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

// rapidjson
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

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

template<typename Allocator>
class JSONFieldsOutput {
	const RomFields& fields;
public:
	explicit JSONFieldsOutput(const RomFields& fields) :fields(fields) {}

	void writeToJSON(Value &fields_array, Allocator &allocator)
	{
		const auto iter_end = fields.cend();
		for (auto iter = fields.cbegin(); iter != iter_end; ++iter) {
			const auto &romField = *iter;
			if (!romField.isValid)
				continue;

			Value field_obj(kObjectType);	// field

			switch (romField.type) {
				case RomFields::RFT_INVALID: {
					assert(!"INVALID field type");
					field_obj.AddMember("type", "INVALID", allocator);
					break;
				}

				case RomFields::RFT_STRING: {
					field_obj.AddMember("type", "STRING", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("format", romField.desc.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					field_obj.AddMember("data", StringRef(*(romField.data.str)), allocator);
					break;
				}

				case RomFields::RFT_BITFIELD: {
					field_obj.AddMember("type", "BITFIELD", allocator);
					const auto &bitfieldDesc = romField.desc.bitfield;

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("elementsPerRow", bitfieldDesc.elemsPerRow, allocator);

					assert(bitfieldDesc.names != nullptr);
					if (bitfieldDesc.names) {
						Value names_array(kArrayType);	// names
						unsigned int count = static_cast<unsigned int>(bitfieldDesc.names->size());
						assert(count <= 32);
						if (count > 32)
							count = 32;
						const auto iter_end = bitfieldDesc.names->cend();
						for (auto iter = bitfieldDesc.names->cbegin(); iter != iter_end; ++iter) {
							const string &name = *iter;
							if (name.empty())
								continue;

							names_array.PushBack(StringRef(name), allocator);
						}
						if (!names_array.Empty()) {
							desc_obj.AddMember("names", names_array, allocator);
						} else {
							desc_obj.AddMember("names", "ERROR", allocator);
						}
					} else {
						desc_obj.AddMember("names", "ERROR", allocator);
					}

					field_obj.AddMember("desc", desc_obj, allocator);
					field_obj.AddMember("data", romField.data.bitfield, allocator);
					break;
				}

				case RomFields::RFT_LISTDATA: {
					field_obj.AddMember("type", "LISTDATA", allocator);
					const auto &listDataDesc = romField.desc.list_data;

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);

					Value names_array(kArrayType);	// names
					if (listDataDesc.names) {
						if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
							// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
							names_array.PushBack("checked", allocator);
						}
						for (auto iter = listDataDesc.names->cbegin();
						     iter != listDataDesc.names->cend(); ++iter)
						{
							const string &name = *iter;
							names_array.PushBack(StringRef(name), allocator);
						}
					}
					desc_obj.AddMember("names", names_array, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					if (!(listDataDesc.flags & RomFields::RFT_LISTDATA_MULTI)) {
						// Single-language ListData.
						Value data_array(kArrayType);	// data
						const auto list_data = romField.data.list_data.data.single;
						assert(list_data != nullptr);
						if (!list_data) {
							// No data...
							field_obj.AddMember("data", "ERROR", allocator);
							break;
						}

						uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
						for (auto it = list_data->cbegin(); it != list_data->cend(); ++it) {
							Value row_array(kArrayType);
							if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
								// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
								row_array.PushBack((checkboxes & 1) ? true : false, allocator);
								checkboxes >>= 1;
							}

							for (auto jt = it->cbegin(); jt != it->cend(); ++jt) {
								row_array.PushBack(StringRef(*jt), allocator);
							}

							data_array.PushBack(row_array, allocator);
						}

						field_obj.AddMember("data", data_array, allocator);
					} else {
						// Multi-language ListData.
						Value data_obj(kObjectType);	// data
						const auto *const list_data = romField.data.list_data.data.multi;
						assert(list_data != nullptr);
						if (!list_data) {
							// No data...
							field_obj.AddMember("data", "ERROR", allocator);
							break;
						}

						for (auto mapIter = list_data->cbegin(); mapIter != list_data->cend(); ++mapIter) {
							// Key: Language code
							// Value: Vector of string data
							char s_lc[8];
							int s_lc_pos = 0;
							for (uint32_t lc = mapIter->first; lc != 0; lc <<= 8) {
								char chr = (char)(lc >> 24);
								if (chr != 0) {
									s_lc[s_lc_pos++] = chr;
								}
							}
							s_lc[s_lc_pos] = '\0';

							Value s_lc_name;
							s_lc_name.SetString(s_lc, s_lc_pos, allocator);

							Value lc_array(kArrayType);	// language data
							// TODO: Consolidate single/multi here?
							const auto &lc_data = mapIter->second;
							if (lc_data.empty()) {
								// No data...
								data_obj.AddMember(s_lc_name, "ERROR", allocator);
								continue;
							}

							uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
							for (auto lcIter = lc_data.cbegin(); lcIter != lc_data.cend(); ++lcIter) {
								Value row_array(kArrayType);
								if (listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES)
								{
									// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
									row_array.PushBack((checkboxes & 1) ? true : false, allocator);
									checkboxes >>= 1;
								}

								for (auto jt = lcIter->cbegin(); jt != lcIter->cend(); ++jt) {
									row_array.PushBack(StringRef(*jt), allocator);
								}

								lc_array.PushBack(row_array, allocator);
							}

							data_obj.AddMember(s_lc_name, lc_array, allocator);
						}

						field_obj.AddMember("data", data_obj, allocator);
					}
					break;
				}

				case RomFields::RFT_DATETIME: {
					field_obj.AddMember("type", "DATETIME", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("flags", romField.desc.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					field_obj.AddMember("data", romField.data.date_time, allocator);
					break;
				}

				case RomFields::RFT_AGE_RATINGS: {
					field_obj.AddMember("type", "AGE_RATINGS", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					const RomFields::age_ratings_t *age_ratings = romField.data.age_ratings;
					assert(age_ratings != nullptr);
					if (!age_ratings) {
						field_obj.AddMember("data", "ERROR", allocator);
						break;
					}

					Value data_array(kArrayType);	// data
					const unsigned int age_ratings_max = static_cast<unsigned int>(age_ratings->size());
					for (unsigned int j = 0; j < age_ratings_max; j++) {
						const uint16_t rating = age_ratings->at(j);
						if (!(rating & RomFields::AGEBF_ACTIVE))
							continue;

						Value rating_obj(kObjectType);
						const char *const abbrev = RomFields::ageRatingAbbrev(j);
						if (abbrev) {
							rating_obj.AddMember("name", StringRef(abbrev), allocator);
						} else {
							// Invalid age rating.
							// Use the numeric index.
							rating_obj.AddMember("name", j, allocator);
						}

						const string s_age_rating = RomFields::ageRatingDecode(j, rating);
						Value rating_val;
						rating_val.SetString(s_age_rating, allocator);
						rating_obj.AddMember("rating", rating_val, allocator);

						data_array.PushBack(rating_obj, allocator);
					}

					field_obj.AddMember("data", data_array, allocator);
					break;
				}

				case RomFields::RFT_DIMENSIONS: {
					field_obj.AddMember("type", "DIMENSIONS", allocator);

					const int *const dimensions = romField.data.dimensions;
					Value data_obj(kObjectType);	// data
					data_obj.AddMember("w", dimensions[0], allocator);
					if (dimensions[1] > 0) {
						data_obj.AddMember("h", dimensions[1], allocator);
						if (dimensions[2] > 0) {
							data_obj.AddMember("d", dimensions[2], allocator);
						}
					}
					field_obj.AddMember("data", data_obj, allocator);
					break;
				}

				case RomFields::RFT_STRING_MULTI: {
					// TODO: Act like RFT_STRING if there's only one language?
					field_obj.AddMember("type", "STRING_MULTI", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("format", romField.desc.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					Value data_obj(kObjectType);	// data
					const auto *const pStr_multi = romField.data.str_multi;
					for (auto iter = pStr_multi->cbegin(); iter != pStr_multi->cend(); ++iter) {
						// Convert the language code to ASCII.
						char s_lc[8];
						int s_lc_pos = 0;
						for (uint32_t lc = iter->first; lc != 0; lc <<= 8) {
							char chr = (char)(lc >> 24);
							if (chr != 0) {
								s_lc[s_lc_pos++] = chr;
							}
						}
						s_lc[s_lc_pos] = '\0';

						Value s_lc_name;
						s_lc_name.SetString(s_lc, s_lc_pos, allocator);

						data_obj.AddMember(s_lc_name, StringRef(iter->second), allocator);
					}

					field_obj.AddMember("data", data_obj, allocator);
					break;
				}

				default: {
					assert(!"Unknown RomFieldType");
					field_obj.AddMember("type", "NYI", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					field_obj.AddMember("desc", desc_obj, allocator);
					break;
				}
			}

			fields_array.PushBack(field_obj, allocator);
		}
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

	Document document;
	document.SetObject();	// document should be an object, not an array
	Document::AllocatorType& allocator = document.GetAllocator();
	document.AddMember("system", StringRef(systemName ? systemName : "unknown"), allocator);
	document.AddMember("filetype", StringRef(fileType ? fileType : "unknown"), allocator);

	// Fields.
	const RomFields *const fields = romdata->fields();
	assert(fields != nullptr);
	if (fields) {
		Value fields_array(kArrayType);	// fields
		JSONFieldsOutput<Document::AllocatorType>(*fields).writeToJSON(fields_array, allocator);
		if (!fields_array.Empty()) {
			document.AddMember("fields", fields_array, allocator);
		}
	}

	// Internal images.
	const uint32_t imgbf = romdata->supportedImageTypes();
	if (imgbf != 0) {
		Value imgint_array(kArrayType);	// imgint

		for (int i = RomData::IMG_INT_MIN; i <= RomData::IMG_INT_MAX; i++) {
			if (!(imgbf & (1U << i)))
				continue;

			auto image = romdata->image((RomData::ImageType)i);
			if (!image || !image->isValid())
				continue;

			Value imgint_obj(kObjectType);
			imgint_obj.AddMember("type", StringRef(RomData::getImageTypeName((RomData::ImageType)i)), allocator);
			imgint_obj.AddMember("format", StringRef(rp_image::getFormatName(image->format())), allocator);

			Value sizebuf_val;
			char sizebuf[32];
			int snret = snprintf(sizebuf, sizeof(sizebuf), "%d,%d", image->width(), image->height());
			sizebuf_val.SetString(sizebuf, snret, allocator);
			imgint_obj.AddMember("size", sizebuf_val, allocator);

			const uint32_t ppf = romdata->imgpf((RomData::ImageType)i);
			if (ppf) {
				imgint_obj.AddMember("postprocessing", ppf, allocator);
			}

			if (ppf & RomData::IMGPF_ICON_ANIMATED) {
				auto animdata = romdata->iconAnimData();
				if (animdata) {
					imgint_obj.AddMember("frames", animdata->count, allocator);

					Value animseq(kArrayType);	// sequence
					for (int j = 0; j < animdata->seq_count; j++) {
						animseq.PushBack((unsigned)animdata->seq_index[j], allocator);
					}
					imgint_obj.AddMember("sequence", animseq, allocator);

					Value animdelay(kArrayType);	// delay
					for (int j = 0; j < animdata->seq_count; j++) {
						animdelay.PushBack(animdata->delays[j].ms, allocator);
					}
					imgint_obj.AddMember("delay", animdelay, allocator);
				}
			}

			imgint_array.PushBack(imgint_obj, allocator);
		}
		if (!imgint_array.Empty()) {
			document.AddMember("imgint", imgint_array, allocator);
		}

		// External images.
		// NOTE: IMGPF_ICON_ANIMATED won't ever appear in external image
		Value imgext_array(kArrayType);	// imgext
		vector<RomData::ExtURL> extURLs;
		for (int i = RomData::IMG_EXT_MIN; i <= RomData::IMG_EXT_MAX; i++) {
			if (!(imgbf & (1U << i)))
				continue;

			// NOTE: extURLs may be empty even though the class supports it.
			// Check extURLs before doing anything else.

			extURLs.clear();	// NOTE: May not be needed...
			// TODO: Customize the image size parameter?
			// TODO: Option to retrieve supported image size?
			int ret = romdata->extURLs((RomData::ImageType)i, &extURLs, RomData::IMAGE_SIZE_DEFAULT);
			if (ret != 0 || extURLs.empty())
				continue;

			Value imgext_obj(kObjectType);
			imgext_obj.AddMember("type", StringRef(RomData::getImageTypeName((RomData::ImageType)i)), allocator);

			Value exturls_obj(kObjectType);
			for (auto iter = extURLs.cbegin(); iter != extURLs.cend(); ++iter) {
				Value url_val;
				const string url_str = urlPartialUnescape(iter->url);
				url_val.SetString(url_str.data(), url_str.size(), allocator);
				exturls_obj.AddMember("url", url_val, allocator);

				Value cache_key_val;
				cache_key_val.SetString(iter->cache_key.data(), iter->cache_key.size(), allocator);
				exturls_obj.AddMember("cache_key", cache_key_val, allocator);
			}
			imgext_obj.AddMember("exturls", exturls_obj, allocator);
			imgext_array.PushBack(imgext_obj, allocator);
		}
		if (!imgext_array.Empty()) {
			document.AddMember("imgext", imgext_array, allocator);
		}
	}

	OStreamWrapper oswr(os);
	PrettyWriter<OStreamWrapper> writer(oswr);
	document.Accept(writer);

	return os;
}

}

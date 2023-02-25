/***************************************************************************
 * ROM Properties Page shell extension. (librpbase)                        *
 * TextOut.hpp: Text output for RomData. (JSON output)                     *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * Copyright (c) 2016-2018 by Egor.                                        *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "TextOut.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
using std::ostream;
using std::string;
using std::vector;

// librpbase
#include "RomData.hpp"
#include "RomFields.hpp"
#include "img/IconAnimData.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// rapidjson
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
using namespace rapidjson;

// TextOut_json isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern "C" {
	extern uint8_t RP_LibRpBase_TextOut_json_ForceLinkage;
	uint8_t RP_LibRpBase_TextOut_json_ForceLinkage;
}

namespace LibRpBase {

template<typename Allocator>
class JSONFieldsOutput {
	const RomFields& fields;
public:
	explicit JSONFieldsOutput(const RomFields& fields) :fields(fields) {}

private:
	static Value lcToValue(uint32_t lc, Allocator &allocator)
	{
		char s_lc[8];
		int s_lc_pos = 0;
		for (; lc != 0; lc <<= 8) {
			char chr = (char)(lc >> 24);
			if (chr != 0) {
				s_lc[s_lc_pos++] = chr;
			}
		}
		s_lc[s_lc_pos] = '\0';

		Value s_lc_name;
		s_lc_name.SetString(s_lc, s_lc_pos, allocator);
		return s_lc_name;
	}

	static Value listDataToValue(const RomFields::Field &romField,
		const RomFields::ListData_t *list_data, Allocator &allocator)
	{
		Value data_array(kArrayType);	// data
		assert(list_data != nullptr);
		if (!list_data) {
			// No data...
			return data_array;
		}

		const bool has_checkboxes = !!(romField.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
		uint32_t checkboxes = romField.data.list_data.mxd.checkboxes;
		const auto list_data_cend = list_data->cend();
		for (auto it = list_data->cbegin(); it != list_data_cend; ++it) {
			Value row_array(kArrayType);
			if (has_checkboxes) {
				// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
				row_array.PushBack((checkboxes & 1) ? true : false, allocator);
				checkboxes >>= 1;
			}

			const auto it_cend = it->cend();
			for (auto jt = it->cbegin(); jt != it_cend; ++jt) {
				row_array.PushBack(StringRef(*jt), allocator);
			}

			data_array.PushBack(row_array, allocator);
		}

		return data_array;
	}

public:
	void writeToJSON(Value &fields_array, Allocator &allocator)
	{
		const auto fields_cend = fields.cend();
		for (auto iter = fields.cbegin(); iter != fields_cend; ++iter) {
			const auto &romField = *iter;
			assert(romField.isValid());
			if (!romField.isValid())
				continue;

			Value field_obj(kObjectType);	// field
			switch (romField.type) {
				case RomFields::RFT_INVALID:
					// Should not happen due to the above check...
					assert(!"Field type is RFT_INVALID");
					break;

				case RomFields::RFT_STRING: {
					field_obj.AddMember("type", "STRING", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("format", romField.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					field_obj.AddMember("data",
						romField.data.str ? StringRef(romField.data.str) : StringRef(""),
						allocator);
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
						const auto names_cend = bitfieldDesc.names->cend();
						for (auto iter = bitfieldDesc.names->cbegin(); iter != names_cend; ++iter) {
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
						if (romField.flags & RomFields::RFT_LISTDATA_CHECKBOXES) {
							// TODO: Better JSON schema for RFT_LISTDATA_CHECKBOXES?
							names_array.PushBack("checked", allocator);
						}
						const auto names_cend = listDataDesc.names->cend();
						for (auto iter = listDataDesc.names->cbegin();
						     iter != names_cend; ++iter)
						{
							const string &name = *iter;
							names_array.PushBack(StringRef(name), allocator);
						}
					}
					desc_obj.AddMember("names", names_array, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					if (!(romField.flags & RomFields::RFT_LISTDATA_MULTI)) {
						// Single-language ListData.
						Value data_array = listDataToValue(romField,
							romField.data.list_data.data.single, allocator);
						if (!data_array.Empty()) {
							field_obj.AddMember("data", data_array, allocator);
						} else {
							// No data...
							field_obj.AddMember("data", "ERROR", allocator);
							break;
						}
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

						const auto list_data_cend = list_data->cend();
						for (auto mapIter = list_data->cbegin(); mapIter != list_data_cend; ++mapIter) {
							// Key: Language code
							// Value: Vector of string data
							Value s_lc_name = lcToValue(mapIter->first, allocator);

							Value lc_array = listDataToValue(romField,
								&mapIter->second, allocator);
							if (!lc_array.Empty()) {
								data_obj.AddMember(s_lc_name, lc_array, allocator);
							} else {
								// No data...
								data_obj.AddMember(s_lc_name, "ERROR", allocator);
								continue;
							}
						}

						field_obj.AddMember("data", data_obj, allocator);
					}
					break;
				}

				case RomFields::RFT_DATETIME: {
					field_obj.AddMember("type", "DATETIME", allocator);

					Value desc_obj(kObjectType);	// desc
					desc_obj.AddMember("name", StringRef(romField.name), allocator);
					desc_obj.AddMember("flags", romField.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					field_obj.AddMember("data", static_cast<int64_t>(romField.data.date_time), allocator);
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
						const char *const abbrev = RomFields::ageRatingAbbrev((RomFields::AgeRatingsCountry)j);
						if (abbrev) {
							rating_obj.AddMember("name", StringRef(abbrev), allocator);
						} else {
							// Invalid age rating.
							// Use the numeric index.
							rating_obj.AddMember("name", j, allocator);
						}

						const string s_age_rating = RomFields::ageRatingDecode((RomFields::AgeRatingsCountry)j, rating);
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
					desc_obj.AddMember("format", romField.flags, allocator);
					field_obj.AddMember("desc", desc_obj, allocator);

					Value data_obj(kObjectType);	// data
					const auto *const pStr_multi = romField.data.str_multi;
					const auto pStr_multi_cend = pStr_multi->cend();
					for (auto iter = pStr_multi->cbegin(); iter != pStr_multi_cend; ++iter) {
						Value s_lc_name = lcToValue(iter->first, allocator);
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


JSONROMOutput::JSONROMOutput(const RomData *romdata, uint32_t lc, unsigned int flags)
	: romdata(romdata)
	, lc(lc)
	, flags(flags)
	, crlf_(false) { }
RP_LIBROMDATA_PUBLIC
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

	// Fields
	const RomFields *const fields = romdata->fields();
	assert(fields != nullptr);
	if (fields) {
		Value fields_array(kArrayType);	// fields
		JSONFieldsOutput<Document::AllocatorType>(*fields).writeToJSON(fields_array, allocator);
		if (!fields_array.Empty()) {
			document.AddMember("fields", fields_array, allocator);
		}
	}

	const uint32_t imgbf = romdata->supportedImageTypes();
	if (imgbf != 0) {
		if (!(fo.flags & OF_SkipInternalImages)) {
			// Internal images
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

				Value size_array(kArrayType);	// size
				size_array.PushBack(image->width(), allocator);
				size_array.PushBack(image->height(), allocator);
				imgint_obj.AddMember("size", size_array, allocator);

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
		}

		// External image URLs
		// NOTE: IMGPF_ICON_ANIMATED won't ever appear in external images.
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
			const auto extURLs_cend = extURLs.cend();
			for (auto iter = extURLs.cbegin(); iter != extURLs_cend; ++iter) {
				Value url_val;
				const string url_str = urlPartialUnescape(iter->url);
				url_val.SetString(url_str, allocator);
				exturls_obj.AddMember("url", url_val, allocator);

				Value cache_key_val;
				cache_key_val.SetString(iter->cache_key, allocator);
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
	if (fo.flags & OF_JSON_NoPrettyPrint) {
		// Don't use pretty-printing. (minimal JSON)
		Writer<OStreamWrapper> writer(oswr);
		document.Accept(writer);
	} else {
		// Use pretty-printing.
		PrettyWriter<OStreamWrapper> writer(oswr);
		writer.SetNewlineMode(fo.crlf_);
		document.Accept(writer);
	}

	os.flush();
	return os;
}

}

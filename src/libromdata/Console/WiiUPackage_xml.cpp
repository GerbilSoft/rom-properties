/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage_xml.cpp: Wii U NUS Package reader. (XML parsing)            *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "WiiUPackage_p.hpp"

#ifndef ENABLE_XML
#  error Cannot compile EXE_manifest.cpp without XML support.
#endif

// for Wii U application types
#include "data/WiiUData.hpp"

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRpText;

// for the system language code
#include "SystemRegion.hpp"

// PugiXML
#include <pugixml.hpp>
using namespace pugi;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

// Wii U region code bitfield names
const array<const char*, 7> WiiUPackagePrivate::wiiu_region_bitfield_names = {{
	NOP_C_("Region", "Japan"),
	NOP_C_("Region", "USA"),
	NOP_C_("Region", "Europe"),
	nullptr,	//NOP_C_("Region", "Australia"),	// NOTE: Not actually used?
	NOP_C_("Region", "China"),
	NOP_C_("Region", "South Korea"),
	NOP_C_("Region", "Taiwan"),
}};

#if defined(_MSC_VER) && defined(XML_IS_DLL)
/**
 * Check if PugiXML can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

/** PugiXML macros **/

/**
 * Load a Wii U system XML file.
 *
 * The XML is loaded and parsed using the specified
 * PugiXML document.
 *
 * NOTE: DelayLoad must be checked by the caller, since it's
 * passing an xml_document reference to this function.
 *
 * @param doc		[in/out] XML document
 * @param filename	[in] XML filename
 * @param rootNode	[in] Root node for verification
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUPackagePrivate::loadSystemXml(xml_document &doc, const char *filename, const char *rootNode)
{
	assert(this->isValid);
	assert(rootNode != nullptr);	// not checking in release builds
	if (!this->isValid) {
		// Can't load the XML file.
		return -EIO;
	}

	IRpFilePtr f_xml = this->open(filename);
	if (!f_xml) {
		// Icon not found?
		return -ENOENT;
	}

	// Read the entire resource into memory.
	// Assuming a limit of 64 KB for Wii U system XML files.
	const size_t xml_size = static_cast<size_t>(f_xml->size());
	if (xml_size > 65536) {
		// Manifest is too big.
		// (Or, it's negative, and wraps around due to unsigned.)
		return -ENOMEM;
	}

	// PugiXML memory allocation functions
	allocation_function xml_alloc = get_memory_allocation_function();
	deallocation_function xml_dealloc = get_memory_deallocation_function();

	char *const xml_data = static_cast<char*>(xml_alloc(xml_size));
	if (!xml_data) {
		// malloc() failure!
		return -ENOMEM;
	}
	size_t size = f_xml->read(xml_data, xml_size);
	if (size != xml_size) {
		// Read error.
		xml_dealloc(xml_data);
		int err = f_xml->lastError();
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}
	f_xml.reset();

	// Parse the XML.
	doc.reset();
	xml_parse_result result = doc.load_buffer_inplace_own(xml_data, xml_size, parse_default, encoding_utf8);
	if (!result) {
		// Error parsing the manifest XML.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// Verify the root node.
	xml_node theRootNode = doc.child(rootNode);
	if (!theRootNode) {
		// Root node not found.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// Verify assembly attributes.
	// Wii U System XMLs always have 'type' and 'access' attributes.
	// 'type' should be "complex".
	// 'access' might not necessarily be "777", so not checking it.
	xml_attribute attr_type = theRootNode.attribute("type");
	xml_attribute attr_access = theRootNode.attribute("access");
	if (!attr_type || !attr_access || strcmp(attr_type.value(), "complex") != 0) {
		// Incorrect attributes.
		// TODO: Better error code.
		doc.reset();
		return -EIO;
	}

	// XML document loaded.
	return 0;
}

/**
 * Parse an "unsignedInt" element.
 * @param rootNode	[in] Root node
 * @param name		[in] Node name
 * @param defval	[in] Default value to return if the node isn't found
 * @return unsignedInt data (returns 0 on error)
 */
unsigned int WiiUPackagePrivate::parseUnsignedInt(xml_node rootNode, const char *name, unsigned int defval)
{
	if (!rootNode) {
		return defval;
	}

	xml_node elem = rootNode.child(name);
	if (!elem) {
		return defval;
	}

	xml_attribute attr = elem.attribute("type");
	if (!attr || strcmp(attr.value(), "unsignedInt") != 0) {
		return defval;
	}

	attr = elem.attribute("length");
	assert(attr && strcmp(attr.value(), "4") == 0);
	if (!attr || strcmp(attr.value(), "4") != 0) {
		return defval;
	}

	xml_text text = elem.text();
	if (!text) {
		return defval;
	}

	// Parse the value as an unsigned int.
	return text.as_uint(defval);
}

/**
 * Parse a "hexBinary" element.
 * NOTE: Some fields are 64-bit hexBinary, so we'll return a 64-bit value.
 * @param rootNode	[in] Root node
 * @param name		[in] Node name
 * @return hexBinary data (returns 0 on error)
 */
uint64_t WiiUPackagePrivate::parseHexBinary(xml_node rootNode, const char *name)
{
	if (!rootNode) {
		return 0;
	}

	xml_node elem = rootNode.child(name);
	if (!elem) {
		return 0;
	}

	xml_attribute attr = elem.attribute("type");
	if (!attr || strcmp(attr.value(), "hexBinary") != 0) {
		return 0;
	}

	attr = elem.attribute("length");
	const char *const attr_value = attr.value();
	assert(attr && (strcmp(attr_value, "4") == 0 || strcmp(attr_value, "8") == 0));
	if (!attr || (strcmp(attr_value, "4") != 0 && strcmp(attr_value, "8") != 0)) {
		return 0;
	}

	xml_text text = elem.text();
	if (!text) {
		return 0;
	}

	// Parse the value as a uint64_t.
	// NOTE: PugiXML's as_*int*() functions expect decimal, not hex.
	// Use strtoull() instead.
	char *endptr;
	uint64_t val = strtoull(text.get(), &endptr, 16);
	return (*endptr == '\0') ? val : 0;
}

#define ADD_TEXT(rootNode, name, desc) do { \
	xml_text text = rootNode.child(name).text(); \
	if (text) { \
		fields.addField_string((desc), text.get()); \
	} \
} while (0)

/**
 * Add fields from the Wii U System XML files.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUPackagePrivate::addFields_System_XMLs(void)
{
#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	// Load the three XML files.
	xml_document appXml, cosXml, metaXml;
	{
		int retSys = loadSystemXml(appXml, "/code/app.xml", "app");
		int retCos = loadSystemXml(cosXml, "/code/cos.xml", "app");
		int retMeta = loadSystemXml(metaXml, "/meta/meta.xml", "menu");

		if (retSys != 0 && retCos != 0 && retMeta != 0) {
			// Unable to load any of the XMLs.
			return retSys;
		}
	}

	// NOTE: Not creating a separate tab.

	// app.xml root node: "app"
	xml_node appRootNode = appXml.child("app");
	// cos.xml root node: "app"
	xml_node cosRootNode = cosXml.child("app");
	// meta.xml root node: "menu"
	xml_node metaRootNode = metaXml.child("menu");

	if (!appRootNode && !cosRootNode && !metaRootNode) {
		// Missing root elements from all three XMLs.
		// TODO: Better error code.
		return -EIO;
	}

	// Title (shortname), full title (longname), publisher
	// TODO: Wii U language codes? The XMLs are strings, so we'll
	// just use character-based codes for now.
	// TODO: Do we need both shortname and longname?

	// Language code map is required.
	// Most Wii U language codes match standard codes,
	// except for "zht" ('hant') and "zhs" ('hans').
	// Ordering matches Wii U meta.xml, which is likely the internal ordering.
	static constexpr size_t WiiU_LC_COUNT = 12U;
	struct xml_lc_map_t {
		char xml_lc[4];	// LC in the XML file
		uint32_t lc;	// Our LC
	};
	static const array<xml_lc_map_t, WiiU_LC_COUNT> xml_lc_map = {{
		{"ja",    'ja'},
		{"en",    'en'},
		{"fr",    'fr'},
		{"de",    'de'},
		{"it",    'it'},
		{"es",    'es'},
		{"zhs", 'hans'},
		{"ko",    'ko'},
		{"nl",    'nl'},
		{"pt",    'pt'},
		{"ru",    'ru'},
		{"zht", 'hant'},
	}};

	if (metaRootNode) {
		array<const char*, WiiU_LC_COUNT> longnames;
		array<const char*, WiiU_LC_COUNT> shortnames;
		array<const char*, WiiU_LC_COUNT> publishers;
		char longname_key[16] = "longname_";
		char shortname_key[16] = "shortname_";
		char publisher_key[16] = "publisher_";
		for (size_t i = 0; i < xml_lc_map.size(); i++) {
			// NOTE: xml_lc_map[i].xml_lc cannot be more than 3 letters.
			strcpy(&longname_key[9],   xml_lc_map[i].xml_lc);
			strcpy(&shortname_key[10], xml_lc_map[i].xml_lc);
			strcpy(&publisher_key[10], xml_lc_map[i].xml_lc);

			longnames[i] = metaRootNode.child(longname_key).text().as_string(nullptr);
			shortnames[i] = metaRootNode.child(shortname_key).text().as_string(nullptr);
			publishers[i] = metaRootNode.child(publisher_key).text().as_string(nullptr);
		}

		// If English is valid, we'll deduplicate titles.
		// TODO: Constants for Wii U languages. (Extension of Wii languages?)
		const bool dedupe_titles = (longnames[1] && longnames[1][0] != '\0');

		RomFields::StringMultiMap_t *const pMap_longname = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_shortname = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_publisher = new RomFields::StringMultiMap_t();
		for (int langID = 0; langID < static_cast<int>(xml_lc_map.size()); langID++) {
			// Check for empty strings first.
			if (( !longnames[langID] ||  longnames[langID][0] == '\0') &&
			    (!shortnames[langID] || shortnames[langID][0] == '\0') &&
			    (!publishers[langID] || publishers[langID][0] == '\0'))
			{
				// Strings are empty.
				continue;
			}

			if (dedupe_titles && langID != 1 /* English */) {
				// Check if the title matches English.
				if ( longnames[langID] &&  longnames[1] && !strcmp( longnames[langID], longnames[1]) &&
				    shortnames[langID] && shortnames[1] && !strcmp(shortnames[langID], shortnames[1]) &&
				    publishers[langID] && publishers[1] && !strcmp(publishers[langID], publishers[1]))
				{
					// All three title fields match English.
					continue;
				}
			}

			const uint32_t lc = xml_lc_map[langID].lc;
			assert(lc != 0);
			if (lc == 0)
				continue;

			if (longnames[langID] && longnames[langID][0] != '\0') {
				pMap_longname->emplace(lc, longnames[langID]);
			}
			if (shortnames[langID] && shortnames[langID][0] != '\0') {
				pMap_shortname->emplace(lc, shortnames[langID]);
			}
			if (publishers[langID] && publishers[langID][0] != '\0') {
				pMap_publisher->emplace(lc, publishers[langID]);
			}
		}

		// NOTE: Using the same descriptions as Nintendo3DS.
		const char *const s_title_title = C_("Nintendo", "Title");
		const char *const s_full_title_title = C_("Nintendo", "Full Title");
		const char *const s_publisher_title = C_("RomData", "Publisher");
		const char *const s_unknown = C_("RomData", "Unknown");

		// Get the system language code and see if we have a matching title.
		uint32_t def_lc = SystemRegion::getLanguageCode();
		if (pMap_longname->find(def_lc) == pMap_longname->end()) {
			// Not valid. Check English.
			if (pMap_longname->find('en') != pMap_longname->end()) {
				// English is valid.
				def_lc = 'en';
			} else {
				// Not valid. Check Japanese.
				if (pMap_longname->find('jp') != pMap_longname->end()) {
					// Japanese is valid.
					def_lc = 'jp';
				} else {
					// Not valid...
					// Default to English anyway.
					def_lc = 'en';
				}
			}
		}

		if (!pMap_shortname->empty()) {
			fields.addField_string_multi(s_title_title, pMap_shortname, def_lc);
		} else {
			delete pMap_shortname;
			fields.addField_string(s_title_title, s_unknown);
		}
		if (!pMap_longname->empty()) {
			fields.addField_string_multi(s_full_title_title, pMap_longname, def_lc);
		} else {
			delete pMap_longname;
			fields.addField_string(s_full_title_title, s_unknown);
		}
		if (!pMap_publisher->empty()) {
			fields.addField_string_multi(s_publisher_title, pMap_publisher, def_lc);
		} else {
			delete pMap_publisher;
			fields.addField_string(s_publisher_title, s_unknown);
		}
	}

	// Product code
	ADD_TEXT(metaRootNode, "product_code", C_("Nintendo", "Product Code"));

	// SDK version
	if (appRootNode) {
		const unsigned int sdk_version = parseUnsignedInt(appRootNode, "sdk_version");
		if (sdk_version != 0) {
			fields.addField_string(C_("WiiU", "SDK Version"),
				fmt::format(FSTR("{:d}.{:0>2d}.{:0>2d}"),
					sdk_version / 10000, (sdk_version / 100) % 100, sdk_version % 100));
		}
	}

	// argstr (TODO: Better title)
	ADD_TEXT(cosRootNode, "argstr", "argstr");

	// app_type (TODO: Decode this!)
	if (appRootNode) {
		const unsigned int app_type = parseHexBinary32(appRootNode, "app_type");
		if (app_type != 0) {
			const char *const s_app_type_title = C_("RomData", "Type");
			const char *const s_app_type = WiiUData::lookup_application_type(app_type);
			if (s_app_type) {
				fields.addField_string(s_app_type_title, s_app_type);
			} else {
				fields.addField_string_numeric(s_app_type_title, app_type,
					RomFields::Base::Hex, 8, RomFields::STRF_MONOSPACE);
			}
		}
	}

	if (metaRootNode) {
		// Region code
		// Maps directly to the region field.
		const uint32_t region_code = parseHexBinary32(metaRootNode, "region");

		vector<string> *const v_wiiu_region_bitfield_names = RomFields::strArrayToVector_i18n(
			"Region", wiiu_region_bitfield_names);
		fields.addField_bitfield(C_("RomData", "Region Code"),
			v_wiiu_region_bitfield_names, 3, region_code);

		// Age rating(s)
		// The fields match other Nintendo products, but it's in XML
		// instead of a binary field.
		// TODO: Exclude ratings that aren't actually used?
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-4, 6-11 (excludes old BBFC and Finland/MEKU)
		static constexpr uint16_t valid_ratings = 0xFDB;
		static const array<const char*, 12> age_rating_nodes = {{
			"pc_cero", "pc_esrb", "pc_bbfc", "pc_usk",
			"pc_pegi_gen", "pc_pegi_fin", "pc_pegi_prt", "pc_pegi_bbfc",
			"pc_cob", "pc_grb", "pc_cgsrr", "pc_oflc",
			/*"pc_reserved0", "pc_reserved1", "pc_reserved2", "pc_reserved3",*/
		}};
		static_assert(age_rating_nodes.size() == static_cast<size_t>(RomFields::AgeRatingsCountry::MaxAllocated), "age_rating_nodes is out of sync with age_ratings_t");

		for (int i = static_cast<int>(age_ratings.size())-1; i >= 0; i--) {
			if (!(valid_ratings & (1U << i))) {
				// Rating is not applicable for Wii U.
				age_ratings[i] = 0;
				continue;
			}

			// Wii U ratings field:
			// - 0x00-0x1F: Age rating
			// - 0x80: No rating
			// - 0xC0: Rating pending
			unsigned int val = parseUnsignedInt(metaRootNode, age_rating_nodes[i], ~0U);
			if (val == ~0U) {
				// Not found...
				age_ratings[i] = 0;
				continue;
			}

			if (val == 0x80) {
				// Rating is unused
				age_ratings[i] = 0;
			} else if (val == 0xC0) {
				// Rating pending
				age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_PENDING;
			} /*else if (val == 0) {
				// No age restriction
				// FIXME: Can be confused with some other ratings, so disabled for now.
				// Maybe Wii U has the same 0x20 bit as 3DS?
				age_ratings[i] = RomFields::AGEBF_ACTIVE | RomFields::AGEBF_NO_RESTRICTION;
			}*/ else {
				// Set active | age value.
				age_ratings[i] = RomFields::AGEBF_ACTIVE | (val & RomFields::AGEBF_MIN_AGE_MASK);
			}
		}
		fields.addField_ageRatings(C_("RomData", "Age Ratings"), age_ratings);

		// Controller support
		uint32_t controllers = 0;
		static const array<const char*, 6> controller_nodes = {{
			"ext_dev_nunchaku",
			"ext_dev_classic",
			"ext_dev_urcc",
			"ext_dev_board",
			"ext_dev_usb_keyboard",
			//"ext_dev_etc",	// TODO
			//"ext_dev_etc_name",	// TODO
			"drc_use",
		}};
		for (size_t i = 0; i < controller_nodes.size(); i++) {
			unsigned int val = parseUnsignedInt(metaRootNode, controller_nodes[i]);
			if (val > 0) {
				// This controller is supported.
				controllers |= (1U << i);
			}
		}

		static const array<const char*, 6> controllers_bitfield_names = {{
			NOP_C_("WiiU|Controller", "Nunchuk"),
			NOP_C_("WiiU|Controller", "Classic"),
			NOP_C_("WiiU|Controller", "Pro"),
			NOP_C_("WiiU|Controller", "Balance Board"),
			NOP_C_("WiiU|Controller", "USB Keyboard"),
			NOP_C_("WiiU|Controller", "Gamepad"),
		}};
		vector<string> *const v_controllers_bitfield_names = RomFields::strArrayToVector_i18n(
			"WiiU|Controller", controllers_bitfield_names);
		fields.addField_bitfield(C_("WiiU", "Controllers"),
			v_controllers_bitfield_names, 3, controllers);
	}

	// System XML files read successfully.
	return 0;
}

/**
 * Add metadata from the Wii U System XML files.
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUPackagePrivate::addMetaData_System_XMLs(void)
{
#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	// Load meta.xml.
	xml_document metaXml;
	int ret = loadSystemXml(metaXml, "/meta/meta.xml", "menu");
	if (ret != 0) {
		return ret;
	}

	// meta.xml root node: "menu"
	xml_node metaRootNode = metaXml.child("menu");
	if (!metaRootNode) {
		// No "menu" element.
		// TODO: Better error code.
		return -EIO;
	}

	// Get the system language code and see if we have a matching title.
	// NOTE: Using the same LC for all fields once we find a matching title.
	string s_def_lc = SystemRegion::lcToString(SystemRegion::getLanguageCode());
	string nodeName = fmt::format(FSTR("shortname_{:s}"), s_def_lc);

	xml_text shortname = metaRootNode.child(nodeName.c_str()).text();
	if (!shortname) {
		// Not valid. Check English.
		shortname = metaRootNode.child("shortname_en").text();
		if (shortname) {
			// English is valid.
			s_def_lc = "en";
		} else {
			// Not valid. Check Japanese.
			shortname = metaRootNode.child("shortname_jp");
			if (shortname) {
				// Japanese is valid.
				s_def_lc = "jp";
			} else {
				// Not valid...
				// Default to English anyway.
				s_def_lc = "en";
			}
		}
	}

	// Title
	// TODO: Shortname vs. longname?
	if (shortname) {
		metaData.addMetaData_string(Property::Title, shortname.get());
	}

	// Publisher
	nodeName = fmt::format(FSTR("publisher_{:s}"), s_def_lc);
	xml_text publisher = metaRootNode.child(nodeName.c_str()).text();
	if (publisher) {
		metaData.addMetaData_string(Property::Publisher, publisher.get());
	}

	/** Custom properties! **/

	// Product code (as Game ID)
	metaData.addMetaData_string(Property::GameID, metaRootNode.child("product_code").text().as_string(nullptr));

	// Region code
	// For multi-region titles, region will be formatted as: "JUECKT"
	// (Australia is ignored...)
	const uint32_t region_code = parseHexBinary32(metaRootNode, "region");
	const char *i18n_region = nullptr;
	for (size_t i = 0; i < wiiu_region_bitfield_names.size(); i++) {
		if (region_code == (1U << i)) {
			i18n_region = wiiu_region_bitfield_names[i];
			break;
		}
	}

	// Special-case check for Europe+Australia.
	// TODO: Constants for Wii U? (Same values as Nintendo 3DS...)
	if (region_code == (4 | 8)) {
		i18n_region = wiiu_region_bitfield_names[2];
	}

	if (i18n_region) {
		metaData.addMetaData_string(Property::RegionCode, pgettext_expr("Region", i18n_region));
	} else {
		// Multi-region
		static const char all_n3ds_regions[] = "JUECKT";
		string s_region_code;
		s_region_code.resize(sizeof(all_n3ds_regions)-1, '-');
		for (size_t i = 0; i < wiiu_region_bitfield_names.size(); i++) {
			size_t chr_pos = i;
			if (chr_pos >= 3) {
				chr_pos--;
			}

			if (region_code & (1U << i)) {
				s_region_code[chr_pos] = all_n3ds_regions[chr_pos];
			}
		}
		metaData.addMetaData_string(Property::RegionCode, s_region_code);
	}

	// System XML files read successfully.
	return 0;
}

/**
 * Get the product code from meta.xml, and application type from app.xml.
 * @param pApplType	[out] Pointer to uint32_t for application type
 * @return Product code, or empty string on error.
 */
string WiiUPackagePrivate::getProductCodeAndApplType_xml(uint32_t *pApplType)
{
#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return {};
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	xml_document metaXml;
	int retMeta = loadSystemXml(metaXml, "/meta/meta.xml", "menu");
	if (retMeta != 0) {
		// Unable to load meta.xml.
		return {};
	}

	xml_node metaRootNode = metaXml.child("menu");
	if (!metaRootNode) {
		// No root node.
		return {};
	}

	const char *const product_code = metaRootNode.child("product_code").text().as_string(nullptr);

	if (pApplType) {
		// Get the application type.
		xml_document appXml;
		int retApp = loadSystemXml(appXml, "/code/app.xml", "app");
		if (retApp == 0) {
			xml_node appRootNode = appXml.child("app");
			if (appRootNode) {
				*pApplType = parseHexBinary32(appRootNode, "app_type");
			}
		} else {
			// Unable to load app.xml.
			*pApplType = 0;
		}
	}

	return (product_code) ? product_code : string();
}

}

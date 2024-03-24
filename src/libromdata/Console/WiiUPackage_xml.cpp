/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * WiiUPackage_xml.cpp: Wii U NUS Package reader. (XML parsing)            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
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

// TinyXML2
#include "tinyxml2.h"
using namespace tinyxml2;

// C++ STL classes
using std::array;
using std::string;
using std::unique_ptr;
using std::vector;

namespace LibRomData {

#if defined(_MSC_VER) && defined(XML_IS_DLL)
/**
 * Check if TinyXML2 can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_TinyXML2(void);
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

/** TinyXML2 macros **/

/**
 * Load a Wii U system XML file.
 *
 * The XML is loaded and parsed using the specified
 * TinyXML document.
 *
 * NOTE: DelayLoad must be checked by the caller, since it's
 * passing an XMLDocument reference to this function.
 *
 * @param doc		[in/out] XML document
 * @param filename	[in] XML filename
 * @param rootNode	[in] Root node for verification
 * @return 0 on success; negative POSIX error code on error.
 */
int WiiUPackagePrivate::loadSystemXml(XMLDocument &doc, const char *filename, const char *rootNode)
{
	assert(this->isValid);
	assert(this->fst != nullptr);
	assert(rootNode != nullptr);	// not checking in release builds
	if (!this->isValid || !this->fst) {
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
	unique_ptr<char[]> xml(new char[xml_size+1]);
	size_t size = f_xml->read(xml.get(), xml_size);
	if (size != xml_size) {
		// Read error.
		int err = f_xml->lastError();
		if (err == 0) {
			err = EIO;
		}
		return -err;
	}
	f_xml.reset();
	xml[xml_size] = 0;

	// Parse the XML.
	// FIXME: TinyXML2 2.0.0 added XMLDocument::Clear().
	// Ubuntu 14.04 has an older version that doesn't have it...
	// Assuming the original document is empty.
#if TINYXML2_MAJOR_VERSION >= 2
	doc.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
	int xerr = doc.Parse(xml.get());
	if (xerr != XML_SUCCESS) {
		// Error parsing the manifest XML.
		// TODO: Better error code.
#if TINYXML2_MAJOR_VERSION >= 2
		doc.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
		return -EIO;
	}

	// Verify the root node.
	const XMLElement *const theRootNode = doc.FirstChildElement(rootNode);
	if (!theRootNode) {
		// Root node not found.
		// TODO: Better error code.
#if TINYXML2_MAJOR_VERSION >= 2
		doc.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
		return -EIO;
	}

	// Verify assembly attributes.
	// Wii U System XMLs always have 'type' and 'access' attributes.
	// 'type' should be "complex".
	// 'access' might not necessarily be "777", so not checking it.
	const char *const attr_type = theRootNode->Attribute("type");
	const char *const attr_access = theRootNode->Attribute("access");
	if (!attr_type || !attr_access || strcmp(attr_type, "complex") != 0) {
		// Incorrect attributes.
		// TODO: Better error code.
#if TINYXML2_MAJOR_VERSION >= 2
		doc.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
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
unsigned int WiiUPackagePrivate::parseUnsignedInt(const XMLElement *rootNode, const char *name, unsigned int defval)
{
	if (!rootNode)
		return 0;

	const XMLElement *const elem = rootNode->FirstChildElement(name);
	if (!elem)
		return 0;

	const char *attr = elem->Attribute("type");
	if (!attr || strcmp(attr, "unsignedInt") != 0)
		return 0;

	attr = elem->Attribute("length");
	assert(strcmp(attr, "4") == 0);
	if (!attr || strcmp(attr, "4") != 0)
		return 0;

	const char *const text = elem->GetText();
	if (!text)
		return 0;

	// Parse the value as an unsigned int.
	char *endptr;
	unsigned int val = strtoul(text, &endptr, 10);
	return (*endptr == '\0') ? val : defval;
}

/**
 * Parse a "hexBinary" element.
 * NOTE: Some fields are 64-bit hexBinary, so we'll return a 64-bit value.
 * @param rootNode	[in] Root node
 * @param name		[in] Node name
 * @return hexBinary data (returns 0 on error)
 */
uint64_t WiiUPackagePrivate::parseHexBinary(const XMLElement *rootNode, const char *name)
{
	if (!rootNode)
		return 0;

	const XMLElement *const elem = rootNode->FirstChildElement(name);
	if (!elem)
		return 0;

	const char *attr = elem->Attribute("type");
	if (!attr || strcmp(attr, "hexBinary") != 0)
		return 0;

	attr = elem->Attribute("length");
	assert(strcmp(attr, "4") == 0 || strcmp(attr, "8") == 0);
	if (!attr || (strcmp(attr, "4") != 0 && strcmp(attr, "8") != 0))
		return 0;

	const char *const text = elem->GetText();
	if (!text)
		return 0;

	// Parse the value as a uint64_t.
	char *endptr;
	uint64_t val = strtoull(text, &endptr, 16);
	return (*endptr == '\0') ? val : 0;
}

/**
 * Get text from an XML element.
 * @param rootNode	[in] Root node
 * @param name		[in] Node name
 * @return Node text, or nullptr if not found or empty.
 */
inline const char *WiiUPackagePrivate::getText(const tinyxml2::XMLElement *rootNode, const char *name)
{
	if (!rootNode)
		return nullptr;

	const XMLElement *const elem = rootNode->FirstChildElement(name);
	if (!elem)
		return nullptr;
	return elem->GetText();
}

#define ADD_TEXT(rootNode, name, desc) do { \
	const char *const text = getText(rootNode, name); \
	if (text) { \
		fields.addField_string((desc), text); \
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
	// TODO: Only if linked with /DELAYLOAD?
	int ret_dl = DelayLoad_test_TinyXML2();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	// Load the three XML files.
	XMLDocument appXml, cosXml, metaXml;
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
	const XMLElement *const appRootNode = appXml.FirstChildElement("app");
	// cos.xml root node: "app"
	const XMLElement *const cosRootNode = cosXml.FirstChildElement("app");
	// meta.xml root node: "menu"
	const XMLElement *const metaRootNode = metaXml.FirstChildElement("menu");

	if (!appRootNode && !cosRootNode && !metaRootNode) {
		// Missing root elements from all three XMLs.
		// TODO: Better error code.
		//return -EIO;
	}

	// Title (shortname), full title (longname), publisher
	// TODO: Wii U language codes? The XMLs are strings, so we'll
	// just use character-based codes for now.
	// TODO: Do we need both shortname and longname?

	// Language code map is required.
	// Most Wii U language codes match standard codes,
	// except for "zht" ('hant') and "zhs" ('hans').
	// Ordering matches Wii U meta.xml, which is likely the internal ordering.
#define WiiU_LC_COUNT 12
	struct xml_lc_map_t {
		char xml_lc[4];	// LC in the XML file
		uint32_t lc;	// Our LC
	};
	static const std::array<xml_lc_map_t, WiiU_LC_COUNT> xml_lc_map = {{
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
		const char *longnames[WiiU_LC_COUNT];
		const char *shortnames[WiiU_LC_COUNT];
		const char *publishers[WiiU_LC_COUNT];
		string longname_key = "longname_";
		string shortname_key = "shortname_";
		string publisher_key = "publisher_";
		longname_key.reserve(13);
		shortname_key.reserve(14);
		publisher_key.reserve(14);
		for (size_t i = 0; i < xml_lc_map.size(); i++) {
			longname_key.resize(9);
			shortname_key.resize(10);
			publisher_key.resize(10);

			longname_key += xml_lc_map[i].xml_lc;
			shortname_key += xml_lc_map[i].xml_lc;
			publisher_key += xml_lc_map[i].xml_lc;

			longnames[i] = getText(metaRootNode, longname_key.c_str());
			shortnames[i] = getText(metaRootNode, shortname_key.c_str());
			publishers[i] = getText(metaRootNode, publisher_key.c_str());
		}

		// If English is valid, we'll deduplicate titles.
		// TODO: Constants for Wii U languages. (Extension of Wii languages?)
		const bool dedupe_titles = (longnames[1] && longnames[1][0] != '\0');

		RomFields::StringMultiMap_t *const pMap_longname = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_shortname = new RomFields::StringMultiMap_t();
		RomFields::StringMultiMap_t *const pMap_publisher = new RomFields::StringMultiMap_t();
		for (int langID = 0; langID < (int)xml_lc_map.size(); langID++) {
			// Check for empty strings first.
			if ((!longnames[langID] || longnames[langID][0] == '\0') &&
			(!shortnames[langID] || shortnames[langID][0] == '\0') &&
			(!publishers[langID] || publishers[langID][0] == '\0'))
			{
				// Strings are empty.
				continue;
			}

			if (dedupe_titles && langID != 1 /* English */) {
				// Check if the title matches English.
				if (longnames[langID] && longnames[1] && !strcmp(longnames[langID], longnames[1]) &&
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
			char s_sdk_version[32];
			snprintf(s_sdk_version, sizeof(s_sdk_version), "%u.%02u.%02u",
				sdk_version / 10000, (sdk_version / 100) % 100, sdk_version % 100);
			fields.addField_string(C_("WiiU", "SDK Version"), s_sdk_version);
		}
	}

	// argstr (TODO: Better title)
	ADD_TEXT(cosRootNode, "argstr", "argstr");

	// app_type (TODO: Decode this!)
	if (appRootNode) {
		const unsigned int app_type = parseHexBinary(appRootNode, "app_type");
		if (app_type != 0) {
			const char *const s_app_type_title = C_("WiiU", "Type");
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
		const uint32_t region_code = parseHexBinary(metaRootNode, "region");

		static const char *const wiiu_region_bitfield_names[] = {
			NOP_C_("Region", "Japan"),
			NOP_C_("Region", "USA"),
			NOP_C_("Region", "Europe"),
			nullptr,	//NOP_C_("Region", "Australia"),	// NOTE: Not actually used?
			NOP_C_("Region", "China"),
			NOP_C_("Region", "South Korea"),
			NOP_C_("Region", "Taiwan"),
		};
		vector<string> *const v_wiiu_region_bitfield_names = RomFields::strArrayToVector_i18n(
			"Region", wiiu_region_bitfield_names, ARRAY_SIZE(wiiu_region_bitfield_names));
		fields.addField_bitfield(C_("RomData", "Region Code"),
			v_wiiu_region_bitfield_names, 3, region_code);

		// Age rating(s)
		// The fields match other Nintendo products, but it's in XML
		// instead of a binary field.
		// TODO: Exclude ratings that aren't actually used?
		RomFields::age_ratings_t age_ratings;
		// Valid ratings: 0-1, 3-4, 6-11 (excludes old BBFC and Finland/MEKU)
		static const uint16_t valid_ratings = 0xFDB;
		static const array<const char*, 12> age_rating_nodes = {{
			"pc_cero", "pc_esrb", "pc_bbfc", "pc_usk",
			"pc_pegi_gen", "pc_pegi_fin", "pc_pegi_prt", "pc_pegi_bbfc",
			"pc_cob", "pc_grb", "pc_cgsrr", "pc_oflc",
			/*"pc_reserved0", "pc_reserved1", "pc_reserved2", "pc_reserved3",*/
		}};
		static_assert(age_rating_nodes.size() == (int)RomFields::AgeRatingsCountry::MaxAllocated, "age_rating_nodes is out of sync with age_ratings_t");

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
		static const std::array<const char*, 6> controller_nodes = {{
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

		static const char *const controllers_bitfield_names[] = {
			NOP_C_("WiiU|Controller", "Nunchuk"),
			NOP_C_("WiiU|Controller", "Classic"),
			NOP_C_("WiiU|Controller", "Pro"),
			NOP_C_("WiiU|Controller", "Balance Board"),
			NOP_C_("WiiU|Controller", "USB Keyboard"),
			NOP_C_("WiiU|Controller", "Gamepad"),
		};
		vector<string> *const v_controllers_bitfield_names = RomFields::strArrayToVector_i18n(
			"WiiU|Controller", controllers_bitfield_names, ARRAY_SIZE(controllers_bitfield_names));
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
	// TODO: Only if linked with /DELAYLOAD?
	int ret_dl = DelayLoad_test_TinyXML2();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	// Load meta.xml.
	XMLDocument appXml, cosXml, metaXml;
	int ret = loadSystemXml(metaXml, "/meta/meta.xml", "menu");
	if (ret != 0)
		return ret;

	// meta.xml root node: "menu"
	const XMLElement *const metaRootNode = metaXml.FirstChildElement("menu");
	if (!metaRootNode) {
		// No "menu" element.
		// TODO: Better error code.
		return -EIO;
	}

	// Get the system language code and see if we have a matching title.
	// NOTE: Using the same LC for all fields once we find a matching title.
	string s_def_lc = SystemRegion::lcToString(SystemRegion::getLanguageCode());
	char nodeName[16];
	snprintf(nodeName, sizeof(nodeName), "shortname_%s", s_def_lc.c_str());

	const char *shortname = getText(metaRootNode, nodeName);
	if (!shortname) {
		// Not valid. Check English.
		shortname = getText(metaRootNode, "shortname_en");
		if (shortname) {
			// English is valid.
			s_def_lc = "en";
		} else {
			// Not valid. Check Japanese.
			shortname = getText(metaRootNode, "shortname_jp");
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
		metaData->addMetaData_string(Property::Title, shortname);
	}

	// Publisher
	snprintf(nodeName, sizeof(nodeName), "publisher_%s", s_def_lc.c_str());
	const char *const publisher = getText(metaRootNode, nodeName);
	if (publisher) {
		metaData->addMetaData_string(Property::Publisher, publisher);
	}

	// System XML files read successfully.
	return 0;
}

}

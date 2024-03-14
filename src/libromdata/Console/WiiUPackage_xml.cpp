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

// Other rom-properties libraries
using namespace LibRpBase;
using namespace LibRpFile;

// TinyXML2
#include "tinyxml2.h"
using namespace tinyxml2;

// C++ STL classes
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

#define FIRST_CHILD_ELEMENT(var, parent_elem, child_elem_name) \
	const XMLElement *var = parent_elem->FirstChildElement(child_elem_name); \
	if (!var) { \
		var = parent_elem->FirstChildElement(child_elem_name); \
	} \
do { } while (0)

#define ADD_TEXT(parent_elem, child_elem_name, desc) do { \
	FIRST_CHILD_ELEMENT(child_elem, parent_elem, child_elem_name); \
	if (child_elem) { \
		const char *const text = child_elem->GetText(); \
		if (text) { \
			fields.addField_string((desc), text); \
		} \
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

	// Load meta.xml first.
	// TODO: app.xml, cos.xml.
	XMLDocument doc;
	int ret = loadSystemXml(doc, "/meta/meta.xml", "menu");
	if (ret != 0) {
		return ret;
	}

	// NOTE: Not creating a separate tab.

	// Root node: "menu"
	const XMLElement *const rootNode = doc.FirstChildElement("menu");
	if (!rootNode) {
		// No "menu" element.
		// TODO: Better error code.
#if TINYXML2_MAJOR_VERSION >= 2
		doc.Clear();
#endif /* TINYXML2_MAJOR_VERSION >= 2 */
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
#define WiiU_LC_COUNT 12
	struct xml_lc_map_t {
		char xml_lc[4];	// LC in the XML file
		uint32_t lc;	// Our LC
	};
	static const xml_lc_map_t xml_lc_map[WiiU_LC_COUNT] = {
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
	};

	const char *longnames[WiiU_LC_COUNT];
	const char *shortnames[WiiU_LC_COUNT];
	const char *publishers[WiiU_LC_COUNT];
	string longname_key = "longname_";
	string shortname_key = "shortname_";
	string publisher_key = "publisher_";
	longname_key.reserve(13);
	shortname_key.reserve(14);
	publisher_key.reserve(14);
	for (unsigned int i = 0; i < WiiU_LC_COUNT; i++) {
		longname_key.resize(9);
		shortname_key.resize(10);
		publisher_key.resize(10);

		longname_key += xml_lc_map[i].xml_lc;
		shortname_key += xml_lc_map[i].xml_lc;
		publisher_key += xml_lc_map[i].xml_lc;

		const XMLElement *elem = rootNode->FirstChildElement(longname_key.c_str());
		longnames[i] = (elem) ? elem->GetText() : nullptr;

		elem = rootNode->FirstChildElement(shortname_key.c_str());
		shortnames[i] = (elem) ? elem->GetText() : nullptr;

		elem = rootNode->FirstChildElement(publisher_key.c_str());
		publishers[i] = (elem) ? elem->GetText() : nullptr;
	}

	// If English is valid, we'll deduplicate titles.
	// TODO: Constants for Wii U languages. (Extension of Wii languages?)
	const bool dedupe_titles = (longnames[1] && longnames[1][0] != '\0');

	RomFields::StringMultiMap_t *const pMap_longname = new RomFields::StringMultiMap_t();
	RomFields::StringMultiMap_t *const pMap_shortname = new RomFields::StringMultiMap_t();
	RomFields::StringMultiMap_t *const pMap_publisher = new RomFields::StringMultiMap_t();
	for (int langID = 0; langID < WiiU_LC_COUNT; langID++) {
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

	// TODO: Implement getDefaultLC().
	const uint32_t def_lc = 'en'; //d->getDefaultLC();
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

	// Product code
	ADD_TEXT(rootNode, "product_code", C_("WiiU", "Product Code"));

	// Controller support
	uint32_t controllers = 0;
	static const char *const controller_nodes[] = {
		"ext_dev_nunchaku",
		"ext_dev_classic",
		"ext_dev_urcc",
		"ext_dev_board",
		"ext_dev_usb_keyboard",
		//"ext_dev_etc",	// TODO
		//"ext_dev_etc_name",	// TODO
		"drc_use",
	};
	for (unsigned int i = 0; i < ARRAY_SIZE(controller_nodes); i++) {
		const XMLElement *const elem = rootNode->FirstChildElement(controller_nodes[i]);
		if (!elem)
			continue;

		// This should be an unsignedInt with length 4.
		const char *attr = elem->Attribute("type");
		if (!attr || strcmp(attr, "unsignedInt") != 0)
			continue;
		attr = elem->Attribute("length");
		if (!attr || strcmp(attr, "4") != 0)
			continue;

		const char *const text = elem->GetText();
		if (!text)
			continue;

		// Parse the value as an unsigned int.
		char *endptr;
		unsigned int val = strtoul(text, &endptr, 10);
		if (*endptr == '\0' && val > 0) {
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


	// System XML files read successfully.
	return 0;
}

}

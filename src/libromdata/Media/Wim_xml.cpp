/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * Wim.cpp: Microsoft WIM header reader (XML parsing)                      *
 *                                                                         *
 * Copyright (c) 2023 by ecumber.                                          *
 * Copyright (c) 2019-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "Wim_p.hpp"

#ifndef ENABLE_XML
#  error Cannot compile EXE_manifest.cpp without XML support.
#endif

// Other rom-properties libraries
#include "librpbase/timeconv.h"
using namespace LibRpBase;

#ifdef ENABLE_XML
// PugiXML
#  include <pugixml.hpp>
using namespace pugi;
#endif /* ENABLE_XML */

// C includes
#include <uchar.h>

// C++ STL classes
using std::array;
using std::string;
using std::vector;

namespace LibRomData {

#if defined(_MSC_VER) && defined(XML_IS_DLL)
/**
 * Check if PugiXML can be delay-loaded.
 * @return 0 on success; negative POSIX error code on error.
 */
extern int DelayLoad_test_PugiXML(void);
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

struct WimWindowsLanguages {
	string language;
	//string default_language;	//not used right now
};

struct WimWindowsVersion {
	unsigned int majorversion = 0;
	unsigned int minorversion = 0;
	unsigned int buildnumber = 0;
	unsigned int spbuildnumber = 0;
	unsigned int splevel = 0; // only in windows 7+, added some time around build 6608-6730
};

struct WimWindowsInfo {
	WimWindowsArchitecture arch = Wim_Arch_x86;
	string productname;
	string editionid;
	//string installationtype;	// not used right now
	//string hal;			// not used right now
	//string producttype;		// not used right now
	//string productsuite;		// not used right now
	WimWindowsLanguages languages;
	WimWindowsVersion version;
	string systemroot;
};

struct WimIndex {
	// if you have more than 2^32 indices in a wim
	// you probably have bigger issues
	uint32_t index = 0;		// main image index
	uint64_t dircount = 0;
	uint64_t filecount = 0;
	uint64_t totalbytes = 0;
	//time_t creationtime = 0;	// not used right now
	time_t lastmodificationtime = 0;
	WimWindowsInfo windowsinfo;
	string name, description;
	//string flags;			// not used right now
	string dispname, dispdescription;
	bool containswindowsimage = false;
	bool is_unstaged = false;	// unstaged images have sets of components
	char unstaged_idx = 0;		// unstaged image sub-index
};

/**
 * Add fields from the WIM image's XML manifest.
 * @return 0 on success; non-zero on error.
 */
int WimPrivate::addFields_XML() 
{
	if (!file || !file->isOpen()) {
		return -EIO;
	}

#if defined(_MSC_VER) && defined(XML_IS_DLL)
	// Delay load verification.
	int ret_dl = DelayLoad_test_PugiXML();
	if (ret_dl != 0) {
		// Delay load failed.
		return ret_dl;
	}
#endif /* defined(_MSC_VER) && defined(XML_IS_DLL) */

	// Sanity check: Minimum of 1 image; allow up to 256 images.
	unsigned int number_of_images = wimHeader.number_of_images;
	assert(number_of_images > 0);
	assert(number_of_images <= 256);
	if (number_of_images == 0) {
		// No images...
		return -ENOENT;
	} else if (number_of_images > 256) {
		number_of_images = 256;
	}

	// the eighth byte of the "size" is used for flags so we have to AND it
	static constexpr uint64_t XML_MAX_SIZE = 16U*1024U*1024U;
	const uint64_t size64 = (wimHeader.xml_resource.size & 0x00FFFFFFFFFFFFFFULL);
	assert(size64 <= XML_MAX_SIZE);
	if (size64 > XML_MAX_SIZE) {
		// XML is larger than 16 MB, which doesn't make any sense.
		return -ENOMEM;
	}

	// XML data is UTF-16LE, so the size should be a multiple of 2.
	// TODO: Error out if it isn't?
	size_t xml_size = static_cast<size_t>(size64);
	assert(xml_size % 2 == 0);
	xml_size &= ~1ULL;

	// Read the WIM XML data.
	file->rewind();
	file->seek(wimHeader.xml_resource.offset_of_xml); 
	// if seek is invalid
	if (file->tell() != static_cast<off64_t>(wimHeader.xml_resource.offset_of_xml)) {
		return -EIO;
	}

	// PugiXML memory allocation functions
	allocation_function xml_alloc = get_memory_allocation_function();
	deallocation_function xml_dealloc = get_memory_deallocation_function();

	char16_t *const xml_data = static_cast<char16_t*>(xml_alloc(xml_size));
	if (!xml_data) {
		// malloc() failure!
		return -ENOMEM;
	}
	size_t size = file->read(xml_data, static_cast<size_t>(xml_size));
	if (size != xml_size) {
		// Read error.
		xml_dealloc(xml_data);
		return -EIO;
	}

	xml_document doc;
	xml_parse_result result = doc.load_buffer_inplace_own(xml_data, xml_size, parse_default, encoding_utf16_le);
	if (!result) {
		return 3;
	}

	xml_node wim_element = doc.child("WIM");
	if (!wim_element) {
		// No wim element.
		// TODO: Better error code.
		return -EIO;
	}

	vector<WimIndex> images;
	images.reserve(number_of_images);

	const char *const s_unknown = C_("Wim", "(unknown)");
	uint32_t image_index = 1;
	for (xml_node currentimage = wim_element.child("IMAGE");
	     currentimage; currentimage = currentimage.next_sibling())
	{
		assert(currentimage != nullptr);
		if (!currentimage) {
			break;
		}

		WimIndex currentindex;
		currentindex.index = currentimage.attribute("INDEX").as_uint(image_index++);

		// the last modification time is split into a high part and a
		// low part so we shift and add them together
		xml_node creationTime = currentimage.child("CREATIONTIME");
		if (creationTime) {
			xml_text highPart = creationTime.child("HIGHPART").text();
			xml_text lowPart = creationTime.child("LOWPART").text();
			if (highPart && lowPart) {
				// Parse HIGHPART and LOWPART, then combine them like FILETIME.
				const uint32_t lastmodtime_high = strtoul(highPart.get(), nullptr, 16);
				const uint32_t lastmodtime_low = strtoul(lowPart.get(), nullptr, 16);
				currentindex.lastmodificationtime = WindowsSplitTimeToUnixTime(lastmodtime_high, lastmodtime_low);
			}
		}

		xml_node windowsinfo = currentimage.child("WINDOWS");
		xml_node editionid;
		xml_node language;	// first language only
		if (windowsinfo) {
			currentindex.containswindowsimage = true;
			currentindex.windowsinfo.arch = static_cast<WimWindowsArchitecture>(
				windowsinfo.child("ARCH").text().as_int(0));
			editionid = windowsinfo.child("EDITIONID");
			currentindex.windowsinfo.editionid = editionid.text().as_string(s_unknown);

			xml_node languages = windowsinfo.child("LANGUAGES");
			if (languages) {
				// NOTE: Only retrieving the first language.
				language = languages.child("LANGUAGE");
				currentindex.windowsinfo.languages.language = language.text().get();
			}
			if (currentindex.windowsinfo.languages.language.empty()) {
				currentindex.windowsinfo.languages.language = s_unknown;
			}

			xml_node version = windowsinfo.child("VERSION");
			if (version) {
				currentindex.windowsinfo.version.majorversion = version.child("MAJOR").text().as_int(0);
				currentindex.windowsinfo.version.minorversion = version.child("MINOR").text().as_int(0);
				currentindex.windowsinfo.version.buildnumber = version.child("BUILD").text().as_int(0);
				currentindex.windowsinfo.version.spbuildnumber = version.child("SPBUILD").text().as_int(0);
			}
		} else {
			currentindex.containswindowsimage = false;
		}

		// some wims don't have these fields, so we
		// need to set up fallbacks - the hierarchy goes
		// display name -> name -> "(None)"
		const char *const s_none = C_("Wim", "(none)");
		currentindex.name = currentimage.child("NAME").text().as_string(s_none);
		xml_node description = currentimage.child("DESCRIPTION");
		currentindex.description = currentimage.child("DESCRIPTION").text().as_string(s_none);
		currentindex.dispname = currentimage.child("DISPLAYNAME").text().as_string(s_none);
		currentindex.dispdescription = currentimage.child("DISPLAYDESCRIPTION").text().as_string(s_none);

		// Check for an unstaged image.
		if (currentindex.containswindowsimage && editionid.empty() && language.empty()) {
			// This may be an unstaged image.
			// Check for "EDITIONS:" in the description.
			// TODO: Verify that this is the correct field.
			const char *const s_description = description.text().get();
			const char *p;
			if (s_description[0] != '\0' && (p = strstr(s_description, "EDITIONS:")) != nullptr) {
				// Found editions. Split it on commas and make each a separate image.
				currentindex.windowsinfo.languages.language = "N/A";	// TODO
				currentindex.description.assign(s_description, p - s_description);
				// Remove trailing spaces, if any.
				size_t cur_size = currentindex.description.size();
				while (cur_size > 0 && currentindex.description[cur_size - 1] == ' ') {
					cur_size--;
				}
				currentindex.description.resize(cur_size);

				currentindex.is_unstaged = true;
				char unstaged_idx = 'a';
				char *const dupdesc = strdup(p + 9);
				char *saveptr = nullptr;
				for (const char *token = strtok_r(dupdesc, ",", &saveptr);
				     token != nullptr; token = strtok_r(nullptr, ",", &saveptr))
				{
					currentindex.unstaged_idx = unstaged_idx++;
					currentindex.windowsinfo.editionid = token;
					images.push_back(currentindex);
				}
				free(dupdesc);

				if (unstaged_idx == 'a') {
					// Malformed entry; no actual sub-images.
					currentindex.is_unstaged = false;
				}
			}
		}
		if (!currentindex.is_unstaged) {
			// Not an unstaged image. Use the display description as-is.
			currentindex.description = description.text().as_string(s_none);
			images.push_back(std::move(currentindex));
		}
	}

	auto *const vv_data = new RomFields::ListData_t();
	vv_data->reserve(number_of_images);

	// loop for the rows
	for (const auto &image : images) {
		vv_data->resize(vv_data->size()+1);
		auto &data_row = vv_data->at(vv_data->size()-1);
		data_row.reserve(10);

		if (likely(!image.is_unstaged)) {
			// Staged images use the format "1", "2", "3", etc.
			data_row.push_back(fmt::to_string(image.index));
		} else {
			// Unstaged sub-images will use the format "1a", "1b", "1c", etc.
			// TODO: What if there's more than 26 sub-images?
			assert(image.unstaged_idx <= 'z');
			data_row.push_back(fmt::format(FSTR("{:d}{:c}"), image.index, image.unstaged_idx));
		}

		data_row.push_back(image.name);
		data_row.push_back(image.description);
		data_row.push_back(image.dispname);
		data_row.push_back(image.dispdescription);

		// Pack the 64-bit time_t into a string.
		RomFields::TimeString_t time_string;
		time_string.time = image.lastmodificationtime;
		data_row.emplace_back(time_string.str, sizeof(time_string.str));

		if (image.containswindowsimage == false) {
			// No Windows image. Add empty strings to complete the row.
			for (unsigned int i = 4; i > 0; i--) {
				data_row.emplace_back();
			}
			continue;
		}

		const auto &windowsver = image.windowsinfo.version;
		data_row.push_back(
			fmt::format(FSTR("{:d}.{:d}.{:d}.{:d}"),
				windowsver.majorversion, windowsver.minorversion,
				windowsver.buildnumber, windowsver.spbuildnumber));

		const auto &windowsinfo = image.windowsinfo;
		data_row.push_back(windowsinfo.editionid);
		const char *archstring;
		switch (windowsinfo.arch) {
			default:
				archstring = nullptr;
				break;
			case Wim_Arch_x86:
				archstring = "x86";
				break;
			case Wim_Arch_ARM32:
				archstring = "ARM32";
				break;
			case Wim_Arch_IA64:
				archstring = "IA64";
				break;
			case Wim_Arch_AMD64:
				archstring = "x64";
				break;
			case Wim_Arch_ARM64:
				archstring = "ARM64";
				break;
		}
		if (archstring) {
			data_row.emplace_back(archstring);
		} else {
			data_row.push_back(fmt::format(FRUN(C_("RomData", "Unknown ({:d})")),
				static_cast<int>(windowsinfo.arch)));
		}
		data_row.push_back(windowsinfo.languages.language);
	}	

	static const array<const char*, 10> field_names = {{
		NOP_C_("Wim|Images", "#"),
		NOP_C_("Wim|Images", "Name"),
		NOP_C_("Wim|Images", "Description"),
		NOP_C_("Wim|Images", "Display Name"),
		NOP_C_("Wim|Images", "Display Desc."),
		NOP_C_("Wim|Images", "Last Modified"),
		NOP_C_("Wim|Images", "OS Version"),
		NOP_C_("Wim|Images", "Edition"),
		NOP_C_("Wim|Images", "Architecture"),
		NOP_C_("Wim|Images", "Language"),
	}};
	vector<string> *const v_field_names = RomFields::strArrayToVector_i18n("Wim|Images", field_names);

	RomFields::AFLD_PARAMS params;
	params.flags = RomFields::RFT_LISTDATA_SEPARATE_ROW;
	params.col_attrs.is_timestamp = (1U << 5);
	params.col_attrs.dtflags = static_cast<RomFields::DateTimeFlags>(
		RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME);
	params.headers = v_field_names;
	params.data.single = vv_data;
	// TODO: Header alignment?
	fields.addField_listData(C_("Wim", "Images"), &params);

	return 0;
}

} // namespace LibRomData
